%define SYS_argvlen	12
%define SYS_argv	13

  .file "crt0.s"
  .text
  .globl _start
  .align 4
  .type _start,@function
_start:
  ; Start by setting up a stack.
  A0 OR 0, hi16(_stack)           ; The linker script specifies the address
                                  ; where the stack should start with the
                                  ; _stack symbol. Get the high part of it
                                  ; on A0X...

  A0 OR A0X, lo16(_stack)         ; or in the low part...

  RA14 = A0X                      ; and assign it to the stack pointer register
                                  ; (RA14).

  ; Now that we have a stack we can start using function calls if needed.

  A0 ADD RA14, -4                 ; Decrement stack pointer.
  C0 LINK                         ; Get return address.

  RA14 = A0X
  E0 STW A0X[0], C0X              ; Store return address in the stack.

  ; Zero the memory in the .bss section.
  A0 OR 0, hi16(__bss_start)      ; The linker script specifies the address
                                  ; where the .bss section should start with
                                  ; the __bss_start symbol. Get the high part
                                  ; of it on A0X.
  S0 OR 0, hi16(_end)             ; The linker script specifies the address
                                  ; where the .bss section should end with
                                  ; the _end symbol. Get the high part
                                  ; of it on S0X.

  A0 OR A0X, lo16(__bss_start)    ; Or the low part of the address of the
                                  ; __bss_start symbol into the high part
                                  ; already on A0X.
  S0 OR S0X, lo16(_end)           ; Or the low part of the address of the
                                  ; _end symbol into the high part already on
                                  ; S0X.

; This commented out section of code uses the memset function to do the actual
; zeroing of the .bss section. If we have an optimized implementation of the
; memset function we might want to use this instead of the code below.
; Currently however it just makes the executable bigger.
;
;  RA6 = A0X                       ; The first parameter of the memset function
;                                  ; is the pointer to the block of memory to
;                                  ; fill. Set it to the address the .bss
;                                  ; section (__bss_start).
;  A0 SUB S0X, A0X                 ; Calculate the size of the .bss section
;                                  ; (_end - __bss_start)
;
;  RB6 = 0                         ; The second parameter of the memset function
;                                  ; is the value the block of memory should be
;                                  ; set to. Set it to 0.
;  RC6 = A0X                       ; The third parameter of the memset function
;                                  ; is the number of bytes to set. Set it to
;                                  ; the size of the .bss section we just
;                                  ; calculated.
;
;  C0 BR memset                    ; Call the memset function to do the actual
;                                  ; zeroing.
;
;  NOP 2
;

  P0 CMPLTU A0X, S0X               ; Check if current address in .bss,
                                   ; __bss_start, (A0X) is less than _end (S0X).

  C0 BRZ P0X, .L2                  ; If it is not, the size of .bss is 0, so no
                                   ; need to initialize .bss. Jump over the
                                   ; .bss initialization loop.

  NOP 2

  ; Begin .bss initialization loop
.L1:
  E0 STB A0X[0], 0                 ; Write 0 to current address in .bss.
  A0 ADD A0X, 1                    ; Increment current address in .bss

  P0 CMPLTU A0X, S0X               ; Check if new current address in .bss is
                                   ; still less than _end.

  C0 BRNZ P0X, .L1                 ; If it is jump back to the beginning of the
                                   ; .bss initialization loop.

  NOP 2

  ; End .bss initialization loop/

  ; Initialisation for calls to main with arguments
  ; from the command line
.L2:
	C0 TRAP SYS_argvlen

	C0 TRAP SYS_argv

.L3:
  C0 BR main                      ; Call the main function.

  NOP 2

  E0 LDW RA14[0]
  A0 ADD RA14, +4

  RA14 = A0X

  C0 BRA E0X                      ; Jump back to the MPPB Xentium bootloader.

  NOP 2

.Ltmp0:
  .size _start, .Ltmp0-_start
