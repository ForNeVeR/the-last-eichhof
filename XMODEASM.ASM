;---------------------------------------------------------;
;                                                         ;
;                                                         ;
;   This module contains GRAPHICS PRIMITIVES for          ;
;   VGA undocumented 320x240 256 color mode.              ;
;                                                         ;
;       written by Dany Schoch for Alpha-Helix            ;
;   Parts of this code first appeard in Dr.Dobb's         ;
;   September 1991 issue (during my military service).    ;
;                                                         ;




	IDEAL
	P286N
	JUMPS

	MODEL	compact, c

include "xmode.ash"


	dataseg

; CRT port parameters to switch to 320x240 256c mode.

CRTParm	dw	00b06h			; Vertical total.
	dw	03e07h			; Overflow.
	dw	04109h			; Cell height (1 to double scan).
	dw	0d610h			; v sync start.
	dw	08c11h			; v sync end and protect cr0-cr7.
	dw	0b712h			; Vertical display.
	dw	00014h			; turn off dword mode.
	dw	0bf15h			; v blank start.
	dw	00416h			; v blank end.
	dw	0e317h			; turn on byte mode.
CRT_PARM_LENGTH	=	(($-CRTParm)/2)


	codeseg


; Uses int 10h function 0 to set a video mode.
proc	screenmode	mode:word

	mov	ax, [mode]
	int	10h

	ret

endp	screenmode


proc	setxmode	uses si di

	mov	ax, 13h			; Let the BIOS set standard
	int	10h			;   mode 320x200 linear.

	mov	dx, SC_INDEX
	mov	ax, 0604h
	out	dx, ax			; disable chain4 mode.
	mov	ax, 0100h
	out	dx, ax			; Synchronous reset while
					;   switching clock.

	mov	dx, MISC_OUTPUT
	mov	al, 0e3h
	out	dx, al			; Select 25MHz dot clock & 60Hz
					;   scanning rate.

	mov	dx, SC_INDEX
	mov	ax, 0300h
	out	dx, ax			; Undo reset (restart sequencer).

	mov	dx, CRTC_INDEX
	mov	al, 11h
	out	dx, al
	inc	dx
	in	al, dx
	and	al,7fh
	out	dx, al
	dec	dx
	cld
	mov	si, offset CRTParm
	mov	cx, CRT_PARM_LENGTH
@@loop:
	lodsw
	out	dx, ax
	loop	@@loop

;	mov	ax, 1001h		; set overscan color.
;	mov	bh, 0eh
;	int	10h

	mov	dx, SC_INDEX
	mov	ax, 0f02h
	out	dx, ax
	mov	es, [base]
	sub	di, di
	sub	ax, ax
	mov	cx, 8000h
	rep	stosw

	ret

endp	setxmode


; VGApresent is taken from the book "PC&PS/2 Video Systems" published
; by Microsoft Press.
; VGApresent returns a non zero value if a VGA or MCGA was found.
proc	VGApresent

	mov	ax, 1a00h
	int	10h                     ; call video bios for info.

	cmp	al, 1ah
	je	@@exit			; Exit if VGA or MCGA detected.

	xor	ax, ax

@@exit:
	ret

endp	VGApresent


proc	plot,	x:word, y:word, c:word

	mov	ax, BYTESPERLINE
	mul	[y]
	mov	bx, [x]
	shr	bx, 1
	shr	bx, 1
	add	bx, ax
	mov	es, [base]

	mov	cl, [byte x]
	and	cl, 011b
	mov	ax, 0100h + MAP_MASK
	shl	ah, cl
	mov	dx, SC_INDEX
	out	dx, ax
	mov	dx, GC_INDEX
	mov	ax, 0ff00h + BIT_MASK
	out	dx, ax

	mov	al, [byte c]
	mov	[es:bx], al

	ret

endp	plot


proc	getpixel,	x:word, y:word

	mov	ax, BYTESPERLINE
	mul	[y]
	mov	bx, [x]
	shr	bx, 1
	shr	bx, 1
	add	bx, ax
	mov	es, [base]

	mov	ah, [byte x]
	and	ah, 011b
	mov	al, 04h
	mov	dx, GC_INDEX
	out	dx, ax

	mov	al, [es:bx]
	sub	ah, ah

	ret

endp	getpixel


