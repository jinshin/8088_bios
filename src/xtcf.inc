;=========================================================================
;XT-CF 8088_bios module for Book8088
;(C) 2023 Serhii Liubshin
;

;40:74	byte	Status of last hard disk operation (see INT 13,1)
;40:75	byte	Number of hard disks attached
;40:76	byte	XT fixed disk drive control byte
;40:77	byte	Port offset to current fixed disk adapter

hdd_last_error	equ	74h
hdd_count	equ	75h	;Should be one so far

;We will use it as a segment offset to our data
hdd_segment	equ	76h	;Word

hdd_control	equ	76h	;Might be useful
hdd_port_offset	equ	77h	;Might be useful

hdd_status	equ	8Ch	;40:8C	byte	Hard disk status returned by controller
hdd_error	equ	8Dh	;40:8D	byte	Hard disk error returned by controller
hdd_int		equ	8Eh	;40:8E	byte	Hard disk interrupt control flag(bit 7=working int)

;XTIDE is PCem mode

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

;Control register
;taken from XT-IDE BIOS
CTREG   equ	PORT+1Eh

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

CTREG	equ	PORT+200h+6

%endif

;We use some reserved BDA bytes
;to store media info
;We use some in CH375 too
cf_cylinders	equ	0B0h  ;Word
cf_heads	equ	0B2h  ;Byte
cf_sectors	equ	0B3h  ;Byte

xtcf_reserve_ram:
	cld
	push	ds
	push	es
	
        mov	ax,biosdseg
	mov	ds,ax
	
	mov	al,[hdd_count]
	test	al,al
	jz	.exit

	mov	ax,[memory_size]
	dec	ax
	mov	[memory_size],ax
	mov	[hdd_segment],ax

	mov	es,ax
;offset for int 41h, so far 0
	xor	di,di

	mov	ax,[cf_cylinders]
	stosw
	mov	al,[cf_heads]
	stosb
	xor	ax,ax
	stosw
	stosw
	;ECC data burst length?
	stosb
	;Control byte?
	stosb

	mov	bx,es
	
;Create INT 41h pointer with HDD table	
	xor	ax,ax
	mov	es,ax
	
	mov	di,41h*4
;offset for int 41h, so far 0
	xor	ax,ax
	stosw
	mov	ax,bx
	stosw	

.exit:
	pop	es
	pop	ds
	ret

%define UNROLL	8

; Read data from port
; ES:DI - dest buffer
; BL - CPU type

xtide_read_sector:
	cld
	mov	dx,PORT

	test	bl,bl
	jz	.j8088

	mov	cx,256
	cpu	186
	rep	insw
	cpu	8086
	jmp	.exit

.j8088:

	mov	cx,256/UNROLL
.read_loop:

%rep	UNROLL

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

%endrep

	loop	.read_loop
.exit:
	ret

; Write data
; DS:SI - source buffer
; BL - CPU type
; OsDev recommends make it slow
xtide_write_sector:
	cld
	mov	dx,PORT

	test	bl,bl
	jz	.j8088

	mov	cx,256
	cpu	186
	rep	outsw
	cpu	8086
	jmp	.exit

.j8088:

	mov	cx,256/UNROLL

.write_loop:

%rep	UNROLL

%ifndef XTIDE
	lodsw
	out	dx,ax
%else
	lodsw
	xchg	ah,al
	xor	dx,8
	out	dx,al
	xor	dx,8
	mov	al,ah
	out	dx,al
%endif

%endrep

	loop	.write_loop
.exit:
	ret



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
%ifndef XTIDE
	call	set_pio
	jc	.exit_error
%endif
	call	send_identify
	jc	.exit_error

	call	wait_poll
	jc	.exit_error

	xor	di,di
	mov	ax,2000h
	mov	es,ax
	mov	ax,biosdseg
	mov	ds,ax

	mov	bl,[cpu_type]
	call	xtide_read_sector

;Extract drive info

	push	es
	push	ds
	pop	es
	pop	ds

	xor	si,si
	lodsw
	lodsw
	test	ax,ax
	jz	.not_good
	mov	es:[cf_cylinders],ax
	lodsw
	lodsw
	test	ax,ax
	jz	.not_good
	mov	es:[cf_heads],al
	lodsw
	lodsw
	lodsw
	test	ax,ax
	jz	.not_good
	mov	es:[cf_sectors],al
	mov	al,1
	mov	es:[hdd_count],al

