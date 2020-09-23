
/**
 * This creates a number processing nodes in a processing network.
 * Two special trackers are used for input and output.
 */


#include <kernel/kernel.h>
#include <kernel/kmem.h>
#include <kernel/kthread.h>
#include <kernel/err.h>
#include <kernel/smp.h>
#include <asm/io.h>



#include <data_proc_task.h>
#include <data_proc_tracker.h>
#include <data_proc_net.h>



#define SRC_BUF_ELEM	(1024 * 256)
#define COMPR_BUF_ELEM	(1024 * 256)

#define CRIT_LEVEL	10

#define OP_PREPROC_NLC		0x1234
#define OP_DECORR_DIFF		0x1235
#define OP_LOSSY3_ROUND2	0x1236
#define OP_LLC_ARI1		0x1237


struct CompressedBuf {
	unsigned int datatype;
	unsigned int llcsize;     /* compressed size (bytes) */
	unsigned int nelements;   /* number of elements that went into llc */
	unsigned int xelements;
	unsigned int yelements;
	unsigned int zelements;   /* number of frames, e.g. for imagettes */
	unsigned int lossyCrc;    /* CRC after lossy steps */
	void *data;
};

struct ScienceBuf {
  unsigned int datatype;
  unsigned int nelements;
  unsigned int xelements;
  unsigned int yelements;
  unsigned int zelements;
  void *data;
};






struct ProcData {
	ktime start;
	struct ScienceBuf    source;
	struct ScienceBuf    swap;
	struct CompressedBuf compressed;
};


/* A union which permits us to convert between a float and a 32 bit
   int.  */

typedef union
{
  float value;
  unsigned int word;
} ieee_float_shape_type;



/* Get a 32 bit int from a float.  */

#define GET_FLOAT_WORD(i,d)                                     \
do {                                                            \
  ieee_float_shape_type gf_u;                                   \
  gf_u.value = (d);                                             \
  (i) = gf_u.word;                                              \
} while (0)

/* Set a float from a 32 bit int.  */

#define SET_FLOAT_WORD(d,i)                                     \
do {                                                            \
  ieee_float_shape_type sf_u;                                   \
  sf_u.word = (i);                                              \
  (d) = sf_u.value;                                             \
} while (0)




/*
   from newlib/libm/common/sf_round.c

   NOTE: this funny implementation does not make use of the sign bit, but works with signed data
*/

float roundf(float x)
{
  int w;
  /* Most significant word, least significant word. */
  int exponent_less_127;

  GET_FLOAT_WORD(w, x);

  /* Extract exponent field. */
  exponent_less_127 = ((w & 0x7f800000) >> 23) - 127;

  if (exponent_less_127 < 23)
    {
      if (exponent_less_127 < 0)
        {
          w &= 0x80000000;
          if (exponent_less_127 == -1)
            /* Result is +1.0 or -1.0. */
            w |= (127 << 23);
        }
      else
        {
          unsigned int exponent_mask = 0x007fffff >> exponent_less_127;
          if ((w & exponent_mask) == 0)
            /* x has an integral value. */
            return x;

          w += 0x00400000 >> exponent_less_127;
          w &= ~exponent_mask;
        }
    }
  else
    {
      if (exponent_less_127 == 128)
        /* x is NaN or infinite. */
        return x + x;
      else
        return x;
    }

  SET_FLOAT_WORD(x, w);
  return x;
}







/**
 * @brief    apply bit rounding for unsigned integers in place
 * @param    source    pointer to the input data
 * @param    nbits     number of bits to round
 * @param    n         number of samples to process
 *
 * @note     the result is right-shifted by nbits, but we round in float
 */

void BitRounding32u (unsigned int *source, unsigned int nbits, unsigned int n)
{
  unsigned int i;
  unsigned int cellwidth;
  float reciprocal;

  if (nbits >= 32)
    return;

  cellwidth = 1u << nbits;
  reciprocal = 1.0f / (float)cellwidth;

  for (i=0; i < n; i++)
    source[i] = (unsigned int)roundf((float)source[i] * reciprocal);

  return;
}


#define SPLINE_SEGMENTS 28

unsigned int rborder[SPLINE_SEGMENTS]; /* right borders of spline segment intervals */

double A[SPLINE_SEGMENTS]; /* 0th order coefficients for nlcSplineCorr28 */
double B[SPLINE_SEGMENTS]; /* 1st order coefficients for nlcSplineCorr28 */
double C[SPLINE_SEGMENTS]; /* 2nd order coefficients for nlcSplineCorr28 */
double D[SPLINE_SEGMENTS]; /* 3rd order coefficients for nlcSplineCorr28 */


/**
 * @brief    Function used by @ref NlcSplineCorr28. It returns the index of the right (=upper) border of the interval that the
 *           given number belongs to. This function is designed to handle exactly 28 intervals.
 * @param    value    the value which is sought within the intervals given by rborder
 * @param    rb       an array of right (=upper) borders
 * @note     the right border is assumed to belong to the interval
 * @note     this is implemented as a bisection to be as fast as possible
 *
 * @returns  the index of the interval to which the input value belongs
 */

