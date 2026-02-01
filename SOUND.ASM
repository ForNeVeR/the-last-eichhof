
; SoundBlaster support library.
; Written by Dany Schoch.
; [c] copyright 1993 by alpha-helix.
; SPDX-FileCopyrightText: 1993 alpha-helix <http://www.ife.ee.ethz.ch/~ammann/alphahelix/>
; SPDX-FileCopyrightText: 1993-1995 Dany Schoch <dschoch@stud.ee.ethz.ch>
; SPDX-License-Identifier: LicenseRef-TLE

	IDEAL
	P286N
	JUMPS

	MODEL	compact, c

DMACHUNKSIZE		equ	8000h	; Bytes being transfered in one
					;   DMA operation.

; Sound Blaster equates.

DSP_RESET		equ	006h
FM_STATUS		equ	008h
FM_ADDRESS		equ	008h
FM_DATA			equ	009h
DSP_READ_DATA		equ	00Ah
DSP_WRITE_DATA		equ	00Ch
DSP_WRITE_STATUS	equ	00Ch
DSP_DATA_AVAIL		equ	00Eh

ADLIB_FM_STATUS		equ	0388h
ADLIB_FM_ADDRESS	equ	0388h
ADLIB_FM_DATA		equ	0389h


; DSP Commands
DIRECT_8_BIT_DAC	equ	010h
DMA_8_BIT_DAC		equ	014h
DMA_2_BIT_DAC		equ	016h
DMA_2_BIT_REF_DAC	equ	017h
DIRECT_ADC		equ	020h
DMA_ADC			equ	024h
TIME_CONSTANT		equ	040h
DMA_4_BIT_DAC	    	equ	074h
DMA_4_BIT_REF_DAC   	equ	075h
DMA_26_BIT_DAC	    	equ	076h
DMA_26_BIT_REF_DAC  	equ	077h
HALT_DMA	    	equ	0D0h
CONTINUE_DMA	    	equ	0D4h
SPEAKER_ON	    	equ	0D1h
SPEAKER_OFF	    	equ	0D3h
DSP_ID		    	equ	0E0h
DSP_VER 	    	equ	0E1h
MDAC1		    	equ	061h
MDAC2		    	equ	062h
MDAC3		    	equ	063h
MDAC4		    	equ	064h
MDAC5		    	equ	065h
MDAC6		    	equ	066h
MDAC7		    	equ	067h


; dmachan equates

DMA_COMMAND	    EQU     008h
DMA_STATUS          EQU     008h
DMA_MODE            EQU     00bh
DMA_REQUEST         EQU     009h
DMA_MASK            EQU     00ah
DMAC_MEMTOMEM       EQU     001h
DMAC_CH0HOLD        EQU     002h
DMAC_NOCONTROLLE    EQU     004h
DMAC_BURST          EQU     008h
DMAC_ROTATING       EQU     010h
DMAC_EXTENDEDWRI    EQU     020h
DMAC_DREQLOW        EQU     040h
DMAC_DACKHIGH       EQU     080h
DMAM_VERIFY         EQU     0
DMAM_WRITE          EQU     004h
DMAM_READ           EQU     008h
DMAM_AUTOINIT       EQU     010h
DMAM_INCADDR        EQU     0
DMAM_DECADDR        EQU     020h
DMAM_DEMAND         EQU     0
DMAM_SINGLE         EQU     040h
DMAM_BLOCK          EQU     080h
DMAR_RESETREQUES    EQU     0
DMAR_SETREQUEST     EQU     004h
DMAM_CLEARMASK      EQU     0
DMAM_SETMASK        EQU     004h
DMA_CLEARFLIPFLOP   EQU     00ch
DMA_ADDR            EQU     0
DMA_COUNT           EQU     001h


; Dos function call eqautes.
DOS_READ		equ	3f00h
DOS_LSEEK		equ	4200h