proc	gostarfield	uses si di

	mov	[_sfield.go], TRUE

	mov	dx, GC_INDEX
	mov	ax, 0ff00h + BIT_MASK
	out	dx, ax

	mov	dx, SC_INDEX
	mov	ax, 0100h + MAP_MASK
	out	dx, ax

	mov	ax, BYTESPERLINE
	mul	[windowy1]

	mov	es, [base]
	mov	di, [page]              ; Flip page address so di is
	xor	di, PAGESIZE            ;   pointing to the hidden screen.
	mov	dl, [byte backgrndcolor]
; Clear stars on second page.
	mov	cx, [_sfield.n]
	lds	si, [_sfield.star]

@@next:
	mov	bx, [(starstrc ptr si).x]
	add	bx, [(starstrc ptr si).speed]
	cmp	bx, ax
	jb	@@skip
	sub	bx, ax
@@skip:
	mov	[es:bx+di], dl		; Clear pixel.
	add	si, size starstrc
	loop	@@next

	mov	ax, DGROUP
	mov	ds, ax

	ret

endp	gostarfield


proc	stopstarfield	uses si di

	mov	[_sfield.go], FALSE

	mov	dx, GC_INDEX
	mov	ax, 0ff00h + BIT_MASK
	out	dx, ax

	mov	dx, SC_INDEX
	mov	ax, 0100h + MAP_MASK
	out	dx, ax

	mov	es, [base]
	mov	di, [page]              ; Flip page address so di is
	xor	di, PAGESIZE            ;   pointing to the hidden screen.
	mov	dl, [byte backgrndcolor]
; Clear stars on second page.
	mov	cx, [_sfield.n]
	lds	si, [_sfield.star]
@@next:
	mov	bx, [(starstrc ptr si).x]
	mov	[byte es:bx+di], dl	; Clear pixel.
	add	si, size starstrc
	loop	@@next

	mov	ax, DGROUP
	mov	ds, ax

	ret

endp	stopstarfield


proc	starfield uses si di

	cmp	[_sfield.active], FALSE
	je	@@exit

	mov	dx, GC_INDEX
	mov	ax, 0ff00h + BIT_MASK
	out	dx, ax

	mov	dx, SC_INDEX
	mov	ax, 0100h + MAP_MASK
	out	dx, ax

	mov	es, [base]

	mov	ax, BYTESPERLINE
	mul	[windowy1]

	mov	di, [page]
	mov	cx, [_sfield.n]
	mov	dl, [byte backgrndcolor]	; Preload backgroundcolor.

	cmp	[_sfield.go], TRUE
	jne	@@stay

	lds	si, [_sfield.star]
@@next1:
	mov	bx, [(starstrc ptr si).x]
	mov	[byte es:bx+di], dl	; Clear pixel.

	add	bx, [(starstrc ptr si).speed]
	cmp	bx, ax				; Check for wrap around.
	jb	@@skip1
	sub	bx, ax
@@skip1:
	mov	[(starstrc ptr si).x], bx
	add	bx, [(starstrc ptr si).speed]
	cmp	bx, ax
	jb	@@skip2
	sub	bx, ax
@@skip2:
	mov	dh, [byte (starstrc ptr si).color]
	mov	[es:bx+di], dh		; Set pixel.

	add	si, size starstrc
	loop	@@next1
	jmp	@@exit

@@stay:
	lds	si, [_sfield.star]

@@next2:
	mov	bx, [(starstrc ptr si).x]
	add	bx, [(starstrc ptr si).speed]
	cmp	bx, ax
	jb	@@skip3
	sub	bx, ax
@@skip3:
	mov	dh, [byte (starstrc ptr si).color]
	mov	[es:bx+di], dh		; Set pixel.
	add	si, size starstrc
	loop	@@next2

@@exit:
	mov	ax, DGROUP
	mov	ds, ax

	ret

endp	starfield


; defobject.
; This sub defines an animated object.
; args: A HANDLE of a sprite and the xstart and ystart coords.
; ret : HANDLE if free slot found or else ...  have a guess.

proc	defobject uses si,	sprite:word, x:word, y:word, flags:word

	mov	dx, 3

@@start:
	dec	dx
	mov	ax, -1
	jz	@@ret

	mov	si, (offset _obj) + (MAXOBJS/2)*size objstrc
	mov	cx, MAXOBJS/2
	mov	ax, -size objstrc
	test	[flags], OBJ_HIGH
	jz	@@next
	mov	ax, size objstrc
@@next:
	cmp	[(objstrc ptr si).active], FALSE
	je	@@found
	add	si, ax
	dec	cx
	jnz	@@next
	xor	[flags], OBJ_HIGH		; Invert priority bit.
	jmp	@@start

