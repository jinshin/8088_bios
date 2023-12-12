; bootplay.inc - PWM sound module for 8088_bios
; Plays ACOMP compressed audio on PC Speaker
; 4.7Mhz 8088 should be enough
; This is a compilation of research results of MUSHROOM.COM and ACOMP sound compressor,
; done by Serhii Liubshin, with mush appreciated help of mkarcher @ vogons
; bootsnd.inc should be 18356Hz 8-bit audio, compressed with ACOMP
; Audio byte should be in range 01-40h, but other values should be tolerated, but somewhat decrease sound quality 
; MUSHROOM.COM: R&D Team at the Cleveland Corp. of Australia at Hendra, date and license unknown
; ACOMP: Written by John W. Ratcliff, 1991, license unknown 
; So now it's GPL
; Decompression is done to memory segment 0x2000:0
; Works for me

%macro ShiftR 2
    times %2 shr %1, 1
%endmacro

%macro ShiftL 2
    times %2 shl %1, 1
%endmacro

%macro Delta1 0
    shl ah, 1      ; Rotate bit mask out.
    jc %%up
    sub dx, bx
    jns %%store
    xor dx, dx     ; Zero it out.
    jmp short %%store

%%up:
    add dx, bx
    or dh, dh
    jz %%store
    mov dx, 255

%%store:
    mov al, dl     ; Store result.
    stosb
%endmacro

%macro DeModulate 0
    xlat		; Translate into lookup table.
    cbw            ; Make it a signed word.
    add dx, ax     ; Do word-sized add, into previous.
    jns %%high
    xor dx, dx     ; Underflowed.

%%high:
    or dh, dh      ; Did it overflow?
    jz %%ok
    mov dx, 255    ; Maxed out.

%%ok:
    mov al, dl
    stosb
%endmacro

bootsound:	
%ifdef BIOS_SETUP	
		call	get_config_a
		test	al,conf_bootsnd ;0 - enabled, default
		jz	.go_on
		ret		
%endif
.go_on:	
		jmp sndstart
;
int08_offset	equ 08h*4
;Uncompressor data
SQLCH		equ	64      ; Squelch byte flag
RESYNC		equ	128      ; Resync byte flag.

DELTAMOD	equ	00110000b   ; Bit mask for delta mod bits.

ONEBIT		equ	00010000b   ; Bit pattern for one bit delta mod.
TWOBIT		equ	00100000b   ; Bit pattern for two bit delta mod.
FOURBIT		equ	00110000b   ; Bit pattern for two bit delta mod.

;base   		dw	0      ; Base address inside translate table.

TRANS   db   -8,-7,-6,-5,-4,-3,-2,-1,1,2,3,4,5,6,7,8
   db   -16,-14,-12,-10,-8,-6,-4,-2,2,4,6,8,10,12,14,16
   db   -24,-21,-18,-15,-12,-9,-6,-3,3,6,9,12,15,18,21,24
   db   -32,-28,-24,-20,-16,-12,-8,-4,4,8,12,16,20,24,28,32
   db   -40,-35,-30,-25,-20,-15,-10,-5,5,10,15,20,25,30,35,40
   db   -48,-42,-36,-30,-24,-18,-12,-6,6,12,18,24,30,36,42,48
   db   -56,-49,-42,-35,-28,-21,-14,-7,7,14,21,28,35,42,49,56
   db   -64,-56,-48,-40,-32,-24,-16,-8,8,16,24,32,40,48,56,64
   db   -72,-63,-54,-45,-36,-27,-18,-9,9,18,27,36,45,54,63,72
   db   -80,-70,-60,-50,-40,-30,-20,-10,10,20,30,40,50,60,70,80
   db   -88,-77,-66,-55,-44,-33,-22,-11,11,22,33,44,55,66,77,88


int08_handler:
		lodsb
		dec	bx
                jz      short terminate

		out	42h,al

		mov	al,ch
		;Signal EOI		
                out     20h,al
		;avoid stack overflow
		add	sp,di        	
                sti
                hlt

ethernal_wait:                 
                in      al,20h
                and     al,1
                jnz     short ethernal_wait
                hlt
                jmp     short ethernal_wait

;Outro
terminate:
                cli

		mov	sp,dx

                mov     al,48h
                out     61h,al

                mov     al,54h
                out     43h,al         
                mov     al,12h
                out     41h,al         
                mov     al,36h
                out     43h,al
                mov     al,0
                out     40h,al 
                out     40h,al 
                mov     al,0B6h
                out     43h,al         
                mov     ax,533h
                out     42h,al         
                mov     al,ah
                out     42h,al         
                mov     al,0BCh
                out     21h,al
                mov     al,20h
                out     20h,al

		cld

		push	cs
		pop	ds

		xor	ax,ax
		mov	es,ax
		mov	di,int08_offset
		pop	ax
		mov	bx,ax
		pop	ax
		stosw
		mov	ax,bx
		stosw

		pop	es
		pop	ds

		ret		


		sti 

;Bye!
                ret  

sndstart:
		push	ds
		push	es

		;decompress sound
		mov	ax,2000h
		mov	ds,ax
		push	ds
		;mov	ax,waveend
		;Destination buffer
		xor	ax,ax
		push    ax

		;Source buffer
		push	cs
		mov	ax,wavestart
		push    ax

		call	UnCompressAudio
		add	sp,8   ;remove call parameters
		;return AX as uncompressed size       		

;counter for player
		mov	bx,ax