int GetInterval28 (unsigned int value, unsigned int *rb)
{
  int r=0;

  if (value <= rb[13])
    {
      /* 0..13 */
      if (value <= rb[6])
	{
	  /* 0..6 */
	  if (value <= rb[3])
	    {
	      /* 0..3 */
	      if (value <= rb[1])
		{
		  /* 0..1 */
		  if (value <= rb[0])
		    {
		      /* 0 */
		      r = 0;
		    }
		  else
		    {
		      /* 1 */
		      r = 1;
		    }
		}
	      else
		{
		  /* 2..3 */
		  if (value <= rb[2])
		    {
		      /* 2 */
		      r = 2;
		    }
		  else
		    {
		      /* 3 */
		      r = 3;
		    }
		}
	    }
	  else
	    {
	      /* 4..6 */
	      if (value <= rb[5])
		{
		  /* 4..5 */
		  if (value <= rb[4])
		    {
		      /* 4 */
		      r = 4;
		    }
		  else
		    {
		      /* 5 */
		      r = 5;
		    }
		}
	      else
		{
		  /* 6 */
		  r = 6;
		}
	    }
	}
      else
	{
	  /* 7..13 */
	  if (value <= rb[10])
	    {
	      /* 7..10 */
	      if (value <= rb[8])
		{
		  /* 7..8 */
		  if (value <= rb[7])
		    {
		      /* 7 */
		      r = 7;
		    }
		  else
		    {
		      /* 8 */
		      r = 8;
		    }
		}
	      else
		{
		  /* 9..10 */
		  if (value <= rb[9])
		    {
		      /* 9 */
		      r = 9;
		    }
		  else
		    {
		      /* 10 */
		      r = 10;
		    }
		}
	    }
	  else
	    {
	      /* 11..13 */
	      if (value <= rb[12])
		{
		  /* 11..12 */
		  if (value <= rb[11])
		    {
		      /* 11 */
		      r = 11;
		    }
		  else
		    {
		      /* 12 */
		      r = 12;
		    }
		}
	      else
		{
		  /* 13 */
		  r = 13;
		}
	    }
	}
    }
  else
    {
      /* 14..27 */
      if (value <= rb[20])
	{
	  /* 14..20 */
	  if (value <= rb[17])
	    {
	      /* 14..17 */
	      if (value <= rb[15])
		{
		  /* 14..15 */
		  if (value <= rb[14])
		    {
		      /* 14 */
		      r = 14;
		    }
		  else
		    {
		      /* 15 */
		      r = 15;
		    }
		}
	      else
		{
		  /* 16..17 */
		  if (value <= rb[16])
		    {
		      /* 16 */
		      r = 16;
		    }
		  else
		    {
		      /* 17 */
		      r = 17;
		    }
		}
	    }
	  else
	    {
	      /* 18..20 */
	      if (value <= rb[20])
		{
		  /* 18..19 */
		  if (value <= rb[18])
		    {
		      /* 18 */
		      r = 18;
		    }
		  else
		    {
		      /* 19 */
		      r = 19;
		    }
		}
	      else
		{
		  /* 20 */
		  r = 20;
		}
	    }
	}
      else
	{
	  /* 21..27 */
	  if (value <= rb[24])
	    {
	      /* 21..24 */
	      if (value <= rb[22])
		{
		  /* 21..22 */
		  if (value <= rb[21])
		    {
		      /* 21 */
		      r = 21;
		    }
		  else
		    {
		      /* 22 */
		      r = 22;
		    }
		}
	      else
		{
		  /* .. */
		  if (value <= rb[23])
		    {
		      /* 23 */
		      r = 23;
		    }
		  else
		    {
		      /* 24 */
		      r = 24;
		    }
		}
	    }
	  else
	    {
	      /* 25..27 */
	      if (value <= rb[26])
		{
		  /* 25..26 */
		  if (value <= rb[25])
		    {
		      /* 25 */
		      r = 25;
		    }
		  else
		    {
		      /* 26 */
		      r = 26;
		    }
		}
	      else
		{
		  /* 27 */
		  r = 27;
		}
	    }
	}
    }

  return r;
}





/**
 * @brief    Nonlinearity correction for the CHEOPS CCD readout values.
 *           It uses a set of splines as correction function.
 * @param[in,out]  data   the array of pixel values stored as unsigned ints;
 *                        this will be overwritten by the corrected values
 * @param          n      the number of pixel values to be corrected
 * @note     overwrites input array
 * @note     saturates the corrected values at 65535
 */

void NlcSplineCorr28 (unsigned int *data, unsigned int n)
{
  unsigned int i, value, rightBorderIndex;
  double x, xx, xxx;
  unsigned short utemp16 = 0;
  float ftemp;

  for (i=0; i < SPLINE_SEGMENTS; i++)
    {
	    ftemp = 1.0;
//      CrIaCopyArrayItem (NLCBORDERS_ID, &utemp16, i);
      rborder[i] = (unsigned int) utemp16;

 //     CrIaCopyArrayItem (NLCCOEFF_A_ID, &ftemp, i);
      A[i] = (double) ftemp;

//      CrIaCopyArrayItem (NLCCOEFF_B_ID, &ftemp, i);
      B[i] = (double) ftemp;

//      CrIaCopyArrayItem (NLCCOEFF_C_ID, &ftemp, i);
      C[i] = (double) ftemp;

 //     CrIaCopyArrayItem (NLCCOEFF_D_ID, &ftemp, i);
      D[i] = (double) ftemp;
    }

  for (i=0; i < n; i++)
    {
      value = data[i];

      /* get the index of the right border of the interval the current value belongs to */
      rightBorderIndex = GetInterval28 (value, rborder);

      /* The spline coefficients assume that x starts at 0 within the interval,
	 but our x counts from 0 to 65k, so we have to shift the x axis for
	 every interval back to zero by subtracting the left border.
	 The first interval starts at 0, so nothing has to be done for it. */

      if (rightBorderIndex != 0)
	{
	  x = (double) (value - rborder[rightBorderIndex-1]);
	}
      else
	{
	  x = (double) value;
	}

      /* this saves one multiplication */
      xx = x*x;
      xxx = x*xx;

      x = D[rightBorderIndex]*xxx + C[rightBorderIndex]*xx + B[rightBorderIndex]*x + A[rightBorderIndex];

      /* the result is not truncated to integer, but rounded with the help of the inbuilt fdtoi instruction */
      value = (unsigned int) roundf ((float)x);

      /* saturate a corrected value at 16 bits */
      if (value > 0xffff)
	value = 0xffff;

      data[i] = value;
    }

  return;
}





/**
 * @brief       reversible differencing of a buffer
 * @param       buf     an integer pointer to a buffer
 * @param       words   number of values to process
 *
 * Differences are made in place, from bottom to top
 * @note        Is applied in place.
 */

void Delta32 (int *buf, int words)
{
  int i;

  for (i=words-1; i>0; i--)
    {
      buf[i] = (buf[i] - buf[i-1]);
    }

  return;
}


/**
 * @brief       fold negative values into positive, interleaving the positive ones
 * @param       buffer     an integer pointer to a buffer
 * @param       N          number of values to process
 *
 * @note        Is applied in place.
 */

void Map2Pos32 (int *buffer, unsigned int N)
{
  unsigned int i;

  for (i=0; i < N; i++)
    {
      if (buffer[i] < 0)
        buffer[i] = ((0xFFFFFFFF - buffer[i]) << 1) + 1; /* NOTE: the integer overflow is intended */
      else
        buffer[i] = buffer[i] << 1;
    }

  return;
}