@@found:

	mov	ax, [sprite]
	mov	[(objstrc ptr si).sprite], ax
	call	getspritesize,	ax
	mov	[(objstrc ptr si).xs], ax
	mov	[(objstrc ptr si).ys], dx
	mov	[(objstrc ptr si).nadd], bx
	mov	[(objstrc ptr si).maxn], cx
	mov	[(objstrc ptr si).n], 0

	mov	ax, [x]
	mov	[(objstrc ptr si).x], ax
	mov	[(objstrc ptr si).xa], ax
	mov	[(objstrc ptr si).xb], ax
	mov	bx, [y]
	mov	[(objstrc ptr si).y], bx
	mov	[(objstrc ptr si).ya], bx
	mov	[(objstrc ptr si).yb], bx
	mov	ax, [flags]
	and	ax, OBJ_ONECYCLE
	mov	[(objstrc ptr si).cycle], ax
	mov	[(objstrc ptr si).flags], 0

	mov	[(objstrc ptr si).destroy], 0
	mov	[(objstrc ptr si).active], TRUE
	mov	ax, si
	sub	ax, offset _obj
	mov	bl, size objstrc
	div	bl

@@ret:
	ret

endp	defobject


; moveobject.
; Moves the OBJECT 'obj' to the given location.
; RETURNS: ax = 0 if sprite cycled. ax != 0 otherwise.

proc	moveobject,	obj:word, x:word, y:word

	mov	bx, [obj]
	imul	bx, size objstrc

	add	bx, offset _obj

	mov	ax, [x]
	mov	[(objstrc ptr bx).x], ax
	mov	ax, [y]
	mov	[(objstrc ptr bx).y], ax

	ret

endp	moveobject


; moveobjectdelta.
; Moves the OBJECT 'obj' relative by the given values.

proc	moveobjectdelta,	obj:word, deltax:word, deltay:word

	mov	bx, [obj]
	MASM
	imul	bx, type objstrc
	IDEAL

	add	bx, offset _obj

	mov	ax, [deltax]
	add	[(objstrc ptr bx).x], ax
	mov	ax, [deltay]
	add	[(objstrc ptr bx).y], ax


	ret

endp	moveobjectdelta


; Set the object's flash bit. This will paint it
; in the current flashcolor for one frame.
proc	flash,	obj:word

	mov	bx, [obj]
	imul	bx, size objstrc
	add	bx, offset _obj

	or	[(objstrc ptr bx).flags], O_FLASH

	ret

endp	flash


; Change the object's look.
proc	changesprite uses si,	obj:word, sprite:word

	mov	si, [obj]
	imul	si, size objstrc
	add	si, offset _obj

	mov	ax, [sprite]
	mov	[(objstrc ptr si).sprite], ax
	call	getspritesize, ax
	mov	[(objstrc ptr si).maxn], cx
	mov	[(objstrc ptr si).n], 0
	mov	[(objstrc ptr si).xs], ax
	mov	[(objstrc ptr si).ys], dx

	ret

endp	changesprite


; Kills object and removes it from both screen pages during next
; two calls to 'updatescreen'.
proc	abandonobject, 	obj:word

	mov	bx, [obj]
	imul	bx, size objstrc
	add	bx, offset _obj

	mov	[(objstrc ptr bx).destroy], 2

	ret

endp	abandonobject


; Kills object and imediately removes it from both screen pages.
proc	killobject,	obj:word

	mov	bx, [obj]
	imul	bx, size objstrc
	add	bx, offset _obj

	call	removesprite, [(objstrc ptr bx).sprite],\
			      [(objstrc ptr bx).x],\
			      [(objstrc ptr bx).y]
	xor	[page], PAGESIZE	; Flip pages.
	call	removesprite, [(objstrc ptr bx).sprite],\
			      [(objstrc ptr bx).xa],\
			      [(objstrc ptr bx).ya]
	xor	[page], PAGESIZE
	mov	[(objstrc ptr bx).active], FALSE

	ret

endp	killobject


proc	killallobjects uses si di

	mov	si, MAXOBJS
	mov	di, offset _obj

@@next:
	mov	[(objstrc ptr di).active], FALSE

	add	di, size objstrc

	dec	si
	jnz	@@next

	ret

endp	killallobjects


; crashtest.
; This procy tests whether the given objects 'obj1' and 'obj2'
; are overlapping each other. If so 1 will be returned, otherwise 0.

proc	crashtest uses si di,	obj1:word, obj2:word

	mov	si, [obj1]
	imul	si, size objstrc
	add	si, offset _obj
	mov	di, [obj2]
	imul	di, size objstrc
	add	di, offset _obj

