
;---------------------------------------------------------;
;                                                         ;
;                                                         ;
;       Baller Game Driver module. [c] 1992 by            ;
;                   ALPHA - HELIX                         ;
;               written by Dany Schoch                    ;
;                                                         ;
;  This module is graphic mode independant. Just link     ;
;  in the proper LOW LEVEL GRAPHICS module and it works.  ;
;                                                         ;
;                                                         ;
;---------------------------------------------------------;



	IDEAL			; Idealismus immer und ueberall.
	P286N                   ; Unleash the power of the i286, hehe...
	JUMPS			; Easy conditional jumping.

	MODEL	compact, c

include "xmode.ash"
include "sound.ash"
include "baller.ash"



	dataseg


; Structure to save a complete position during the game.
; This is used to get back to a 'try again' point if the
; bottle broke.
struc	lastposstrc
score		dd	?               ; Player's score.
nattacks	dw	?		; Attacks to go.
attack		dd	?               ; ptr to attacks.
nbigboss	dw	?               ; N of Big Bosses to go.
count		dw	?               ; Frame counter.
ends    lastposstrc

lastposition	lastposstrc	?
invincible	dw	?		; Is Eichli invincible?
frameinc	dw	?		; Frame count increment step.
nbigboss	dw	?		; Number of endlevel monsters that
					;   must still be destroyed.

; Variables used to draw the score.
scoredigit	dw	?		; # of digits already drawn.
scorepos	dw	?               ; Current x position.
playscore	dd	?               ; Remaining score.

const10d	dw	10d            	; Faster Division.



; Make number positive.
; NOTE: SIGN flag must be set to indicate negative number.
macro	abs	x
local	skip

	jns	skip
	neg	x
skip:

endm	abs


	codeseg

; copytoCS: Dies ist der zweite teil des witzes beschrieben in
;           'BEER.C'.
int08cs	dd	?			; Addr of old int 08 routine.
proc	copytoCS

	les	ax, [int08]
	mov	[word cs:int08cs+00], ax
	mov	[word cs:int08cs+02], es

	ret

endp	copytoCS


; defarm: Define a friendly object.
;          Arguments are: far pointer to a 'armstrc'.
;			 x and y as a start location.
proc	defarm	uses si, x:word, y:word, arm:far ptr

	mov	si, offset _arm
	mov	cx, MAXARMS
; Get free arm slot.
@@next:
	cmp	[(aniarm ptr si).object], -1
	je	@@found
	add	si, size aniarm
	loop	@@next
	jmp	@@error

@@found:
	les     bx, [arm]

	mov	ax, [es:(armstrc ptr bx).flags]
	mov	[(aniarm ptr si).flags], ax
	mov	ax, [es:(armstrc ptr bx).shot]
	mov	[(aniarm ptr si).shot], ax
	mov	ax, [es:(armstrc ptr bx).period]
	mov	[(aniarm ptr si).period], ax
	mov	[(aniarm ptr si).periodcnt], 0

	mov	bx, [es:(armstrc ptr bx).sprite]
	shl	bx, 1
	call	defobject, [bx + offset intindex], [x], [y], OBJ_HIGH
	mov	[(aniarm ptr si).object], ax

@@error:

	ret

endp	defarm


; defshot: Define a shot.
; Arguments:
;    shot:	Which shot should we define?
;    x, y:	Define coordinates.
;    addr:      addr of shot, that defines this shot.
proc	defshot	uses si, shot:word, x:word, y:word, addr:word

	mov	si, offset _shot
	mov	cx, MAXSHOTS
@@next:
	cmp	[(anishot ptr si).object], -1
	je	@@found
	add	si, size anishot
	loop	@@next
	jmp	@@error

@@found:
	mov	ax, 0
	cmp	si, [addr]
	ja	@@hold
	mov	ax, 1
@@hold:
	mov	[(anishot ptr si).go], ax

; Get pointer to shot struc in es:bx.
	mov	bx, [shot]
	shl	bx, 2
	les	bx, [bx + offset ptrindex]
; Copy data.
	lea	ax, [es:(shotstrc ptr bx).data]
	mov	[word (anishot ptr si+00).data], ax
	mov	[word (anishot ptr si+02).data], es
	mov	ax, [es:(shotstrc ptr bx).power]
	mov	[(anishot ptr si).power], ax
	mov	ax, [es:(shotstrc ptr bx).speed]
	mov	[(anishot ptr si).speed], ax

	mov	ax, [x]
	add	ax, [es:(shotstrc ptr bx).shotx]
	mov	dx, [y]
	add	dx, [es:(shotstrc ptr bx).shoty]
	mov	bx, [es:(shotstrc ptr bx).sprite]
	shl	bx, 1
	call	defobject, [bx + offset intindex], ax, dx, OBJ_LOW
	mov	[(anishot ptr si).object], ax

@@error:
	ret

endp	defshot



; deffoe:
; This one defines any kind of foe. (Path-followers, Shots, ...).

