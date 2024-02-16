;=========================================================================
;XT-CF 8088_bios module for Book8088
;(C) 2023 Serhii Liubshin
;

;40:74	byte	Status of last hard disk operation (see INT 13,1)
;40:75	byte	Number of hard disks attached
;40:76	byte	XT fixed disk drive control byte
;40:77	byte	Port offset to current fixed disk adapter

data_seg	equ	1000h
set_offset	equ	200h+190h
floppy_byte	equ	set_offset+1
set_byte	equ	set_offset+2

hdd_last_error	equ	74h
hdd_count	equ	75h	;Should be one so far

;We will use it as a segment offset to our data
hdd_segment	equ	76h	;Word

hdd_control	equ	76h	;Might be useful
hdd_port_offset	equ	77h	;Might be useful

hdd_status	equ	8Ch	;40:8C	byte	Hard disk status returned by controller
hdd_error	equ	8Dh	;40:8D	byte	Hard disk error returned by controller
hdd_int		equ	8Eh	;40:8E	byte	Hard disk interrupt control flag(bit 7=working int)

setup_byte	equ	15h	;40:15 Unused/Reserved

settings_ok	equ	4	;settings are fine
can_save	equ	8	;can use settings area
check_error	equ	80h	;checksum error
cf_present	equ	2	;CF disk present
necv20_cpu	equ	1	;is NEC V20

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

%ifdef XT_IDE_FULL
;We use some reserved BDA bytes
;to store media info
;We use some in CH375 too
cf_cylinders	equ	0B0h  ;Word
cf_heads	equ	0B2h  ;Byte
cf_sectors	equ	0B3h  ;Byte
%endif

;Reserved area to store config bytes
setup_a		equ	0B0h
setup_floppy	equ	0B1h

;If i ever get to it
%ifdef XT_IDE_FULL
%define UNROLL	8
%else
%define UNROLL	1
%endif

; Read data from port
; ES:DI - dest buffer
; BL - CPU type

xtide_read_sector:
	cld
	mov	dx,PORT

;	test	bl,bl
;	jz	.j8088
	
	jmp	.j8088

	mov	cx,256
	cpu	186
	rep	insw
	cpu	8086
	jmp	.exit

.j8088:
	mov	bh,8
	mov	cx,256/UNROLL
.read_loop:

%rep	UNROLL

%ifndef XTIDE
	in	ax,dx
	stosw
%else

;	16 bit mem io

;       in	al,dx
;	mov	ah,al
;	xor	dl,bh
;	in	al,dx
;	xchg	ah,al
;	xor	dl,bh
;	stosw

	in	al,dx
	stosb
	xor	dl,bh
	in	al,dx
	stosb
	xor	dl,bh	

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

;	test	bl,bl
;	jz	.j8088

	jmp	.j8088

	mov	cx,256
	cpu	186
	rep	outsw
	cpu	8086
	jmp	.exit

.j8088:
	mov	bh,8
	mov	cx,256/UNROLL

.write_loop:

%rep	UNROLL

%ifndef XTIDE
	lodsw
	out	dx,ax
%else
	lodsw
	xchg	ah,al
	xor	dl,bh
	out	dx,al
	xor	dl,bh
	mov	al,ah
	out	dx,al
%endif

%endrep

	loop	.write_loop
.exit:
	ret

%ifdef BIOS_SETUP
;===============================================================
;XT-CF apply BIOS settings
;CH - config, CL - floppy setup
xtcf_apply:
	push	ds
	mov	ax,biosdseg
	mov	ds,ax
;Store current setting to reserved bytes, it's might be called by detect_floppy later
	mov	[setup_floppy],cl	
	mov	[setup_a],ch
	or	byte [setup_byte],settings_ok		;Settings OK
	mov	si,msg_crlf
	call	print
	mov	si,msg_save_ok
	call	print
	pop	ds
	ret

;===============================================================
;XT-CF save BIOS settings
;We not booted and has our boot sector at 1000:0200
;data_seg:set_offset
;CH - config, CL - floppy setup
xtcf_save:
	push	ds
	push	es

	push	ax
	push	cx
	push	bx
	push	dx

	mov	ax,biosdseg
	mov	ds,ax