;Eat RAM
		

.not_good:	
	test	ax,ax	;Will be zero if something wrong
	jz	.exit_error	

;Get drive name

	push	ds
	pop	es

	mov	si,36h
	mov	di,si
	mov	cx,30/2
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
	call	send_chs_zeros

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
;	mov	al,081h	;Disable 8bit PIO
	mov	al,01h	;Enable 8bit PIO
	mov	dx,FEREG
	out	dx,al

	call	send_chs_zeros	

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

send_chs_zeros:
	xor	dx,dx
	xor	cx,cx
	xor	bx,bx
	jmp	send_chs_values

;	mov	al,0A0h		;CHS packet
;	mov	dx,DHREG
;	out	dx,al

;	xor	ax,ax
;	mov	dx,SCREG
;	out	dx,al
;	mov	dx,SNREG
;	out	dx,al

;	mov	dx,CLREG
;	out	dx,al
;	mov	dx,CHREG
;	out	dx,al
;	ret

;DH - head
;DL - Sector count
;CL - Sector
;BX - Cylinder

send_chs_values:

	mov	ax,dx
	xchg	ah,al

	or	al,0A0h
	mov	dx,DHREG
	out	dx,al

	mov	al,ah
	mov	dx,SCREG
	out	dx,al

	mov	al,cl
	mov	dx,SNREG
	out	dx,al

	mov	al,bl
	mov	dx,CLREG
	out	dx,al

	mov	al,bh
	mov	dx,CHREG
	out	dx,al

	ret

;Wait for ATA interrupt or error
;AX CX DX
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

space	db " ",0

;==========================================
;CF card int 13h handler
;
xtcf_int_13:
	cmp	dl,80h
	jne	hdd_exit

	push	ax
	push	bx
	push	cx
	push	dx
	push	si
	push	di
	push	ds
	push	bp
	mov	bp,sp

	mov	bx,biosdseg
	mov	ds,bx

	mov	bl,[hdd_count]
	test	bl,bl
	jnz	.go13

	xor	ah,ah
	mov	[hdd_last_error],ah
	clc
	jmp	hdd_13_exit

.go13:

	mov	ax,[bp+int_13_ax]
	mov	bx,[bp+int_13_bx]
	mov	cx,[bp+int_13_cx]
	mov	dx,[bp+int_13_dx]

	cmp	ah,00
	je	hdd_13_reset
	cmp	ah,0Dh
	je	hdd_13_reset

	cmp	ah,01
	je	hdd_13_geterror

	cmp	ah,02
	je	hdd_13_read

	cmp	ah,03
	je	hdd_13_write


	cmp	ah,08
	je	hdd_13_drivetype
	cmp	ah,09
	je	hdd_13_inittable

hdd_13_exit:
	mov	byte [bp+int_13_ah],ah	; pass AH to the caller
	mov	ax,201h			; set IF and CF
	jc	.set_error		; there is an error
	and	byte [bp+int_13_flags_l],0FEh ; no errors - clear CF
	dec	ax			; clear CF in AX too

.set_error:
	or	word [bp+int_13_flags],ax
	pop	bp
	pop	ds
	pop	di
	pop	si
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	iret

hdd_exit: 
	iret

;Reset device
hdd_13_reset:
	mov	dx,CTREG
	mov	al,4
	out	dx,al
	mov	cx,10
.wl:	loop	.wl
	xor	ax,ax
	out	dx,al
	mov	cx,10
.wl2:	loop	.wl2
	call	wait_poll
	jnc	.next	
	mov	byte [hdd_count],0
	mov	ah,5
	mov	byte [hdd_last_error],ah
	jmp	hdd_13_exit	
.next:
%ifndef XTIDE
	call	set_pio
	jnc	.next2
	mov	byte [hdd_count],0
	mov	ah,5
	mov	byte [hdd_last_error],ah
%endif
.next2:
	jmp	hdd_13_exit

;Get status
hdd_13_geterror:
	mov	al,[hdd_last_error]
	clc
	jmp	hdd_13_exit