/**
 * @brief    safe (but slow) way to put the value of a single bit into a bitstream accessed as 32-bit RAM
 *           in big endian
 * @param    value      the value to put, either 0 or 1
 * @param    bitOffset  index of the bit as seen from the very beginning of the bitstream
 * @param    destAddr   this is the pointer to the beginning of the bitstream
 * @note     Do not use values like 23 and assume that the LSB will be set. It won't.
 * @note     works in SRAM2
 */

void PutBit32 (unsigned int value, unsigned int bitOffset, unsigned int *destAddr)
{
  unsigned int wordpos, bitpos;
  unsigned int destval, mask;

  wordpos = bitOffset >> 5; /* division by 32 */
  /*  bitpos = bitOffset - 32*wordpos; */
  bitpos = bitOffset & 0x1f; /* 5 bits */

  /* shape a mask with the required bit set true */
  mask = 1 << (31-bitpos);

  /* get the destination word and clear the bit */
  destval = destAddr[wordpos];
  destval &= ~mask;

  /* set bit if the value was true */
  if (value == 1)
    destval |= mask;

  /* write it back */
  destAddr[wordpos] = destval;

  return;
}


/**
 * @brief    safe (but slow) way to put the value of up to 32 bits into a bitstream accessed as 32-bit RAM
 *           in big endian
 * @param    value      the value to put, it will be masked
 * @param    bitOffset  bit index where the bits will be put, seen from the very beginning of the bitstream
 * @param    nBits      number of bits to put
 * @param    destAddr   this is the pointer to the beginning of the bitstream
 * @returns  number of bits written or 0 if the number was too big
 * @note     works in SRAM2
 */

unsigned int PutNBits32 (unsigned int value, unsigned int bitOffset, unsigned int nBits, unsigned int *destAddr)
{
  unsigned int *localAddr;
  unsigned int bitsLeft, shiftRight, shiftLeft, localEndPos;
  unsigned int mask, n2;

  /* leave in case of erroneous input */
  if (nBits == 0)
    return 0;
  if (nBits > 32)
    return 0;

  /* separate the bitOffset into word offset (set localAddr pointer) and local bit offset (bitsLeft) */
  localAddr = destAddr + (bitOffset >> 5);
  bitsLeft = bitOffset & 0x1f;

  /* (M) we mask the value first to match its size in nBits */
  /* the calculations can be re-used in the unsegmented code, so we have no overhead */
  shiftRight = 32 - nBits;
  mask = 0xffffffff >> shiftRight;
  value &= mask;

  /* to see if we need to split the value over two words we need the right end position */
  localEndPos = bitsLeft + nBits;

  if (localEndPos <= 32)
    {
      /*         UNSEGMENTED

       |-----------|XXXXX|----------------|
          bitsLeft    n       bitsRight

      -> to get the mask:
      shiftRight = bitsLeft + bitsRight = 32 - n
      shiftLeft = bitsRight

      */

      /* shiftRight = 32 - nBits; */ /* see (M) above! */
      shiftLeft = shiftRight - bitsLeft;

      /* generate the mask, the bits for the values will be true */
      /* mask = (0xffffffff >> shiftRight) << shiftLeft; */ /* see (M) above! */
      mask <<= shiftLeft;

      /* clear the destination with inverse mask */
      *(localAddr) &= ~mask;

      /* assign the value */
      *(localAddr) |= (value << (32-localEndPos)); /* NOTE: 32-localEndPos = shiftLeft can be simplified */
    }

  else
    {
      /*                             SEGMENTED

       |-----------------------------|XXX| |XX|------------------------------|
                 bitsLeft              n1   n2          bitsRight

       -> to get the mask part 1:
       shiftright = bitsleft
       n1 = n - (bitsleft + n - 32) = 32 - bitsleft

       -> to get the mask part 2:
       n2 = bitsleft + n - 32
       shiftleft = 32 - n2 = 32 - (bitsleft + n - 32) = 64 - bitsleft - n

      */

      n2 = bitsLeft + nBits - 32;

      /* part 1: */
      shiftRight = bitsLeft;
      mask = 0xffffffff >> shiftRight;

      /* clear the destination with inverse mask */
      *(localAddr) &= ~mask;

      /* assign the value part 1 */
      *(localAddr) |= (value >> n2);

      /* part 2: */
      /* adjust address */
      localAddr += 1;
      shiftLeft = 64 - bitsLeft - nBits;
      mask = 0xffffffff << shiftLeft;

      /* clear the destination with inverse mask */
      *(localAddr) &= ~mask;

      /* assign the value part 2 */
      *(localAddr) |= (value << (32-n2));
    }

  return nBits;
}





/**
 * ARI parameters
 */
#define ARIHDR 2
#define FMARIROWS 256
#define FMARISPILL 257
#define MAXFREQ 8192
#define SPILLCUT FMARIROWS
#define PHOT_STANDARD 0


/**
 * @brief structure used by the @ref fmari_compress algorithm
 */

struct arimodel {
  unsigned int *freqtable;
  unsigned int *cptable;   /* cumulative probability table */
  unsigned int *ncptable;  /* next cumulative probability table */
  unsigned int *spillover; /* swap buffer for spillover, i.e. words > 8 Bit */
  int spillctr;
  int probability;
};


/**
 * This number defines the number of bits used for the codeword in the @ref vbwl_midsize
 * algorithm, which is located at the start of each group and says how many bits are used for
 * the symbols in this group.
 *
 * set to 3 if you are sure that there are no larger values than 16 bits (the length code L = 9..16
 * will be encoded as L-VBWLMINW = C = 0..7 in 4 bits. Each symbol of the group will be encoded in L bits)
 *
 * set to 4 for a cap of 24 bits, set to 5 for a cap of 40 bits
 *
 * @warning Larger values than what you set as cap will corrupt the output stream
 *          and it would be hard to decode such a stream.
 *
 */

#define VBWLCODE 5

/**
 * The minimum number of bits to encode a spillover value is 9,
 * because if it was 8, it would not have landed in the spill.
 * There is one exception, because the bias @ref FMARIROWS is subtracted
 * from the spill before calling the @ref vbwl_midyize function.
 * This leaves a small range of values to get a width of < 9, but
 * at present vbwl does not make efficient use of it and encodes them in 9 bits.
 */

