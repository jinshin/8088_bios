;=========================================================================
; messages.inc - Messages printed by BIOS POST (Power On Self Test)
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

msg_copyright	db	0Dh, 0Ah
%ifdef MACHINE_XI8088
		db	"Xi 8088"
%endif ; MACHINE_XI8088
%ifdef MACHINE_FE2010A
		db	"Micro 8088"
%endif ; MACHINE_FE2010A
%ifdef MACHINE_BOOK8088
		db	"Book8088"
%endif ; MACHINE_BOOK8088
%ifdef MACHINE_MISTER
		db	"MiSTer PC XT"
%endif
%ifdef MACHINE_XT
		db	"XT 8088"
%endif ; MACHINE_XT
%ifdef MACHINE_MARTYPC
		db	"MartyPC"
%endif ; MACHINE_MARTYPC
		db	" BIOS, Version "
		db	VERSION
		db	". "
		db	"Copyright (C) 2010 - ",FULLYEAR," Sergey Kiselev", 0Dh, 0Ah
%ifdef MACHINE_BOOK8088 or MACHINE_MARTYPC or MACHINE_MISTER
		db	"Parts (C) 2023 - ",FULLYEAR," Serhii Liubshin. Built ",__UTC_DATE__," ",__UTC_TIME__," UTC", 0Dh, 0Ah
%endif
		db	"Distributed under the terms of "
		db	"the GNU General Public License", 0Dh, 0Ah
%ifdef MACHINE_MISTER
		db	00h ;Or the topmost copyright string will be overwritten by XUB
%endif
msg_crlf	db      0Dh, 0Ah, 00h
msg_none	db	'none', 00h
msg_colon	db	': ', 00h
msg_semicolon	db	'; ', 00h
%ifdef TURBO_MODE
msg_cpu_clk	db	'CPU clock frequency:        ', 00h
msg_at		db	' @ ', 00h
msg_clk_4_77mhz	db	'4.77 MHz ', 00h
%ifdef MACHINE_FE2010A
msg_clk_7_16mhz	db	'7.16 MHz ', 00h
msg_clk_9_55mhz	db	'9.55 MHz ', 00h
%else ; MACHINE_FE2010A
msg_clk_turbo	db	'Turbo ', 00h
%endif ; MACHINE_FE2010A
%endif ; TURBO_MODE
msg_cpu		db      'Main Processor:             ', 00h
msg_fpu		db      'Mathematics Co-processor:   ', 00h
msg_cpu_8088_78	db      "Intel 8088 '78", 00h
msg_cpu_bug	db      'WARNING: This CPU does not disable interrupts '
		db      'after loading segment registers!', 0Dh, 0Ah, 00h
msg_cpu_8088_81	db      "Intel 8088 '81 or later, "
		db      'or OKI-designed 80C88', 00h
msg_cpu_harris  db      'Harris-designed 80C88', 00h
msg_cpu_nec_v20 db      'NEC V20', 00h
msg_fpu_present db      'Intel 8087', 0Dh, 0Ah, 00h
%ifdef MACHINE_FE2010A
msg_chipset	db	'Chipset:                    ', 00h
msg_fe2010a	db	'Faraday FE2010A', 0Dh, 0Ah, 00h
msg_pt8010af	db	'Proton PT8010AF', 0Dh, 0Ah, 00h
%endif ; MACHINE_FE2010A
msg_disp	db      'Display Adapter Type:       ', 00h
msg_disp_ega	db      'EGA/VGA (Video BIOS Present)', 0Dh, 0Ah, 00h
msg_disp_cga_80	db      'CGA (80x25)', 0Dh, 0Ah, 00h
msg_disp_cga_40	db      'CGA (40x25)', 0Dh, 0Ah, 00h
msg_disp_mda	db      'MDA or Hercules', 0Dh, 0Ah, 00h
msg_rtc		db	'RTC (Real Time Clock):      ', 00h
msg_floppy	db	'Floppy Disk Drives:         Drive 0: ', 00h
msg_floppy_2	db	'; Drive 1: ', 00h
msg_floppy_360	db	'360 KB, 5.25"', 00h
msg_floppy_1200	db	'1.2 MB, 5.25"', 00h
msg_floppy_720	db	'720 KB, 3.5"', 00h
msg_floppy_1440	db	'1.44 MB, 3.5"', 00h
msg_floppy_2880	db	'2.88 MB, 3.5"', 00h

%ifdef CH375_FLOPPY
msg_floppy_ch375 db	'CH375 USB', 00h
%endif

%ifdef XT_CF
%ifdef MACHINE_MISTER
msg_cf		db	'XT-IDE Master Drive:        ', 00h
msg_nocf	db	'XT-IDE Master Drive:        None', 00h
%else
msg_cf		db	'Compact Flash Drive:        ', 00h
msg_nocf	db	'Compact Flash Drive:        None', 00h
%endif
%endif