;=======================================
; Read
;
hdd_13_read:
 
	test	al,al
	jz	.read_exit_error
	xor	ah,ah

	mov	si,ax		;Sectors to read
	mov	di,bx		;Buffer pointer	

	;Construct cylinder number in BX
	mov	bx,cx
	xchg	bh,bl
	rol	bh,1
	rol	bh,1
	and	bh,11b

	;Sector in CL, should not be 0
	and	cl,111111b
	test	cl,cl
	jz	.read_exit_error
	;Sector count in DL
	mov	dl,al
	;Head in DH, unchanged	

	call	send_chs_values

	mov	bl,[cpu_type]

	mov	al,020h		;Read
	mov     dx,COREG
	out	dx,al

	call	wait_poll
	jc	.read_exit_error	

	mov	cx,si		;saved value
.read_loop:
	push	cx

	call	xtide_read_sector
	call	wait_poll
	jc	.read_exit_error	
	pop	cx
	loop	.read_loop

	clc
	jmp	hdd_13_exit

.read_exit_error:

	mov	byte [hdd_last_error],1
	stc
	jmp	hdd_13_exit

;======================================
; Write
;
hdd_13_write:
	test	al,al
	jz	.write_exit_error
	xor	ah,ah

	mov	di,ax		;Sectors to write
	mov	si,bx		;Buffer pointer	

	;Construct cylinder number in BX
	mov	bx,cx
	xchg	bh,bl
	rol	bh,1
	rol	bh,1
	and	bh,11b

	;Sector in CL, should not be 0
	and	cl,111111b
	test	cl,cl
	jz	.write_exit_error
	;Sector count in DL
	mov	dl,al
	;Head in DH, unchanged	

	call	send_chs_values

	mov	bl,[cpu_type]

	push	es	
	pop	ds

	mov	al,030h		;Write
	mov     dx,COREG
	out	dx,al

	call	wait_poll
	jc	.write_exit_error	

	mov	cx,di		;saved value
.write_loop:
	push	cx

	call	xtide_write_sector
	call	wait_poll
	jc	.write_exit_error	
	pop	cx
	loop	.write_loop

	clc
	jmp	hdd_13_exit

.write_exit_error:

	mov	byte [hdd_last_error],1
	stc
	jmp	hdd_13_exit


;======================================
; Get drive parameters
;
hdd_13_drivetype:
	mov	cx,[cf_cylinders]
	dec	cx
	mov	al,[cf_sectors]
	xchg	ch,cl
	ror	cl,1
	ror	cl,1	
	or	cl,al
	mov	dl,[hdd_count]

	;0 based heads!
	mov	dh,[cf_heads]
	dec	dh

	mov	[bp+int_13_cx],cx
	mov	[bp+int_13_dx],dx

	xor	ah,ah

;	mov	ax,bx
;	call	print_hex
;	mov	si,space
;	call	print

;	mov	ax,cx
;	call	print_hex
;	mov	si,space
;	call	print

;	mov	ax,dx
;	call	print_hex
;	mov	si,space
;	call	print

	clc
	jmp	hdd_13_exit


hdd_13_inittable:         

        jmp	hdd_13_exit

;	   00  no error
;	   01  bad command passed to driver
;	   02  address mark not found or bad sector
;	   03  diskette write protect error
;	   04  sector not found
;	   05  fixed disk reset failed
;	   06  diskette changed or removed
;	   07  bad fixed disk parameter table
;	   08  DMA overrun
;	   09  DMA access across 64k boundary
;	   0A  bad fixed disk sector flag
;	   0B  bad fixed disk cylinder
;	   0C  unsupported track/invalid media
;	   0D  invalid number of sectors on fixed disk format
;	   0E  fixed disk controlled data address mark detected
;	   0F  fixed disk DMA arbitration level out of range
;	   10  ECC/CRC error on disk read
;	   11  recoverable fixed disk data error, data fixed by ECC
;	   20  controller error (NEC for floppies)
;	   40  seek failure
;	   80  time out, drive not ready
;	   AA  fixed disk drive not ready
;	   BB  fixed disk undefined error
;	   CC  fixed disk write fault on selected drive
;	   E0  fixed disk status error/Error reg = 0
;	   FF  sense operation failed
