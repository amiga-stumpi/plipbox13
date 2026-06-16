
	XREF	_plipbox13_device_name
	XREF	_plipbox13_id_string
	XREF	_plipbox13_auto_init_tables
	XREF	_plipbox13_rx_task

	XDEF	_plipbox13_rx_seglist

RTC_MATCHWORD:	equ	$4afc
RTF_AUTOINIT:	equ	(1<<7)
NT_DEVICE:	equ	3
VERSION:	equ	5
PRIORITY:	equ	0

	section	"text",code

	moveq	#-1,d0
	rts

romtag:
	dc.w	RTC_MATCHWORD
	dc.l	romtag
	dc.l	endcode
	dc.b	RTF_AUTOINIT
	dc.b	VERSION
	dc.b	NT_DEVICE
	dc.b	PRIORITY
	dc.l	_plipbox13_device_name
	dc.l	_plipbox13_id_string
	dc.l	_plipbox13_auto_init_tables
endcode:

	cnop	0,4
	dc.l	16
_plipbox13_rx_seglist:
	dc.l	0
	jmp	_plipbox13_rx_task

	cnop	0,4
