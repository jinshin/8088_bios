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
	setloc	0EC59h			; INT 13 (Floppy) Entry Point

int_13:
;only first floppy
	sti
	test	dl,dl
	jz	.go_floppy
%ifdef XT_CF_FULL
	jmp	xtcf_int_13
%else
        jmp	@exit
%endif
.go_floppy:
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

	mov	bl,[fd_enabled]
	test	bl,bl
	jnz	.go13
	xor	ah,ah
	mov	[fdc_last_error],ah
	clc
	jmp	int_13_exit

.go13:
	mov	al,[fd_media]
	test	al,al
	jz	.go13_reset

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
	jmp	int_13_exit

.no_disk:
	xor	ah,ah
	mov	[fd_media],ah
	dec	ah
	stc
	jmp	int_13_exit		

.allgoodgo:
	mov	ax,[bp+int_13_ax]
	mov	cx,[bp+int_13_cx]
	mov	dx,[bp+int_13_dx]

	cmp	ah,00
	je	int_13_reset
	cmp	ah,01
	je	int_13_geterror
	cmp	ah,02
	je	int_13_read
	cmp	ah,03
	je	int_13_write
	cmp	ah,08
	je	int_13_drivetype
	cmp	ah,15h
	je	int_13_fn15
	cmp	ah,16h
	je	int_13_fn16

	clc
	xor	ah,ah		

int_13_exit:
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

@exit: 
	iret


int_13_fn15:
	xor	ax,ax
	mov	al,[fd_enabled]
	test	al,al
	je	.over
	mov	ah,02h
.over:	
	clc
	mov	byte [fdc_last_error],0
	jmp	int_13_exit

int_13_fn16:
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
	jmp	int_13_exit


;Return drive type as 1.44, 0 if disabled
int_13_drivetype:
	mov	al,[fd_enabled]
	je	return_1440
	xor	cx,cx
	xor	di,di
	mov	byte [bp+int_13_bl],cl	; drive type is zero
	mov	byte [bp+int_13_dh],cl	; maximal head number is zero
	mov	es,cx			; disk parameter table segment = 0000h
	jmp	int_13_comp

return_1440:
	mov	ah,1
	mov	byte [bp+int_13_dl],ah	; pass number of drives to caller
	mov	byte [bp+int_13_dh],1	; maximal head number is 1 for floppy
	mov	al,4
	mov	byte [bp+int_13_bl],al	; pass drive type to caller
	mov	cx,cs
	mov	es,cx			; diskette parameter table segment

	mov	cx,4F12h		; 1.44M - 80 cylinders, 18 sectors

	mov	di,media_1440	

int_13_comp:
	xor	ax,ax			; AH = 00h - successful completion
	mov	byte [bp+int_13_al],al	; successful completion
	mov	byte [fdc_last_error],al
	mov	byte [bp+int_13_bh],al	; clear BH just in case
	mov	word [bp+int_13_cx],cx	; cylinders / sectors
	mov	word [bp+int_13_di],di	; diskette parameter table pointer
	jmp	int_13_exit

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
int_13_reset:
	call	reset_controller
	call	read_boot
	jc	.no_disk
	mov	al,[fd_media]
	cmp	al,1
	jne	.changed
	xor	ah,ah
	jmp	int_13_exit	
	
.changed:
	mov	al,1
	mov	[fd_media],al
	mov	ah,fdc_e_changed
	mov	[fdc_last_error],ah
	jmp	int_13_exit

.no_disk:
	xor	ah,ah
	mov	[fd_media],ah
	dec	ah
	stc
	jmp	int_13_exit		



int_13_geterror:
	mov	al,[fdc_last_error]
	mov	byte [bp+int_13_al],al
	clc
	jmp	int_13_exit

;Read sectors
int_13_read:
	call	chs_to_lba
	jc	int_13_exit
	call	read_sectors
	jnc	int_13_exit
	;call	beep
	jmp	int_13_exit

;Write sectors
int_13_write:
	call	chs_to_lba
	jc	int_13_exit
	call	write_sectors
	jnc	int_13_exit
	;call	beep
	jmp	int_13_exit