#define VBWLMINW 9



/**
 * @brief    initialize the model table for arithmetic compression (internal function)
 * @param    model    pointer to the @ref arimodel structure
 * @param    buffer   pointer to the buffer where the table will reside
 * @param    symbols  number of symbols covered by the model (i.e. the number of histogram bins)
 */

void init_arimodel (struct arimodel *model, unsigned int *buffer, int symbols)
{
  /* note: symbols is counted here without the spill probability */
  model->freqtable   = buffer;
  model->cptable     = model->freqtable + symbols + 1;  /* cumulative probability table  */
  model->ncptable    = model->cptable + 1;              /* next cumulative probability table */
  model->spillover   = model->ncptable + symbols + 1;   /* swap buffer for spillover, i.e. words > 8 Bit    */
  model->spillctr    = 0;
  model->probability = 0;

  return;
}


/**
 * @brief    initialize the cumulative frequency in the model table for arithmetic compression (internal function)
 * @param    table    pointer to the frequency table of the @ref arimodel (freqtable)
 * @param    cumu     pointer to the cumulative frequency table of the @ref arimodel (cptable)
 * @param    nrows    number of symbols covered by the model (i.e. the number of histogram bins)
 * @returns  last value of the cumulative table, i.e. the number of samples, the sum of the histogram
 */

int makeCumulative (unsigned int *table, unsigned int nrows, unsigned int *cumu)
{
  unsigned int ctr;

  for (ctr=0; ctr < nrows; ctr++) /* clean table for the "cumulative probabilities" */
    cumu[ctr] = 0;

  for (ctr=0; ctr < nrows; ctr++) /* the new table is +1 in size !!     */
    cumu[ctr+1] = cumu[ctr] + table[ctr];

  return cumu[nrows];
}


/**
 * @brief    create a new model from a histogram of a buffer
 * @param    buffer   pointer to the buffer of samples to make the histogram
 * @param    nwords   number of samples in that buffer
 * @param    symbols  number of samples to skip at the beginning of the buffer (where the spillover values are)
 * @param    initval  bias value used in every histogram bin. We recommend 1.
 * @param    model    pointer to the @ref armodel structure
 */

void update_fm_ari_model (unsigned int *buffer, unsigned int nwords, unsigned int symbols, int initval, struct arimodel *model)
{
  unsigned int ctr;
  unsigned int value;

  /* start model with 0 or 1 in every entry -> smooth */
  for (ctr=0; ctr < FMARISPILL; ctr++)
    model->freqtable[ctr] = initval;

  /* count freqs over the buffer, but leave out the first FMACROWS words for smoothing   */
  for (ctr=symbols; ctr < nwords; ctr++)
    {
      value = buffer[ctr];
      /*SDPRINT ("updatemodel [%d] = %d\n", ctr, buffer[ctr]); */

      if (value < FMARIROWS)
        model->freqtable[value]++;
      else
        model->freqtable[FMARIROWS]++; /* spillover */
    }

  /* make new (n)cp array */
  model->probability = makeCumulative (model->freqtable, FMARISPILL, model->cptable);

  return;
}


/**
 * @brief    set the initial values for the arithemtic compression model (histogram) for the first chunk
 * @param    destmodel    pointer to the histogram buffer in the @ref arimodel
 * @param    ModeID       select which model to use
 * @note     this is still from PACS, need a CHEOPS statistic here
 */

void initAriTable (int *destmodel, int ModeID)
{
  switch ( ModeID )
    {
    case ( PHOT_STANDARD ) :
    default :
    {
      /* startmodel for default full-frame compression */
      destmodel[0] = 201;
      destmodel[1] = 200;
      destmodel[2] = 200;
      destmodel[3] = 197;
      destmodel[4] = 199;
      destmodel[5] = 194;
      destmodel[6] = 195;
      destmodel[7] = 190;
      destmodel[8] = 192;
      destmodel[9] = 184;
      destmodel[10] = 186;
      destmodel[11] = 178;
      destmodel[12] = 181;
      destmodel[13] = 172;
      destmodel[14] = 174;
      destmodel[15] = 165;
      destmodel[16] = 167;
      destmodel[17] = 157;
      destmodel[18] = 160;
      destmodel[19] = 150;
      destmodel[20] = 153;
      destmodel[21] = 143;
      destmodel[22] = 145;
      destmodel[23] = 134;
      destmodel[24] = 138;
      destmodel[25] = 127;
      destmodel[26] = 130;
      destmodel[27] = 120;
      destmodel[28] = 123;
      destmodel[29] = 113;
      destmodel[30] = 116;
      destmodel[31] = 107;
      destmodel[32] = 109;
      destmodel[33] = 99;
      destmodel[34] = 102;
      destmodel[35] = 93;
      destmodel[36] = 95;
      destmodel[37] = 87;
      destmodel[38] = 89;
      destmodel[39] = 81;
      destmodel[40] = 83;
      destmodel[41] = 75;
      destmodel[42] = 78;
      destmodel[43] = 70;
      destmodel[44] = 72;
      destmodel[45] = 65;
      destmodel[46] = 67;
      destmodel[47] = 60;
      destmodel[48] = 62;
      destmodel[49] = 56;
      destmodel[50] = 58;
      destmodel[51] = 51;
      destmodel[52] = 54;
      destmodel[53] = 47;
      destmodel[54] = 49;
      destmodel[55] = 44;
      destmodel[56] = 46;
      destmodel[57] = 40;
      destmodel[58] = 42;
      destmodel[59] = 37;
      destmodel[60] = 39;
      destmodel[61] = 34;
      destmodel[62] = 36;
      destmodel[63] = 31;
      destmodel[64] = 33;
      destmodel[65] = 28;
      destmodel[66] = 30;
      destmodel[67] = 26;
      destmodel[68] = 28;
      destmodel[69] = 24;
      destmodel[70] = 26;
      destmodel[71] = 22;
      destmodel[72] = 23;
      destmodel[73] = 20;
      destmodel[74] = 22;
      destmodel[75] = 18;
      destmodel[76] = 19;
      destmodel[77] = 16;
      destmodel[78] = 18;
      destmodel[79] = 15;
      destmodel[80] = 16;
      destmodel[81] = 14;
      destmodel[82] = 15;
      destmodel[83] = 12;
      destmodel[84] = 14;
      destmodel[85] = 11;
      destmodel[86] = 12;
      destmodel[87] = 10;
      destmodel[88] = 11;
      destmodel[89] = 10;
      destmodel[90] = 10;
      destmodel[91] = 9;
      destmodel[92] = 9;
      destmodel[93] = 8;
      destmodel[94] = 9;
      destmodel[95] = 7;
      destmodel[96] = 8;
      destmodel[97] = 7;
      destmodel[98] = 7;
      destmodel[99] = 6;
      destmodel[100] = 7;
      destmodel[101] = 5;
      destmodel[102] = 6;
      destmodel[103] = 5;
      destmodel[104] = 5;
      destmodel[105] = 4;
      destmodel[106] = 5;
      destmodel[107] = 4;
      destmodel[108] = 5;
      destmodel[109] = 4;
      destmodel[110] = 4;
      destmodel[111] = 4;
      destmodel[112] = 4;
      destmodel[113] = 3;
      destmodel[114] = 4;
      destmodel[115] = 3;
      destmodel[116] = 3;
      destmodel[117] = 3;
      destmodel[118] = 3;
      destmodel[119] = 3;
      destmodel[120] = 3;
      destmodel[121] = 2;
      destmodel[122] = 3;
      destmodel[123] = 2;
      destmodel[124] = 2;
      destmodel[125] = 2;
      destmodel[126] = 2;
      destmodel[127] = 2;
      destmodel[128] = 2;
      destmodel[129] = 2;
      destmodel[130] = 2;
      destmodel[131] = 2;
      destmodel[132] = 2;
      destmodel[133] = 2;
      destmodel[134] = 2;
      destmodel[135] = 2;
      destmodel[136] = 2;
      destmodel[137] = 2;
      destmodel[138] = 2;
      destmodel[139] = 2;
      destmodel[140] = 2;
      destmodel[141] = 1;
      destmodel[142] = 2;
      destmodel[143] = 1;
      destmodel[144] = 2;
      destmodel[145] = 1;
      destmodel[146] = 2;
      destmodel[147] = 1;
      destmodel[148] = 1;
      destmodel[149] = 1;
      destmodel[150] = 1;
      destmodel[151] = 1;
      destmodel[152] = 1;
      destmodel[153] = 1;
      destmodel[154] = 1;
      destmodel[155] = 1;
      destmodel[156] = 1;
      destmodel[157] = 1;
      destmodel[158] = 1;
      destmodel[159] = 1;
      destmodel[160] = 1;
      destmodel[161] = 1;
      destmodel[162] = 1;
      destmodel[163] = 1;
      destmodel[164] = 1;
      destmodel[165] = 1;
      destmodel[166] = 1;
      destmodel[167] = 1;
      destmodel[168] = 1;
      destmodel[169] = 1;
      destmodel[170] = 1;
      destmodel[171] = 1;
      destmodel[172] = 1;
      destmodel[173] = 1;
      destmodel[174] = 1;
      destmodel[175] = 1;
      destmodel[176] = 1;
      destmodel[177] = 1;
      destmodel[178] = 1;
      destmodel[179] = 1;
      destmodel[180] = 1;
      destmodel[181] = 1;
      destmodel[182] = 1;
      destmodel[183] = 1;
      destmodel[184] = 1;
      destmodel[185] = 1;
      destmodel[186] = 1;
      destmodel[187] = 1;
      destmodel[188] = 1;
      destmodel[189] = 1;
      destmodel[190] = 1;
      destmodel[191] = 1;
      destmodel[192] = 1;
      destmodel[193] = 1;
      destmodel[194] = 1;
      destmodel[195] = 1;
      destmodel[196] = 1;
      destmodel[197] = 1;
      destmodel[198] = 1;
      destmodel[199] = 1;
      destmodel[200] = 1;
      destmodel[201] = 1;
      destmodel[202] = 1;
      destmodel[203] = 1;
      destmodel[204] = 1;
      destmodel[205] = 1;
      destmodel[206] = 1;
      destmodel[207] = 1;
      destmodel[208] = 1;
      destmodel[209] = 1;
      destmodel[210] = 1;
      destmodel[211] = 1;
      destmodel[212] = 1;
      destmodel[213] = 1;
      destmodel[214] = 1;
      destmodel[215] = 1;
      destmodel[216] = 1;
      destmodel[217] = 1;
      destmodel[218] = 1;
      destmodel[219] = 1;
      destmodel[220] = 1;
      destmodel[221] = 1;
      destmodel[222] = 1;
      destmodel[223] = 1;
      destmodel[224] = 1;
      destmodel[225] = 1;
      destmodel[226] = 1;
      destmodel[227] = 1;
      destmodel[228] = 1;
      destmodel[229] = 1;
      destmodel[230] = 1;
      destmodel[231] = 1;
      destmodel[232] = 1;
      destmodel[233] = 1;
      destmodel[234] = 1;
      destmodel[235] = 1;
      destmodel[236] = 1;
      destmodel[237] = 1;
      destmodel[238] = 1;
      destmodel[239] = 1;
      destmodel[240] = 1;
      destmodel[241] = 1;
      destmodel[242] = 1;
      destmodel[243] = 1;
      destmodel[244] = 1;
      destmodel[245] = 1;
      destmodel[246] = 1;
      destmodel[247] = 1;
      destmodel[248] = 1;
      destmodel[249] = 1;
      destmodel[250] = 1;
      destmodel[251] = 1;
      destmodel[252] = 1;
      destmodel[253] = 1;
      destmodel[254] = 1;
      destmodel[255] = 1;
      destmodel[256] = 131; /* NOTE: spillover, yes this table has 257 entries! */
      break;
    }
    }
  return;
}


/**
 *
 * these variables are shared among the core coding functions of fmari
 *
 *@{*/

/** lower bound of local encoding interval */
unsigned int fm_ari_low[CONFIG_SMP_CPUS_MAX];

/** upper bound of local encoding interval */
//unsigned int fm_ari_high[CONFIG_SMP_CPUS_MAX] =  {0xffff, 0xfff, 0xffff, 0xffff};
unsigned int fm_ari_high[CONFIG_SMP_CPUS_MAX]; /* XXX */

/** flag to signal underflow */
unsigned int fm_ari_underflow[CONFIG_SMP_CPUS_MAX];