proc	deffoe	uses si di, foe:word, x:word, y:word, x0:word, y0:word

; Scan for a free enemy slot.

	mov	si, offset _foe
	mov	cx, MAXFOES
@@next:
	cmp	[(anifoe ptr si).object], -1
	je	@@found
	add	si, size anifoe
	loop	@@next
	jmp	@@error

@@found:
	mov	di, [foe]		; es:di = ptr to a foestrc.
	add	di, [lfoeofs]		; Add offset into pointer array.
	shl	di, 2
	les	di, [di+offset ptrindex]

; Copy flags and check type of foe.
	mov	cx, [es:(foestrc ptr di).flags]
	mov	[(anifoe ptr si).flags], cx
	test	cx, FOE_STOPCOUNT
	jz	@@m0
	mov	[frameinc], 0
@@m0:
	test	cx, FOE_TRANSPARENT or FOE_INVINCIBLE
	jnz	@@m1
; Foe can explode.
	mov	ax, [es:(foestrc ptr di).shield]
	mov	[(anifoe ptr si).shield], ax
	mov	ax, [es:(foestrc ptr di).score]
	mov	[(anifoe ptr si).score], ax
	mov	ax, [es:(foestrc ptr di).expl]
	mov	[(anifoe ptr si).expl], ax
@@m1:
	test	cx, FOE_PATH
	jz	@@m2
; Foe follows a predefined path.
	lea	bx, [es:(foestrc ptr di).path]
	mov	[word (anifoe ptr si+00h).cpath], bx
	mov	[word (anifoe ptr si+02h).cpath], es
@@m2:
	test	cx, FOE_LINE
	jz	@@m3
; Foe uses a runtime defined path (line mode).
	mov	ax, [es:(foestrc ptr di).speed]
	mov	[(anifoe ptr si).speed], ax
; Calculate line draw variables z0, z1, dx, dy, dz.
	mov	ax, [x0]			; Destination
	mov	dx, [y0]
	test	cx, FOE_PATH
	jnz	@@p0
	push	es
	call	getobjectpos, [_arm.object]
	pop	es
@@p0:
	mov	bx, 1
	sub	ax, [x]
	jns	@@p1
	neg	ax				; Must be positive.
	neg	bx
@@p1:
	mov	[(anifoe ptr si).dx], ax
	mov	[(anifoe ptr si).z0], bx
	mov	bx, 1
	sub	dx, [y]
	jns	@@p2
	neg	dx
	neg	bx
@@p2:
	mov	[(anifoe ptr si).dy], dx
	mov	[(anifoe ptr si).z1], bx

	cmp	ax, dx
	jg	@@p3
	mov	ax, dx
@@p3:
	mov	[(anifoe ptr si).pixelcnt], ax
	shr	ax, 1
	mov	[(anifoe ptr si).dz], ax
@@m3:

	mov	bx, [es:(foestrc ptr di).sprite]
	add	bx, [lsprofs]
	shl	bx, 1
	call	defobject, [bx+offset intindex], [x], [y], OBJ_LOW
	mov     [(anifoe ptr si).object], ax

@@error:
	ret

endp	deffoe


; defexpl: Define an explosion.
;          Arguments: object of impact and # of explosion.

proc	defexpl	uses si di, obj:word, explosion:word

	mov	si, offset _expl
	mov	cx, MAXEXPLS
@@next:
	cmp	[(aniexpl ptr si).object], -1
	je	@@found
	add	si, size aniexpl
	loop	@@next
	jmp	@@error

@@found:
	mov	bx, [explosion]
	add	bx, [lexplofs]
	shl	bx, 2
	les	bx, [bx + offset ptrindex]

	lea	di, [es:(explstrc ptr bx).data]
	mov	[word (aniexpl ptr si+00h).data], di
	mov	[word (aniexpl ptr si+02h).data], es

	mov	ax, [obj]
	mov	[(aniexpl ptr si).object], ax
	call	getobjectpos, ax
	mov	[(aniexpl ptr si).x], ax
	mov	[(aniexpl ptr si).y], dx
	shr	bx, 1
	add	[(aniexpl ptr si).x], bx
	shr	cx, 1
	add	[(aniexpl ptr si).y], cx

@@error:
	ret

endp	defexpl



proc	a_arm	uses si di, x:word, y:word

	mov	si, offset _arm

; Check whether main bottle is off the play field.

	mov	di, [(aniarm ptr si).object]
	test	[invincible], 2
	jz	@@m1
	call    flash, di
@@m1:
	call	getobjectpos, di
	add	ax, [x]
	cmp	ax, [windowx0]
	jge	@@ok1
	sub	ax, [windowx0]
	sub	[x], ax
	jmp	@@ok2
@@ok1:
	add	ax, bx
	cmp	ax, [windowx1]
	jle	@@ok2
	sub	ax, [windowx1]
	sub	[x], ax
@@ok2:
	add	dx, [y]
	cmp	dx, [windowy0]
	jge	@@ok3
	sub	dx, [windowy0]
	sub	[y], dx
	jmp	@@ok4
