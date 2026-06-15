	XREF	_plipbox_device_name
	XREF	_plipbox_id_string
	XREF	_plipbox_auto_init_tables
	XREF	_ServerTask

	XDEF	_ServerTaskSegList

RTC_MATCHWORD:	equ	$4afc
RTF_AUTOINIT:	equ	(1<<7)
NT_DEVICE:	equ	3
VERSION:	equ	0
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
	dc.l	_plipbox_device_name
	dc.l	_plipbox_id_string
	dc.l	_plipbox_auto_init_tables
endcode:

	cnop	0,4

	dc.l	16
_ServerTaskSegList:
	dc.l	0
	jmp	_ServerTask