; Check x coords.

	mov	ax, [(objstrc ptr si).x]
	mov	bx, ax
	add	bx, [(objstrc ptr si).xs]
	sub	ax, [(objstrc ptr di).xs]
	mov	dx, [(objstrc ptr di).x]
	cmp	ax, dx
	jg	@@skip2
	cmp	bx, dx
	jl	@@skip2

; Check y coords.

	mov	ax, [(objstrc ptr si).y]
	mov	bx, ax
	add	bx, [(objstrc ptr si).ys]
	sub	ax, [(objstrc ptr di).ys]
	mov	dx, [(objstrc ptr di).y]
	cmp	ax, dx
	jg	@@skip2
	cmp	bx, dx
	jl	@@skip2

	mov	ax, 1			; H I T  !!!!!
	jmp	@@exit

@@skip2:
	xor	ax, ax

@@exit:
	ret

endp	crashtest



; getobjectpos: returns current (x, y) coordinates and size (xs, ys)
;               of object obj.
;		(x, y) go into (ax, dx).
;		(xs, ys) will be returned in (bx, cx).

proc	getobjectpos,	obj:word

	mov	bx, [obj]
	imul	bx, size objstrc
	add	bx, offset _obj

	mov	ax, [(objstrc ptr bx).xa]	; x pos.
	mov	dx, [(objstrc ptr bx).ya]       ; y pos;
	mov	cx, [(objstrc ptr bx).ys]       ; y size.
	mov	bx, [(objstrc ptr bx).xs]	; x size.

	ret

endp	getobjectpos


proc	getobjectsize,	obj:word

	mov	bx, [obj]
	imul	bx, size objstrc
	add	bx, offset _obj

	mov	ax, [(objstrc ptr bx).xs]
	mov	dx, [(objstrc ptr bx).ys]

	ret

endp	getobjectsize


proc	getspritesize,	handle:word

	mov	bx, [handle]
	imul	bx, size lowspr
	add	bx, offset _sprite

	mov	ax, [(lowspr ptr bx).xs]
	mov	dx, [(lowspr ptr bx).ys]
	mov	cx, [(lowspr ptr bx).maxn]
	mov	bx, [(lowspr ptr bx).nadd]

	ret

endp	getspritesize


; Checks whether the object is out of the currently defined
; action window.
proc	outofwindow,	obj:word

	mov	bx, [obj]
	imul	bx, size objstrc
	add	bx, offset _obj

	mov	ax, [(objstrc ptr bx).x]
	cmp	ax, [windowx0]
	jl	@@out
	cmp	ax, [windowx1]
	jg	@@out

	mov	ax, [(objstrc ptr bx).y]
	cmp	ax, [windowy0]
	jl	@@out
	cmp	ax, [windowy1]
	jg	@@out

	xor	ax, ax
	jmp	@@exit

@@out:
	mov	ax, 1

@@exit:
	ret

endp	outofwindow


; showpage: displays the page (0 or 1) on the screen.

proc	showpage,	p:word

	mov	ax, [p]
	imul	ax, PAGESIZE

	call	showofs, ax

	ret

endp	showpage


proc    showline,	line:word

	mov	ax, [line]
	imul	ax, BYTESPERLINE

	call	showofs, ax

	ret

endp	showline


proc	showofs,	ofs:word

; Wait for display enable to be active, to be sure
;   both halfes of the start address will take in the same frame.

	mov	bl, START_ADDR_LOW
	mov	bh, [byte ofs]
	mov	cl, START_ADDR_HIGH
	mov	ch, [byte ofs + 01h]

	mov	dx, INPUT_STATUS

@@wait1:
	in	al, dx
	test	al, 01h
	jnz	@@wait1

	mov	dx, CRTC_INDEX
	mov	ax, bx
	out	dx, ax
	mov	ax, cx
	out	dx, ax

; Now wait for vertical sync, so the other page will be invisible if
;   we start draw to it.

	mov	dx, INPUT_STATUS

@@wait2:
	in	al, dx
	test	al, 08h
	jz	@@wait2

	ret

endp	showofs


; putspritedirect.
; Without the aid of defsprite directly draw a sprite.
; This routine doesn't perform any clipping.
; The sprite will be draw to BOTH screens.

proc	putspritedirect uses si di,	sprite:far ptr, x:word, y:word, n:word

local	xs:word, ofs:word

	mov	ax, 0ff00h + BIT_MASK
	mov	dx, GC_INDEX
	out	dx, ax

	mov	cl, [byte x]
	and	cl, 011b		; Calculate Map.
	mov	bh, 00010001b
	shl	bh, cl

