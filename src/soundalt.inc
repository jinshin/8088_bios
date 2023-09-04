;=========================================================================
; soundalt.inc - Play alternate power-on sound. This also tests PIT and PORT B functionality
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

notes   dw	pic_freq/262		;C G D
	dw	pic_freq/393
	dw      pic_freq/589
	dw	0

;=========================================================================
; sound - Play power-on sound.
; Input:
;	none
; Output:
;	none, destroys some registers
;-------------------------------------------------------------------------
sound:
	cld
	mov	si,notes		; set SI to notes table

	mov	al,0B6h			; set PIC channel 2 to mode 3
	out	pit_ctl_reg,al

.loop:
    cs	lodsw				; load next note to AX
	cmp	ax,0
	jz	.exit
	jl	.silent			; don't turn on speaker if silent note
	out	pit_ch2_reg,al		; load divisor's low byte to PIC
	mov	al,ah
	out	pit_ch2_reg,al		; load divisor's high byte to PIC

	in	al,ppi_pb_reg		; read current value of PORT B
        or      al,03h			; turn on the speaker
        out     ppi_pb_reg,al		; write the new value

.silent:
	mov	cx,2000h		; was 3000h
	call	delay_15us		; delay while note is playing

	in	al,ppi_pb_reg		; read current value of PORT B
	and	al,0FCh			; turn off the speaker
        out     ppi_pb_reg,al		; write the new value

        mov     cx,0050h		; was 100h
	call	delay_15us		; delay after the note
        
        jmp     .loop			; play the next note
.exit:
	ret
