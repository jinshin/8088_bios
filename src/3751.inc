; This code implements floppy int 13h
; emulation via ch375 chip
; parts taken from floppy1.inc and floppy2.inc for compatibility
; Made by Serhii Liubshin, 2023
; First part, comtains mostly CH375 interfacing procs

cpu		8086

int_num		equ	13h

cmd_port	equ	261h
data_port	equ	260h

fdc_last_error	equ	41h	; byte - status of last diskette operation
fdc_e_changed	equ	06h	; 
fdc_dor_reg	equ	3F2h		; FDC digital output register (W)

fdc_irq_flag	equ	80h		; IRQ6 had occurred

dmac_mode_reg		equ	0Bh	; DMAC mode register (R/W)

equipment_list	equ	10h	; word - equpment list
equip_floppies	equ	0000000000000001b	; floppy drivers installed

; int 13h skeleton fully taken from Sergey Kiselev's BIOS 
;-------------------------------------------------------------------------
; offsets for registers on stack
int_13_bp	equ	0
int_13_ds	equ	int_13_bp+2
int_13_di	equ	int_13_ds+2
int_13_si	equ	int_13_di+2
int_13_dx	equ	int_13_si+2
int_13_dl	equ	int_13_dx
int_13_dh	equ	int_13_dx+1
int_13_cx	equ	int_13_dx+2
int_13_cl	equ	int_13_cx
int_13_ch	equ	int_13_cx+1
int_13_bx	equ	int_13_cx+2
int_13_bl	equ	int_13_bx
int_13_bh	equ	int_13_bx+1
int_13_ax	equ	int_13_bx+2
int_13_al	equ	int_13_ax
int_13_ah	equ	int_13_ax+1
int_13_ip	equ	int_13_ax+2
int_13_cs	equ	int_13_ip+2
int_13_flags	equ	int_13_cs+2
int_13_flags_l	equ	int_13_flags
	
biosdseg	equ	0040h

;We use some reserved BDA bytes
;to store media info
;I've checked - it's unused in Book8088
fd_enabled	equ	0ACh
fd_media	equ	0ADh
fd_heads	equ	0AEh
fd_sectors	equ	0AFh
fd_totalsectors	equ	0B0h

%define	cmd_get_status	22h
%define	cmd_disk_unlock	23h
%define cmd_disk_init	51h
%define	cmd_disk_size	53h
%define	cmd_disk_ready	59h
%define	cmd_read_disk	54h
%define	cmd_read_cont	55h
%define	cmd_write_disk	56h
%define	cmd_write_cont	57h
%define	cmd_disk_sense	59h
%define	cmd_disk_sense	5Ah

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
	mov	[fd_totalsectors],ax
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