; Sound mode and flag equates
SND_FILE		equ	01h	; Playing sound from a file.
SND_MEM			equ	02h	; Playing sound from memory.
SND_LOOP		equ	04h	; Play looping.
SND_NOREAD		equ	01h	; No buffer read allowed.
SND_READPENDING		equ	02h	; Data must be read immediately.
SND_EOF			equ	04h	; End of sound file reached.

include	"sound.ash"

	dataseg
; DMA page table registers.
pagetable	dw	87h, 83h, 81h, 82h

; Possible Soundblaster settings.
iocheck		dw	220h, 240h, 210h, 230h, 250h, 260h, -1
irqcheck	db	7, 5, 2, 3, -1	; Irqs to check for sb use.

; Card parameters.
ioaddr		dw	?		; i/o base addr of soundblaster.
irq		dw	?		; IRQ used by soundblaster.
dmachan		dw	01h             ; DMA channel to shuffle data.
board		dw	0		; Sound board installed ?
soundon		dw	0		; Sound on or off ?

; Allround variables.
mode		dw	?		; Play mode.
samplerate	dw	0		; Current sample rate.
command		dw	?		; Play Command for sample.
priority	dw	?               ; Priority of sample being played.
dlen		dd	?		; Samplelength still to be played.
addr		dd	?		; Physical addr of remaining sample.
oldirq		dd	?               ; Addr of original irq.
oldmask		dw	?		; Original irq mask.

; Variables used for playfile.
filehandle	dw	?		; FileHandle returned by dos_open.
fptr		dd	?		; File pointer to start of data.
header		sndstrc	?		; Header of song.
flen		dd	?		; N of bytes not read yet.
slen		dd	?		; N of bytes not played yet.
buffer0		dd	?		; Ptr to first halfe of file buffer.
physbuffer0	dd	?		; 20 bit physical addr of buffer 0.
buffer1		dd	?		; Ptr to second halfe.
physbuffer1	dd	?		; 20 bit physical addr of buffer 1.
bsize		dw	?		; Buffer size in bytes.
just_played	dw	?		; buffer0 or buffer1 finished playing.
readstate	dw	?		; Buffer Read status flag.


; Define a useful macro for writing data to the DAC
; NOTE: Be aware that ax and dx will be destroyed by using this macro.

macro	outdac x
local	skip
	mov	dx, [ioaddr]
	add	dx, DSP_WRITE_STATUS
skip:
	in	al, dx
	test	al, 080h
	jnz	skip

ife (DSP_WRITE_DATA eq DSP_WRITE_STATUS)
	add	dx, (DSP_WRITE_DATA - DSP_WRITE_STATUS)
endif
	mov	al, x
	out	dx, al

endm	outdac


	codeseg


proc	nolanguage _play near

	mov	si, [word addr + 00]	; Get phisical addr in cl:si.
	mov	cl, [byte addr + 02]

	xor     di, di                  ; Calculate DMA limit
	sub	di, si			; di = number of bytes that could
					;   be transfered in this 64kB chunk.
	jz	@@m1			; If di == 0 -> use DMACHUNKSIZE.
	cmp	di, DMACHUNKSIZE
	jb	@@m2
@@m1:
	mov	di, DMACHUNKSIZE
@@m2:
; di now containes the number of bytes we maximally can transfer.
	mov	ax, [word dlen + 00h]
	sub	[word dlen + 00h], di	; Calculate new sample length.
	sbb	[word dlen + 02h], 0	;
	jnc     @@l1
	mov	di, ax
	mov	[word dlen + 00h], 0
	mov	[word dlen + 02h], 0
@@l1:
	sub	[word slen + 00h], di	; Subtract number of bytes that
	sbb	[word slen + 02h], 0    ;   will be played till next intr.
	jnc	@@l2
	mov	[word slen + 00h], 0
	mov	[word slen + 02h], 0
@@l2:
	add	[word addr + 00], di
	adc	[byte addr + 02], 0