%ifdef PS2_MOUSE
msg_mouse	db      'PS/2 Aux Device (Mouse):    ', 00h
%endif ; PS2_MOUSE
msg_present	db      'Present', 0Dh, 0Ah, 00h
msg_absent	db      'Absent', 0Dh, 0Ah, 00h
msg_serial	db	'Serial Ports:               ', 00h
msg_serial_com	db	'COM', 00h
msg_parallel	db	'Parallel Ports:             ', 00h
msg_parallel_lpt db	'LPT', 00h
msg_ram_testing	db	'Testing RAM (ESC to skip):  ', 00h
msg_ram_error	db	0Dh, 0Ah, 'ERROR: Faulty memory detected at ', 00h
msg_ram_total	db      0Dh, 'Total Conventional RAM:     ', 00h
%ifdef EBDA_SIZE
msg_ebda	db      'Reserved for EBDA:          ', 00h
msg_ram_avail	db      'Available Conventional RAM: ', 00h
%endif ; EBDA_SIZE
msg_kib		db      ' KiB', 0Dh, 0Ah, 00h
msg_boot_failed	db      'Boot failed, press any key to try again...', 0Dh, 0Ah, 0
msg_no_basic    db      'No ROM BASIC', 0Dh, 0Ah, 0
msg_rom_found   db      'Found BIOS extension ROM at ', 0
msg_rom_init	db	'0, initializing...', 0Dh, 0Ah, 0
msg_boot	db      'Booting OS...', 0Dh, 0Ah, 0
%ifdef AT_RTC
msg_rtc_bad	db	'ERROR: RTC battery is bad', 0Dh, 0Ah, 00h
%endif ; AT_RTC
%ifdef AT_RTC_NVRAM
msg_rtc_sum	db	'ERROR: NVRAM checksum is invalid, '
		db	'loading default values to NVRAM', 0Dh, 0Ah, 00h
%endif ; AT_RTC_NVRAM
%ifdef BIOS_SETUP
msg_setup	db	'Press F1 to run BIOS setup...',0Dh, 0Ah, 0Dh, 0Ah, 00h
msg_set_welcome	db	0Dh, 0Ah, 'Welcome to the BIOS setup utility', 00h
msg_set_help	db	0Dh, 0Ah
		db	'BIOS setup commands:', 0Dh, 0Ah
		db	'f - Set first floppy drive type', 0Dh, 0Ah
		db	'g - Set second floppy drive type', 0Dh, 0Ah
		db	'e - BIOS extension ROM scan settings', 0Dh, 0Ah
		db	'm - Memory test settings', 0Dh, 0Ah

%ifdef BOOT_SOUND
		db	'b - Boot sound settings', 0Dh, 0Ah
%endif

%ifdef CH375_FLOPPY
		db	'u - USB floppy emulation settings', 0Dh, 0Ah
%endif

%ifdef TURBO_MODE
		db	'c - Set default CPU clock frequency', 0Dh, 0Ah
%endif ; TURBO_MODE
%ifdef MACHINE_FE2010A
		db	's - Change wait state settings', 0Dh, 0Ah
%endif ; MACHINE_FE2010A
		db	00h
%ifdef AT_RTC
msg_set_rtc_help db	't - Set time', 0Dh, 0Ah
		db	'd - Set date', 0Dh, 0Ah, 00h
%endif ; AT_RTC
msg_set_def_help db	'p - Print current settings', 0Dh, 0Ah
		db	'w - Save changes and exit', 0Dh, 0Ah
%ifdef XT_CF
		db	'a - Apply (w/o save) changes and exit', 0Dh, 0Ah
%endif
		db	'q - Exit without saving changes', 0Dh, 0Ah
		db	'h - Show this help', 0Dh, 0Ah, 00h
msg_set_prompt	db	0Dh, 0Ah, 'Enter command (h for help): ', 00h
%ifdef TURBO_MODE
msg_set_clk_help db 	0Dh, 0Ah
		db	'CPU clock frequency options:', 0Dh, 0Ah
		db	'4 - 4.77 MHz', 0Dh, 0Ah
%ifdef MACHINE_FE2010A
		db	'7 - 7.16 MHz', 0Dh, 0Ah
		db	'9 - 9.55 MHz', 0Dh, 0Ah, 00h
%else ; MACHINE_FE2010A
		db	't - Turbo', 0Dh, 0Ah, 00h
%endif ; MACHINE_FE2010A
msg_set_clk_prmt db	0Dh, 0Ah, 'Enter CPU clock frequency (h for help): '
		db	00h
