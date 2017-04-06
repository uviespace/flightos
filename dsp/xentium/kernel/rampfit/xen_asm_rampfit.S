; description: rampfit assembler function for the NGAPP project
; authors: A. Luntzer
; version: 0.2
; date: 24.10.2013
; history: -
; note: this is the .75 cycles/sample variant
;
;
; equivalent C implementation (author: R. Ottensamer)
;int FastIntFixedRampFitBufferC (volatile long *data,
;				unsigned int n_samples,
;				unsigned int ramplen,
;				long *slopes)
;
;{
;	int i = 0;
;	int r = 0;	
;	int ampl = ramplen;
;	int SyTerm = 0;
;
;	int pos = 0; /* temporary offset sorage */
;	int value = 0; /* temporary sample storage */
;
;	int Sy = 0;
;	int Sxy = 0;
;
;	for (pos = 0; pos < (n_samples-ramplen+1); )
;	{
;		Sy = 0;
;		Sxy = 0;
;
;		for (i=1; i <= ramplen; i++) /* equation starts with 1 */
;		{
;			value = data[pos++];
;			Sy += value;
;			Sxy += i * value;
;			printk ("Sxy: %d value %d\n", Sxy, value);
;		}
;
;		SyTerm = ampl*((ramplen+1) * Sy) >> 1;
;		slopes[r++] = (ampl*Sxy - iSyTerm);
;		/* denomination has to be done outside */
;	}
;
;	return r;
;}

;; fast ramp fit
;; int FastIntFixedRampFitBuffer (int *piBANK0, int *piBANK2, unsigned int number_of_samples, unsigned int ramp_length, int *iSlopes); 
.globl FastIntFixedRampFitBuffer
.align 4
.type FastIntFixedRampFitBuffer,@function

;; function call arguments
%define RFdatabuf1	RA6
%define RFdatabuf2	RB6
%define num_samples	RC6
%define ramp_len	RD6
%define RFslopebuf	RE6
;; reserved registers
%define RFretptr 	RA7
%define RFretval 	RA6
%define cond0		RC0
%define cond1		RB0

;; constants and local variables
%define RFdata1		RA5
%define RFdata2		RC2
%define rl1        	RC5 
%define Sy		RE2
%define Sxy		RD5
%define r		RA6
%define RFiloop	RE1
%define tmp		RA4

%define RFoloop	RD4

%define i1		RA2
%define i2		RB2

%define i3		RC3
%define i4		RD3
%define RFamp	RE3

FastIntFixedRampFitBuffer:
	;; block 1
	A0 ADD  ramp_len, 1	; calculate ramplen +1 = rl1
	A1 OR   0, RFdatabuf1	; RFdata1
	S1 OR   0, RFdatabuf2	; RFdata2
	C0 LINK
	S0 SL   num_samples, 1		; RFns * 4 / 2 = offset to end of buffer in words, split into two banks
	cond0 = 0		
	r     = 0
	
	;; block 2
	S0 ADD A1X, S0X		; end of buffer 
	S1 SRU ramp_len, 2		; RFiloop = ramp_len/4 
	rl1      = A0X
	RFdata1  = A1X
	RFdata2  = S1X
	RFretptr = C0X
	
	;; block 3
	RFiloop = S1X
	tmp =  S0X
		
.RFloop:
	;; block 4
	C0 LOOP 3, 8, RFiloop		
	M0 MUL  ramp_len, rl1		; RFamp  - compute every time, saves 1 cycle during lead in plus unit is free anyway
	A0 SUB tmp, 8			
	Sy = 0
	
	;; block 5; loop delay slot 1
	A0 SUB 0, 3		; i1	prepare indices with negative offset
	A1 SUB 0, 2		; i2	before entering the loop, they are
	C0 SUB 0, 1		; i3    incremented to 1,2,3,4 on the first pass
	RFoloop = A0X
	

	;; block 6; loop delay slot 2
	S1 OR 0, 0			; prep initial Sxy (need that to properly fold the loop)
	RFamp = M0X
	i1    = A0X
	i2    = A1X
	i3    = C0X
	i4    = 0
	
	;; block 7; loop body block 1
	A0 ADD RFdata1, 8		; forwared 2 samples (words)
	A1 ADD i1,  4			; increment loop indices
	C0 ADD i2,  4
	E0 LD2 RFdata1			; load from current
	E1 LD2 RFdata2

	;; block 8; loop body block 2
	A0 ADD RFdata2, 8		; forward 2 samples
	A1 ADD i3,  4			; increment loop indices
	C0 ADD i4,  4
	i1  = A1X			; update loop indices
	i2  = C0X
	RFdata1 = A0X			; updated buffer1 pointer
	
	;; block 9; loop body block 3
	A0 ADD E0X, E0Y			; sum first pair
	A1 ADD E1X, E1Y			; and second pair
	M0 MUL E0X, i1			; multiply first two samples with
	M1 MUL E1X, i2			; loop index
	C0 CMPGT RFdata1, RFoloop	; check if we reached end of buffer
	i3   = A1X			; update loop indices
	i4   = C0X
	RFdata2 = A0X			; updated buffer2 pointer

	;; block 10; loop body block 4
	P0 ADD A0X, A1X			; final sum of all samples
	M0 MUL E0Y, i3			; multiply 3rd and 4th sample with loop
	M1 MUL E1Y, i4			; index
	cond0 = C0X			; update result of condition
	
	;; block 11; loop body block 5
	S0 ADD Sy,  P0X			; add sum of 4 samples to Sy
	P0 ADD M0X, M1X			; add result of first two samples * loop idx

	;; block 12; loop body block 6
	S0 ADD M0X, M1X			; add result of 3rd and 4th sample * loop idx
	Sy = S0X			; assign new Sy
	
	;; block 13; loop body block 7
	S0 ADD P0X, S0X			; final sum of idx * sample multiplications	
	Sxy = S1X			; update Sxy from previous cycle

	;; block 14; loop body block 8
	S1 ADD Sxy, S0X	 		; add new idx * sample to Sxy
		
	;; block 15 - back in outer loop (the inner loop ends automatically)
	M0 MUL Sy,  RFamp		; mulitply Sy with amplifier
	M1 MUL S1X, ramp_len		; use latest Sxy and multiply with ramplen (iAmplify)

	;; block 16 - wait 
	A1 ADD r, 1			; increment number of ramps processed 

	;; block 17
	C0 BRZ, cond0 .RFloop		; init branch to start of outer loop
	S0 SRU M0X, 1			; divide iSyTerm

	;; block 18	
	A0 SUB M1X, S0X			; calculate slope

	;; block 19 - BR delay slot 1
	E0 STW RFslopebuf[r], A0X	; A0X from block 18
	r = A1X				; update r (in loop) ; RFretval == r!

	;; block 20 - BR delay slot 2
	C0 BRA RFretptr			; init branch back to caller

	;; block 21+22
	NOP 2				; final delay slots

	;; Elvis has left the building



.E_FastIntFixedRampFitBuffer:
.size   FastIntFixedRampFitBuffer, .E_FastIntFixedRampFitBuffer-FastIntFixedRampFitBuffer