;run

		cli
		cld

		xor 	ax,ax
	        mov     ds,ax

        	mov     si,int08_offset
		lodsw
		push	ax
		lodsw
		push	ax


        	push    ds
        	pop     es
        	mov     di,int08_offset
        	mov     ax,int08_handler
        	stosw
        	mov     ax,cs
        	stosw

		mov	al,36h
		out	43h,al
		mov	ax,41h
		out	40h,al
		mov	al,ah
		out	40h,al
		mov	al,90h
		out	43h,al

		in	al,61h
		or	al,3
		out	61h,al

		mov	al,0ah
		out	20h,al

		mov	ch,20h

		mov	cl,4Bh
		mov	di,6 

		mov	ax,2000h
		mov	ds,ax
		xor	si,si

		mov	dx,sp

		sti
		jmp ethernal_wait

UnCompressAudio:

;   		ARG   SHAN:DWORD,DHAN:DWORD
;   		LOCAL   SLEN:WORD,FREQ:WORD,FRAME:WORD,BITS:WORD = LocalSpace
;   		PENTER   LocalSpace
;   		PushCREGS

;SLEN -2
;FREQ -4
;FRAME -6

        	push    bp
        	mov     bp,sp
        	sub     sp,8 	;size of local vars
        	cld

   		lds   si,[bp+4]      ; Get source segment
   		les   di,[bp+8]      ; Get destination segment

   		lodsw            ; Get length.

		mov   [bp-2],ax      ; Save length.
		mov   cx,ax         ; Into CX
		lodsw            ; Frequency.
		mov   [bp-4],ax      ; Save frequency
		lodsb            ; Get frame size.
		xor   ah,ah         ; Zero high byte
		mov   [bp-6],ax      ; Save it.
		lodsb            ; Get squelch, and skip it.
		lodsw            ; Get maximum error, and skip it.
		lodsb            ; Get initial previous data point.
		stosb            ; Store it.
		xor   ah,ah         ; zero high byte.
		mov   dx,ax      ; Save into previous word.
		dec   cx      ; Decrement total by one.
		jz   @@DONE      ; Exit
		mov   ah,al      ; AH, always the previous.
@@DCMP:
		lodsb         ; Get sample.
		test   al,RESYNC   ; Resync byte?
		jz   @@NOTR      ; no, skip.
		shl   al,1      ; Times two.
		mov   dl,al      ; Into previous.
		xor   dh,dh      ; Zero high word.
		stosb         ; Store it.
		loop   @@DCMP      ; Next one.
		jmp   @@DONE

@@NOTR: 
		test   al,SQLCH   ; Squelch byte?
		jz   @@FRAM      ; no, then it is a frame.
		and   al,00111111b   ; Leave just the count.
		push   cx      ; Save current countdown counter.
		mov   cl,al      ; get repeat count
		xor   ch,ch      ; zero high byte of CX
		mov   bx,cx      ; Repeat count in DX
		mov   al,dl      ; Repeat of previous.
		rep   stosb      ; Repeat it.
		pop   cx      ; Get back remaining count.
		sub   cx,bx      ; Less.
   		;check over
   		jc	 @@DONE
		jnz   @@DCMP      ; Keep going.
		jmp   @@DONE
@@FRAM:
		mov   bx,ax      ; command byte into BX
		and   bx,0Fh      ; Multiplier being used.
		ShiftL   bx,4      ; Times 16.
		add   bx,TRANS ; Plus address of translate table.
		and   al,DELTAMOD   ; Leave just delta mod.
		push   cx
		mov   cx,[bp-6]   ; Get frame size.
		cmp   al,ONEBIT   ; In one bit delta mod?
		jne   @@NEXT1    ; no, try other.
		ShiftR   cx,3      ; /8
		mov   bl,cs:[bx+8]   ; Get up amount
		xor   bh,bh      ; Zero high byte.
@@GO:
		lodsb
		xchg   al,ah      ; Place prev in AL, Bit mask in AH
		Delta1
		Delta1
		Delta1
		Delta1
		Delta1
		Delta1
		Delta1
		Delta1
		mov   ah,al
		dec	cx
		jnz	@@GO
		jmp   @@RENTER
@@NEXT1:
		cmp   al,TWOBIT   ; In two bit delta mod mode?
		jne   @@NEXT2
		add   bx,6      ; Point at +- 2 bit's in table.
		shr   cx,1
		shr   cx,1      ; 4 samples per byte.
@@GOGO:
		lodsb
		ShiftR   al,6
		DeModulate
		mov   al,ds:[si-1]
		ShiftR   al,4
		and   al,3
		DeModulate
		mov   al,ds:[si-1]
		ShiftR   al,2
		and   al,3
		DeModulate
		mov   al,ds:[si-1]
		and   al,3
		DeModulate
   		loop   @@GOGO
		jmp short @@RENTER
@@NEXT2:
		shr   cx,1      ; Two samples per byte.
@@GO2:
		lodsb         ; Get sample.
		ShiftR   al,4
		DeModulate
		mov   al,ds:[si-1]
		and   al,0Fh
		DeModulate
		loop   @@GO2
@@RENTER:
		pop   cx
		sub   cx,[bp-6]
		jc	 @@DONE
		jnz   @@DCMP      ; Continue decompress
@@DONE:
		mov   ax,[bp-2]   ; Uncompressed length.

;   PopCREGS
;   PLEAVE

		mov     sp,bp
		pop     bp
   		ret
		
wavestart:
		%include "bootsnd.inc"
waveend:
