/*
 * Copyright (c) 2011, Recore Systems B.V., The Netherlands,
 * web: www.recoresystems.com, email: info@recoresystems.com
 *
 * Any reproduction in whole or in parts is prohibited
 * without the written consent of the copyright owner.
 *
 * All Rights Reserved.
 */

STARTUP(crt0.o)
INPUT(-lc -lcompiler_rt)

OUTPUT_FORMAT("elf32-xentium")
OUTPUT_ARCH(xentium)
ENTRY(_start)

_MINIMUM_STACK_SIZE = 1K;

MEMORY
{
  dbank0 : ORIGIN = 0x00000000, LENGTH = 8K
  dbank1 : ORIGIN = 0x00002000, LENGTH = 8K
  dbank2 : ORIGIN = 0x00004000, LENGTH = 8K
  dbank3 : ORIGIN = 0x00006000, LENGTH = 8K
  ram    : ORIGIN = 0x50000000, LENGTH = 256M
}

PHDRS
{
  code    PT_LOAD;
  data    PT_LOAD;
  tcm     PT_LOAD;
}

SECTIONS
{
  /* RAM code and data */
  .text : {
    *(.text)
    . = ALIGN(16*4) ; /* Align to the next cache line boundary */
  } > ram : code
  .rodata ALIGN(0x40) : {
    *(.rodata .rodata.*)
    . = ALIGN(4) ;
  } > ram : data
  .data ALIGN(0x40) : {
    *(.data)
    . = ALIGN(4) ;
  } > ram : data
  .bss ALIGN(0x40) : {
   __bss_start = .;
    *(.bss)
    . = ALIGN(4) ;
  } > ram : data
  _end = __bss_start + SIZEOF(.bss);

  /* TCM */
  .dbank0        ORIGIN(dbank0)      : { *(.dbank0) }        > dbank0 : tcm
  .dbank0_noinit ALIGN(0x4) (NOLOAD) : { *(.dbank0_noinit) } > dbank0 : tcm
  .dbank1        ORIGIN(dbank1)      : { *(.dbank1) }        > dbank1 : tcm
  .dbank1_noinit ALIGN(0x4) (NOLOAD) : { *(.dbank1_noinit) } > dbank1 : tcm
  .dbank2        ORIGIN(dbank2)      : { *(.dbank2) }        > dbank2 : tcm
  .dbank2_noinit ALIGN(0x4) (NOLOAD) : { *(.dbank2_noinit) } > dbank2 : tcm
  .dbank3        ORIGIN(dbank3)      : { *(.dbank3) }        > dbank3 : tcm
  .dbank3_noinit ALIGN(0x4) (NOLOAD) : { *(.dbank3_noinit) } > dbank3 : tcm

  /*
   * Set the stack pointer to top of data bank 3 or top of the RAM depending on
   * the available space in data bank 3.
   */
  _remaining_db3 = (LENGTH(dbank3) - SIZEOF(.dbank3) - SIZEOF(.dbank3_noinit));
  _top_db3       = ORIGIN(dbank3)+LENGTH(dbank3);
  _top_ram       = ORIGIN(ram)+LENGTH(ram);
  _stack         = (_remaining_db3 > _MINIMUM_STACK_SIZE) ? _top_db3 : _top_ram;

  /* Check that there is enough space in the RAM for the stack */
  _remaining_ram = (LENGTH(ram) - (_end - _start));
  ASSERT(_remaining_ram > 0, "The stack does not fit in ram")
}
