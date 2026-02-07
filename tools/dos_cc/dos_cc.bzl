# SPDX-FileCopyrightText: 2026 Friedrich von Never <friedrich@fornever.me>
#
# SPDX-License-Identifier: MIT

"""Custom Bazel rule for compiling DOS C/C++ via Borland C++ 3.1 in DosBox-X."""

def _dos_cc_binary_impl(ctx):
    out = ctx.actions.declare_file(ctx.attr.out)

    args = ctx.actions.args()
    compiler_file = ctx.files.compiler[0]
    short = compiler_file.short_path
    repo_relative = short[len("../"):]
    repo_name = repo_relative.split("/")[0]
    bcpp_root = compiler_file.root.path + "/external/" + repo_name if compiler_file.root.path else "external/" + repo_name
    args.add("--bcpp-dir", bcpp_root)
    args.add("--output", out)
    args.add("--project-file", ctx.attr.project_file)
    args.add_all(ctx.files.srcs)

    ctx.actions.run(
        executable = ctx.executable._compiler_wrapper,
        arguments = [args],
        inputs = depset(ctx.files.srcs, transitive = [ctx.attr.compiler[DefaultInfo].files]),
        outputs = [out],
        mnemonic = "DosCC",
        progress_message = "Compiling %s via Borland C++ in DosBox-X" % ctx.attr.out,
        execution_requirements = {
            "no-sandbox": "1",
            "local": "1",
        },
    )

    return [DefaultInfo(files = depset([out]))]

dos_cc_binary = rule(
    implementation = _dos_cc_binary_impl,
    attrs = {
        "srcs": attr.label_list(allow_files = True),
        "compiler": attr.label(mandatory = True),
        "project_file": attr.string(mandatory = True),
        "out": attr.string(mandatory = True),
        "_compiler_wrapper": attr.label(
            default = "//Build:compiler",
            executable = True,
            cfg = "exec",
        ),
    },
)