%endif ; TURBO_MODE
%ifdef AT_RTC
msg_set_time	db	'Enter time (hh:mm:ss): ', 00h
msg_set_date	db	'Enter date (YYYY-MM-DD): ', 00h
msg_time_inval	db	'ERROR: Invalid time.', 0Dh, 0Ah, 00h
msg_date_inval	db	'ERROR: Invalid date.', 0Dh, 0Ah, 00h
%endif ; AT_RTC
msg_set_fd_help	db	0Dh, 0Ah
		db	'Floppy drive types:', 0Dh, 0Ah
		db	'0 - Not installed   3 - 720 KB, 3.5"', 0Dh, 0Ah
		db	'1 - 360 KB, 5.25"   4 - 1.44 MB, 3.5"', 0Dh, 0Ah
		db	'2 - 1.2 MB, 5.25"   6 - 2.88 MB, 3.5"', 0Dh, 0Ah, 00h
msg_set_sub_help db	'Setup commands:', 0Dh, 0Ah
		db	'r - Return to the main menu', 0Dh, 0Ah
		db	'h - Show this help', 0Dh, 0Ah, 00h
msg_set_fd_prmt	db	0Dh, 0Ah, 'Enter floppy drive type (h for help): '
		db	00h
msg_set_ext	db	'Scan F0000-F7FFF area for BIOS extension ROMs (y/n): '
		db	00h
msg_ext_scan	db	'F0000-F7FFF BIOS ext. scan: ', 00h
msg_set_mem_tst	db	'Enable memory test (y/n): ', 00h
msg_mem_test	db	'Memory test:                ', 00h
%ifdef BOOT_SOUND
msg_set_bootsnd	db	'Enable boot sound (y/n): ', 00h
msg_bootsnd	db	'Boot sound:                 ', 00h
%endif
%ifdef CH375_FLOPPY
msg_usbemu_help db	0Dh, 0Ah, 0Dh, 0Ah
		db	'USB floppy emulation is always autoenabled', 0Dh, 0Ah
		db	'if floppy drive A:(0) type is set to zero', 0Dh, 0Ah
		db	'and USB flash drive is inserted on boot', 0Dh, 0Ah
		db	'This option will enforce it regardless of drive', 0Dh, 0Ah
		db	'type selection and actual USB flash drive presence.', 
		db	0Dh, 0Ah, 0Dh, 0Ah, 00h
msg_set_usbemu	db	'Enforce USB floppy emulation (y/n, h for help): ', 00h
msg_usbemu	db	'USB floppy enforcement:     ', 00h
%endif
%ifdef MACHINE_FE2010A
msg_set_ws	db	'Fast wait states (y/n, h for help): ', 00h
msg_ws_help	db	0Dh, 0Ah
		db	'Wait states configuration for FE2010A and PT8010AF:',
		db	0Dh, 0Ah
		db	'7.15 MHz, normal: I/O - 4 WS; Upper memory - 2 WS'
		db	0Dh, 0Ah
		db	'7.15 MHz, fast:   I/O - 4 WS; Upper memory - 0 WS'
		db	0Dh, 0Ah
		db	'9.54 MHz, normal: I/O - 6 WS; Upper memory - 4 WS'
		db	0Dh, 0Ah
		db	'9.54 MHz, fast:   I/O - 4 WS; Upper memory - 2 WS'
		db	0Dh, 0Ah, 00h
msg_ws		db	'Fast wait states:           ', 00h
%endif ; MACHINE_FE2010A
msg_enabled	db	'Enabled', 00h
msg_disabled	db	'Disabled', 00h
%ifdef FLASH_NVRAM
msg_saving	db	0Dh, 0Ah, 'Saving configuration to Flash ROM... ', 00h
msg_save_ok	db	'OK', 0Dh, 0Ah, 00h
msg_save_error	db	'ERROR', 0Dh, 0Ah, 00h
%endif ; FLASH_NVRAM
%ifdef XT_CF
msg_cannot	db	0Dh, 0Ah, "Boot sector data present, cannot save!", 0Dh, 0Ah, 00h
msg_saving	db	0Dh, 0Ah, "Saving configuration to boot sector."
		db	0Dh, 0Ah, "Don't do that often!", 0Dh, 0Ah, 00h 
msg_save_ok	db	'OK', 0Dh, 0Ah, 00h
msg_save_error	db	'ERROR', 0Dh, 0Ah, 00h
%endif
%endif ; BIOS_SETUP
tbl_floppy	dw	msg_none
		dw	msg_floppy_360
		dw	msg_floppy_1200
		dw	msg_floppy_720
		dw	msg_floppy_1440
		dw	msg_none
		dw	msg_floppy_2880
%ifdef CH375_FLOPPY
		dw	msg_floppy_ch375
%else
		dw	msg_none
%endif