; At this moment things are like this:
; cl:si      -> 20 bit physical address of sample data.
; di         -> length of sample in this 64kB window.

	dec	di
; Set up dmachan
	cli				; No interrupts now.
	mov	al, DMAM_SETMASK	; Mask channel, so we can freely
	or	al, [byte dmachan]      ; program it.
	mov	dx, DMA_MASK
	out	dx, al

	mov	dx, DMA_CLEARFLIPFLOP	; Clear address flipflop by writing
	out	dx, al                  ; any value to it.
	mov	dx, [dmachan]
	shl	dx, 1			; Port addr of dmachannel.
	mov	bx, dx
	add	dx, DMA_ADDR
; Now data address goes out.
	mov	ax, si
	out     dx, al
	mov	al, ah
	out	dx, al
	mov	dx, [bx + pagetable]	; Fetch addr of page register.
	mov	al, cl
	out	dx, al			; MS 4 bits also go out.
; It's time to write the length of the transfer.
	mov	dx, bx
	add	dx, DMA_COUNT
	mov	ax, di
	out	dx, al
	mov	al, ah
	out	dx, al
; Select Mode of dmachan transfer
	mov	dx, DMA_MODE
	mov	al, DMAM_READ or DMAM_SINGLE or DMAM_INCADDR;
	or	al, [byte dmachan]
	out	dx, al
; Unmask channel.
	mov	dx, DMA_MASK
	mov	al, DMAM_CLEARMASK
	or	al, [byte dmachan]
	out	dx, al

; Let's tell Sound Blaster what to do.
	outdac  <[byte command]>
	mov	bx, di
	outdac  bl
	outdac	bh
	sti				; Interrupts are welcome again.

; Just use reference byte the first time.
	cmp	[command], DMA_4_BIT_REF_DAC
	jne	@@exit
	mov	[command], DMA_4_BIT_DAC

@@exit:
	ret

endp	_play


proc	nolanguage _load near

	test	[readstate], SND_NOREAD	; Is disk access allowed?
	jz	@@diskok
	or	[readstate], SND_READPENDING
	jmp     @@exit
@@diskok:
	mov	ax, DOS_READ
	mov	bx, [filehandle]
	mov	cx, [bsize]
	push	ds
	cmp	[just_played], 0	; Which buffer has to be read.
	je	@@11
	lds	dx, [buffer0]
	jmp	@@m1
@@11:
	lds	dx, [buffer1]
@@m1:
	mov	si, dx			; Store buffer addr for later use.
	mov	di, ds
	int	21h
	pop	ds

	mov	ax, [bsize]
	sub	[word flen+00], ax
	sbb	[word flen+02], 0
	jnc	@@exit			; Jump if flen >= 0
	or	[readstate], SND_EOF	; Indicate EOF reached.

; The whole file has been read. So we have to start over and fill
; the rest of the buffer with data from the beginning of the file.
	mov	ax, DOS_LSEEK + 00h	; Seek from file start.
	mov	bx, [filehandle]
	mov	dx, [word fptr+00]
	mov	cx, [word fptr+02]
	int	21h

	mov	ax, DOS_READ
	mov	bx, [filehandle]
	mov	cx, [word flen+00]
	neg	cx
	les	dx, [header.len]
	mov	[word flen+00], dx	; N of bytes still to read.
	mov	[word flen+02], es
	add	si, [bsize]
	sub	si, cx
	push	ds
	mov	dx, si
	mov	ds, di
	int	21h
	pop	ds

	sub	[word flen+00], ax
	sbb	[word flen+02], 0

@@exit:
	ret

endp	_load


; Interrupt handler for dmachan complete irq from Soundblaster
proc	nolanguage newirq

	push	ax bx cx dx si di ds es

	mov	ax, DGROUP
	mov	ds, ax