/** the write counter for the output bitstream */
unsigned int fm_ari_wctr[CONFIG_SMP_CPUS_MAX];

/**@}*/


/**
 * @brief calculate the new interval and output bits to the bitstream if necessary
 * @param dest    pointer to the base of the output bitstream
 * @param cp      the cumulative probability of that value (taken from the @ref arimodel)
 * @param ncp     the next cumulative probability of that value (taken from the @ref arimodel)
 * @par Globals
 * @ref fm_ari_low[smp_cpu_id()], @ref fm_ari_high[smp_cpu_id()], @ref fm_ari_underflow[smp_cpu_id()], @ref fm_ari_wctr[smp_cpu_id()]
 */

void fmari_encodeSym8k (unsigned int *dest, unsigned int cp, unsigned int ncp)
{
  unsigned int width;
  unsigned int a;

  /* calculate the new interval */
  width = (fm_ari_high[smp_cpu_id()] - fm_ari_low[smp_cpu_id()]) + 1;

  fm_ari_high[smp_cpu_id()] = fm_ari_low[smp_cpu_id()] + ((ncp * width) >> 13) - 1; /* L + Pni * (H - L) */

  fm_ari_low[smp_cpu_id()]  = fm_ari_low[smp_cpu_id()] + ((cp * width) >> 13);       /* L + Pci * (H - L) */

  for ( ; ; )
    {
      a = fm_ari_high[smp_cpu_id()] & 0x8000;

      /* write out equal bits */
      if (a == (fm_ari_low[smp_cpu_id()] & 0x8000))
        {
          PutBit32 (a >> 15, fm_ari_wctr[smp_cpu_id()]++, dest);

          while (fm_ari_underflow[smp_cpu_id()] > 0)
            {
              PutBit32 ((~fm_ari_high[smp_cpu_id()] & 0x8000) >> 15, fm_ari_wctr[smp_cpu_id()]++, dest);
              fm_ari_underflow[smp_cpu_id()]--;
            }
        }

      /* underflow coming up, because <> and the 2nd bits are just one apart       */
      else if ((fm_ari_low[smp_cpu_id()] & 0x4000) && !(fm_ari_high[smp_cpu_id()] & 0x4000))
        {
          fm_ari_underflow[smp_cpu_id()]++;
          fm_ari_low[smp_cpu_id()]  &= 0x3fff;
          fm_ari_high[smp_cpu_id()] |= 0x4000;
        }
      else
        {
          return;
        }

      fm_ari_low[smp_cpu_id()]  <<= 1;
      fm_ari_low[smp_cpu_id()]   &= 0xffff;
      fm_ari_high[smp_cpu_id()] <<= 1;
      fm_ari_high[smp_cpu_id()]  |= 1;
      fm_ari_high[smp_cpu_id()]  &= 0xffff;
    }

  /* the return is inside the for loop */
}


/**
 * @brief at the end of an encoding chunk, flush out necessary remaining bits
 * @param dest    pointer to the base of the output bitstream
 * @par Globals
 * @ref fm_ari_low[smp_cpu_id()], @ref fm_ari_underflow[smp_cpu_id()], @ref fm_ari_wctr[smp_cpu_id()]
 */

void fmari_flushEncoder (unsigned int *dest)
{

  PutBit32 ((fm_ari_low[smp_cpu_id()] & 0x4000) >> 14, fm_ari_wctr[smp_cpu_id()]++, dest);

  fm_ari_underflow[smp_cpu_id()]++;

  while (fm_ari_underflow[smp_cpu_id()]-- > 0)
    PutBit32 ((~fm_ari_low[smp_cpu_id()] & 0x4000) >> 14, fm_ari_wctr[smp_cpu_id()]++, dest);

  return;
}


/**
 * @brief encode a chunk of symbols to an output bitstream. Spillover values are saved in the @ref arimodel's dedicated buffer
 * @param chunk   pointer to the input data
 * @param chunksize  number of symbols in this chunk, best use @ref MAXFREQ (or less if the chunk is smaller)
 * @param dest    pointer to the base of the output bitstream of that chunk segment
 * @param model   pointer to the @ref arimodel structure
 * @par Globals
 * A number of (local) globals are initialized here
 * @ref fm_ari_low[smp_cpu_id()], @ref fm_ari_high[smp_cpu_id()], @ref fm_ari_underflow[smp_cpu_id()], @ref fm_ari_wctr[smp_cpu_id()]
 * @note  make the (local) globales either static or move to arimodel or pass as arguments (or live with it)
 */

int fmari_encode_chunk (int *chunk, int chunksize, int *dest, struct arimodel *model)
{
  int ctr, tail;
  unsigned int symbol, cp, ncp;

  /* now init ari */
  fm_ari_low[smp_cpu_id()]       = 0;
  fm_ari_high[smp_cpu_id()]      = 0xffff;
  fm_ari_underflow[smp_cpu_id()] = 0;
  fm_ari_wctr[smp_cpu_id()]      = 32; /* offset for chunksize_w */

  for (ctr=0; ctr < chunksize; ctr++)
    {
      symbol = chunk[ctr]; /* get next symbol */

      /* look it up in the tables */
      /* first we check for spillover */
      if (symbol >= SPILLCUT)
        {
          /* encode spillover signal in ari stream */
          cp = model->cptable[FMARIROWS];
          ncp = model->ncptable[FMARIROWS];

          fmari_encodeSym8k ((unsigned int *) dest, cp, ncp);

          /* put the symbol into the spillover buffer and increment counter  */
          model->spillover[(model->spillctr)++] = symbol;
        }
      else /* valid symbol */
        {
          cp = model->cptable[symbol];
          ncp = model->ncptable[symbol];

          fmari_encodeSym8k ((unsigned int *)dest, cp, ncp);
        }

    }

  /* encode the rest */
  fmari_flushEncoder ((unsigned int *) dest);

  /* calc fillup and fill up with 0s */
  tail = (32 - fm_ari_wctr[smp_cpu_id()] % 32) % 32;
  fm_ari_wctr[smp_cpu_id()] += tail;
  dest[0] = (fm_ari_wctr[smp_cpu_id()] / 32);

  return dest[0]; /* now in words  */
}


unsigned int bits_used (unsigned int num)
{
  unsigned int u;

  for (u=0; num != 0; u++)
    {
      num >>= 1;
    }

  return u;
}