@@ok3:
	add	dx, cx
	cmp	dx, [windowy1]
	jle	@@ok4
	sub	dx, [windowy1]
	sub	[y], dx
@@ok4:

	mov	di, MAXARMS
@@next:
	mov	ax, [(aniarm ptr si).object]
	cmp	ax, -1
	je	@@skip

	call	moveobjectdelta, ax, [x], [y]

@@skip:
	add	si, size aniarm
	dec	di
	jnz	@@next

@@exit:
	ret

endp	a_arm


proc	a_shot	uses si di

local	x:word, y:word

	mov	si, offset _shot
	mov	di, MAXSHOTS
@@next:
	cmp	[(anishot ptr si).object], -1
	je	@@skip
	cmp	[(anishot ptr si).go], 1
	je	@@go
	mov	[(anishot ptr si).go], 1
	jmp	@@skip
@@go:

@@nextcoord:
	les	bx, [(anishot ptr si).data]
	mov	ax, [word es:bx + 00h]		; Read word of path array.
	mov	dx, ax
	and	dx, 0fff0h
	cmp	dx, 8000h			; Command ?
	jne	@@notcommand

@@c1:
	cmp	ax, SHOTEND
	jne	@@c2
	call	abandonobject, [(anishot ptr si).object]
	mov	[(anishot ptr si).object], -1
	jmp     @@skip
@@c2:
	cmp	ax, SHOTRELEASE
	jne	@@c3
	push	bx es
	call	getobjectpos, [(anishot ptr si).object]
	pop	es bx
	call    defshot, [word es:bx + 02], ax, dx, si
	add	[word (anishot ptr si).data], 4
	jmp	@@nextcoord
@@c3:
	cmp	ax, SHOTHOMING
	jne	@@c4
	add	[word (anishot ptr si).data], 2	; Increment ptr to new cmd.
	call	getobjectpos, [(anishot ptr si).object]
	mov	[x], ax
	mov	[y], dx
	jmp	a_shotline
@@c4:
	cmp	ax, SHOTREFLECT
	jne	@@c5
	add	[word (anishot ptr si).data], 2	; Inc command ptr.
	call	getobjectpos, [(anishot ptr si).object]
	add	ax, [(anishot ptr si).dx]
	cmp	ax, [windowx0]
	jge	@@r1
; Shot reflects at left border.
	neg	[(anishot ptr si).dx]		; Change direction.
	add	ax, [(anishot ptr si).dx]
	add	ax, [(anishot ptr si).dx]
	jmp	@@r3
@@r1:
	add	ax, bx
	cmp	ax, [windowx1]
	jle	@@r2
; Shot reflects at right border.
	neg	[(anishot ptr si).dx]
	add	ax, [(anishot ptr si).dx]
	add	ax, [(anishot ptr si).dx]
@@r2:
	sub	ax, bx
@@r3:
	add	dx, [(anishot ptr si).dy]
	cmp	dx, [windowy0]
	jge	@@r4
; Shot reflects at top border.
	neg	[(anishot ptr si).dy]		; Change direction.
	add	dx, [(anishot ptr si).dy]
	add	dx, [(anishot ptr si).dy]
	jmp	@@r6
@@r4:
	add	dx, cx
	cmp	dx, [windowy1]
	jle	@@r5
; Shot reflects at bottom border.
	neg	[(anishot ptr si).dy]
	add	dx, [(anishot ptr si).dy]
	add	dx, [(anishot ptr si).dy]
@@r5:
	sub	dx, cx
@@r6:
	call    moveobject, [(anishot ptr si).object], ax, dx
	jmp	@@inwindow
@@c5:


@@notcommand:
	add	[word (anishot ptr si).data], 4
	mov	[(anishot ptr si).dx], ax
	mov	dx, [word es:bx+02]
	mov	[(anishot ptr si).dy], dx
	call	moveobjectdelta, [(anishot ptr si).object], ax, dx

a_shot_skip:
	call    outofwindow, [(anishot ptr si).object]
	or	ax, ax
	jz	@@inwindow

	call	abandonobject, [(anishot ptr si).object]
	mov	[(anishot ptr si).object], -1

@@inwindow:
@@skip:
	add	si, size anishot
	dec	di
	jnz	@@next

	ret

endp	a_shot


proc	nolanguage a_shotline

	mov	bx, offset _foe
	mov	cx, MAXFOES
@@next:
	cmp	[(anifoe ptr bx).object], -1
	je	@@next2
	test	[(anifoe ptr bx).flags], (FOE_INVINCIBLE or FOE_TRANSPARENT)
	jz	@@found
@@next2:
	add	bx, size anifoe
	loop	@@next
	call	moveobjectdelta, [(anishot ptr si).object],\
				 [(anishot ptr si).dx],\
				 [(anishot ptr si).dy]
	jmp	a_shot_skip