; Acknowledge interrupt.
	mov	dx, [ioaddr]
	add	dx, DSP_DATA_AVAIL
	in	al, dx			; Soundblaster.

	mov	al, 20h			; Send irq done to PIC.
	cmp	[irq], 07h		; IRQ occured on second controller ?
	jbe	@@skip
	out	0a0h, al		; Acknoledge IRQ on second controller.
@@skip:
	out	020h, al		; Acknoledge IRQ on first controller.

	test	[mode], SND_FILE
	jz	@@m0

; Play sound from a file.

	test	[mode], SND_LOOP	; Skip 'slen test' if looping.
	jnz	@@m3
; Check for finished playing file.
	cmp	[word slen+00], 0
	jne	@@m3
	cmp	[word slen+02], 0
	jne	@@m3
	mov	[priority], 0
	jmp	@@exit
@@m3:
	cmp	[word dlen+00], 0	; Buffer exhausted?
	jne	@@go

; Buffer is empty, so we have to do a fast switch to the other one.
	cmp	[just_played], 0	; Which buffer?
	je	@@1
; Play buffer0.
	les	bx, [physbuffer0]	; Get physical addr of buffer0
	jmp	@@m1
; Play buffer1.
@@1:
	les	bx, [physbuffer1]
@@m1:
	mov	[word addr+00], bx	; Store buffer address.
	mov	[word addr+02], es
; Compute the length of the partial sample.
	test	[mode], SND_LOOP
	jz	@@noloop
@@m2:
	mov	ax, [bsize]
	mov	[word dlen+00], ax
	mov	[word dlen+02], 0	; Never more than 64kB, so this is
					;   always 0.
	call	_play
	xor	[just_played], 1
; Fill exhausted buffer with cool fresh samples.
	call	_load
	jmp	@@exit

@@noloop:
	test	[readstate], SND_EOF
	jz	@@m2
	mov	ax, [word slen+00]	; Get tail length of sample.
	mov	[word dlen+00], ax
	mov	[word dlen+02], 0	; Never more than 64kB, so this is
	call	_play
	jmp	@@exit

@@m0:
	cmp	[word dlen + 00], 0
	jne	@@go
	cmp	[word dlen + 02], 0
	jne	@@go
	test	[mode], SND_LOOP
	jnz	@@loop
	mov	[priority], 0
	jmp	@@exit

@@loop:
	les	ax, [physbuffer0]
	mov	[word addr+00], ax
	mov	[word addr+02], es
	les	ax, [header.len]
	mov	[word dlen+00], ax
	mov	[word dlen+02], es

@@go:
	call	_play

@@exit:

	pop	es ds di si dx cx bx ax
	iret

endp	newirq


; Sets the sample rate in kHz to be used for digitising or playback */

proc	setsamplerate,	rate:word

; Set new sample rate if necessary.
	mov	bx, [rate]
	cmp	bx, [samplerate]
	je	@@noadjust
	mov	[samplerate], bx	; Store new sample rate.
	outdac	TIME_CONSTANT		; Command byte for sample rate.
	mov	ax, 1000d		; Load ax with 1000Hz.
	xor	dx, dx
	div	bx
	mov	bl, al
	neg	bl
	outdac	bl			; .. and write sample rate.

@@noadjust:
	ret

endp	setsamplerate


proc	playsample uses si di,	snd:far ptr

	cmp	[soundon], 0		; No sound ?
	je	@@exit

	les	di, [snd]
	mov	ax, [es:(sndstrc ptr di).priority]
					; Is sample priority higher than
	cmp	ax, [priority]          ; priority of sample currently
	jb	@@exit                  ; played? If yes abort current
	mov     [priority], ax          ; sample.

	outdac	HALT_DMA                ; No sound not to confuse SBLASTER
					;   by reprogramming DMA.

	call	setsamplerate, [es:(sndstrc ptr di).samplerate]

	mov	ax, DMA_8_BIT_DAC
	test	[es:(sndstrc ptr di).flags], SND_PACKED4
	jz	@@8bit
	mov	ax, DMA_4_BIT_REF_DAC