/**
 * @brief variable block word length encoding. Used for the spillover in FmAri
 * @param source    pointer to the input data
 * @param words     number of symbols to encode
 * @param dest      pointer to the base of the output bitstream
 * @param BS        block size, i.e. how many symbols are put into a group
 * @note  this function is the weak point of the FmAri (ARI1) implementation.
 *        Ok, it has worked for Herschel, where we had very few spill counts, but we want to get rid of it in CHEOPS.
 * @returns  size in 32-bit words of the output stream, rounded up
 */

int vbwl_midsize (int *source, int words, int *dest, int BS)
{
  int ctr, bctr;
  int bits, width;
  int outbits = 32; /* keep track of the output bits; we start at dest[1] */

  /* main loop counts through the source words */
  for (ctr=0; ctr < words; ctr++)
    {
      /* block-loop, we count through the words of a block */
      for (width=0, bctr=ctr; (bctr < ctr+BS) & (bctr < words); bctr++)
        {
          /* determine used bits of current word */
          /* bits = 32-lzeros */
	  bits = bits_used(((unsigned int *)source)[bctr]);

          width = bits > width ? bits : width;
        }

      /* now we know width = the number of bits to encode the block */
      /* first code the width */
      if (width < VBWLMINW) /* ... due to the optional -FMARIROWS */
        width = VBWLMINW;

      /* use VBWLCODE bits for the encoding of the width */
      PutNBits32(width-VBWLMINW, outbits, VBWLCODE, (unsigned int *) dest);
      outbits += VBWLCODE;

      /* now code the words of the block in width bits */
      for (bctr=ctr; (ctr < bctr+BS) & (ctr < words); ctr++)
        {
          PutNBits32 (source[ctr], outbits, width, (unsigned int *)dest);
          outbits += width;
        }
      ctr--;
    }

  /* store the original size */
  dest[0] = words;

  /* return the size in words, rounding up */
  return (outbits+31)/32;
}


/**
 * @brief The FM Arithmetic compression function. ARI1 in CHEOPS-slang.
 * @param source    pointer to the input data.
 * @param nwords    number of symbols to encode
 * @param dest      pointer to the base of the output bitstream
 * @param swap      a working buffer is needed with a size of (strictly speaking) nwords+257+258 words,
 *                  but if you can guess the spillcount, use spillcount+257+258
 * @param modeltype initial probability model to start with. Choose from @ref initAriTable
 * @returns  size in 32-bit words of the output stream, rounded up
 * @note  The spillover is encoded with @ref vbwl_midsize and that algorithm is quite inefficient.
 *        Ok, it is difficult to encode the spill, but that algorithm really does a bad job at it.
 *        In particular, the @ref VBWLCODE define is limiting the range of values.
 */

int fmari_compress (unsigned int *source, unsigned int nwords, unsigned int *dest, unsigned int *swap, unsigned int modeltype)
{
  int ctr;
  int src_ctr;
  int remaining;
  int *streamctr_w;
  unsigned int *stream;

  struct arimodel model;

  init_arimodel (&model, swap, FMARIROWS);

  dest[0]   = nwords; /* store original size in words */
  remaining = nwords;

  src_ctr = 0;

  streamctr_w  = (int *) (dest + 1); /* here the size of the ari stream in words will grow */
  *streamctr_w = 0;                     /* we start with 2, because we have the osize and the streamctr */

  stream = dest + ARIHDR; /* set dest stream and counter */

  initAriTable((int *) model.freqtable, modeltype);  /* initialize starting model */

  /* make probability model */
  model.probability = makeCumulative(model.freqtable, FMARISPILL, model.cptable);

  /* start compressing chunks with initial model     */
  while (remaining > MAXFREQ)
    {
      *streamctr_w += fmari_encode_chunk((int *)(source + src_ctr), MAXFREQ, \
                                         (int *)(stream + *streamctr_w), &model);

      /* derive new model from current data */
      update_fm_ari_model (source + src_ctr, MAXFREQ, FMARISPILL, 1, &model);

      src_ctr   += MAXFREQ;
      remaining -= MAXFREQ;
    }

  /* encode the last chunk */
  if (remaining > 0)
    *streamctr_w += fmari_encode_chunk ((int *)(source + src_ctr), remaining, \
                                        (int *)(stream + *streamctr_w), &model);


  /* .. treat the spillover here */
  /* subtract FMARIROWS from the spill values */
  for (ctr=0; ctr < model.spillctr; ctr++)
    model.spillover[ctr] -= FMARIROWS;

  model.spillctr = vbwl_midsize ((int *) model.spillover, model.spillctr, \
                                 (int *)(dest + ARIHDR + (*streamctr_w)), 4);


  return (int)(ARIHDR + *streamctr_w + model.spillctr);
}






int op_output(unsigned long op_code, struct proc_task *t)
{
	ssize_t n;

	struct ProcData *p = NULL;


	n = pt_get_nmemb(t);
	if (!n)
		goto exit;

	p = (struct ProcData *) pt_get_data(t);
	if (!p)
		goto exit;

	if (smp_cpu_id() == 0) 
	{
	printk("took %lld ms\n", ktime_to_ms(ktime_delta(ktime_get(), p->start)));

	printk("compressed size: %d down from %d, factor %g\n",
			p->compressed.llcsize,
			p->source.nelements * sizeof(unsigned int),
			(double) p->source.nelements * sizeof(unsigned int) /
			p->compressed.llcsize);
	}

exit:
	/* clean up our data buffers */
	if (p) {
		kfree(p->source.data);
		kfree(p->swap.data);
		kfree(p->compressed.data);
		kfree(p);
	}

	pt_destroy(t);

	return PN_TASK_SUCCESS;
}



int op_preproc_nlc(unsigned long op_code, struct proc_task *t)
{
	ssize_t n;

	struct ProcData *p;


	n = pt_get_nmemb(t);
	if (!n)
		return PN_TASK_SUCCESS;


	p = (struct ProcData *) pt_get_data(t);
	if (!p)	/* we have elements but data is NULL, error*/
		return PN_TASK_DESTROY;


	NlcSplineCorr28 ((unsigned int *)(p->source.data), p->source.nelements);

	return PN_TASK_SUCCESS;
}



