;=========================================================================
;XT-CF 8088_bios module for Book8088
;(C) 2023 Serhii Liubshin
;

;%define XTIDE

%ifndef XTIDE

PORT	equ	300h
FEREG	equ	PORT+(1*2)
DHREG	equ	PORT+(6*2)
SCREG	equ	PORT+(2*2)
SNREG	equ	PORT+(3*2)
CLREG	equ	PORT+(4*2)
CHREG	equ	PORT+(5*2)
STREG	equ	PORT+(7*2)
COREG	equ	PORT+(7*2)

%else

PORT	equ	300h
FEREG	equ	PORT+(1)
DHREG	equ	PORT+(6)
SCREG	equ	PORT+(2)
SNREG	equ	PORT+(3)
CLREG	equ	PORT+(4)
CHREG	equ	PORT+(5)
STREG	equ	PORT+(7)
COREG	equ	PORT+(7)

%endif

detect_xtcf:

	push	ds
	push	es
	push	dx
	push	di
	push	si

	mov	dx,STREG
	in	al,dx
	cmp	al,0FFh
	jnz	.go_on
	jmp	.error_nodrive
.go_on:
	call	set_pio
	jc	.exit_error

	call	send_identify
	jc	.exit_error

	call	wait_poll
	jc	.exit_error

	xor	di,di
	mov	ax,2000h
	mov	es,ax
	mov	ds,ax

	cld
	mov	dx,PORT
	mov	cx,256
.read_loop:
%ifndef XTIDE
	in	ax,dx
	stosw
%else
        in	al,dx
	mov	ah,al
	xor	dx,8
	in	al,dx
	xchg	ah,al
	xor	dx,8
	stosw
%endif
	loop	.read_loop

	mov	si,36h
	mov	di,si
	mov	cx,18/2
.xch:
	lodsw
	xchg	ah,al
	stosw
	loop	.xch
	xor	ax,ax
	stosw

	mov	si,msg_cf
	call	print

	mov	si,36h
	call	printds

	mov	si,msg_crlf
	call	print

	pop	si
	pop	di
	pop	dx
	pop	es
	pop	ds

	ret

.exit_error:
.error_nodrive:

	push	cs
	pop	ds

	mov	si,msg_nocf
	call	print
	mov	si,msg_crlf
	call	print

	pop	si
	pop	di
	pop	dx
	pop	es
	pop	ds

	ret

msg_cf		db	'Compact Flash drive:        ', 00h
msg_nocf	db	'Compact Flash drive:        None', 00h
;msg_ch375	db	'Floppy disk drives:         Drive 0: CH375 USB', 00h
;msg_nousb	db	'Floppy disk drives:         None', 00h


;---
;Identify drive command
send_identify:
	mov	al,0A0h		;CHS packet
	mov	dx,DHREG
	out	dx,al

	xor	ax,ax
	mov	dx,SCREG
	out	dx,al
	mov	dx,SNREG
	out	dx,al

	mov	dx,CLREG
	out	dx,al
	mov	dx,CHREG
	out	dx,al

	mov	al,0ECh		;Identify
	mov     dx,COREG
	out	dx,al

	in	al,dx
	or	al,al
	jnz	.ident_ok
	stc
	ret
.ident_ok:
	call	wait_poll
	ret
;---

set_pio:
	mov	al,0A0h
	mov	dx,DHREG
	out	dx,al

;	mov	al,081h	;Disable 8bit PIO
	mov	al,01h	;Enable 8bit PIO
	mov	dx,FEREG
	out	dx,al

	xor	ax,ax
	mov	dx,SCREG
	out	dx,al
	mov	dx,SNREG
	out	dx,al

	mov	dx,CLREG
	out	dx,al
	mov	dx,CHREG
	out	dx,al

	mov	al,0EFh		;Set Features
	mov     dx,COREG
	out	dx,al

	in	al,dx
	or	al,al
	jnz	set_ok
	stc
	ret
set_ok:
	call	wait_poll
	ret

;Wait for ATA interrupt or error
wait_poll:
	mov	dx,STREG
	xor	cx,cx
.wait_fin:
        dec	cx
	jz	.error_timeout

	in	al,dx

	test	al,80h
	jnz	.wait_fin

	test	al,1
	jnz	.error_error

	test	al,20h
	jnz	.error_drivefail

	test	al,8
	jnz	.drive_ready

	jmp	.all_good	;No errors and no pending interrupt

.error_timeout:
.error_error:
.error_drivefail:
	
	stc
	ret

.all_good:
.drive_ready:

	clc
	ret