@@8bit:
	mov	[command], ax

	or	[mode], SND_MEM		; Set memory mode.


; Calculate physical 20 bit base addr of data. -> addr = (cl << 16) + si
	lea	bx, [es:(sndstrc ptr di).data]
	mov	ax, es
	rol	ax, 4
	mov	cl, al
	and	al, 0f0h
	add	ax, bx
	adc	cl, 0
	and	cl, 0fh
	mov	si, ax

	mov	[word physbuffer0+00], si	; physbuffer0 is used for loop.
	mov	[byte physbuffer0+02], cl
	mov	[word addr+00], si		; addr is the running addr.
	mov	[byte addr+02], cl

	les	ax, [es:(sndstrc ptr di+00).len]
	mov	[word (header+00).len], ax	; Save sample length for loop playing.
	mov	[word (header+02).len], es
	mov	[word dlen+00], ax	; Store N of to do bytes.
	mov	[word dlen+02], es

	call	_play

@@exit:
	ret

endp	playsample


proc	playloop,	snd:far ptr

	cmp	[soundon], 0
	je	@@exit

	or	[mode], SND_LOOP	; Indicate looping sound.
	call	playsample, [snd]	; Call player routine.

@@exit:
	ret

endp	playloop



; playfile:
; Accepts a file handle of a file opened for reading.
; The file pointer must be properly set to the start of a 'sndstrct'.
; : buffer is pointer to a buffer of 'bs' kBytes.
; Note: bs must be even and less than 128.
proc	playfile,	filvar:word, buffer:far ptr, bs:word

	cmp	[soundon], 0
	je	@@exit

	mov	ax, [filvar]
	mov	[filehandle], ax

; buffer0 addresses.
	les	bx, [buffer]
	mov	[word buffer0+00], bx
	mov	[word buffer0+02], es
; Calculate physical 20 bit base addr of buffer0
	mov	ax, es
	rol	ax, 4
	mov	cl, al
	and	al, 0f0h
	add	ax, bx
	adc	cl, 0
	and	cl, 0fh
	mov	[word physbuffer0+00], ax
	mov	[byte physbuffer0+02], cl
; buffer1 addresses.
	mov	dx, [bs]
	shl	dx, 9
	mov	[bsize], dx
	add	ax, dx
	adc	cl, 0
	mov	[word physbuffer1+00], ax
	mov	[byte physbuffer1+02], cl
	shr	dx, 4
	add	dx, [word buffer0+02]
	mov	[word buffer1+00], bx
	mov	[word buffer1+02], dx

; Now we are ready to concentrate on the sound.
	mov	ax, DOS_READ		; Read sample header.
	mov	bx, [filehandle]
	mov	cx, size sndstrc
	mov	dx, offset header
	int	21h

	mov	ax, DOS_LSEEK + 01h	; Get current file pointer.
	mov	bx, [filehandle]
	xor	cx, cx
	xor	dx, dx
	int	21h
	mov	[word fptr+00], ax	; Store file pointer for looping.
	mov	[word fptr+02], dx

	les	ax, [header.len]	; Get sample length.
	mov	[word flen+00], ax
	mov	[word flen+02], es
	mov	[word slen+00], ax
	mov	[word slen+02], es

; Set priority level.
	mov	ax, [header.priority]
	mov	[priority], ax

; Fill buffer0 with cool sound data.
	mov	ax, DOS_READ
	mov	bx, [filehandle]
	mov	cx, [bsize]		; Read one buffer.
	push	ds
	lds	dx, [buffer0]
	int	21h
	pop	ds

	mov	ax, [bsize]
	sub	[word flen+00], ax      ; Subtract N of bytes we've just read.
	sbb	[word flen+02], 0

