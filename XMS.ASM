;
; XMS support library.
;
; [c] copyright 1993 by Alpha-Helix
; written by Dany Schoch
;

	IDEAL
	P286N
	JUMPS

	MODEL	compact, c

include "xms.ash"

	dataseg

xmsentry	dd	?		; Entry point of xms driver.
xmsparam	dd	?		; Length of block.
		dw	?		; Source XMS handle.
		dd	?		; 32 offset within source.
		dw	?		; Destination block.
		dd	?		; 32 offset within destination.



	codeseg


proc	initXMS

; Check if driver available.
	mov	ax, 4300h		; Get install state.
	int	2fh
	cmp	al, 80h			; status = installed ?
	jne	@@absent

	mov	ax, 4310h		; Get driver entry point.
	int	2fh
	mov	[word xmsentry+00], bx
	mov	[word xmsentry+02], es

	mov	ax, 1			; return ok.
	jmp	@@exit

@@absent:
	mov	ax, 0

@@exit:
	ret

endp	initXMS


proc	shutXMS

	mov	ax, 1
	ret

endp	shutXMS



; Gets size of the largest free XMS in kB.
proc	getfreeXMS

	mov	ah, 08h
	call	[xmsentry]

	ret

endp	getfreeXMS



; Tries to allocate size kilobytes of xms memory.
; On success: handle will be returned. Otherwise: -1.
proc	allocXMS,	size:word

	mov	ah, 09h
	mov	dx, [size]
	call	[xmsentry]

	or	ax, ax			; Check for error.
	mov	ax, dx
	jnz	@@exit
	mov	ax, -1

@@exit:
	ret

endp	allocXMS


proc	freeXMS,	handle:word

	mov	ah, 0ah
	mov	dx, [handle]
	call	[xmsentry]

	ret

endp	freeXMS


; Move a 'size' bytes from conventional to extended memory.
; src : normal far ptr into conventional memory.
; dst : handle of destination block.
; size: number of bytes to move. Must be even.

proc	movememtoXMS uses si,	src:far ptr, dst:word, size:dword

	les	ax, [size]
	and	ax, 0fffeh              ; Make even.
	mov	[word xmsparam+00h], ax
	mov	[word xmsparam+02h], es
	mov	[word xmsparam+04h], 0	; No source handle for conv. memory.
	les	ax, [src]
	mov	[word xmsparam+06h], ax
	mov	[word xmsparam+08h], es
	mov	ax, [dst]
	mov	[word xmsparam+0ah], ax
	mov	[word xmsparam+0ch], 0	; Offset 0 into destination block.
	mov	[word xmsparam+0eh], 0

	mov	ah, 0bh
	mov	si, offset xmsparam
	call	[xmsentry]

	ret

endp	movememtoXMS


proc	moveXMStomem uses si,	src:word, dst:far ptr, size:dword

	les	ax, [size]
	and	ax, 0fffeh              ; Make even.
	mov	[word xmsparam+00h], ax
	mov	[word xmsparam+02h], es
	mov	ax, [src]
	mov	[word xmsparam+04h], ax
	mov	[word xmsparam+06h], 0
	mov	[word xmsparam+08h], 0
	mov	[word xmsparam+0ah], 0	; No dest handle.
	les	ax, [dst]
	mov	[word xmsparam+0ch], ax
	mov	[word xmsparam+0eh], es

	mov	ah, 0bh
	mov	si, offset xmsparam
	call	[xmsentry]

	ret

endp	moveXMStomem



; Move any memory at any size from anywhere to anywhere.
; Except overlapping blocks won't do.
proc	moveXMS uses si,	src:word, saddr:far ptr, \
				dst:word, daddr:far ptr, size:dword

	les	ax, [size]
	and	ax, 0fffeh              ; Make even.
	mov	[word xmsparam+00h], ax
	mov	[word xmsparam+02h], es
	mov	ax, [src]
	mov	[word xmsparam+04h], ax
	les	ax, [saddr]
	mov	[word xmsparam+06h], ax
	mov	[word xmsparam+08h], es
	mov	ax, [dst]
	mov	[word xmsparam+0ah], ax
	les	ax, [daddr]
	mov	[word xmsparam+0ch], ax
	mov	[word xmsparam+0eh], es

	mov	ah, 0bh
	mov	si, offset xmsparam
	call	[xmsentry]

	ret

endp	moveXMS


; Locks XMS and returns 32-bit linear address of locked block.
; 0 if unsuccessful.
proc	lockXMS,	handle:word

	mov	ah, 0ch
	mov	dx, [handle]
	call	[xmsentry]
	or	ax, ax			; Check for error.
	mov	ax, bx
	jnz	@@exit
	xor	ax, ax			; Null pointer to indicate error.
	xor	dx, dx
@@exit:
	ret

endp	lockXMS


proc	unlockXMS,	handle:word

	mov	ah, 0dh
	mov	dx, [handle]
	call	[xmsentry]

	ret

endp	unlockXMS



; get size of allocated XMS. -1 on error.
proc	getXMSsize,	handle:word

	mov	ah, 0eh
	mov	dx, [handle]
	call	[xmsentry]

	or	ax, ax
	mov	ax, dx
	jnz	@@exit
	mov	ax, -1
@@exit:
	ret

endp	getXMSsize



	end