@@found:
	call	getobjectpos, [(anifoe ptr bx).object]
	shr	bx, 1
	add	ax, bx				; Get mid point of object.
	shr	cx, 1
	add	dx, cx
; What's done so far: We looked for a foe to shot at and pumped the
; target coordinates into (ax, dx).

	sub	ax, [x]
	mov	bx, ax
	abs	ax
	sub	dx, [y]
	mov	cx, dx
	abs	dx

	cmp	ax, dx
	jbe	@@y

	xor	dx, dx
	div	[(anishot ptr si).speed]
	inc	ax
	xchg	ax, cx
	cwd
	idiv	cx

	mov	dx, ax
	mov	ax, [(anishot ptr si).speed]
	cmp	bx, 0
	jge	@@p1
	neg	ax
@@p1:
; done for the moment: dxnew = ax, dynew = dx.
	jmp	@@turncheck

@@y:
	mov	ax, dx
	xor	dx, dx
	div	[(anishot ptr si).speed]
	inc	ax
	xchg	ax, bx
	cwd
	idiv	bx

	mov	dx, [(anishot ptr si).speed]
	cmp	cx, 0
	jge	@@p2
	neg	dx
@@p2:
; also done. dxnew = ax, dynew = dx.


@@turncheck:
	mov	bx, [(anishot ptr si).dx]
	cmp	ax, bx
	jg	@@p3
	sub	bx, 2
@@p3:
	inc	bx

	mov	cx, [(anishot ptr si).dy]
	cmp	dx, cx
	jg	@@p4
	sub	cx, 2
@@p4:
	inc	cx

; Final deltax and deltay in ax and dx now.

	mov	[(anishot ptr si).dx], bx
	mov	[(anishot ptr si).dy], cx

	call	moveobjectdelta, [(anishot ptr si).object], bx, cx

	jmp	a_shot_skip

endp	a_shotline




;------------------------------------------------------
;Function: a_foe
;
;Description: This proc interprets the next command of
;	      each enemy in his path and performs proper
;	      action.
;------------------------------------------------------

proc	a_foe	uses bp si

	mov	si, offset _foe
	mov	bp, MAXFOES
@@next:
	push	si
	cmp	[(anifoe ptr si).object], -1
	je	@@skip

	test	[(anifoe ptr si).flags], FOE_LINE
	jnz	a_foeline

@@nextcoord:
	les	bx, [(anifoe ptr si).cpath]
	mov	ax, [word es:bx + 00h]		; Read word of path array.
	mov	dx, ax
	and	dx, 0fff0h
	cmp	dx, 8000h			; Command ?
	jne	@@notcommand

; Interpret the command given in ax.

@@c1:
	cmp	ax, FOEENDPATH
	jne	@@c2
; Object has reached end of path and must be destroyed.
	call	abandonobject, [(anifoe ptr si).object]
	mov	[(anifoe ptr si).object], -1
	jmp	@@skip
@@c2:
	cmp	ax, FOECHANGESPRITE
	jne	@@c3
; Object wants to change it's look.
	mov	bx, [word es:bx + 02h]
	add	bx, [lsprofs]
	shl	bx, 1
	call	changesprite, [(anifoe ptr si).object], [bx+offset intindex]
	add	[word (anifoe ptr si).cpath], 4
	jmp	@@nextcoord
@@c3:
	cmp	ax, FOERELEASEFOE
	jne	@@c4
; Object releases another object.
	push	bx es
	call	getobjectpos, [(anifoe ptr si).object]
	pop	es bx
	add	ax, [word es:bx + 04h]
	add	dx, [word es:bx + 06h]
	call    deffoe, [word es:bx + 02], ax, dx
	add	[word (anifoe ptr si).cpath], 8
	jmp	@@nextcoord
@@c4:
	cmp	ax, FOECYCLEPATH
	jne	@@c5
	les	bx, [(anifoe ptr si).path]
	mov	[word (anifoe ptr si + 00h).cpath], bx
	mov	[word (anifoe ptr si + 02h).cpath], es
	call	moveobject, [(anifoe ptr si).object],\
			    [(anifoe ptr si).savex], [(anifoe ptr si).savey]
	jmp	@@nextcoord
@@c5:
	cmp	ax, FOEMARK
	jne	@@c6
	add	bx, 2
	mov	[word (anifoe ptr si+00h).path], bx
	mov	[word (anifoe ptr si+02h).path], es
	mov	[word (anifoe ptr si).cpath], bx
	call	getobjectpos, [(anifoe ptr si).object]
	mov	[(anifoe ptr si).savex], ax
	mov	[(anifoe ptr si).savey], dx
	jmp	@@nextcoord
@@c6:
	cmp	ax, FOESOUND
	jne	@@c7
	mov	bx, [word es:bx + 02h]
	add	bx, [lsndofs]
	shl	bx, 2
	les	bx, [bx + offset ptrindex]
	call	playsample, es bx
	add	[word (anifoe ptr si).cpath], 4
	jmp	@@nextcoord
@@c7:
	jmp	@@skip

