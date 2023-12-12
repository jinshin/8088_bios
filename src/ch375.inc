; This code implements floppy int 13h
; emulation via ch375 chip
; parts taken from floppy1.inc and floppy2.inc for compatibility
; Made by Serhii Liubshin, 2023
; First part, comtains mostly CH375 interfacing procs

cmd_port	equ	261h
data_port	equ	260h

;We use some reserved BDA bytes
;to store media info
;I've checked - it's unused in Book8088
fd_enabled	equ	0ACh
fd_media	equ	0ADh
fd_heads	equ	0AEh
fd_sectors	equ	0AFh
;fd_totalsectors	equ	0B0h

%define	cmd_get_status	22h
%define	cmd_disk_unlock	23h
%define cmd_disk_init	51h
%define	cmd_disk_size	53h
%define	cmd_disk_ready	59h
%define	cmd_read_disk	54h
%define	cmd_read_cont	55h
%define	cmd_write_disk	56h
%define	cmd_write_cont	57h
;%define	cmd_disk_sense	59h
%define	cmd_disk_sense	5Ah
%define cmd_disk_inquiry 58h 

%define	cmd_read_buffer	28h
%define	cmd_write_buffer 2Bh		

%define	cmd_set_mode	15h
%define	cmd_abort_nak	17h

;Status
%define usb_success	14h
%define usb_connect	15h
%define usb_disconnect	16h
%define	usb_ready	18h
%define	read_ok		1Dh
%define	write_ok	1Eh

;Logic for reading/writing:
;Init read by sending command, following by LBA offset and number of sectors
;Then send a command to read buffer and read data port
;Next, send 'continue read' command, following by read buffer command
;buffer size is 64 bytes, so for one sector it would be 1 read and 7 continue commands
;i didn't find a way to abort initiated read/write, so only dumb exhaust here.

wait_status:
	call	wait_interrupt
	mov	al,cmd_get_status
	call	write_cmd
	call	read_data
	ret

get_status:
	mov	al,cmd_get_status
	call	write_cmd
	call	read_data
	ret


;Do a full reset

;Mode:
;00H = Disabled device mode, 01H = Enabled device mode using external firmware mode,
;02H = Enabled device mode using internal firmware mode
;04H = Disabled host mode, 05H = Enabled host mode,
;06H = Enabled host mode with automatic generation of SOF packets, 07H = Enabled host mode and reset USB bus
;Output: Operation status (CMD_RET_SUCCESS or CMD_RET_ABORT, other values indicate that the operation is not completed) 

reset_controller:
	mov	al,cmd_set_mode
	call	write_cmd
	mov	al,6
	call	write_data
	call	wait_status
	cmp	al,usb_connect
	je	.next1
	stc
	ret
.next1:
	call	long_delay
	mov	al,cmd_disk_init
	call	write_cmd
	call	wait_status
	cmp	al,usb_success
	je	.next2
	stc
	ret
.next2:
;You need to do that, or init is not considered complete by CH375
	mov	cx,4
.rep1:
	mov	al,cmd_disk_size
	call	write_cmd
	call	wait_status
	cmp	al,usb_success
	je	.next3
	mov	al,cmd_disk_sense
	call	write_cmd
	call	long_delay
	loop	.rep1
	stc
	ret
.next3:
;You need to do that, or init is not considered complete by CH375
	mov	cx,4
.rep2:
	mov	al,cmd_disk_ready
	call	write_cmd
	call	wait_status
	cmp	al,usb_success
	je	.next4
	mov	al,cmd_disk_sense
	call	write_cmd
	call	long_delay
	loop	.rep2
	stc
	ret
.next4:
	clc
	ret

;Write sectors to USB
write_sectors:
	mov	dx,ax
	mov	si,[bp+int_13_bx]
	cld
	xor	cx,cx
	mov	cl,[bp+int_13_al]
	test	cl,cl
	jnz	.next1
	stc
	jmp	.exit
.next1:
;convert 512b sectors to 64b chunks
	push	cx
	shl	cx,1
	shl	cx,1
	shl	cx,1	
;start
	mov	al,cmd_write_disk
	call	write_cmd
;LBA
	mov	ax,dx
	call	write_data
	mov	al,ah
	call	write_data
	xor	al,al
	call	write_data
	call	write_data