int op_decorr_diff(unsigned long op_code, struct proc_task *t)
{
	ssize_t n;

	struct ProcData *p;


	n = pt_get_nmemb(t);
	if (!n)
		return PN_TASK_SUCCESS;


	p = (struct ProcData *) pt_get_data(t);
	if (!p)	/* we have elements but data is NULL, error*/
		return PN_TASK_DESTROY;


	Delta32(  (int *) p->source.data, p->source.nelements);
	Map2Pos32((int *) p->source.data, p->source.nelements);

	return PN_TASK_SUCCESS;
}


int op_lossy3_round2(unsigned long op_code, struct proc_task *t)
{
	ssize_t n;

	struct ProcData *p;


	n = pt_get_nmemb(t);
	if (!n)
		return PN_TASK_SUCCESS;


	p = (struct ProcData *) pt_get_data(t);
	if (!p)	/* we have elements but data is NULL, error*/
		return PN_TASK_DESTROY;

	BitRounding32u((unsigned int *)(p->source.data), 2, p->source.nelements);

	return PN_TASK_SUCCESS;
}

int op_llc_ari1(unsigned long op_code, struct proc_task *t)
{
	ssize_t n;

	struct ProcData *p;


	n = pt_get_nmemb(t);
	if (!n)
		return PN_TASK_SUCCESS;


	p = (struct ProcData *) pt_get_data(t);
	if (!p)	/* we have elements but data is NULL, error*/
		return PN_TASK_DESTROY;


	p->compressed.llcsize = fmari_compress((unsigned int *)(p->source.data),
					       p->source.nelements,
					       (unsigned int *)p->compressed.data,
					       (unsigned int *)p->swap.data, 0);

	p->compressed.llcsize *= 4; /* ARI counts words, we want bytes */

	return PN_TASK_SUCCESS;
}


int pn_prepare_nodes(struct proc_net *pn)
{
	struct proc_tracker *pt;


	/* create and add processing node trackers for the each operation */

	pt = pt_track_create(op_preproc_nlc, OP_PREPROC_NLC, CRIT_LEVEL);
	BUG_ON(!pt);
	BUG_ON(pn_add_node(pn, pt));

	pt = pt_track_create(op_decorr_diff, OP_DECORR_DIFF, CRIT_LEVEL);
	BUG_ON(!pt);
	BUG_ON(pn_add_node(pn, pt));

	pt = pt_track_create(op_lossy3_round2, OP_LOSSY3_ROUND2, CRIT_LEVEL);
	BUG_ON(!pt);
	BUG_ON(pn_add_node(pn, pt));

	pt = pt_track_create(op_llc_ari1, OP_LLC_ARI1, CRIT_LEVEL);
	BUG_ON(!pt);
	BUG_ON(pn_add_node(pn, pt));

	BUG_ON(pn_create_output_node(pn, op_output));

	return 0;
}


void pn_new_input_task(struct proc_net *pn)
{
	struct proc_task *t;

	struct ProcData *p;

	static int seq;

	int i;
	int n_elem;


	p = kmalloc(sizeof(struct ProcData));
	BUG_ON(!p);


	t = pt_create(p, sizeof(struct ProcData), 10, 0, seq++);
	pt_set_nmemb(t, 1);	/* 1 element */
	BUG_ON(!t);

	BUG_ON(pt_add_step(t, OP_PREPROC_NLC,   NULL));
	BUG_ON(pt_add_step(t, OP_LOSSY3_ROUND2, NULL));
	BUG_ON(pt_add_step(t, OP_DECORR_DIFF,   NULL));
	BUG_ON(pt_add_step(t, OP_LLC_ARI1,      NULL));


	/* allocate buffers */
	n_elem = SRC_BUF_ELEM;
	p->source.data      = kmalloc(n_elem * sizeof(unsigned int));
	p->source.nelements = n_elem;
	BUG_ON(!p->source.data);

	n_elem = SRC_BUF_ELEM +  256 + 257; /* for ARI */
	p->swap.data      = kmalloc(n_elem * sizeof(unsigned int));
	p->swap.nelements = n_elem;
	BUG_ON(!p->swap.data);

	n_elem = COMPR_BUF_ELEM;
	p->compressed.data = kmalloc(COMPR_BUF_ELEM * sizeof(unsigned int));
	p->compressed.nelements = n_elem;
	BUG_ON(!p->compressed.data);

	for (i = 0; i < SRC_BUF_ELEM; i++)
		((unsigned int *) p->source.data)[i] = i;


	p->start = ktime_get();
	pn_input_task(pn, t);
}


int demo(void *data)
{
	int i;
	int *go;

	struct proc_net *pn;


	pn = pn_create();

	BUG_ON(!pn);

	pn_prepare_nodes(pn);


	go = (int *) data;

	(*go) = 1;	/* signal ready */

	/* wait for trigger */
	while (ioread32be(go) != CONFIG_SMP_CPUS_MAX);

	/* execute test 5 times */
	for (i = 0; i < 25; i++) {

		pn_new_input_task(pn);

		pn_process_inputs(pn);

		while (pn_process_next(pn));

		pn_process_outputs(pn);
	}


	(*go)--;

	return 0;
}


void demo_start(void)
{
	int i;
	int go;

	struct task_struct *t;


	printk("PROC NET DEMO STARTING\n");

	printk("Creating tasks, please stand by\n");

	for (i = 0; i < CONFIG_SMP_CPUS_MAX; i++) {

		fm_ari_high[i] = 0xffff; /* init ARI */

		go = 0;


		t = kthread_create(demo, &go, i, "DEMO");

		if (!IS_ERR(t)) {
			/* allocate 95% of the cpu, period = 100 ms */
			kthread_set_sched_edf(t, 100 * 1000, 98 * 1000, 95 * 1000);

			if (kthread_wake_up(t) < 0) {
				printk("---- %s NOT SCHEDUL-ABLE---\n", t->name);
				BUG();
			}

			while (!ioread32be(&go)); /* wait for task to become ready */

		} else {
			printk("Got an error in kthread_create!");
			break;
		}


		printk("Task ready on cpu %d\n", i);
	}

	printk("Triggering...\n");

	go = CONFIG_SMP_CPUS_MAX; /* set trigger */
	sched_yield();

	while (ioread32be(&go)); /* wait for completion */


	printk("PROC NET DEMO DONE\n");

}
