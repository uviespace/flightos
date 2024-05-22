/* ld script for sparc32 kernel */
/* based on macros from linux/include/asm-generic/vmlinux.lds.h */

OUTPUT_FORMAT("elf32-sparc", "elf32-sparc", "elf32-sparc")
OUTPUT_ARCH(sparc)
ENTRY(_start)

SECTIONS
{
	 . = ALIGN(0x1000);
	.text TEXTSTART :
	{
		_text = .;
		*(.head.text)
		*.(text .text.*)
		*(.gnu.warning)
	} = 0
	_etext = .;

	. = ALIGN(16);
	.cdtors : {
               ___CTOR_LIST__ = .;
	       LONG((___CTOR_END__ - ___CTOR_LIST__) / 4 - 2)
               *(SORT(.ctors.*))
               *(.ctors)
               ___CTOR_END__ = .;
               LONG(-1)
	}

	. = ALIGN(16);
	.rodata : {
		__start_rodata = .;
		*(.rodata) *(.rodata.*)
	}


	/* start of data section */
	_sdata = .;
	.data : {
		*(.data .data.*)
	}
	/* end of data section */
	_edata = .;


	. = ALIGN(16);
	__init_begin = .;
	.init.data : {
		/* linux-style init calls */
		__initcall_start = .;
		KEEP(*(.initcall1.init))
		KEEP(*(.initcall2.init))
		KEEP(*(.initcall3.init))
		KEEP(*(.initcall4.init))
		KEEP(*(.initcall5.init))
		KEEP(*(.initcall6.init))
		KEEP(*(.initcall7.init))
		__initcall_end = .;
	}
	__init_end = .;



	. = ALIGN(8);
	__bss_start = .;

	. = ALIGN(8);
	.bss : {
		*(.bss .bss.*)
		*(COMMON)
	}

	. = ALIGN(8);
	__bss_stop = .;
	_end = . ;

	. = ALIGN(16);
	.eh_frame : {*(.eh_frame)}

	.stab 		0 : { *(.stab) }
	.stabstr 	0 : { *(.stabstr) }
	.stab.excl 	0 : { *(.stab.excl) }
	.stab.exclstr 	0 : { *(.stab.exclstr) }
	.stab.index 	0 : { *(.stab.index) }
	.stab.indexstr 	0 : { *(.stab.indexstr) }
	.comment 	0 : { *(.comment) }


	.debug           	0 : { *(.debug) }
	.line            	0 : { *(.line) }
	.debug_srcinfo   	0 : { *(.debug_srcinfo) }
	.debug_sfnames   	0 : { *(.debug_sfnames) }
	.debug_aranges   	0 : { *(.debug_aranges) }
	.debug_pubnames  	0 : { *(.debug_pubnames) }
	.debug_info      	0 : { *(.debug_info .gnu.linkonce.wi.*) }
	.debug_abbrev    	0 : { *(.debug_abbrev) }
	.debug_line      	0 : { *(.debug_line) }
	.debug_frame     	0 : { *(.debug_frame) }
	.debug_str       	0 : { *(.debug_str) }
	.debug_loc       	0 : { *(.debug_loc) }
	.debug_macinfo   	0 : { *(.debug_macinfo) }
	.debug_pubtypes  	0 : { *(.debug_pubtypes) }
	.debug_ranges	 	0 : { *(.debug_ranges) }
	.debug_gnu_pubnames 	0 : { *(.debug_gnu_pubnames) }
	.debug_gnu_pubtypes 	0 : { *(.debug_gnu_pubtypes) }
	.debug_types		0 : { *(.debug_types) }
	.debug_macro		0 : { *(.debug_macro) }
	.debug_addr		0 : { *(.debug_addr) }
}