; Calculate screen memory location of first write. (es:di)
	mov	ax, BYTESPERLINE
	mul	[y]
	mov	di, [x]
	shr	di, 2
	add	di, ax
	mov	es, [base]

	lds	si, [sprite]
	mov	ax, [(sprstrc ptr si).xs]
	mov	[xs], ax
	mov	dx, [(sprstrc ptr si).ys]
	mov	bl, dl
	mul	dx
	xor	dx, dx
	mul	[n]
	lea	si, [(sprstrc ptr si).data]
	add	si, ax
	mov	dx, SC_INDEX

; Now:
; bl : y loop counter
; cx : x loop counter
; al : alround
; ah : MAP_MASK value that will be updated every dot.
; bh : MAP_MASK of the first x coordinate.
; dx : Port addr.

@@yloop:
	mov	cx, [xs]
	mov	ah, bh
	mov	[ofs], di
@@xloop:
	mov	al, MAP_MASK
	out	dx, al
	mov	al, ah
	and	al, 0fh
	inc	dx
	out	dx, al
	dec	dx
	lodsb
	mov	[byte es:di], al
	add	di, PAGESIZE
	mov	[byte es:di], al
	sub	di, PAGESIZE
	rol	ah, 1
	adc	di, 0
	dec	cx
	jnz	@@xloop

	mov	di, BYTESPERLINE
	add	di, [ofs]
	dec	bl
	jnz	@@yloop

	mov	ax, DGROUP
	mov	ds, ax

	ret

endp	putspritedirect


; putsprite.
; Hence, as the name says, this routine displays the sprite 'handle'
; at coords 'x' and 'y'.
;
; NOTE: x coordinate will be truncated according to the 'align' member
;       of the 'lowspr' structure.

proc	putsprite uses si di,	handle:word, x:word, y:word, n:word

local	nx:word, ny:word, nxs:word, nys:word, cspace:word

; First clipping will be performed.

	mov	si, [handle]
	imul	si, size lowspr
	add	si, offset _sprite

	mov	ax, [x]
	and	ax, 0fffch		; Truncate to a multiple of 4
	cmp	ax, [windowx0]
	jge	@@p1
	mov	di, [windowx0]
	neg	ax
	add	ax, di
	mov	dx, [(lowspr ptr si).xsalign]
	cmp	ax, dx
	jae	@@remove
	sub	dx, ax
	shr	ax, 2
	mov	[cspace], ax
	mov	bx, ax
	mov	[nxs], dx
	mov	[nx], di
	jmp	@@ycheck
@@p1:
	mov	cx, ax
	mov	dx, [(lowspr ptr si).xsalign]
	add	cx, dx
	cmp	cx, [windowx1]
	jl	@@p2
	mov	di, [windowx1]
	cmp	ax, di
	jge	@@remove
	mov	[nx], ax
	inc	di
	sub	di, ax
	mov	[nxs], di
	sub	dx, di
	shr	dx, 2
	mov	[cspace], dx
	xor	bx, bx
	jmp	@@ycheck
@@p2:
	mov	[nx], ax
	mov	[nxs], dx
	mov	[cspace], 0
	xor	bx, bx

@@ycheck:
	mov     ax, [y]
	cmp	ax, [windowy0]
	jge	@@p3
	mov	di, [windowy0]
	neg	ax
	add	ax, di
	mov	dx, [(lowspr ptr si).ys]
	cmp	ax, dx
	jae	@@remove
	sub	dx, ax
	mov	[nys], dx
	mov	cx, [(lowspr ptr si).xsalign]
	shr	cx, 2
	imul	cx
	add	bx, ax
	mov	[ny], di
	jmp	@@donecheck
@@p3:
	mov	cx, ax
	mov	dx, [(lowspr ptr si).ys]
	add	cx, dx
	cmp	cx, [windowy1]
	jl	@@p4
	mov	di, [windowy1]
	cmp	ax, di
	jge	@@remove
	mov	[ny], ax
	sub	di, ax
	mov	[nys], di
	jmp	@@donecheck
@@p4:
	mov	[ny], ax
	mov	[nys], dx

@@donecheck:

; Truncate clipped coordinated to a multiple of 4.
	shr	[nx], 2
	shr	[nxs], 2

	mov	di, [ny]
	imul	di, BYTESPERLINE
	add	di, [nx]			; es:di -> screen memory
	add	di, [page]

	mov	es, [base]

	mov	ax, [n]
	mul     [word (lowspr ptr si).picsize]
	mov	cx, [word (lowspr ptr si).data]
	test	[x], 10b
	jz	@@align4
	add	ax, [(lowspr ptr si).seqsize]
