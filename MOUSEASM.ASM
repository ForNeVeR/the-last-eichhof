;---------------------------------------------------------;
;                                                         ;
;                    M O U S E . A S M                    ;
;                                                         ;
;                                                         ;
;           [c] Copyright by Dany Schoch 1989             ;
;                                                         ;
;---------------------------------------------------------;



	.model  compact, c



IF      @DataSize

LES_            equ     LES
ES_             equ     ES:
SS_             equ     SS:
LDS_            equ     LDS

ELSE

LES_            equ     MOV
ES_             equ     DS:
SS_             equ     DS:
LDS_            equ     MOV

ENDIF


Mouse           equ     33h


        .data

eventofs        dw      ?               ; pointer to event-handler
eventseg        dw      ?               ;



        .code


	public	m_deviceinit
m_deviceinit    proc    state :ptr word, buttons :ptr word

        mov     ax, 0000h               ; function 00h
        int     Mouse                   ; call Mouse functions
        LES_    di, buttons             ; get address of buttons
        mov     ES_[di], bx             ; *Buttons=bx
        LES_    di, state               ; get address of state
        mov     ES_[di], ax             ; *state=ax
        ret                             ;

m_deviceinit    endp


	public	m_showcursor
m_showcursor    proc

        mov     ax, 0001h               ; function 01h
        int     Mouse                   ; call Mouse functions
        ret                             ; return

m_showcursor    endp


	public	m_hidecursor
m_hidecursor    proc

        mov     ax, 0002h               ; function 02h
        int     Mouse                   ; call Mouse functions
        ret                             ; return

m_hidecursor    endp


	public	m_getpos
m_getpos        proc    b :ptr word, x :ptr word, y :ptr word

        mov     ax, 0003h               ; function 03h
        int     Mouse                   ; call Mouse functions
        LES_    di, y                   ; get address of y
        mov     ES_[di], dx             ; *y=dx
        LES_    di, x                   ; get address of x
        mov     ES_[di], cx             ; *x=cx
        LES_    di, b                   ; get address of Button
        mov     ES_[di], bx             ; *Button=bx
        ret                             ;

m_getpos        endp


	public	m_setpos
m_setpos        proc    x :word, y :word

        mov     ax, 0004h               ; function 04h
        mov     dx, y                   ; dx=y
        mov     cx, x                   ; cx=x
        int     Mouse                   ; call Mouse functions
        ret                             ;

m_setpos        endp



	public	m_setxborder
m_setxborder    proc    x1 :word, x2 :word

        mov     ax, 0007h               ; function 07h
        mov     dx, x2                  ; dx=x2  (max x position)
        mov     cx, x1                  ; cx=x1  (min x position)
        int     Mouse                   ; call Mouse functions
        ret                             ;

m_setxborder    endp



	public	m_setyborder
m_setyborder    proc    y1 :word, y2 :word

        mov     ax, 0008h               ; function 08h
        mov     dx, y2                  ; dx=y2  (max y position)
        mov     cx, y1                  ; cx=y1  (min y position)
        int     Mouse                   ; call Mouse function
        ret

m_setyborder    endp


_privhdlr       proc    far

        push    ax
        push    bx
        push    cx
        push    dx
        push    es
        push    ds
        push    si
        push    di
        push    bp
        mov     bp,DGROUP
        mov     ds,bp

        call    dword ptr eventofs

        pop     bp
        pop     di
        pop     si
        pop     ds
        pop     es
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret

_privhdlr       endp


	public	m_seteventhdlr
m_seteventhdlr  proc    hdlr :far ptr, maskbits :word

        mov     ax, 000ch               ; function 0ch
        mov     cx, maskbits            ;
        les     dx, hdlr                ; es:dx = pointer to subroutine
        mov     eventseg, es            ;
        mov     eventofs, dx            ;
        push    cs                      ;
        pop     es                      ; set up pointer to _privhdlr
        lea     dx, cs:_privhdlr        ;
        int     Mouse                   ;
        ret                             ;

m_seteventhdlr  endp


	public	m_definecursor
m_definecursor  proc    x :word, y :word, shape :far ptr

        mov     ax, 0009h
        mov     bx, x
        mov     cx, y
        les     dx, shape

        int     Mouse

        ret

m_definecursor  endp


        end

