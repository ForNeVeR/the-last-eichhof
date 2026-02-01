module Meganob.Tests.TaskExecutorTests

open System
open System.Collections.Concurrent
open System.Collections.Immutable
open System.Threading.Tasks
open Xunit
open Meganob

type TestOutput(value: int) =
    member _.Value = value

    interface IArtifact with
        member _.GetContentHash() = Task.FromResult <| CacheKey.ComputeCombinedHash [string value]

let createMockReporter() =
    { new IReporter with
        member _.Status _ = ()
        member _.Log _ = ()
        member _.WithProgress(_, _, action) = action ProgressReporter.Null
    }

let createTask(name: string, inputs: BuildTask seq, execute: IArtifact seq -> Task<IArtifact>) =
    {
        Name = name
        Inputs = inputs.ToImmutableArray()
        CacheData = None
        Execute = fun (_, inputs) -> execute inputs
    }

let createValueTask(name: string, value: int, inputs: BuildTask seq) =
    createTask(name, inputs, fun _ -> Task.FromResult(TestOutput(value) :> IArtifact))

[<Fact>]
let ``Single task with no dependencies executes successfully``(): Task = task {
    let reporter = createMockReporter()
    let singleTask = createValueTask("single", 42, [])

    let! result = BuildSystem.ExecuteTask(singleTask, reporter)

    let testOutput = result :?> TestOutput
    Assert.Equal(42, testOutput.Value)
}

[<Fact>]
let ``Dependencies execute before dependents``(): Task = task {
    let reporter = createMockReporter()
    let executionOrder = ResizeArray<string>()

    let taskD = createTask("D", [], fun _ -> task {
        lock executionOrder (fun() -> executionOrder.Add("D"))
        return TestOutput(1) :> IArtifact
    })
    let taskC = createTask("C", [taskD], fun _ -> task {
        lock executionOrder (fun() -> executionOrder.Add("C"))
        return TestOutput(2) :> IArtifact
    })
    let taskB = createTask("B", [taskD], fun _ -> task {
        lock executionOrder (fun() -> executionOrder.Add("B"))
        return TestOutput(3) :> IArtifact
    })
    let taskA = createTask("A", [taskB; taskC], fun _ -> task {
        lock executionOrder (fun() -> executionOrder.Add("A"))
        return TestOutput(4) :> IArtifact
    })

    let! _ = BuildSystem.ExecuteTask(taskA, reporter)

    // D must be before B and C; B and C must be before A:
    let indexOf name = executionOrder.IndexOf(name)
    let idxD = indexOf "D"
    let idxB = indexOf "B"
    let idxC = indexOf "C"
    let idxA = indexOf "A"
    Assert.True(idxD < idxB, $"D ({idxD}) should be before B ({idxB})")
    Assert.True(idxD < idxC, $"D ({idxD}) should be before C ({idxC})")
    Assert.True(idxB < idxA, $"B ({idxB}) should be before A ({idxA})")
    Assert.True(idxC < idxA, $"C ({idxC}) should be before A ({idxA})")
}

[<Fact>]
let ``Independent tasks run concurrently``(): Task = task {
    let reporter = createMockReporter()
    let concurrentCount = ref 0
    let maxConcurrent = ref 0
    let lockObj = obj()

    let createSlowTask name =
        createTask(name, [], fun _ -> task {
            lock lockObj (fun () ->
                concurrentCount.Value <- concurrentCount.Value + 1
                if concurrentCount.Value > maxConcurrent.Value then
                    maxConcurrent.Value <- concurrentCount.Value
            )
            do! Task.Delay(50)  // Simulate work
            lock lockObj (fun () ->
                concurrentCount.Value <- concurrentCount.Value - 1
            )
            return TestOutput(1) :> IArtifact
        })

    let taskB = createSlowTask "B"
    let taskC = createSlowTask "C"
    let taskD = createSlowTask "D"
    let taskA = createTask("A", [taskB; taskC; taskD], fun _ ->
        Task.FromResult(TestOutput(1) :> IArtifact))

    let! _ = BuildSystem.ExecuteTask(taskA, reporter)

    // At least 2 tasks should have run concurrently (B, C, D are independent)
    Assert.True(maxConcurrent.Value >= 2, $"Expected concurrent execution, but max was {maxConcurrent.Value}")
}

[<Fact>]
let ``Diamond dependency pattern executes correctly``(): Task = task {
    let reporter = createMockReporter()
    let executionCounts = ConcurrentDictionary<string, int>()

    let trackExecution name =
        executionCounts.AddOrUpdate(name, 1, fun _ v -> v + 1)

    //     A
    //    / \
    //   B   C
    //    \ /
    //     D
    let taskD = createTask("D", [], fun _ -> task {
        trackExecution "D" |> ignore
        return TestOutput(1) :> IArtifact
    })
    let taskB = createTask("B", [taskD], fun inputs -> task {
        trackExecution "B" |> ignore
        let dOutput = inputs |> Seq.head :?> TestOutput
        return TestOutput(dOutput.Value + 1) :> IArtifact
    })
    let taskC = createTask("C", [taskD], fun inputs -> task {
        trackExecution "C" |> ignore
        let dOutput = inputs |> Seq.head :?> TestOutput
        return TestOutput(dOutput.Value + 10) :> IArtifact
    })
    let taskA = createTask("A", [taskB; taskC], fun inputs -> task {
        trackExecution "A" |> ignore
        let sum = inputs |> Seq.sumBy (fun o -> (o :?> TestOutput).Value)
        return TestOutput(sum) :> IArtifact
    })

    let! result = BuildSystem.ExecuteTask(taskA, reporter)

    let testOutput = result :?> TestOutput
    // D=1, B=D+1=2, C=D+10=11, A=B+C=13
    Assert.Equal(13, testOutput.Value)
    // Each task should execute exactly once
    Assert.Equal(1, executionCounts["D"])
    Assert.Equal(1, executionCounts["B"])
    Assert.Equal(1, executionCounts["C"])
    Assert.Equal(1, executionCounts["A"])
}

[<Fact>]
let ``Task failure propagates exception``(): Task = task {
    let reporter = createMockReporter()

    let failingTask = createTask("failing", [], fun _ -> task {
        failwith "Intentional failure"
        return TestOutput(1) :> IArtifact
    })
    let rootTask = createTask("root", [failingTask], fun _ ->
        Task.FromResult(TestOutput(1) :> IArtifact))

    let! ex = Assert.ThrowsAnyAsync<Exception>(fun () -> BuildSystem.ExecuteTask(rootTask, reporter))

    Assert.Contains("Intentional failure", ex.Message)
}