;sectors
	pop	ax ;cx->ax
	call	write_data

	mov	dx,data_port
	xor	ax,ax
	push	es
	pop	ds

.main:	
	call	wait_status
	cmp	al,write_ok
	je	.writebuffer
	stc
	jmp	.exit
.writebuffer:
	mov	al,cmd_write_buffer
	call	write_cmd
	mov	al,40h
	call	write_data

        push	cx
	mov	cx,ax
.writel:
	lodsb
	out	dx,al
	loop	.writel
	pop	cx
;continue
	mov	al,cmd_write_cont
	call	write_cmd
	loop	.main
	clc
	test	cx,cx
	jz	.exit
	stc
.exit:		
	mov	bx,biosdseg
	mov	ds,bx
	
	ret

;Read sectors from USB
read_sectors:
	mov	bx,ax
	mov	di,[bp+int_13_bx]
	cld
	xor	cx,cx
	mov	cl,[bp+int_13_al]
	test	cl,cl
	jnz	.next1
	stc
	jmp	.exit
.next1:
;convert 512b sectors to 64b chunks
	push	cx
	shl	cx,1
	shl	cx,1
	shl	cx,1	
;start
	mov	al,cmd_read_disk
	call	write_cmd
;65535 sectors are our maximum
	mov	al,bl
	call	write_data
	mov	al,bh
	call	write_data
	xor	al,al
	call	write_data
	call	write_data
;sectors
	pop	ax ;cx->ax
	call	write_data

	mov	dx,data_port
	xor	ax,ax

.main:	
	call	wait_status
	cmp	al,read_ok
	je	.readbuffer
	stc
	jmp	.exit
.readbuffer:
	mov	al,cmd_read_buffer
	call	write_cmd
	call	read_data
;buffer should be 64
	cmp	al,40h
	je	.transfer
	stc
	jmp	.exit
.transfer:
        push	cx
	mov	cx,ax
.readl:
	in	al,dx
	stosb
	loop	.readl
	pop	cx
;continue
	mov	al,cmd_read_cont
	call	write_cmd
	loop	.main
	clc
	test	cx,cx
	jz	.exit
	stc
.exit:		
	ret

;Read boot sector, enable emulation if something resembling floppy image is there
;We store only needed stuff as we don't have a memory buffer, it's ROM
read_boot:
	mov	al,cmd_read_disk
	call	write_cmd
;Read one boot sector and extract media info
	xor	al,al
	call	write_data
	call	write_data
	call	write_data
	call	write_data
;1 sector
	mov	al,1
	call	write_data

	mov	dx,data_port
	xor	ax,ax

	call	wait_status
	cmp	al,read_ok
	je	.readbuffer
	stc
	ret
.readbuffer:
	mov	al,cmd_read_buffer
	call	write_cmd
	call	read_data
;buffer should be 64
	cmp	al,40h
	je	.transfer
	stc
	ret
.transfer:
	mov	cx,13h
;skip to number of total sectors
.skip1:
	in	al,dx
	loop	.skip1
;get that
	in	al,dx
	mov	ah,al
	in	al,dx
	xchg	al,ah

;	mov	[fd_totalsectors],ax

;skip to sectors per track/head
	in	al,dx
	in	al,dx
	in	al,dx
;get sectors
	in	al,dx
	mov	[fd_sectors],al
;skip high part
	in	al,dx
;get heads
	in	al,dx
	mov	[fd_heads],al

;that's dumb, but you need to 'continue read' until usb_success
;i coudn't find a better way

	mov	cx,8
dumbloop:	
	mov	al,cmd_read_cont
	call	write_cmd
	call	wait_status
	cmp	al,usb_success
	je	exit_dumb
	loop	dumbloop
exit_dumb:

;quick sanity check
	mov	al,[fd_heads]
	cmp	al,0
	je	bad_exit
	cmp	al,2
	ja	bad_exit
	
	mov	al,[fd_sectors]
	cmp	al,8
	jb	bad_exit
	cmp	al,36
	ja	bad_exit
	clc
	ret
bad_exit:
	stc
	ret

;delays, most probably not even close to correct
short_delay:
	nop
	nop
	ret

