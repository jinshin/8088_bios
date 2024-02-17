;=========================================================================
; time1.inc - BIOS Time Services
;       INT 1Ah - BIOS Time Services
;		functions AH=00h to AH=07h
;	INT 70h - IRQ8 interrupt handler (RTC alarm)
;-------------------------------------------------------------------------
;
; Compiles with NASM 2.13.02, might work with other versions
;
; Copyright (C) 2010 - 2023 Sergey Kiselev.
; Provided for hobbyist use on the Xi 8088 and Micro 8088 boards.
;
; This program is free software: you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation, either version 3 of the License, or
; (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program.  If not, see <http://www.gnu.org/licenses/>.
;
;=========================================================================

;=========================================================================
; int_1A_fn00 - Read current time
; Input:
;	AH = 0 - read current time
; Output:
;	CX = high word of tick count
;	DX = low word of tick count
;	AL = midnight flag: non-zero if midnight passed since time last read
;-------------------------------------------------------------------------
int_1A_fn00:
	mov	dx,word [ticks_lo]
	mov	cx,word [ticks_hi]
	mov	al,byte [new_day]	; read new_day to al
	xor	byte [new_day],al	; new_day = 0
	jmp	int_1A_exit

;=========================================================================
; int_1A_fn01 - Set current time and clear midnight flag
; Input:
;	AH = 01h - function 01h - set current time and clear midnight flag
;	CX = high word of tick count
;	DX = low word of tick count
; Output:
;	None
;-------------------------------------------------------------------------
int_1A_fn01:
	mov	word [ticks_lo],dx
	mov	word [ticks_hi],cx
	mov	byte [new_day],00h
	jmp	int_1A_exit

%ifdef AT_RTC

;=========================================================================
; int_1A_fn02 - Read real time clock (RTC)
; Input:
;	AH = 02h - function 02h - read RTC time
; Output:
;	CF set if RTC update is in progress and operation was not performed
;	CH = BCD hours
;	CL = BCD minutes
;	DH = BCD seconds
;	DL - daylight savings flag: 00h = standard time, 01h = daylight time
;-------------------------------------------------------------------------
int_1A_fn02:
%ifdef AT_RTC_AUTODETECT
	call	rtc_detect		; check that RTC is present
	jc	int_1A_exitf		; error - no RTC
%endif ; AT_RTC_AUTODETECT
	push	ax
	mov	al,cmos_control_a
	call	rtc_read		; read control A register
	test	al,cmos_uip
	jz	.1			; no update in progess
	stc
	pop	ax
	jmp	int_1A_exitf
.1:
	mov	al,cmos_control_b
	call	rtc_read		; read control B register
	and	al,cmos_dse		; mask the daylight savings bit
	mov	dl,al
	mov	al,cmos_seconds
	call	rtc_read		; read seconds
	mov	dh,al
	mov	al,cmos_minutes
	call	rtc_read		; read minutes
	mov	cl,al
	mov	al,cmos_hours
	call	rtc_read		; read hours
	mov	ch,al
	clc
	pop	ax
	jmp	int_1A_exitf

;=========================================================================
; int_1A_fn03 - Set real time clock
; Input:
;	AH = 03h - function 03h - set RTC time
;	CH = BCD hours
;	CL = BCD minutes
;	DH = BCD seconds
;	DL - daylight savings flag: 00h = standard time, 01h = daylight time
; Output:
;	None
;-------------------------------------------------------------------------
int_1A_fn03:
%ifdef AT_RTC_AUTODETECT
	call	rtc_detect		; check that RTC is present
	jc	int_1A_exitf		; error - no RTC
%endif ; AT_RTC_AUTODETECT
	push	ax
	mov	al,cmos_control_b
	call	rtc_read		; read control B register
	mov	ah,al
	or	ah,cmos_set		; set the RTC set bit
	mov	al,cmos_control_b
	call	rtc_write		; write control B register

	and	dl,cmos_dse		; mask the daylight saving flag
	and	ah,~cmos_dse		; clear daylight saving flag for now
	or	ah,dl			; add it from the input
	mov	al,cmos_control_b
	call	rtc_write		; write control B register

	mov	al,cmos_seconds
	mov	ah,dh
	call	rtc_write		; write seconds
	mov	al,cmos_minutes
	mov	ah,cl
	call	rtc_write		; write minutes
	mov	al,cmos_hours
	mov	ah,ch
	call	rtc_write		; write hours

	mov	al,cmos_control_b
	call	rtc_read		; read control B register
	mov	ah,al
	and	ah,~cmos_set		; clear the RTC set bit
	mov	al,cmos_control_b
	call	rtc_write		; write control B register
	pop	ax
	jmp	int_1A_exit

;=========================================================================
; int_1A_fn04 - Read date from real time clock
; Input:
;	AH = 04h - function 04h - read RTC date
; Output:
;	CF set if RTC update is in progress and operation was not performed
;	CH = BCD century
;	CL = BCD year
;	DH = BCD month
;	DL = BCD date
;-------------------------------------------------------------------------
int_1A_fn04:
%ifdef AT_RTC_AUTODETECT
	call	rtc_detect		; check that RTC is present
	jc	int_1A_exitf		; error - no RTC
%endif ; AT_RTC_AUTODETECT
	push	ax
	mov	al,cmos_control_a
	call	rtc_read		; read control A register
	test	al,cmos_uip
	jz	.1			; no update in progess
	stc
	pop	ax
	jmp	int_1A_exitf
.1:
	mov	al,cmos_date
	call	rtc_read		; read date
	mov	dl,al
	mov	al,cmos_month
	call	rtc_read		; read month
	mov	dh,al
	mov	al,cmos_year
	call	rtc_read		; read year
	mov	cl,al
	mov	al,cmos_century
	call	rtc_read		; read century
	mov	ch,al
	clc
	pop	ax
	jmp	int_1A_exitf

;=========================================================================
; int_1A_fn05 - Set date in real time clock
; Input:
;	AH = 05h - function 05h - set RTC date
;	CH = BCD century
;	CL = BCD year
;	DH = BCD month
;	DL = BCD date
; Output:
;	None
;-------------------------------------------------------------------------
int_1A_fn05:
%ifdef AT_RTC_AUTODETECT
	call	rtc_detect		; check that RTC is present
	jc	int_1A_exitf		; error - no RTC
%endif ; AT_RTC_AUTODETECT
	push	ax
	mov	al,cmos_control_b
	call	rtc_read		; read control B register
	mov	ah,al
	or	ah,cmos_set		; set the RTC set bit
	mov	al,cmos_control_b
	call	rtc_write		; write control B register

	mov	al,cmos_date
	mov	ah,dl
	call	rtc_write		; write date

	mov	al,cmos_month
	mov	ah,dh
	call	rtc_write		; write month
	mov	al,cmos_year
	mov	ah,cl
	call	rtc_write		; write year
	mov	al,cmos_century
	mov	ah,ch
	call	rtc_write		; write centry

	mov	al,cmos_control_b
	call	rtc_read		; read control B register
	mov	ah,al
	and	ah,~cmos_set		; clear the RTC set bit
	mov	al,cmos_control_b
	call	rtc_write		; write control B register
	pop	ax
	jmp	int_1A_exit

;=========================================================================
; int_1A_fn06 - Set real time clock alarm
; Input:
;	AH = 06h - function 06h - set RTC alarm time
;	CH = BCD hours
;	CL = BCD minutes
;	DH = BCD seconds
; Output:
;	CF = 1 - alarm already set
;-------------------------------------------------------------------------
int_1A_fn06:
%ifdef AT_RTC_AUTODETECT
	call	rtc_detect		; check that RTC is present
	jc	int_1A_exitf		; error - no RTC
%endif ; AT_RTC_AUTODETECT
	push	ax
	mov	al,cmos_control_b
	call	rtc_read		; read control B register
	test	al,cmos_aie
	jz	.1			; RTC alarm interrupt is not enabled
	stc
	pop	ax
	jmp	int_1A_exitf
.1:
	mov	ah,al
	or	ah,cmos_set		; set the RTC set bit
	mov	al,cmos_control_b
	call	rtc_write		; write control B register

	mov	al,cmos_alarm_secs
	mov	ah,dh
	call	rtc_write		; write alarm seconds
	mov	al,cmos_alarm_mins
	mov	ah,cl
	call	rtc_write		; write alarm minutes
	mov	al,cmos_alarm_hrs
	mov	ah,ch
	call	rtc_write		; write alarm hours

	mov	al,cmos_control_b
	call	rtc_read		; read control B register
	mov	ah,al
	and	ah,~cmos_set		; set the RTC set bit
	mov	al,cmos_control_b
	call	rtc_write		; write control B register

	mov	al,cmos_control_b
	call	rtc_read		; read control B register
	mov	ah,al
	or	ah,cmos_aie		; set alarm interrupt flag
	mov	al,cmos_control_b
	call	rtc_write		; write control B register with AIE set

	pop	ax
	clc
	jmp	int_1A_exitf

;=========================================================================
; int_1A_fn07 - Reset real time clock alarm
; Input:
;	AH = 07h - function 07h - set RTC time
; Output:
;	None
;-------------------------------------------------------------------------
int_1A_fn07:
%ifdef AT_RTC_AUTODETECT
	call	rtc_detect		; check that RTC is present
	jc	int_1A_exitf		; error - no RTC
%endif ; AT_RTC_AUTODETECT
	push	ax

	mov	al,cmos_control_b
	call	rtc_read		; read control B register
	mov	ah,al
	and	ah,~cmos_aie		; clear alarm interrupt flag
	mov	al,cmos_control_b
	call	rtc_write		; write control B reg. with AIE cleared

	pop	ax
	jmp	int_1A_exit
	
%ifdef SECOND_PIC
;=========================================================================
; int_70 - RTC interrupt service routine (IRQ8)
; Notes:
;	Calls INT 4Ah if interrupt is caused by RTC alarm
;-------------------------------------------------------------------------
int_70:
	push	ax
	mov	al,20h
	out	pic2_reg0,al		; signal EOI to the slave PIC
	out	pic1_reg0,al		; signal EOI to the master PIC
	mov	al,cmos_control_c
	call	rtc_read		; read control C register
	test	al,cmos_af		; check for alarm flag
	jz	.1
	int	4Ah			; call INT 4Ah
.1:
	pop	ax
	iret
%endif ; SECOND_PIC

%endif ; AT_RTC

%ifdef MACHINE_MISTER
;=========================================================================
; RTC Functions for MiSTer PC XT
; Made by Serhii Liubshin 2024
;-------------------------------------------------------------------------

mister_cmos	equ	2C0h

;=========================================================================
; int_1A_fn02 - Read real time clock (RTC)
; Input:
;	AH = 02h - function 02h - read RTC time
; Output:
;	CF set if RTC update is in progress and operation was not performed
;	CH = BCD hours
;	CL = BCD minutes
;	DH = BCD seconds
;	DL - daylight savings flag: 00h = standard time, 01h = daylight time
;-------------------------------------------------------------------------
int_1A_fn02:
	mov	dx,mister_cmos
	mov	al,4
	out	dx,al
	inc	dx
	in	al,dx
	mov	ch,al
	dec	dx
	mov	al,2
	out	dx,al
	inc	dx
	in	al,dx
	mov	cl,al
	dec	dx
	mov	al,0
	out	dx,al
	inc	dx
	in	al,dx
	xor	dx,dx	;standard time always
	mov	dh,al	
	clc
	jmp	int_1A_exitf

;=========================================================================
; int_1A_fn03 - Set real time clock
; Input:
;	AH = 03h - function 03h - set RTC time
;	CH = BCD hours
;	CL = BCD minutes
;	DH = BCD seconds
;	DL - daylight savings flag: 00h = standard time, 01h = daylight time
; Output:
;	None
;-------------------------------------------------------------------------
int_1A_fn03:
	jmp	int_1A_exit

;=========================================================================
; int_1A_fn04 - Read date from real time clock
; Input:
;	AH = 04h - function 04h - read RTC date
; Output:
;	CF set if RTC update is in progress and operation was not performed
;	CH = BCD century
;	CL = BCD year
;	DH = BCD month
;	DL = BCD date
;-------------------------------------------------------------------------
int_1A_fn04:
	mov	ch,20h	;century, fixme in year 3000
	mov	dx,mister_cmos
	mov	al,9
	out	dx,al
	inc	dx
	in	al,dx
	mov	cl,al	;year
	dec	dx
	mov	al,8
	out	dx,al
	inc	dx
	in	al,dx
	mov	ah,al	;month
	dec	dx
	mov	al,7
	out	dx,al
	inc	dx
	in	al,dx
	mov	dl,al
	mov	dh,ah
	clc
	jmp	int_1A_exitf

;=========================================================================
; int_1A_fn05 - Set date in real time clock
; Input:
;	AH = 05h - function 05h - set RTC date
;	CH = BCD century
;	CL = BCD year
;	DH = BCD month
;	DL = BCD date
; Output:
;	None
;-------------------------------------------------------------------------
int_1A_fn05:
	jmp	int_1A_exit

;=========================================================================
; int_1A_fn06 - Set real time clock alarm
; Input:
;	AH = 06h - function 06h - set RTC alarm time
;	CH = BCD hours
;	CL = BCD minutes
;	DH = BCD seconds
; Output:
;	CF = 1 - alarm already set
;-------------------------------------------------------------------------
int_1A_fn06:
	jmp	int_1A_exitf

;=========================================================================
; int_1A_fn07 - Reset real time clock alarm
; Input:
;	AH = 07h - function 07h - set RTC time
; Output:
;	None
;-------------------------------------------------------------------------
int_1A_fn07:
	jmp	int_1A_exit

;=========================================================================
; print_rtc - print current RTC date and time
; Input:
;	none
; Output:
;	date and time are printed in YYYY-MM-DD hh:mm:ss format
; Notes:
;	There is a slight probability of getting inconsistent printout.
;	In case the function is called just before midnight, it could
;	print the previos day's date and next day's time.
;-------------------------------------------------------------------------
print_rtc:
	push	ax
	push	bx
	push	cx
	push	dx
	push	si
	mov	si,msg_rtc
	call	print

; print date
	mov	ah,04h
	int	1Ah			; read RTC date
					; CH = BCD century
					; CL = BCD year
					; DH = BCD month
					; DL = BCD date (day of month)
	mov	ax,cx
	call	print_hex		; print 4-digit year
	mov	ax,(0Eh << 8) + '-'
	mov	bx,0007h
	int	10h			; print dash (-)

	mov	al,dh
	call	print_byte		; print 2-digit month
	mov	ax,(0Eh << 8) + '-'
	mov	bx,0007h
	int	10h			; print dash (-)

	mov	al,dl
	call	print_byte		; print 2-digit date (day of month)

	mov	ax,(0Eh << 8) + ' '
	mov	bx,0007h
	int	10h			; print space ( )

; print time
	mov	ah,02h
	int	1Ah			; read RTC time
					; CH = BCD hours
					; CL = BCD minutes
					; DH = BCD seconds
					; DL = daylight saving flag
	mov	al,ch
	call	print_byte		; print 2-digit hours
	
	mov	ax,(0Eh << 8) + ':'
	mov	bx,0007h
	int	10h			; print colon (:)

	mov	al,cl
	call	print_byte		; print 2-digit minutes
	
	mov	ax,(0Eh << 8) + ':'
	mov	bx,0007h
	int	10h			; print colon (:)

	mov	al,dh
	call	print_byte		; print 2-digit seconds
	mov	si,msg_crlf
	call	print

.exit:
	pop	si
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	ret

%endif