@@notcommand:
	add	[word (anifoe ptr si).cpath], 4
	call	moveobjectdelta, [(anifoe ptr si).object], ax, [word es:bx + 02h]

a_foe_skip:
@@skip:
	pop	si
	add	si, size anifoe
	dec	bp
	jnz	@@next

	ret

endp	a_foe


proc	nolanguage a_foeline

	call	getobjectpos, [(anifoe ptr si).object]
	mov	cx, [(anifoe ptr si).speed]
	mov	bx, [(anifoe ptr si).dz]
@@loop:
	cmp	bx, [(anifoe ptr si).dx]
	jge	@@m1
	add	bx, [(anifoe ptr si).dy]
	add	ax, [(anifoe ptr si).z0]
@@m1:
	cmp	bx, [(anifoe ptr si).dx]
	jl	@@m2
	sub	bx, [(anifoe ptr si).dx]
	add	dx, [(anifoe ptr si).z1]
@@m2:
	test	[(anifoe ptr si).flags], FOE_PATH
	jz	@@m3
	dec	[(anifoe ptr si).pixelcnt]
	jnz	@@m3
; Target coordinates have been reached.
	and	[(anifoe ptr si).flags], not FOE_LINE
	call	moveobject, [(anifoe ptr si).object], ax, dx
	jmp	a_foe_skip
@@m3:

	loop	@@loop

	mov	[(anifoe ptr si).dz], bx

	call	moveobject, [(anifoe ptr si).object], ax, dx

	call	outofwindow, [(anifoe ptr si).object]
	or	ax, ax
	jz	@@inwindow

	call	abandonobject, [(anifoe ptr si).object]
	mov	[(anifoe ptr si).object], -1

@@inwindow:

	jmp	a_foe_skip

endp	a_foeline



proc	a_expl	uses bp si

	mov	si, offset _expl
	mov	bp, MAXEXPLS
@@next:
	cmp	[(aniexpl ptr si).object], -1
	je	@@skip

@@nextcommand:
	les	bx, [(aniexpl ptr si).data]
	add	[word (aniexpl ptr si).data], 2
	mov	ax, [word es:bx]		; Fetch command.

	cmp	ax, EXPLEND
	jne	@@c1
	mov	[(aniexpl ptr si).object], -1	; Destroy explosion.
	jmp	@@skip
@@c1:
	cmp	ax, EXPLNEW
	jne	@@c2
	mov	ax, [(aniexpl ptr si).x]
	mov	dx, [(aniexpl ptr si).y]
	add	ax, [word es:bx + 04h]
	add	dx, [word es:bx + 06h]
	mov	bx, [word es:bx + 02h]
	add	bx, [lsprofs]
	shl	bx, 1
	call	defobject, [bx + offset intindex],\
			   ax, dx, OBJ_HIGH or OBJ_ONECYCLE
	add	[word (aniexpl ptr si).data], 6
	jmp	@@nextcommand
@@c2:
	cmp	ax, EXPLWAIT
	jne	@@c3
	jmp	@@skip
@@c3:
	cmp	ax, EXPLSOUND
	jne	@@c4
	mov	bx, [word es:bx + 02h]
	add	bx, [lsndofs]
	shl	bx, 2
	les	bx, [bx + offset ptrindex]
	call	playsample, es bx
	add	[word (aniexpl ptr si).data], 2
	jmp	@@nextcommand
@@c4:
	cmp	ax, EXPLREMOVEOBJ
	jne	@@c5
	call	abandonobject, [(aniexpl ptr si).object]
	jmp	@@nextcommand
@@c5:
	cmp	ax, EXPLRELEASEFOE
	jne	@@c6
	mov	ax, [(aniexpl ptr si).x]
	mov	dx, [(aniexpl ptr si).y]
	add	ax, [word es:bx + 04h]
	add	dx, [word es:bx + 06h]
	call    deffoe, [word es:bx + 02], ax, dx
	add	[word (aniexpl ptr si).data], 6
	jmp	@@nextcommand
@@c6:
	cmp	ax, EXPLNEWPATH
	jne	@@c7
	mov	ax, [(aniexpl ptr si).x]
	mov	dx, [(aniexpl ptr si).y]
	add	ax, [word es:bx + 04h]
	add	dx, [word es:bx + 06h]
	call    deffoe, [word es:bx + 02h], ax, dx,\
			[word es:bx + 08h], [word es:bx + 0ah]
	add	[word (aniexpl ptr si).data], 0ah
	jmp	@@nextcommand
@@c7:

@@skip:
	add	si, size aniexpl
	dec	bp
	jnz	@@next

	ret

endp	a_expl



; foehit: manages bullet vs. enemy crashes.
;
proc	foehit	uses bp si di

	mov	si, offset _foe
	mov	cx, MAXFOES

@@next1:
	push	cx

	cmp	[(anifoe ptr si).object], -1
	je	@@skip1
	test	[(anifoe ptr si).flags], FOE_TRANSPARENT
	jnz	@@skip1

	mov	di, offset _shot
	mov	bp, MAXSHOTS

