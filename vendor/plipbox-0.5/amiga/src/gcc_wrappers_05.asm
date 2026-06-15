	XREF	_DevInit_c
	XREF	_DevOpen_c
	XREF	_DevClose_c
	XREF	_DevExpunge_c
	XREF	_DevBeginIO_c
	XREF	_DevAbortIO_c
	XREF	_hwsend_reg
	XREF	_hwrecv_reg

	XDEF	_DevInit
	XDEF	_DevOpen
	XDEF	_DevClose
	XDEF	_DevExpunge
	XDEF	_DevBeginIO
	XDEF	_DevAbortIO
	XDEF	_hwsend
	XDEF	_hwrecv
	XDEF	_plipbox_call_bm

	section	"text",code

_DevInit:
	move.l	a6,-(sp)
	move.l	a0,-(sp)
	move.l	d0,-(sp)
	jsr	_DevInit_c
	lea	12(sp),sp
	rts

_DevOpen:
	move.l	a6,-(sp)
	move.l	d1,-(sp)
	move.l	d0,-(sp)
	move.l	a1,-(sp)
	jsr	_DevOpen_c
	lea	16(sp),sp
	rts

_DevClose:
	move.l	a6,-(sp)
	move.l	a1,-(sp)
	jsr	_DevClose_c
	lea	8(sp),sp
	rts

_DevExpunge:
	move.l	a6,-(sp)
	jsr	_DevExpunge_c
	addq.l	#4,sp
	rts

_DevBeginIO:
	move.l	a6,-(sp)
	move.l	a1,-(sp)
	jsr	_DevBeginIO_c
	lea	8(sp),sp
	rts

_DevAbortIO:
	move.l	a6,-(sp)
	move.l	a1,-(sp)
	jsr	_DevAbortIO_c
	lea	8(sp),sp
	rts

_hwsend:
	move.l	4(sp),a0
	move.l	8(sp),a1
	jmp	_hwsend_reg

_hwrecv:
	move.l	4(sp),a0
	move.l	8(sp),a1
	jmp	_hwrecv_reg

_plipbox_call_bm:
	movem.l	a2,-(sp)
	move.l	8(sp),a2
	move.l	12(sp),a0
	move.l	16(sp),a1
	move.l	20(sp),d0
	jsr	(a2)
	movem.l	(sp)+,a2
	rts