;Anyway we store current setting to reserved bytes, it's might be called by detect_floppy later
	mov	[setup_floppy],cl	
	mov	[setup_a],ch
	or	byte [setup_byte],settings_ok		;Settings OK
;Continue saving	
	test	byte [setup_byte],can_save
	jnz	.cont
	mov	si,msg_cannot
	jmp	.exit
.cont:

	mov	si,msg_saving
	call	print

	cld

;Reread boot sector, set values and save
	mov	ax,data_seg
	mov	es,ax
	mov	ds,ax

	push	cx

	xor	dx,dx
	xor	cx,cx
	xor	bx,bx
	inc	dx
	inc	cx
	call	send_chs_values

	mov	al,020h		;Read
	mov     dx,COREG
	out	dx,al

	call	wait_poll
	pop	cx
	jc	.exit_error

	push	cx
	mov	di,200h   ;offset to boot sector
	;mov	bl,[cpu_type]
	call	xtide_read_sector
	call	wait_poll
	pop	cx
	jc	.exit_error

	mov	di,set_offset
	xor	ax,ax
	mov	al,90h
	stosb
	add	ah,al
	mov	al,cl
	stosb
	add	ah,al
	mov	al,ch
	stosb
	add	al,ah
	stosb	

	xor	dx,dx
	xor	cx,cx
	xor	bx,bx
	inc	dx
	inc	cx
	call	send_chs_values

	mov	al,030h		;Write
	mov     dx,COREG
	out	dx,al

	call	wait_poll
	jc	.exit_error

	mov	si,200h   ;offset to boot sector
	;mov	bl,[cpu_type]
	call	xtide_write_sector
	call	wait_poll
	jc	.exit_error

	mov	si,msg_save_ok
	jmp	.exit

.exit_error:
	mov	si,msg_save_error
.exit:
	call	print
	pop	dx
	pop	cx
	pop	bx
	pop	ax

	pop	es
	pop	ds
	ret
%endif

;===============================================================
;Print CF card name
;
print_xtcf:
	push	ds
	push	si
	mov	ax,biosdseg
	mov	ds,ax
	test	byte [setup_byte],cf_present
	jz	.nocf
	mov	si,msg_cf
	call	print
	mov	ax,data_seg
	mov	ds,ax
	mov	si,36h
	call	printds
	mov	si,msg_cf
	jmp	.exit
.nocf:
	mov	si,msg_nocf
	call	print
.exit:
	mov	si,msg_crlf
	call	print
	pop	si
	pop	ds
	ret

;===============================================================
;Detect if CF card is present and store identify and boot sector
;
detect_xtcf:
	push	ds
	push	es
	push	dx
	push	di
	push	si

;%ifdef BIOS_SETUP
;	push	ds
;	mov	ax,biosdseg
;	mov	ds,ax
;	mov	byte [setup_byte],0
;	pop	ds
;%endif

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
	mov	ax,data_seg
	mov	es,ax

	;mov	bl,[cpu_type]
	call	xtide_read_sector

;Read boot sector as well

;DH - head
;DL - Sector count
;CL - Sector
;BX - Cylinder
	
	xor	dx,dx
	xor	cx,cx
	xor	bx,bx
	inc	dx
	inc	cx
	call	send_chs_values

	mov	al,020h		;Read
	mov     dx,COREG
	out	dx,al

	call	wait_poll
	jc	.exit_error
	
	;mov	bl,[cpu_type]
	call	xtide_read_sector

;Extract drive info

	push	es
	push	ds
	pop	es
	pop	ds

;%ifdef XT_CF_FULL
	xor	si,si
	lodsw
	lodsw
	test	ax,ax
	jz	.not_good
	;mov	es:[cf_cylinders],ax
	lodsw
	lodsw
	test	ax,ax
	jz	.not_good
	;mov	es:[cf_heads],al
	lodsw
	lodsw
	lodsw
	test	ax,ax
	jz	.not_good
	;mov	es:[cf_sectors],al
	;mov	al,1
	;mov	es:[hdd_count],al

.not_good:	
	test	ax,ax	;Will be zero if something wrong
	jz	.exit_error	
;%endif

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

;Set drive present flag
	mov	ax,biosdseg
	mov	es,ax	
	or	byte es:[setup_byte],cf_present