@@next2:
	cmp	[(anishot ptr di).object], -1
	je	@@skip2

	call	crashtest, [(anifoe ptr si).object], [(anishot ptr di).object]
	or	ax, ax
	jz	@@nohit

; Enemy pointed by si has been hit by shot pointed by di. (real great)

	test	[(anifoe ptr si).flags], FOE_INVINCIBLE
	jz	@@goon
	mov	[(anishot ptr di).power], 0	; Kill shot.
	jmp	@@foealive

@@goon:
	call	flash, [(anifoe ptr si).object]

	mov	ax, [(anifoe ptr si).shield]	; Power of foe.
	mov	bx, [(anishot ptr di).power]	; Power of shot.

	sub	[(anishot ptr di).power], ax
	sub	[(anifoe ptr si).shield], bx
	ja	@@foealive

; Add score.
	mov	dx, [(anifoe ptr si).score]
	add	[word score+00], dx
	adc	[word score+02], 0

; Do explosion.
	call	defexpl, [(anifoe ptr si).object], [(anifoe ptr si).expl]
	mov	[(anifoe ptr si).object], -1

	mov	ax, [(anifoe ptr si).flags]
	test	ax, FOE_ENDLEVEL		; Was it a big boss ?
	jz	@@m1
	dec	[nbigboss]

@@m1:
	test	ax, FOE_STOPCOUNT		; Check for stopped counter.
	jz	@@foealive
	mov	[frameinc], 1

@@foealive:
	cmp	[(anishot ptr di).power], 0
	jg	@@shotalive
	call	abandonobject, [(anishot ptr di).object]
	mov	[(anishot ptr di).object], -1

@@shotalive:

@@nohit:
@@skip2:
	add	di, size anishot

	dec	bp
	jnz	@@next2

@@skip1:
	add	si, size anifoe

	pop	cx
	loop	@@next1

@@exit:
	ret

endp	foehit



; armhit: checks whether the heroic bottle of cool fresh Eichhof
;         Lager has been hit by a nasty enemy beer.
;
; RETURNS: ax = 1 if hit.

proc	armhit	uses bp si di

	cmp	[invincible], 0
	jne	@@nohit

; First we check whether Eichli has been hit by a hostile shot.

	mov	si, offset _foe
	mov	bp, MAXFOES

	mov	di, [_arm.object]

@@next1:
	cmp	[(anifoe ptr si).object], -1
	je	@@skip1

	call	crashtest, [(anifoe ptr si).object], di
	or	ax, ax
	jz	@@skip1

; Eichli has been hit
@@hit:
	call	defexpl, di, 0		; Do a beautiful explosion.
; Destroy all weapons.
	mov	di, offset _arm
	mov	si, MAXARMS
@@next2:
	mov	ax, [(aniarm ptr di).object]
	mov	[(aniarm ptr di).object], -1
	cmp	ax, -1
	je	@@skip2
	call	abandonobject, ax
@@skip2:
	add	di, size aniarm
	dec	si
	jnz	@@next2
	mov	ax, 1			; Indiacte destroyed Eichli.
	jmp	@@exit

@@skip1:
	add	si, size anifoe
	dec	bp
	jnz	@@next1
@@nohit:
	mov	ax, 0
@@exit:
	ret

endp	armhit



; fire: Voll power aus allen Rohren.
;       al == TRUE  : es soll geschossen werden.
;       al == FALSE : Friede. Kein schuss soll fallen.

proc	fire	uses bp si

	mov	si, offset _arm
	mov	bp, MAXARMS
	cmp	al, FALSE
	jne	@@fire

@@next1:
	cmp	[(aniarm ptr si).object], -1
	je	@@skip1
	cmp	[(aniarm ptr si).periodcnt], 0	; Gun already reloaded ?
	je	@@skip1
	dec	[(aniarm ptr si).periodcnt]	; Decrement reload delay.
@@skip1:
	add	si, size aniarm
	dec	bp
	jnz	@@next1

	jmp	@@exit

@@fire:

@@next2:
	cmp	[(aniarm ptr si).object], -1	; Is weapon defined ?
	je	@@skip2

	sub	[(aniarm ptr si).periodcnt], 1	; Ready to shot ?
	jnc	@@skip2
	mov	ax, [(aniarm ptr si).period]	; Restore reload delay.
	mov	[(aniarm ptr si).periodcnt], ax

	call	getobjectpos, [(aniarm ptr si).object]
	call	defshot, [(aniarm ptr si).shot], ax, dx, 0ffffh

@@skip2:
	add	si, size aniarm
	dec	bp
	jnz	@@next2

@@exit:
	ret

endp	fire



; keyboard: Translates the pressed keys into action.

proc	keyboard


	mov	bx, [key_fire]			; Get defined 'FIRE' key.
	mov	al, [byte key + bx]             ; Is key pressed ?
	call	fire

