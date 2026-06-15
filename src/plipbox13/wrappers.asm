
	XREF	_hwsend_reg
	XREF	_hwrecv_reg

	XDEF	_hwsend
	XDEF	_hwrecv
	XDEF	_plipbox_call_bm

	section	"text",code

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