@@align4:
	lds	si, [(lowspr ptr si).mask]	;
	add	bx, ax
	add	si, bx				; ds:si -> mask data.
	add	bx, cx

	mov	dx, GC_INDEX
	mov	ax, 0000h + BIT_MASK
	out	dx, ax

	mov	dx, SC_INDEX
	mov	ax, 0000h + MAP_MASK
	out	dx, ax
	inc	dx

	cld
	mov	ch, [byte nys]
@@next1:
	mov	cl, [byte nxs]

@@next2:
	lodsb
	cmp	ah, al
	je	@@skip
	mov	ah, al
	out	dx, al
@@skip:
	mov	al, [es:bx]
	inc	bx
	stosb

	dec	cl
	jnz	@@next2

	add	di, BYTESPERLINE
	sub	di, [nxs]
	add	bx, [cspace]
	add	si, [cspace]

	dec	ch
	jnz	@@next1

@@remove:
	mov	ax, DGROUP
	mov	ds, ax

	ret

endp	putsprite


	public	putflash
proc	putflash uses si di,	handle:word, x:word, y:word, n:word

local	nx:word, ny:word, nxs:word, nys:word, cspace:word

; First clipping will be performed.

	mov	si, [handle]
	imul	si, size lowspr
	add	si, offset _sprite

	mov	ax, [x]
	and	ax, 0fffch		; Truncate to a multiple of 4
	cmp	ax, [windowx0]
	jge	@@p1
	mov	di, [windowx0]
	neg	ax
	add	ax, di
	mov	dx, [(lowspr ptr si).xsalign]
	cmp	ax, dx
	jae	@@remove
	sub	dx, ax
	shr	ax, 2
	mov	[cspace], ax
	mov	bx, ax
	mov	[nxs], dx
	mov	[nx], di
	jmp	@@ycheck
@@p1:
	mov	cx, ax
	mov	dx, [(lowspr ptr si).xsalign]
	add	cx, dx
	cmp	cx, [windowx1]
	jl	@@p2
	mov	di, [windowx1]
	cmp	ax, di
	jge	@@remove
	mov	[nx], ax
	inc	di
	sub	di, ax
	mov	[nxs], di
	sub	dx, di
	shr	dx, 2
	mov	[cspace], dx
	xor	bx, bx
	jmp	@@ycheck
@@p2:
	mov	[nx], ax
	mov	[nxs], dx
	mov	[cspace], 0
	xor	bx, bx

@@ycheck:
	mov     ax, [y]
	cmp	ax, [windowy0]
	jge	@@p3
	mov	di, [windowy0]
	neg	ax
	add	ax, di
	mov	dx, [(lowspr ptr si).ys]
	cmp	ax, dx
	jae	@@remove
	sub	dx, ax
	mov	[nys], dx
	mov	cx, [(lowspr ptr si).xsalign]
	shr	cx, 2
	imul	cx
	add	bx, ax
	mov	[ny], di
	jmp	@@donecheck
@@p3:
	mov	cx, ax
	mov	dx, [(lowspr ptr si).ys]
	add	cx, dx
	cmp	cx, [windowy1]
	jl	@@p4
	mov	di, [windowy1]
	cmp	ax, di
	jge	@@remove
	mov	[ny], ax
	sub	di, ax
	mov	[nys], di
	jmp	@@donecheck
@@p4:
	mov	[ny], ax
	mov	[nys], dx

@@donecheck:


; Truncate clipped coordinated to a multiple of 4.
	shr	[nx], 2
	shr	[nxs], 2

	mov	di, [ny]
	imul	di, BYTESPERLINE
	add	di, [nx]			; es:di -> screen memory
	add	di, [page]

	mov	es, [base]

	mov	ax, [n]
	mul     [word (lowspr ptr si).picsize]
	mov	cx, [word (lowspr ptr si).data]
	test	[x], 10b
	jz	@@align4
	add	ax, [(lowspr ptr si).seqsize]
@@align4:
	mov	cl, [byte objflashcolor]	; Preload flashcolor.
	lds	si, [(lowspr ptr si).mask]	;
	add	bx, ax
	add	si, bx				; ds:si -> mask data.

	mov	dx, GC_INDEX
	mov	ax, 0ff00h + BIT_MASK
	out	dx, ax

	mov	dx, SC_INDEX
	mov	ax, 0f00h + MAP_MASK
	out	dx, ax
	inc	dx

	cld
	mov	bh, [byte nys]