@@ns:
	xor	ax, ax
	xor	dx, dx

	mov	bx, [key_left]
	cmp	[byte key + bx], TRUE		; left
	jne	@@nl
	sub	ax, [shipspeed]

@@nl:
	mov	bx, [key_right]
	cmp	[byte key + bx], TRUE		; right
	jne	@@nr
	add	ax, [shipspeed]

@@nr:
	mov	bx, [key_up]
	cmp	[byte key + bx], TRUE		; up
	jne	@@nu
	sub	dx, [shipspeed]

@@nu:
	mov	bx, [key_down]
	cmp	[byte key + bx], TRUE		; down
	jne	@@nd
	add	dx, [shipspeed]

@@nd:
	mov	bx, [key_pause]			; Pause
	cmp	[byte key + bx], TRUE
	jne	@@np
@@wait1:
	cmp	[pressedkeys], 0		; Keys must be released first.
	jne	@@wait1
@@wait2:
	cmp	[pressedkeys], 0		; Wait for any key.
	je	@@wait2
	shl	[byte key + bx], 1

@@np:

	call	a_arm, ax, dx

	ret

endp	keyboard


; dispscore
; show player's current score.

proc	dispscore

	push	[windowy1]
	mov	[windowy1], YMAX		; Remove clipping.

@@startover:
	mov	ax, [scoredigit]
	cmp	ax, 0
	jne	@@skip1
	les	bx, [score]			; Revitalize everything.
	mov	[word playscore+00], bx
	mov	[word playscore+02], es
	mov     [scorepos], BARSCOREX
	jmp	@@disp
@@skip1:
	cmp	ax, 4
	jne	@@skip2
	sub	[scorepos], 4         		; Jump over point.
	jmp	@@disp
@@skip2:
	cmp	ax, 16d
	jne	@@disp
	mov	[scoredigit], 0
	jmp	@@startover

@@disp:
; First we clear the digit that should be drawn afterwards.
; Note that we can't use removesprite to do this job, because
; removesprite would delete a box with x-size as a multiple of four.
; So we have to use a blank sprite (BARCLRDIGIT).
	call	putsprite, [barfonthandle], [scorepos], BARSCOREY, BARCLRDIGIT
; Now we calculate the number to be drawn.
	mov	ax, [word playscore+02]	; High word of remaining number.
	xor	dx, dx
	div	[const10d]		; Divide by ten : remainder in dx.
	mov	cx, ax
	mov	ax, [word playscore+00]
	div	[const10d]		; (remainder:low word)/10.
; Each digit must be drawn twice, on page 0 and 1.
; So we must check whether it have already been drawn once.
	test	[scoredigit], 1
	jz	@@notsave		; Jump if first time we draw it.
	mov	[word playscore+00], ax ; Store new score.
	mov	[word playscore+02], cx
	call	putsprite, [barfonthandle], [scorepos], BARSCOREY, dx
	sub	[scorepos], BARSCORESPC ; Advance to next digit.
	jmp	@@ret
@@notsave:
	call	putsprite, [barfonthandle], [scorepos], BARSCOREY, dx

@@ret:
	inc	[scoredigit]

	pop	[windowy1]
	ret

endp	dispscore


proc	displifes uses si di

	push	[windowy1]
	mov	[windowy1], YMAX

	mov	si, 6
	mov	di, BARLIFEX

@@next1:
	call	putsprite, [barfonthandle], di, BARSCOREY, BARCLRDIGIT
	sub	di, 4

	dec	si
	jnz	@@next1

	mov	si, [lifes]
	cmp	si, 6
	jbe	@@ok
	mov	si, 6
@@ok:
	mov	di, BARLIFEX
@@next2:
	sub	si, 1			; Requires carry detection.
	jc	@@done

	call	putsprite, [barfonthandle], di, BARSCOREY, BARLIFEDIGIT
	sub	di, 4

	jmp	@@next2

@@done:
	pop	[windowy1]
	ret

endp	displifes


;
; wait till the next timer tick occurs.
;
proc	waitfortick

@@wait:
	cmp	[tick], TRUE
	jne	@@wait
	mov	[tick], FALSE

	ret

endp	waitfortick


;
; set a starting position of a level.
;
proc	setplayposition,	nattacks:word, \
				attack:far ptr, \
				nbigb:word

	mov	ax, [nattacks]
	mov	[lastposition.nattacks], ax
	les	ax, [attack]
	mov     [word (lastposition + 00).attack], ax
	mov	[word (lastposition + 02).attack], es
	mov	ax, [nbigb]
	mov	[lastposition.nbigboss], ax
	les	ax, [score]
	mov	[word (lastposition+00).score], ax
	mov	[word (lastposition+02).score], es

	mov	[lastposition.count], 0

	ret

endp	setplayposition


proc	play	uses si di