; LET'S GO.
	outdac	HALT_DMA                ; No sound during mucking with DMA.
	call	setsamplerate, [header.samplerate]

	mov	ax, DMA_8_BIT_DAC
	test	[header.flags], SND_PACKED4
	jz	@@8bit
	mov	ax, DMA_4_BIT_REF_DAC
@@8bit:
	mov	[command], ax

	mov	[readstate], 0		; Everything is ok at the moment.
	or	[mode], SND_FILE	; Set file mode to tell the interrupt
					; service routine what's going on.
	mov	[just_played], 1
	mov	[word dlen+00], 0	; Indicate buffer1 finished playing.
	mov	[word dlen+02], 0

; Simulate interrupt from soundblaster.
	pushf
	push	cs
	call	newirq

@@exit:
	ret

endp	playfile


proc	playfileloop,	filvar:word, buffer:far ptr, bs:word

	or	[mode], SND_LOOP	; Indicate looping sound.
	call	playfile, [filvar], [buffer], [bs]

@@exit:
	ret

endp	playfileloop


; snd_cli & snd_sti prevent and allow buffer readings by 'playfile'
; respectivly. This is necessary because DOS isn't reentrant (as you
; know). So always you play a sound using playfile and you want
; to do a int 21h call use snd_cli & snd_sti before and after the call
; to prevent your coputer from going mad.

proc	snd_cli

	cmp	[soundon], 0
	je	@@exit

	or	[readstate], SND_NOREAD	; Deny any use of int 21h calls
					; during interrupts.
@@exit:
	ret

endp	snd_cli


proc	snd_sti	uses si di

	cmp	[soundon], 0		; Sound allowed?
	je	@@exit

	and	[readstate], not SND_NOREAD

; Check whether data should have been read during disabled interrupts.
	test	[readstate], SND_READPENDING
	jz	@@exit
	and	[readstate], not SND_READPENDING

	call	_load

@@exit:
	ret

endp	snd_sti


proc	haltsound

	mov	[mode], 0		; Clear mode register.
	mov	[priority], 0		; Reset priority level.
	cmp	[soundon], 0		;
	je	@@exit
	outdac	HALT_DMA                ; Tell SB to stop action.
@@exit:
	ret

endp	haltsound


; speaker:
; state == 0  : Turn sound off.
; state == 1  : Turn sound on.
; state == -1 : Query sound.
proc	speaker,	state:word

	cmp	[state], -1
	jne	@@go_on
	mov	ax, [soundon]
	jmp	@@exit
@@go_on:
	mov	ax, 0
	cmp	[board], ax
	je	@@exit

	call	haltsound

	cmp	[state], 0
	jne	@@turnon
	outdac	SPEAKER_OFF		; Turn speaker off.
	mov	ax, 0
	jmp	@@exit
@@turnon:
	outdac	SPEAKER_ON		; Turn it on.
	mov	ax, 1
@@exit:
	mov	[soundon], ax
	ret

endp	speaker


proc	soundbusy

	mov	ax, 1
	cmp	[soundon], 0
	je	@@exit
	mov	ax, [priority]
@@exit:
	ret

endp	soundbusy


proc	nolanguage resetsound near

; Reset Sound Blaster
	mov	dx, [ioaddr]
	add	dx, DSP_RESET
	mov	al, 1
	out	dx, al
	in	al, dx
	xor	al, al
	out	dx, al
	mov	dx, [ioaddr]
	add	dx, DSP_READ_DATA
	mov	cx, 100d
@@loop:
	in	al, dx
	cmp	al, 0aah
	je	@@sbok
	loop	@@loop
	xor	ax, ax			; Error.
	jmp	@@exit
@@sbok:
	mov	dx, [ioaddr]
	add	dx, DSP_DATA_AVAIL
	in	al, dx

	mov	ax, 1
@@exit:
	ret

endp	resetsound