@@next1:
	mov	bl, [byte nxs]

@@next2:
	lodsb
	cmp	ah, al
	je	@@skip
	mov	ah, al
	out	dx, al
@@skip:
	mov	al, cl
	stosb

	dec	bl
	jnz	@@next2

	add	di, BYTESPERLINE
	sub	di, [nxs]
	add	si, [cspace]

	dec	bh
	jnz	@@next1

@@remove:
	mov	ax, DGROUP
	mov	ds, ax

	ret

endp	putflash


proc	removesprite uses si di,	handle:word, x:word, y:word

local	nx:word, ny:word, nxs:word, nys:word

; First clipping will be performed.

	mov	si, [handle]
	imul	si, size lowspr
	add	si, offset _sprite

	mov	ax, [x]
	and	ax, 0fffch
	cmp	ax, [windowx0]
	jge	@@p1
	mov	di, [windowx0]
	neg	ax
	add	ax, di
	mov	dx, [(lowspr ptr si).xsalign]
	cmp	ax, dx
	jae	@@remove
	sub	dx, ax
	mov	[nxs], dx
	mov	[nx], di
	jmp	@@ycheck
@@p1:
	mov	cx, ax
	mov	dx, [(lowspr ptr si).xsalign]
	add	cx, dx
	cmp	cx, [windowx1]
	jl	@@p2
	mov	di, [windowx1]
	cmp	ax, di
	jge	@@remove
	mov	[nx], ax
	inc	di
	sub	di, ax
	mov	[nxs], di
	jmp	@@ycheck
@@p2:
	mov	[nx], ax
	mov	[nxs], dx

@@ycheck:
	mov     ax, [y]
	cmp	ax, [windowy0]
	jge	@@p3
	mov	di, [windowy0]
	neg	ax
	add	ax, di
	mov	dx, [(lowspr ptr si).ys]
	cmp	ax, dx
	jae	@@remove
	sub	dx, ax
	mov	[nys], dx
	mov	[ny], di
	jmp	@@donecheck
@@p3:
	mov	cx, ax
	mov	dx, [(lowspr ptr si).ys]
	add	cx, dx
	cmp	cx, [windowy1]
	jl	@@p4
	mov	di, [windowy1]
	cmp	ax, di
	jge	@@remove
	mov	[ny], ax
	sub	di, ax
	mov	[nys], di
	jmp	@@donecheck
@@p4:
	mov	[ny], ax
	mov	[nys], dx

@@donecheck:


; Truncate clipped coordinated to a multiple of 4.
	shr	[nx], 2
	shr	[nxs], 2

	mov	es, [base]

	mov	di, [ny]
	imul	di, BYTESPERLINE
	add	di, [nx]			; es:di -> screen memory
	add	di, [page]			; Second Page.

	mov	dx, GC_INDEX
	mov	ax, 0ff00h + BIT_MASK
	out	dx, ax
	mov	dx, SC_INDEX
	mov	ax, 0f00h + MAP_MASK
	out	dx, ax

	mov	al, [byte backgrndcolor]
	cld
	mov	dx, [nys]
@@next1:
	mov	cx, [nxs]

	rep	stosb

	add	di, BYTESPERLINE
	sub	di, [nxs]

	dec	dx
	jnz	@@next1

@@remove:
	ret

endp	removesprite



; updatescreen.
; updatescreen removes the old objects from the backscreen and
; redraws them at the latest positions.

proc	updatescreen uses bp si

	xor	[page], PAGESIZE

	call	starfield

	mov	si, offset _obj
	mov	bp, MAXOBJS
@@next1:
	cmp	[(objstrc ptr si).active], FALSE
	je	@@skip1

@@cycle:
	mov	ax, [(objstrc ptr si).n]
	add	ax, [(objstrc ptr si).nadd]
	cmp	ax, [(objstrc ptr si).maxn]
	jb	@@ok
	xor	ax, ax
	cmp	[(objstrc ptr si).cycle], 0
	je	@@ok
	mov	[(objstrc ptr si).destroy], 2
	mov	[(objstrc ptr si).cycle], 0
@@ok:
	mov	[(objstrc ptr si).n], ax

	call	removesprite, [(objstrc ptr si).sprite], [(objstrc ptr si).xb], [(objstrc ptr si).yb]
	cmp     [(objstrc ptr si).destroy], 0
	je	@@skip1
	dec	[(objstrc ptr si).destroy]
	jnz	@@skip1
	mov	[(objstrc ptr si).active], FALSE

