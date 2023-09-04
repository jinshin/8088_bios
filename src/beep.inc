;=========================================================================
; beep - Play a beep sound
; Input:
;	BL - duration in 0.1 second
; Output:
;	BL = 0
;-------------------------------------------------------------------------
beep:
	push	ax
	mov	al,0B6h			; set PIC channel 2 to mode 3
	out	pit_ctl_reg,al
	mov	ax,1193			; approximately 1000 Hz
	out	pit_ch2_reg,al		; load divisor's low byte to PIC
	mov	al,ah
	out	pit_ch2_reg,al		; load divisor's high byte to PIC
	in	al,ppi_pb_reg		; read current value of PORT B
	or	al,03h			; turn on the speaker
	out	ppi_pb_reg,al		; write the new value
.loop:
	mov	cx,6666			; 0.1 second delay
	call	delay_15us
	dec	bl
	jnz	.loop
	xor	al,03h			; turn off the speaker
	out	ppi_pb_reg,al		; write the new value
	pop	ax
	ret