long_delay:
	push	ax
	push	cx
	mov	cx,250
.loopa:	
	in	al,20h
	loop	.loopa
	pop	ax
	pop	cx
	ret

write_cmd:
	push	dx
	mov	dx,cmd_port
	out	dx,al
	call	short_delay
	pop	dx
	ret

write_data:
	push	dx
	mov	dx,data_port
	out	dx,al
	call	short_delay
	pop	dx
	ret

read_data:
	push	dx
	mov	dx,data_port
	in	al,dx
	pop	dx
	ret


;Taken from CH37X-BIOS
;Wait for interrupt
wait_interrupt:
	push	dx
	push	cx
	xor	cx,cx
	mov	dx,cmd_port
.waitmore:
	inc	cx
	cmp	cx,0FFFFh
	je	.wait_end
	in	al,dx
	test	al,80h
	jnz	.waitmore
.wait_end:
	pop	cx
	pop	dx
	ret

; This code implements floppy int 13h
; Emulation via ch375 chip
; parts taken from floppy1.inc and floppy2.inc for compatibility
; Made bu Serhii Liubshin, 2023
; Second part to preserve int location

;=========================================================================
; int_13 - BIOS floppy disk services
; Input:
;	AH = function
;		00h - Reset disk system
;		01h - Get status of last operation
;		02h - Read disk sectors
;		03h - Write disk sectors
;		04h - Verify disk sectors
;		05h - Format track
;		08h - Get drive parameters
;		15h - Get disk type
;		16h - Detect disk change
;		17h - Set disk type for format
;		18h - Set media type for format
; Output:
;	- depends on function
;	- for most functions:
;		CF clear if successful
;			AH = 00h - successful completion
;		CF set on error
;			AH = error code
;-------------------------------------------------------------------------

;=========================================================================
; Fill me
;

ch375_int_13:

	mov	al,[fd_media]
	test	al,al
	jz	.go13_reset

.retry:
	call	get_status
	test	al,al
	je	.allgoodgo

	cmp	al,usb_success
	je	.allgoodgo

	cmp	al,usb_connect
	jne	.no_disk

;You might see here an ugly attempt to detect disk change, but it's not working
.go13_reset:
	call	reset_controller
	call	read_boot
	jc	.no_disk
	mov	al,1
	mov	[fd_media],al
	mov	ah,fdc_e_changed
	mov	[fdc_last_error],ah
	jmp	.retry

.no_disk:
	xor	ah,ah
	mov	[fd_media],ah
	dec	ah
	stc
	jmp	ch375_int_13_exit		

.allgoodgo:
	mov	ax,[bp+int_13_ax]
	mov	cx,[bp+int_13_cx]
	mov	dx,[bp+int_13_dx]

	cmp	ah,00
	je	ch375_int_13_reset
	cmp	ah,01
	je	ch375_int_13_geterror
	cmp	ah,02
	je	ch375_int_13_read
	cmp	ah,03
	je	ch375_int_13_write
	cmp	ah,08
	je	ch375_int_13_drivetype
	cmp	ah,15h
	je	ch375_int_13_fn15
	cmp	ah,16h
	je	ch375_int_13_fn16

	clc
	xor	ah,ah		

ch375_int_13_exit:      
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

exit13: 
	iret


ch375_int_13_fn15:
	xor	ax,ax
	mov	al,[fd_enabled]
	test	al,al
	je	.over
	mov	ah,02h
.over:	
	clc
	mov	byte [fdc_last_error],0
	jmp	ch375_int_13_exit

ch375_int_13_fn16:
	xor	ax,ax
	mov	al,[fdc_last_error]
	test	al,al
	je	.over
	xor	ah,ah
	mov	[fdc_last_error],ah
	mov	ah,fdc_e_changed
.over:
	clc
	mov	byte [fdc_last_error],0
	jmp	ch375_int_13_exit


;Return drive type as 1.44, 0 if disabled
ch375_int_13_drivetype:
	mov	al,[fd_enabled]
	je	.return_1440
	xor	cx,cx
	xor	di,di
	mov	byte [bp+int_13_bl],cl	; drive type is zero
	mov	byte [bp+int_13_dh],cl	; maximal head number is zero
	mov	es,cx			; disk parameter table segment = 0000h
	jmp	ch375_int_13_comp