proc	nolanguage setirq near

	mov	ah, 35h			; DOS Get interrupt.
	mov	al, [byte irq]
	add	al, 8			; Map harware irq to CPU interrupt.
	int	21h			; Get addr in es:bx
	mov	[word oldirq + 00h], bx
	mov	[word oldirq + 02h], es

	mov	ah, 25h			; DOS set vector.
	mov	al, [byte irq]
	add	al, 8
	push	ds
	push	cs
	pop	ds
	int	21h
	pop	ds

; Enable hardware irq on PIC
	in	al, 21h			; Get irq mask from PIC.
	mov	[oldmask], ax
	mov	cl, [byte irq]
	mov	ah, 1
	shl	ah, cl
	not	ah
	and	al, ah
	out	21h, al

	ret

endp	setirq


proc	nolanguage resetirq near

; Disable irq on PIC
	mov	ax, [oldmask]
	out	21h, al			; Disable irq.

; Restore vector.
	mov	ah, 25h                 ; DOS set vector.
	mov	al, [byte irq]
	add	al, 8
	mov	dx, [word oldirq + 00h]
	mov	bx, [word oldirq + 02h]
	push	ds
	mov	ds, bx
	int	21h
	pop	ds

	ret

endp	resetirq


proc	nolanguage testirq far

	push	ax dx ds

	mov	ax, DGROUP
	mov	ds, ax

	mov	[board], 1
	mov	dx, [ioaddr]
	add	dx, DSP_DATA_AVAIL
	in	al, dx
	mov	al, 20h
	out	20h, al

	pop	ds dx ax

	iret

endp	testirq


proc	initsound uses si,	snd_io:word, snd_irq:word, snd_dma:word

	mov	ax, [snd_io]
	mov	[ioaddr], ax
	cmp	ax, -1
	jne	@@iofound

	mov	si, offset iocheck
@@ioagain:
	mov	ax, [si]
	cmp	ax, -1
	je	@@notinstalled
	add	si, 2			; Set pointer to next addr.
	mov	[ioaddr], ax		; First I/O addr.
	call	resetsound
	or	ax, ax
	jz	@@ioagain
@@iofound:
	call	resetsound
; At this point the SBlaster base address is known. Now we try to get
; the IRQ.
	mov	ax, [snd_irq]
	mov	[irq], ax
	cmp	ax, -1
	jne	@@irqfound
	mov	[board], 0		; Indicate irq not found yet.

	mov	si, offset irqcheck	; bx pointer to irq array.
@@irqagain:
	mov	al, [byte si]
	cbw
	cmp	ax, -1
	je	@@notinstalled
	inc	si
	mov	[irq], ax
	mov	dx, offset testirq	; Set irq to this rountine.
	call	setirq
	outdac	0f2h			; I don't know why do this.
					; The code for getting to the
					; irq was disassembled from BMASTER.
	xor	ax, ax
	mov	es, ax
	mov	cx, [es:046ch]		; Get current timer ticks.
@@wait:
	mov	ax, [es:046ch]
	sub	ax, cx
	cmp	ax, 3			; Wait 3 ticks.
	jb	@@wait
	call	resetirq
	cmp	[board], 1		; Interrupt occured ?
	jne	@@irqagain

@@irqfound:
	mov	dx, offset newirq
	call	setirq

	mov	ax, [snd_dma]
	cmp	ax, -1
	je	@@skip3
	mov	[dmachan], ax
@@skip3:

	mov	[board], 1
	call	setsamplerate, 11
	call	speaker, 1
	mov	ax, 1
	jmp	@@exit

@@notinstalled:
	mov	[board], 0
	mov	[soundon], 0
	xor	ax, ax

@@exit:
	ret

endp	initsound


proc	shutsound

	cmp	[board], 0
	je	@@exit

	call	haltsound
	call	speaker, 0

	call	resetirq

	mov	[board], 0
@@exit:
	ret

endp	shutsound

	end