detect_floppy:
	push	ds
	push	dx
	mov	ax,biosdseg
	mov	ds,ax
	xor	al,al
	mov	[fd_enabled],al
	mov	[fd_media],al	

	call	reset_controller
	jc	.exit
	call	read_boot
	jc	.exit
	mov	al,1
	mov	[fd_enabled],al
	mov	[fd_media],al
	or	byte [equipment_list],equip_floppies
.exit:
	pop	dx
	pop	ds
	ret

msg_ch375	db	'Floppy disk drives:         Drive 0: CH375 USB', 00h
msg_nousb	db	'Floppy disk drives:         None', 00h

print_floppy:
	push	ax
	push	si
	push	ds
	mov	ax,biosdseg
	mov	ds,ax
	mov	al,[fd_enabled]
	pop	ds
	mov	si,msg_ch375
	test	al,al
	jnz	.printen
	mov	si,msg_nousb
.printen:
	call	print
	mov	si,msg_crlf
	call	print
	pop	si
	pop	ax
	ret


;=========================================================================
; int_0E - IRQ6 (FDC) service routine
; Sets bit 7 in drive recalibration status byte (fdc_calib_state)
;-------------------------------------------------------------------------
	setloc	0EF57h

int_0E:
	push	ax
	push	ds
	mov	ax,biosdseg
	mov	ds,ax
	or	byte [fdc_calib_state],fdc_irq_flag ; set IRQ flag
	pop	ds
	mov	al,20h			; send EOI to PIC
	out	pic1_reg0,al
	mov	ax,9101h		; call "interrupt completed" OS hook
	int	15h
	pop	ax
	iret

;=========================================================================
; Disk parameter tables in INT 1Eh format
; Byte 0: First byte for specify command:
;	bits 7-4: step rate ([32 ms - value * 2], e.g. 0D0h is 32-13*2 = 6 ms)
;	bits 3-0: head unload time (0Fh = 240 ms)
; Byte 1: Second byte for specify command:
;	bits 7-1: head load time (1 = 4 ms)
;	bit 0: non-DMA mode (always 0)
; Byte 2: motor off timeout in clock ticks (25h - approximately 2 seconds)
; Byte 3: sector size (00h - 128, 01h - 256, 02h - 512, 03h - 1024)
; Byte 4: sectors per track
; Byte 5: gap length for read/write (2Ah for 5.25", 1Bh for 3.5")
; Byte 6: special sector size (0FFh - not used)
; Byte 7: gap length for formating (50h for 5.25", 6Ch for 3.5")
; Byte 8: format filler byte (default 0F6h)
; Byte 9: head settle time in milliseconds
; Byte A: motor start time, in 1/8 seconds
; Byte B: number of cylinders minus one
; Byte C: transfer rate (in bits 7 - 6)
;-------------------------------------------------------------------------
media_360_in_360:
	db	0DFh, 02h, 25h, 02h, 09h, 2Ah, 0FFh, 50h, 0F6h, 0Fh, 08h
	db	27h, 80h
media_1200:
	db	0DFh, 02h, 25h, 02h, 0Fh, 1Bh, 0FFh, 54h, 0F6h, 0Fh, 08h
	db	4Fh, 00h
media_720:
	db	0DFh, 02h, 25h, 02h, 09h, 2Ah, 0FFh, 50h, 0F6h, 0Fh, 08h
	db	4Fh, 80h
media_1440:
	db	0BFh, 02h, 25h, 02h, 12h, 1Bh, 0FFh, 6Ch, 0F6h, 0Fh, 08h
	db	4Fh, 00h
media_360_in_1200:
	db	0DFh, 02h, 25h, 02h, 09h, 23h, 0FFh, 50h, 0F6h, 0Fh, 08h
	db	27h, 40h
media_2880:
	db	0AFh, 02h, 25h, 02h, 24h, 1Bh, 0FFh, 50h, 0F6h, 0Fh, 08h
	db	4Fh, 0C0h


;=========================================================================
; Disk parameter table for IBM compatibility
; Using 2.88M disk as it has maximal number of cylinders and sectors
;-------------------------------------------------------------------------
	setloc	0EFC7h
int_1E:
	db	0AFh, 02h, 25h, 02h, 24h, 1Bh, 0FFh, 50h, 0F6h, 0Fh, 08h
;Hmm, this was commented in floppy2
;	db	4Fh, 0C0h