local	terminate:word, nattacks:word

	les	ax, [lastposition.score]
	mov	[word score + 00], ax
	mov	[word score + 02], es
	mov	ax, [lastposition.nbigboss]
	mov	[nbigboss], ax
	mov	ax, [lastposition.nattacks]
	mov	[nattacks], ax
	mov	di, [lastposition.count]
	mov	si, [word (lastposition + 00).attack]
	mov	[frameinc], 1
	mov	[scoredigit], 0

	mov	[invincible], SHIELD_CTIME	; First Eichli is invincible.
	mov	[terminate], 0		; No reason to terminate.

@@next:
	cmp	[nattacks], 0
	je	@@skip

	mov	es, [word (lastposition + 02).attack]

	mov	ax, [es:(attackstrc ptr si).count]
	cmp	ax, di
	jne	@@skip

	mov	ax, [es:(attackstrc ptr si).foe]
	test	ax, A_COMMAND
	jz	@@nocommand

	cmp	ax, A_GOFIELD
	jne	@@c2
	call	gostarfield
	add	si, size attackstrc
	jmp	@@next
@@c2:
	cmp	ax, A_STOPFIELD
	jne	@@c3
	call	stopstarfield
	add	si, size attackstrc
	jmp	@@next
@@c3:
	cmp	ax, A_SOUND
	jne	@@c4
	mov	bx, [es:(attackstrc ptr si).x]
	add	bx, [lsndofs]
	shl	bx, 2
	les	bx, [bx + offset ptrindex]
	call	playsample, es bx
	add	si, size attackstrc
	jmp	@@next
@@c4:
	cmp	ax, A_MARK
	jne	@@c5
	mov	ax, [nattacks]
	mov	[lastposition.nattacks], ax
	mov     [word (lastposition + 00).attack], si
	mov	ax, [nbigboss]
	mov	[lastposition.nbigboss], ax
	mov	[lastposition.count], di
	les	ax, [score]
	mov	[word (lastposition+00).score], ax
	mov	[word (lastposition+02).score], es
	add	si, size attackstrc
	jmp	@@next
@@c5:
@@nocommand:
	call	deffoe, ax,\
			[es:(attackstrc ptr si).x],\
			[es:(attackstrc ptr si).y]
	dec	[nattacks]
	add	si, size attackstrc
	jmp	@@next

@@skip:

; ESC key test.
	cmp	[byte key + KEYESC], TRUE
	jne	@@go_on
	mov	[lifes], -1		; Request game abort.
	jmp	@@bottlehit

@@go_on:
	call	keyboard
	call	waitfortick
	call	updatescreen
	call	foehit
	cmp	[nbigboss], 0		; Any Big Boss left ?
	jne	@@s3
	mov	[nbigboss], 1
	mov	[invincible], WIN_CTIME
	mov	[terminate], 1		; All Endlevel monsters killed.
@@s3:
	test	[cheatlevel], CHEATCRASH; Fair-Play?
	jnz	@@s2
	call	armhit
	or	ax, ax
	jz	@@s2
	mov	[invincible], LOSE_CTIME
	mov	[terminate], 2		; Eichli destroyed.
@@s2:
	call	dispscore
	call	a_shot
	call	a_foe
	call	a_expl

	add	di, [frameinc]		; Inrement frame count.

	cmp	[invincible], 0
	je	@@next
	dec	[invincible]
	jnz	@@next
	mov	ax, [terminate]
	or	ax, ax
	jz	@@next
	cmp	ax, 1
	jne	@@bottlehit

	call	haltsound
	mov	ax, 1			; next level
	jmp	@@exit

@@bottlehit:
	call	haltsound
	mov	ax, 0
@@exit:
	ret

endp	play


proc	nolanguage newint09	far

	push	ax bx ds

	in	al, 60h
	mov	bl, al
	in	al, 61h
	mov	ah, al
	or	al, 80h
	out	61h, al
	mov	al, ah
	out	61h, al

	mov	ax, DGROUP
	mov	ds, ax

	test	bl, 80h
	jz	@@press
	and	bx, 7fh
	cmp	[byte key + bx], FALSE
	je	@@skip1
	mov	[byte key + bx], FALSE
	dec	[pressedkeys]
@@skip1:
	jmp	short @@exit

@@press:
	and	bx, 7fh
	cmp	[byte key + bx], FALSE
	jne	@@skip2
	mov	[byte key + bx], TRUE
	inc	[pressedkeys]
@@skip2:

@@exit:
	mov	al, 20h
	out	20h, al

; Check for Ctrl-Alt-Del
	cmp	[byte key+01dh], TRUE	; Ctrl
	jne	@@e1
	cmp	[byte key+038h], TRUE	; Alt
	jne	@@e1
	cmp	[byte key+053h], TRUE	; Del
	jne	@@e1

	call	error, 0 0, -1		; Do error with code -1.

@@e1:
	pop	ds bx ax

	iret

endp	newint09


proc	nolanguage newint08	far

	push	ax ds

	mov	ax, DGROUP
	mov	ds, ax

	mov	[tick], TRUE

	pop	ds ax

	jmp	[dword cs:int08cs]

endp	newint08



	end