;We'll print it's name a bit later
;1000:0036
;So far we need to detect if settings are present
;1000:0390 (190h in boot sector)

	xor	ax,ax
	mov	si,set_offset
	lodsb
	test	al,al
	jz	.check_zeroes
	cmp	al,90h                          ;90h looks like a good value to mark settings :)
	jne	.alas				;not zeroes and not settings, sorry
	add	ah,al
	lodsb
	mov	cl,al
	add	ah,al
	lodsb	
	mov	ch,al
	add	ah,al
	lodsb
	cmp	ah,al
	je	.settings_ok
	or	byte es:[setup_byte],check_error	;Checksum error
.settings_ok:
	or	byte es:[setup_byte],settings_ok		;Settings OK
	or	byte es:[setup_byte],can_save		;Can use area

	mov     byte es:[setup_floppy],cl
	mov     byte es:[setup_a],ch

	jmp	.ok
.check_zeroes:
	lodsw
	test	ax,ax
	jnz	.alas
	lodsb
	test	al,al
	jnz	.alas
	or	byte es:[setup_byte],can_save	;Clear area, can use
.ok:
.alas:
	;clc
	jmp	.exit

.exit_error:
.error_nodrive:
	;stc
.exit:
	pop	si
	pop	di
	pop	dx
	pop	es
	pop	ds

	ret


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

%ifdef	BIOS_SETUP
;=================================================================
;Get floppy config byte if present
;
get_floppy:	
	push	ds
	push	ax
	mov	ax,biosdseg
	mov	ds,ax
	pop	ax
	mov	al,cs:[default_floppy]
	test	byte [setup_byte],settings_ok
	jz	.exit
	mov	al,[setup_floppy]		
.exit:
	pop	ds
	ret

;=================================================================
;Get BIOS config byte if present
;
get_config_a:
	push	ds
	mov	ax,biosdseg
	mov	ds,ax
	mov	al,cs:[default_config_a]
	test	byte [setup_byte],settings_ok
	jz	.exit
	mov	al,[setup_a]		
.exit:
	pop	ds
	ret
%endif

;==================== Cut here ====================================

%ifdef XT_CF_FULL

xtcf_init:
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

;Reset
	cmp	ah,00
	je	hdd_13_reset
	cmp	ah,0Dh
	je	hdd_13_reset
;Get last error
	cmp	ah,01
	je	hdd_13_geterror
;Read
	cmp	ah,02
	je	hdd_13_read
;Write
	cmp	ah,03
	je	hdd_13_write
;Verify
	cmp	ah,03
	je	hdd_13_verify
;Get drive type
	cmp	ah,08
	je	hdd_13_drivetype
;Init drive table
	cmp	ah,09
	je	hdd_13_inittable

;Init drive table
	cmp	ah,41h
	je	hdd_13_checkext

	push	si
	call	print_hex
	mov	si,space
	call 	print
	pop	si

	clc
	xor	ah,ah		

hdd_13_exit:

	mov	bx,biosdseg
	mov	ds,bx
	mov	[hdd_last_error],ah

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
;=======================================
;Reset device
;
hdd_13_reset:
	xor	ax,ax
	jmp	hdd_13_exit	

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
;=======================================
;Get status
;
hdd_13_geterror:
	mov	al,[hdd_last_error]
	clc
	jmp	hdd_13_exit

;=======================================
; Read
;

rerr	db	"RE ",0

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

	;mov	bl,[cpu_type]

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

	push	si
	mov	si,rerr
	call	print
	pop	si

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

	;mov	bl,[cpu_type]

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
; Verify
;
hdd_13_verify:
	xor	ah,ah
	clc
	jmp	int_13_exit	

;======================================
; Format
;
hdd_13_format:
	xor	ah,ah
	clc
	jmp	int_13_exit	

;======================================
; Check extensions
;
hdd_13_checkext:
	mov	ah,1
	stc
	jmp	int_13_exit


;======================================
; Get drive parameters
;
hdd_13_drivetype:
	mov	cx,[cf_cylinders]
;i dunno, but all bioses return -2, not -1 tracks for some reason
;i think it's because we add up sectors/heads to that last track to get full sectors
	dec	cx
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

	push	cx
	xor	cx,cx
.xx:	loop	.xx
	pop	cx

	clc
	jmp	hdd_13_exit


hdd_13_inittable:         

        jmp	hdd_13_exit

%endif

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