@@skip1:
	add	si, size objstrc
	dec	bp
	jnz	@@next1

	mov	si, offset _obj
	mov	bp, MAXOBJS
@@next2:
	cmp	[(objstrc ptr si).active], FALSE
	je	@@skip2

	mov	ax, [(objstrc ptr si).xa]
	mov	[(objstrc ptr si).xb], ax
	mov	ax, [(objstrc ptr si).ya]
	mov	[(objstrc ptr si).yb], ax
	mov	ax, [(objstrc ptr si).x]
	mov	[(objstrc ptr si).xa], ax
	mov	ax, [(objstrc ptr si).y]
	mov	[(objstrc ptr si).ya], ax

	cmp	[(objstrc ptr si).destroy], 0
	jne	@@skip2

; Draw new object.
	mov	ax, [(objstrc ptr si).n]
	shr	ax, 1
	test	[(objstrc ptr si).flags], O_FLASH
	jnz	@@flash
	call	putsprite, [(objstrc ptr si).sprite], [(objstrc ptr si).x], [(objstrc ptr si).y], ax
	jmp	@@skip2
@@flash:
	call	putflash, [(objstrc ptr si).sprite], [(objstrc ptr si).x], [(objstrc ptr si).y], ax
	and	[(objstrc ptr si).flags], not O_FLASH

@@skip2:
	add	si, size objstrc
	dec	bp
	jnz	@@next2

	call	showofs, [page]

	ret

endp	updatescreen



; copypage: Copies a screen page to another.
;	    scr: source page (0, or 1)
;           dst: destination page (0 or 1)

proc	copypage uses si di ds,	scr:word, dst:word

	mov	si, [scr]
	imul	si, PAGESIZE
	mov	di, [dst]
	imul	di, PAGESIZE

	mov	dx, GC_INDEX
	mov	ax, 0000h + BIT_MASK
	out	dx, ax

	mov	dx, SC_INDEX
	mov	ax, 0f00h + MAP_MASK
	out	dx, ax
	inc	dx

	mov	ax, [base]
	mov	es, ax
	mov	ds, ax

	mov	cx, PAGESIZE
	cld
	rep	movsb

	ret

endp	copypage


; Slowly fades out the screen.
proc	glowout	uses si

	cld
	sub	bx, bx
@@m1:
	mov	si, offset palette
@@m2:

; Wait for vertical retrace.
	mov	dx, INPUT_STATUS
@@wait:
	in	al, dx
	test	al, 08h
	jz	@@wait

; Select starting color.
	mov	dx, 3c8h
	mov	al, bl
	out	dx, al

; Start moving color datas.
	inc	dx
	mov	cx, 80h
@@loop:
	lodsb                           ; Fetch original red value and ...
	sub	al, bh                  ; ...calculate new color value by
					;   simply subtract the loop counter.
	jnc	@@r
	sub	al, al
@@r:
	out	dx, al			; And out to the controller.
	lodsb        			; Fetch green.
	sub	al, bh
	jnc	@@g
	sub	al, al
@@g:
	out	dx, al
	lodsb				; And blue.
	sub	al, bh
	jnc	@@b
	sub	al, al
@@b:
	out	dx, al

	loop	@@loop

	add	bl, 80h
	jnz	@@m2

	add	bh, 6
	cmp	bh, 3fh
	jbe	@@m1

	ret

endp	glowout


proc	glowin	uses si,	dir:word

	cld
	mov	bx, 3c00h
@@m1:
	mov	si, offset palette
@@m2:

; Wait for vertical retrace.
	mov	dx, INPUT_STATUS
@@wait:
	in	al, dx
	test	al, 08h
	jz	@@wait

; Select starting color.
	mov	dx, 3c8h
	mov	al, bl
	out	dx, al

; Start moving color datas.
	inc	dx
	mov	cx, 80h
@@loop:
	lodsb                           ; Fetch original red value and ...
	sub	al, bh                  ; ...calculate new color value by
					;   simply subtract the loop counter.
	jnc	@@r
	sub	al, al
@@r:
	out	dx, al			; And out to the controller.
	lodsb        			; Fetch green.
	sub	al, bh
	jnc	@@g
	sub	al, al
@@g:
	out	dx, al
	lodsb				; And blue.
	sub	al, bh
	jnc	@@b
	sub	al, al
@@b:
	out	dx, al

	loop	@@loop

	add	bl, 80h
	jnz	@@m2

	sub	bh, 6
	jnc	@@m1


	ret

endp	glowin


	end