.return_1440:
	mov	ah,1
	mov	byte [bp+int_13_dl],ah	; pass number of drives to caller
	mov	byte [bp+int_13_dh],1	; maximal head number is 1 for floppy
	mov	al,4
	mov	byte [bp+int_13_bl],al	; pass drive type to caller
	mov	cx,cs
	mov	es,cx			; diskette parameter table segment

	mov	cx,4F12h		; 1.44M - 80 cylinders, 18 sectors

	mov	di,media_1440	

ch375_int_13_comp:
	xor	ax,ax			; AH = 00h - successful completion
	mov	byte [bp+int_13_al],al	; successful completion
	mov	byte [fdc_last_error],al
	mov	byte [bp+int_13_bh],al	; clear BH just in case
	mov	word [bp+int_13_cx],cx	; cylinders / sectors
	mov	word [bp+int_13_di],di	; diskette parameter table pointer
	jmp	ch375_int_13_exit

;simplified for floppy
chs_to_lba:
	mov	ax,cx ;save track/sector
	mov	bx,cx
	and	bl,111111b
	xor	bh,bh
	dec	bx    ;sector number for LBA
	jns	.next ;sanity check
	stc
	jmp	.exit	
.next:
	mov	cl,8  ;we don't need high two bits
	shr	ax,cl ;track number for LBA
	mov	cl,[fd_heads]
	shr	cl,1
	shl	ax,cl  ;track * heads count

	mov	dl,dh ;head
	xor	dh,dh
;+ head
	add	ax,dx
	xor	dx,dx
	xor	cx,cx
	mov	cl,[fd_sectors]
	mul	cx
	test	dx,dx
	jz	.next2
;overflow
	stc
	ret
.next2:
;+ sector	
	add	ax,bx
	clc
.exit:
	ret
	
;You might see here an ugly attempt to detect disk change, but it's not working
ch375_int_13_reset:
	call	reset_controller
	call	read_boot
	jc	.no_disk
	mov	al,[fd_media]
	cmp	al,1
	jne	.changed
	xor	ah,ah
	jmp	ch375_int_13_exit	
	
.changed:
	mov	al,1
	mov	[fd_media],al
	mov	ah,fdc_e_changed
	mov	[fdc_last_error],ah
	jmp	ch375_int_13_exit

.no_disk:
	xor	ah,ah
	mov	[fd_media],ah
	dec	ah
	stc
	jmp	ch375_int_13_exit

ch375_int_13_geterror:
	mov	al,[fdc_last_error]
	mov	byte [bp+int_13_al],al
	clc
	jmp	ch375_int_13_exit

;Read sectors
ch375_int_13_read:
	call	chs_to_lba
	jc	ch375_int_13_exit
	call	read_sectors
	jnc	ch375_int_13_exit
	;call	beep
	jmp	ch375_int_13_exit

;Write sectors
ch375_int_13_write:
	call	chs_to_lba
	jc	ch375_int_13_exit
	call	write_sectors
	jnc	ch375_int_13_exit
	;call	beep
	jmp	ch375_int_13_exit


;======================================================
; This should be called on boot if Drive A is set to 0
;
detect_ch375:
	push	ds
	push	dx

	mov	ax,biosdseg
	mov	ds,ax

	call	get_config_a
	test	al,conf_usbemu
	jz	.detect_next
	or	byte [fd_enabled],2
	or	byte [equipment_list],equip_floppies
	clc
	jmp	.exit

.detect_next:
	call	get_floppy
	shr	al,1
	shr	al,1
	shr	al,1
	shr	al,1
	test	al,al
	jz	.enable_auto
	stc
	jmp	.exit

.enable_auto:

;Clear bit 0
;Bit 1 will mean enforced

	mov	al,[fd_enabled]
	and 	al,11111110b
	mov	[fd_enabled],al

	xor	ax,ax
	mov	[fd_media],al	

	call	reset_controller
	jc	.exit
	call	read_boot
	jc	.exit

	mov	al,1
	or	[fd_enabled],al
	mov	[fd_media],al
	or	byte [equipment_list],equip_floppies

.exit:
	pop	dx
	pop	ds
	ret

;Everything that was below will be reused from floppy2.inc
