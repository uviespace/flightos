#include <kernel/module.h>
#include <kernel/kthread.h>
#include <kernel/sysctl.h>
#include <kernel/printk.h>
#include <kernel/string.h>
#include <kernel/time.h>
#include <kernel/tick.h>


#define XDIM 64
#define YDIM 64

#define SET_PIXEL(img,cols,x,y,val) { img[x + cols * y] = val; }
#define GET_PIXEL(img,cols,x,y) ( img[x + cols * y] )
#define SWAP_SORTU(a,b) { if ((a) > (b)) { a = a ^ b; b = b ^ a; a = a ^ b; } }
#define SIGDIVFWHM 0.42466452f
#define AMPL_PSF 100.0f
#define TWO_PI 6.28318531f
#define MODE_GAUSS 0
#define FGS 1
#define FWHM 5
#define TARGETS 5
#define BINSIZE 3
#define REF 21
#define TARGETSIGNAL 300000
#define SIG_TA 400
#define ITER 5
#define SIGLIM 100
#define SIGMALIM 1
#define PLIM 0.5

/**
 * @brief    calculate the square root of a single precision float using the processor-built-in instruction
 * @param    x the radicand
 * @returns  the principal square root
 */

float fsqrts(float x)
{
  float y;
  __asm__("fsqrts %1, %0 \n\t"
          : "=f" (y)
          : "f" (x));
  return y;
}

typedef union {
  uint32_t u;
  float f;
} float_bits;

int fls(int x)
{
	int r = 32;

	if (!x)
		return 0;
	if (!(x & 0xffff0000u)) {
		x <<= 16;
		r -= 16;
	}
	if (!(x & 0xff000000u)) {
		x <<= 8;
		r -= 8;
	}
	if (!(x & 0xf0000000u)) {
		x <<= 4;
		r -= 4;
	}
	if (!(x & 0xc0000000u)) {
		x <<= 2;
		r -= 2;
	}
	if (!(x & 0x80000000u)) {
		x <<= 1;
		r -= 1;
	}
	return r;
}

int __attribute__((weak)) __clzdi2(long val)
{
	return 32 - fls((int)val);
}

#define FLT_MANT_DIG 24
float __floatundisf(uint64_t a)
{

    const unsigned N = sizeof(uint64_t) * CHAR_BIT;
    int sd = N - __builtin_clzll(a);  /* number of significant digits */
    int e = sd - 1;             /* 8 exponent */


    if (a == 0)
        return 0.0F;

    if (sd > FLT_MANT_DIG)
    {
        /*  start:  0000000000000000000001xxxxxxxxxxxxxxxxxxxxxxPQxxxxxxxxxxxxxxxxxx
         *  finish: 000000000000000000000000000000000000001xxxxxxxxxxxxxxxxxxxxxxPQR
         *                                                12345678901234567890123456
         *  1 = msb 1 bit
         *  P = bit FLT_MANT_DIG-1 bits to the right of 1
         *  Q = bit FLT_MANT_DIG bits to the right of 1
         *  R = "or" of all bits to the right of Q
         */
        switch (sd)
        {
        case FLT_MANT_DIG + 1:
            a <<= 1;
            break;
        case FLT_MANT_DIG + 2:
            break;
        default:
            a = (a >> (sd - (FLT_MANT_DIG+2))) |
                ((a & ((uint64_t)(-1) >> ((N + FLT_MANT_DIG+2) - sd))) != 0);
        };
        /* finish: */
        a |= (a & 4) != 0;  /* Or P into R */
        ++a;  /* round - this step may add a significant bit */
        a >>= 2;  /* dump Q and R */
        /* a is now rounded to FLT_MANT_DIG or FLT_MANT_DIG+1 bits */
        if (a & ((uint64_t)1 << FLT_MANT_DIG))
        {
            a >>= 1;
            ++e;
        }
        /* a is now rounded to FLT_MANT_DIG bits */
    }
    else
    {
        a <<= (FLT_MANT_DIG - sd);
        /* a is now rounded to FLT_MANT_DIG bits */
    }
    float_bits fb;
    fb.u = ((e + 127) << 23)       |  /* exponent */
           ((int64_t)a & 0x007FFFFF);  /* mantissa */
    return fb.f;
}



double __floatundidf(uint64_t a) {
  static const double twop52 = 4503599627370496.0;           // 0x1.0p52
  static const double twop84 = 19342813113834066795298816.0; // 0x1.0p84
  static const double twop84_plus_twop52 =
      19342813118337666422669312.0; // 0x1.00000001p84

  union {
    uint64_t x;
    double d;
  } high = {.d = twop84};
  union {
    uint64_t x;
    double d;
  } low = {.d = twop52};

  high.x |= a >> 32;
  low.x |= a & UINT64_C(0x00000000ffffffff);

  const double result = (high.d - twop84_plus_twop52) + low.d;
  return result;
}



struct valpack{
    float flag;
    unsigned int magnitude;
};

struct coord{
  float x;
  float y;
  struct valpack validity;
};

unsigned int *image = (unsigned int *) 0x60000000;




struct coord ArielCoG (unsigned int *img, unsigned int rows, unsigned int cols, unsigned int iterations, int mode, float fwhm_x, float fwhm_y, int fgs, unsigned int refined_roi_size, unsigned int CenSignalLimit, unsigned int CenSigmaLimit, float PearsonLimit);





void GetArielPSF(float *img, unsigned int cols, unsigned int rows, float x0, float y0, unsigned int Fgs)
{
    int x, y;
    float value;
    float xdist, ydist, dist, a4, b4, a3, b3, a2, b2, a1, b1, a0, b0;

    /* coefficients for intensity(dist) = a*dist + b */
    if (Fgs == 1) {
      a4 = -0.00016272; //* AMPL_PSF; /* TODO: these have too high amplitudes */
        b4 = 0.00162775; // * AMPL_PSF;
        a3 = -0.00266744; // * AMPL_PSF;
        b3 = 0.01665607; // * AMPL_PSF;
        a2 = -0.00833363; // * AMPL_PSF;
        b2 = 0.03932081; // * AMPL_PSF;
        a1 = -0.00894235; // * AMPL_PSF;
        b1 = 0.04053825; // * AMPL_PSF;
        a0 = -0.00370579; // * AMPL_PSF;
        b0 = 0.0353017; // * AMPL_PSF;
    }
    else { /* only FGS == 2 applies */
        a4 = -0.00038028; // * AMPL_PSF;
        b4 = 0.00381385; // * AMPL_PSF;
        a3 = -0.0028086; // * AMPL_PSF;
        b3 = 0.01838377; // * AMPL_PSF;
        a2 = -0.00543297; // * AMPL_PSF;
        b2 = 0.02888125; // * AMPL_PSF;
        a1 = -0.00468256; // * AMPL_PSF;
        b1 = 0.02738044; // * AMPL_PSF;
        a0 = -0.00181719; // * AMPL_PSF;
        b0 = 0.02451507; // * AMPL_PSF;
    }


#define DO_WE_HAVE_TO_CLEAR_THE_IMAGE  /* I guess we do...? just make sure */
#ifdef DO_WE_HAVE_TO_CLEAR_THE_IMAGE
    /* if we don't, this saves 10% cycles */
    for (y = 0; y < rows*cols; y++)
	    img[y] = 0;
#endif

    {
	    int xs, ys, xe, ye;

	   /* 1 extra pixel  to accomodate the outer edge */
	    xs = x0 - 10 - 1;
	    ys = y0 - 10 - 1;

	    xe = x0 + 10 + 1;
	    ye = y0 + 10 + 1;

	/* loop safety, maybe change to more sensible area */
	    if (xs < 0)
		    xs = 0;
	    if (ys < 0)
		    ys = 0;
	    if (xe > cols)
		    xe = cols;
	    if (ye > rows)
		    ye = rows;

     for (y = ys; y < ye; y++)
        for (x = xs; x < xe; x++)
        {
            /* calculate distance square to center */
            xdist = x - x0;
            ydist = y - y0;

            /* speed up */
                dist = fsqrts(xdist*xdist + ydist*ydist);  /* fsqrts takes 22 cycles */

                /* calculate PSF from piecewise linear approximation */
                if (dist > 10) /* outer border */
                    value = 0;
                else if (dist > 6)
                    value = (a4*dist + b4);

                else if (dist > 4) /* 6..20 */
                    value = (a3*dist + b3);

                else if (dist > 2) /* 6..20 */
                    value = (a2*dist + b2);

                else if (dist > 1) /* 2..6 */
                    value = (a1*dist + b1);

                else /* 0..2 */
                    value = (a0*dist + b0);
            
		SET_PIXEL(img, cols, x, y, value);

        }
    }

    return;
}

/**
 * @biref   Region of interest refinement for the centroiding algorithm
 *
 * The function takes the original image and extracts a smaller RoI starting from the defined point
 *
 * @param   image: array containing the original RoI
 * @param   width/height: dimensions of the image
 * @param   target_x/target_y: upper left corner of the RoI
 * @param   roi_size: size of the refined RoI
 *
 * @returns refined Region of Interest
 */

void refine_RoI (unsigned int *image, unsigned int *roi, unsigned int width, unsigned int height, unsigned int target_x, unsigned int target_y, unsigned int roi_size)
{
  unsigned int x_ctr, y_ctr;

  for (x_ctr = 0; x_ctr < roi_size; x_ctr++)
    {
      for (y_ctr = 0; y_ctr < roi_size; y_ctr++)
	     {
	        roi[x_ctr + y_ctr*roi_size] = image[target_x+x_ctr + (target_y + y_ctr)*height];
       }
    }

  return;
}



/**
 * @brief Calculates Weighted Center of Gravity for a 2d image
 *
 * In this algorithm, the inner product of img and weights goes into the
 * @ref CenterOfGravity2D algorithm. That algorithm sums up the pixel values in an unsigned int,
 * so the data and the dimensions that we pass must account for that. Consequently, the range of
 * values in our data array needs to be reduced if they are too large. For this purpose, the
 * maximum value of the products is taken, its number of bits determined and checked against a user-define number
 * (CogBits, default = 16). If that exceeds the CogBits, all pixels are right shifted to fit into the range of values.
 * Remember, we will find the center of gravity (position) from that data set, so multiplicative factors can be ignored.
 *
 * @param       img	 a buffer holding the image
 * @param       weights  a buffer holding the weights
 * @param	rows	 the number of rows in the image
 * @param	cols	 the number of columns in the image
 * @param[out]	x	 x position
 * @param[out]	y	 y position
 *
 * @note   It is fine to work in place, i.e. img and weights can be identical.
 */

void WeightedCenterOfGravity2D (unsigned int *img, float *weights, unsigned int rows, unsigned int cols, float *x, float *y)
{
  unsigned int i, j;
  float a;
    float tmp; /* was: double; precision is still good */

    float posx = 0.0;
    float posy = 0.0;
    float sum = 0.0;

    /*
       start by iterating columns, i.e. contiguous sections of memory,
       so the cache will be primed at least for small images for
       the random access of rows afterwards.
       Large images should be transposed beforehand, the overhead is a lot
       smaller that a series of cache misses for every single datum.
     */


   /* for the x axis */
    for (j = 0; j < cols; j++)
    {
        tmp = 0;

        for (i = 0; i < rows; i++){
	  a = (GET_PIXEL(img, cols, j, i) * GET_PIXEL(weights, cols, j, i));
            tmp += a;
	}
        posx += tmp * (j + 1);
    }



    /* for the y axis */
    for (i = 0; i < rows; i++)
    {
        tmp = 0;

        for (j = 0; j < cols; j++){
	  a = (GET_PIXEL(img, cols, j, i) * GET_PIXEL(weights, cols, j, i));
            tmp += a;
	}
        posy += tmp * (i + 1);
        sum += tmp;
    }

    if (sum != 0.0)
        (*y) = (float)(posy / sum) - 1.0f;

    if (sum != 0.0)
        (*x) = (float)(posx / sum) - 1.0f;

    return;
}


/**
 * @brief Calculates Intensity Weighted Center of Gravity for a 2d image
 *
 * In the IWCoG algorithm, the image is basically squared, before it goes into the @ref CenterOfGravity2D.
 * This is achieved by calling @ref WeightedCenterOfGravity2D with img as data and weights parameter.
 *
 * @param       img	 a buffer holding the image
 * @param	rows	 the number of rows in the image
 * @param	cols	 the number of columns in the image
 * @param[out]	x	 x position
 * @param[out]	y	 y position
 *
 */

unsigned int weight[XDIM*YDIM];
void IntensityWeightedCenterOfGravity2D (unsigned int *img, unsigned int rows, unsigned int cols, float *x, float *y)
{
    /* the IWC just works on the square of the image */
    unsigned int i, j;
    unsigned int a;
    uint64_t tmp; /* was: double */

    uint64_t posy = 0;
    uint64_t posx = 0;
    uint64_t sum  = 0;

    /*
       start by iterating columns, i.e. contiguous sections of memory,
       so the cache will be primed at least for small images for
       the random access of rows afterwards.
       Large images should be transposed beforehand, the overhead is a lot
       smaller that a series of cache misses for every single datum.
     */


    /* for the x axis */
    for (j = 0; j < cols; j++)
    {
        tmp = 0;

        for (i = 0; i < rows; i++){
	    a = GET_PIXEL(img, cols, j, i);
            tmp += a*a;
        }
        posx += tmp * (j + 1);
    }


    /* for the y axis */
    for (i = 0; i < rows; i++)
    {
        tmp = 0;

        for (j = 0; j < cols; j++){
	    a = GET_PIXEL(img, cols, j, i);
            tmp += a * a;
	}
        posy += tmp * (i + 1);
        sum += tmp;
    }
#if 0
    if (sum != 0.0)
        (*y) = (float)(posy / sum) - 1.0f;

    if (sum != 0.0)
        (*x) = (float)(posy / sum) - 1.0f;
#else /* meh, conversion stinks, do it like this for now... */

{
    volatile uint32_t l;
    float b;

    posy = posy/sum;
    l = (uint32_t) (posy);

    b = (float) l;
    (*y) = b;

    posx = posx/sum;
    l = (uint32_t) (posx);

    b = (float) l;
    (*x) = b;
}
#endif
    return;
}



void IterativelyWeightedCenterOfGravity2D (unsigned int *img, float *weights, unsigned int rows, unsigned int cols, unsigned int iter, float *x, float *y, int mode, float fwhm_x, float fwhm_y, int Fgs)
{
    unsigned int i;

    for (i = 0; i < iter; i++)
    {
        if (mode == MODE_GAUSS)
	  {
	  }
        else
	  {
            GetArielPSF (weights, cols, rows, *x, *y, Fgs);
	  }

        WeightedCenterOfGravity2D (img, weights, rows, cols, x, y);
    }
    return;
}



/**
 * @brief    calculates the pearson r for two given 1-D samples
 *
 * Function used for the calculateion of the the pearson coefficient that is
 * used as the quality metric of the Centroid Validation
 *
 * @param    measured_data      array containing the measured data distribution
 * @param    reference_data     array containing the reference data
 * @param    length_set         length of the given data sets
 *
 * @returns the pearson r of the two given samples
 */

float pearson_r(unsigned int *measured_data, const float* reference_data, unsigned int signal, unsigned int length_set)
{
    unsigned int i;
    float mean_data, mean_ref, sum1, sum2, sum3, coeff;

    sum1 = 0;
    sum2 = 0;
    sum3 = 0;
    mean_data = 0;
    mean_ref = 0;

    for(i = 0; i<length_set; i++){
      mean_data = mean_data + measured_data[i];
      mean_ref = mean_ref + reference_data[i]*signal;
    }
    mean_data = mean_data / length_set;
    mean_ref = mean_ref / length_set;

    for(i = 0; i<length_set; i++){
      sum1 = sum1 + (measured_data[i]-mean_data)*(reference_data[i]*signal-mean_ref);
      sum2 = sum2 + (measured_data[i]-mean_data)*(measured_data[i]-mean_data);
      sum3 = sum3 + (reference_data[i]*signal-mean_ref)*(reference_data[i]*signal-mean_ref);
    }

    coeff = sum1/fsqrts(sum2*sum3);
    return coeff;
}

/**
 * @brief    extract two single pixel rows from a given image in a cross shape
 *
 * Function used for the one-dimensional array extraction used for the pearson
 * coefficient that is used as the quality metric of the Centroid Validation
 *
 * @param    data               array of input samples
 * @param    strip_x/strip_y    Pointers of the target strip arrays
 * @param    dim_x/dim_y        size of data in x and y
 * @param    length             length of the strips
 * @param    x_center           estimated center in x
 * @param    y_center           estimated center in y
 *
 */

void extract_strips(unsigned int *data, unsigned int *strip_x, unsigned int *strip_y, unsigned int dim_x, unsigned int dim_y, unsigned int length, float x, float y)
{
    unsigned int i, x_origin, y_origin, pos_x, pos_y;
#if 0
    x_origin = floor(x)-floor(length/2);
    y_origin = floor(y)-floor(length/2);
#else
    x_origin = ((unsigned int) x) - length/2;
    y_origin = ((unsigned int) y) - length/2;
#endif
    pos_x = x_origin;
    pos_y = y_origin;

    for(i = 0; i<length; i++)
    {
      if((pos_x < 0)||(pos_x > (dim_x-1)))
      {
        strip_x[i] = 0;
      }
      else
      {
#if 0
        strip_x[i] = GET_PIXEL(data, dim_x, pos_x, (int)floor(y));
#else
        strip_x[i] = GET_PIXEL(data, dim_x, pos_x, (int) y);
#endif
      }

      if((pos_y < 0)||(pos_y > (dim_y-1)))
      {
        strip_y[i] = 0;
      }
      else
      {
#if 0
        strip_y[i] = GET_PIXEL(data, dim_x, (int)floor(x), pos_y);
#else
        strip_y[i] = GET_PIXEL(data, dim_x, (int)x, pos_y);
#endif
      }
      pos_x = pos_x + 1;
      pos_y = pos_y + 1;
    }


}

/**
 * @brief    calculates mean and standard deviation for a given dataset
 * @param[in]    data    pointer to the input data (integer)
 * @param[in]    len     number of values to process
 * @param[out]   pointer to the mean (float) to store the result
 * @param[out]   pointer to the stdev (float) to store the result
 * @note         considers Bessel correction
 */


void MeanSigma (int *data, int len, float *m, float *sig)
{
  int i;
  double sum = 0;
  double sumq = 0;
  double mean, var, sigma;

  /* avoid division by zero */
  if (len == 0)
    {
      /* m and sig will be undefined */
      return;
    }
  else if (len == 1)
    {
      *m = data[0];
      *sig = 0.0f;
      return;
    }

  for (i=0; i<len; i++)
    sum += data[i];

  mean = (double)sum/len;

  for (i=0; i<len; i++)
    sumq += (data[i]-mean) * (data[i]-mean);

  var = 1./(len-1.) * sumq; /* with Bessel corr. */
  sigma = fsqrts(var);

  *m = (float) mean;
  *sig = (float) sigma;

  return;
}


/**
 * @brief       Median calculation using the Algorithm by Torben Mogensen.
 *              Based on a public domain implementation by N. Devillard.
 * @param       data  place where the data are stored
 * @param       len   number of values to process
 * @returns     median of the given values
 * @note        for an even number of elements, it returns the smaller one
 * @note        The Torben mehtod does not need a separate buffer and it does not mix the input
 */

int Median (int *data, int len)
{
  int i, less, greater, equal;
  int min, max, guess, maxltguess, mingtguess;

  min = max = data[0] ;

  /* find min and max */
  for (i=1 ; i < len ; i++)
    {
      if (data[i] < min)
        min=data[i];

      if (data[i] > max)
        max=data[i];
    }

  while (1)
    {
      /* update guesses */
      guess = (min + max) >> 1;
      less = 0;
      greater = 0;
      equal = 0;
      maxltguess = min;
      mingtguess = max;

      /* find number of smaller and larger elements than guess */
      for (i=0; i < len; i++)
        {
          if (data[i] < guess)
            {
              less++;
              if (data[i] > maxltguess)
                maxltguess = data[i];
            }
          else if (data[i] > guess)
            {
              greater++;
              if (data[i] < mingtguess)
                mingtguess = data[i];
            }
          else
            equal++;
        }

      /* half the elements are less and half are greater, we hav found the median */
      if ((less <= (len+1)>>1) && (greater <= (len+1)/2))
        break;

      else if (less > greater)
        max = maxltguess ;
      else
        min = mingtguess;
    }

  if (less >= (len+1)>>1)
    return maxltguess;
  else if (less+equal >= (len+1)>>1)
    return guess;

  return mingtguess;
}

/**
 * @brief    get minimum and maximum value of an unsigned int buffer
 * @param    data     array of input samples
 * @param    len      number of samples
 * @param    min[out] calculated minimum
 * @param    max[out] calculated maximum
 *
 * @note     If len is 0 then min = 0 and max = 0xffffffff.
 */

void MinMaxU32 (unsigned int *data, unsigned int len, unsigned int *min, unsigned int *max)
{
    unsigned int i;

    *min = 0xffffffffu;
    *max = 0x0u;

    for (i=0; i < len; i++)
    {
        *min = *min > data[i] ? data[i] : *min;
        *max = *max < data[i] ? data[i] : *max;
    }

    return;
}

/**
 * @brief    make basic checks on the ROI to predict the validity of the centroid
 *
 * Function to analyze an image and return a code carrying the validity of the centroid and the magnitude of the target star
 * that will be calculated from this image.
 * The image is binned to 5x5 pixels, then the following checks are carried out:
 *
 *   1. Check for constant image (is the minumum equal to the maximum?)
 *   2. Check for a star (the mean must be larger than the median)
 *   3. The star must be a distinct feature (the maximum should be larger than median + 2 sigma)
 *   4. The range of values must be larger than a certain span
 *   5. The sigma must be larger than a certain span
 *   6. The signal inside of a given radius around the estimated center must surpass a given percentage
 *
 * The background that is subtracted from the image for the photometry is given by the minimum pixel value inside of the
 * ROI with the exception of known dead pixels. If dead pixels are present, they shall be replaced by the minimum value
 * in prior steps.
 *
 * @param    data               array of input samples
 * @param    x_dim              size in x
 * @param    y_dim              size in y
 * @param    CenSignalLimit     threshold for 4
 * @param    CenSigmaLimit      threshold for 5
 * @param    CenPercentLimit    threshold for 6
 * @param    ApertureRad        Aperture radius for the photometry
 * @param    x_center           estimated center in x
 * @param    y_center           estimated center in y
 *
 * @returns  validity code of centroid
 */

const float FGS1_X[25] = {1.47167275e-05, 2.38461646e-05, 3.87655628e-05, 6.75726225e-05, 2.11215257e-04, 5.19455207e-04, 1.19928135e-03, 2.63447848e-03, 4.56727639e-03, 1.08543772e-02, 1.72144731e-02, 2.91691617e-02, 2.51506170e-02, 2.74147190e-02, 2.36298995e-02, 1.53269946e-02, 5.16576064e-03, 1.03382773e-03, 2.64965646e-04, 1.78873205e-04, 8.15633061e-05, 4.72219511e-05, 3.34156503e-05, 9.80079452e-06, 1.41377845e-05};

const float FGS1_Y[25] = {0.00029489, 0.0003827, 0.0006489,0.00116522, 0.00165326, 0.00183629, 0.00184897, 0.00274072, 0.00757158, 0.02271154, 0.04101486, 0.03054224, 0.02515062, 0.03338782, 0.0334162, 0.02987768, 0.01898888, 0.00904966, 0.00433987, 0.00245042, 0.0014353, 0.0009233, 0.00049994, 0.0003136, 0.00019248};

const float FGS2_X[25] = {4.58516318e-05, 1.05016227e-04, 2.02699739e-04, 3.81423208e-04, 5.77229311e-04, 1.06740244e-03, 1.73896758e-03, 2.92656460e-03, 6.47117555e-03, 8.77449011e-03, 1.49745506e-02, 1.70449780e-02, 1.65937231e-02, 2.05735867e-02, 1.63470839e-02, 1.33791399e-02, 8.80076927e-03, 3.88463245e-03, 1.35837579e-03, 4.62881416e-04, 1.76952008e-04, 1.00691379e-04, 6.19276252e-05, 5.27374747e-05, 4.55186973e-05};

const float FGS2_Y[25] = {0.00050931, 0.00081074, 0.00076338, 0.00113407, 0.0012236, 0.00225054, 0.00288255, 0.00549922, 0.01107709, 0.02122439, 0.02363573, 0.01719913, 0.01659372, 0.02472113, 0.02537891, 0.02027583, 0.01737751, 0.01285062, 0.00836713, 0.00425878, 0.00258723, 0.00135676, 0.00097328, 0.00060778, 0.0004461};


unsigned int binned[25];
unsigned int x_strip[25];
unsigned int y_strip[25];
unsigned int ref_x[25];
unsigned int ref_y[25];


/**
 * @brief calculate an integer square root
 * @note from wikipedia (I think)
 */

static int isqrt(int num)
{

	int res = 0;
	int bit = 1 << 30;


	while (bit > num)
		bit >>= 2;

	while (bit) {

		if (num >= res + bit) {
			num -= res + bit;
			res = (res >> 1) + bit;
		} else {
			res >>= 1;
		}

		bit >>= 2;
	}

	return res;
}





struct valpack CheckRoiForStar (unsigned int *data, unsigned int x_dim, unsigned int y_dim, unsigned int CenSignalLimit, unsigned int CenSigmaLimit, float PearsonLimit, unsigned int fgs, double x_center, double y_center)
{
    unsigned int i;
    unsigned int median, minimum, maximum;
    unsigned int binwidth, binheight, binx, biny, x, y, ystart, rad;
    float mean, sigma;
    float pearson_x, pearson_y;
    int xdist, ydist, dist;
    unsigned int sum;
    int mag;
    struct valpack package;

    if (fgs == 1)
    {
      rad = 7;
    }
    else
    {
      rad = 9;
    }
    sum = 0;

    if ((x_dim >= 5) && (x_dim >= 5)) /* 3x3 bins make no sense for images < 3x3 */
    {
        binwidth = x_dim / 5;
        binheight = y_dim / 5;
    }
    else
    {
        package.flag = 111;
        package.magnitude = 0;
        return package;
    }

    /* Note: For dimensions which are not divisible by 3, the remainder is ignored.
       Example: in a 15 by 14 image each of the bins will sample 5 pixels in x * 4 pixels in y
       and the rest is ignored */

    /* clear the binned array first */
    for (i=0; i < 25; i++)
    {
        binned[i] = 0;
    }

     /* bin to 3x3 */
    for (biny = 0; biny < 5; biny++) {

	    for (y = 0; y < binheight; y++) {

		    ystart = (y + biny*binheight) * x_dim;

		    for (binx = 0; binx < 5; binx++) {

			    int bin = binx + 5 * biny;
			    int binoff = binx * binwidth + ystart;
			    int s = 0;

			    for (x = 0; x < binwidth; x++)
				    s += data[x + binoff];

			    binned[bin] = s;

		    }
	    }
    }


    /* convert the sums to averages */
    for (i=0; i < 25; i++)
    {
        binned[i] /= (binwidth * binheight);
    }

    MeanSigma ((int *)binned, 25, &mean, &sigma);

    median = Median ((int *)binned, 25);

    MinMaxU32 (binned, 25, &minimum, &maximum);

    /* DEBUGP("CEN: Mean %f Sig: %f Med: %u Min: %u Max: %u\n", mean, sigma, median, minimum, maximum); */

    /* rule 1: the image must not be constant */
    if (minimum == maximum)
    {
        package.flag = 101;
        package.magnitude = 0;
        return package;
    }

    /* rule 2: there must be a star */
    if (mean < median)
    {
        package.flag = 102;
        package.magnitude = 0;
        return package;
    }

    /* rule 3: the star must be sharp */
    if ((median + 2*sigma) > maximum)
    {
        package.flag = 103;
        package.magnitude = 0;
        return package;
    }

    /* rule 4: there must be a signal */
    if ((maximum - minimum) < CenSignalLimit)
    {
        package.flag = 104;
        package.magnitude = 0;
        return package;
    }

    /* rule 5: the sigma must be large */
    if (sigma < CenSigmaLimit)
    {
        package.flag = 105;
        package.magnitude = 0;
        return package;
    }


    /*rule 6: the signal inside of a 5px radius circle around the estimated center must contain a certain percentage of the total signal*/

    {
	int x0, y0, x1, y1;

	y0 = y_center - rad;
	x0 = x_center - rad;
	y1 = y_center +	rad;
	x1 = x_center + rad;

	/* loop safety, maybe change to more sensible area */
	if (y0 < 0)
		y0 =0;
	if (x0 < 0)
		x0 =0;

	if (y1 > y_dim)
		y1 = y_dim;
	if (x1 > x_dim)
		x1 = x_dim;



    for (y = y_center - rad; y < y_center + rad; y++)
    {
        for (x = x_center - rad; x < x_center +rad; x++)
        {
            /* calculate distance square to center */
            xdist = x - x_center;
            ydist = y - y_center;

            /* speed up */
            if ((xdist <= rad) || (ydist <= rad))
            {
                dist = isqrt(xdist*xdist + ydist*ydist);/* fsqrts takes 22 cycles */

                if (dist <= rad)
                {
                    sum += GET_PIXEL(data, x_dim, x, y);

                }
            }
        }
    }
    }


    extract_strips(data, x_strip, y_strip, x_dim, y_dim, 25, x_center, y_center);


    mag = sum;

    if(fgs = 1)
    {
      pearson_x = pearson_r(x_strip, FGS1_X, mag, 25);
      pearson_y = pearson_r(y_strip, FGS1_Y, mag, 25);
    }
    else
    {
      pearson_x = pearson_r(x_strip, FGS2_X, mag, 25);
      pearson_y = pearson_r(y_strip, FGS2_Y, mag, 25);
    }

    
    if ((pearson_x*pearson_y) < PearsonLimit)
    {
        package.flag = 106;
    }
    else
    {
        package.flag = pearson_x*pearson_y;
    }
    package.magnitude = mag;
    return package;
}


float weights[REF*REF];
unsigned int roi[REF*REF];

struct coord ArielCoG (unsigned int *img, unsigned int rows, unsigned int cols, unsigned int iterations, int mode, float fwhm_x, float fwhm_y, int fgs, unsigned int refined_roi_size, unsigned int CenSignalLimit, unsigned int CenSigmaLimit, float PearsonLimit)
{

  float x_res = (rows/2), y_res = (cols/2);
  float x_roi, y_roi;
  unsigned int x_start, y_start;
  unsigned int i;
  struct coord res;
  
  IntensityWeightedCenterOfGravity2D (img, rows, cols, &x_res, &y_res);
  //printk("%f %f\n", x_res, y_res);
  x_start = (int) x_res - refined_roi_size/2 + 1;
  y_start = (int) y_res - refined_roi_size/2 + 1;

  x_roi = x_res - x_start;
  y_roi = y_res - y_start;

  for (i=0; i < iterations; i++)
  {

    if (i != 0)
    {
      x_start = (int) (x_start + x_roi) - refined_roi_size/2 + 1;
      y_start = (int) (y_start + y_roi) - refined_roi_size/2 + 1;
    }

    if (x_start < 0)
      {
        x_start = 0;
      }
    if (y_start < 0)
      {
        y_start = 0;
      }
    if (x_start > (rows - refined_roi_size))
      {
        x_start = rows - refined_roi_size;
      }
    if (y_start > (cols - refined_roi_size))
      {
        y_start = cols - refined_roi_size;
      }
    refine_RoI (img, roi, rows, cols, x_start, y_start, refined_roi_size);
    
    if (mode == MODE_GAUSS)
      {
      }
    else
      {
        GetArielPSF (weights, refined_roi_size, refined_roi_size, x_roi, y_roi, fgs);
      }

    WeightedCenterOfGravity2D (roi, weights, refined_roi_size, refined_roi_size, &x_roi, &y_roi);

  }


  res.x = x_start + x_roi;
  res.y = y_start + y_roi;

  res.validity = CheckRoiForStar (img, rows, cols, CenSignalLimit, CenSigmaLimit, PearsonLimit,fgs, res.x, res.y);

  return res;
}












__attribute__((unused))
static int show_timer_irq(void)
{
	char buf0[32];
	char buf1[32];
	char buf2[32];
	char buf3[32];
	char buf4[32];

	/* FIXME static declarations are still a problem (module loader issue) */
	struct sysobj *sys_irq = NULL;

	int cur;
	int prev = 0;

	ktime cnt =0;
	ktime p = 0;
	ktime c;
	ktime drift =0;

#if 0
	sys_irq = sysset_find_obj(sysctl_root(), "/sys/irl/primary");

	if (!sys_irq) {
		printk("Error locating sysctl entry\n");
		return -1;
	}
#endif
	p = ktime_get();

	/* loop here because of the static issue (zeroing/allocation?) it's
	 * unaligned errors */
	while (1) {
		c = ktime_get();

		drift = c - p - cnt * 1000000000ULL;
		cnt++;

#if 0
		sysobj_show_attr(sys_irq, "irl" , buf0);
		sysobj_show_attr(sys_irq,   "8",  buf1);
		sysobj_show_attr(sys_irq,   "9",  buf2);
		sysobj_show_attr(sys_irq,  "10",  buf3);
		sysobj_show_attr(sys_irq,  "11",  buf4);

		cur = atoi(buf0);


		printk("IRQ total: %s, delta %d\n"
		       "CPU timers: \n"
		       " [0]: %s\n"
		       " [1]: %s\n"
		       " [2]: %s\n"
		       " [3]: %s\n"
		       " [t]: %lld %lld\n",
		       buf0, cur-prev, buf1, buf2, buf3, buf4, ktime_delta(c,p), drift);
#endif

		printk("ktime: %lld ns; dev.abs.: %lld ns\n", ktime_delta(c,p), drift);
#if 0
		prev = cur;
#endif
		sched_yield();
	}

	return 0;
}


/**
 * this is the equivalent of "main()" in our module
 */


static int test(void *data)
{
	(void) data;
	ktime t0,t1;
	ktime s;
	ktime cnt =0;
	ktime drift;


	/* print irq timers and yield until rescheduled */

	s = ktime_get();

	while (1) {
#if 1
		struct coord obsPos;
		t0 = ktime_get();
		obsPos = ArielCoG (image, XDIM, YDIM, ITER, 1, FWHM, FWHM, FGS, REF, SIGLIM, SIGMALIM, PLIM);
		t1 = ktime_get();
		drift = t1 - ktime_delta(t1, t0) - s - cnt * 100000000ULL;
		cnt++;
		//printk("%f, %f, %f (computed) proc %lld ns drift abs: %lld ns %lld\n", obsPos.x, obsPos.y, obsPos.validity.flag, ktime_delta(t1, t0), drift, t1);
#if 0
		printk("proc %lld us drift abs: %lld ns %lld\n", ktime_us_delta(t1, t0), drift, t1);
#endif
//		printk("31.269337, 31.529366, 0.833741 (best precision reference)\n");
#else
		show_timer_irq();
#endif
		sched_yield();
	}

	return 0;
}



static int time_test(void *data)
{
	while (1) {

//		printk("uptime: %lld ns\n", ktime_get());
		sched_yield();
	}
}



/*
 * This is our startup call, it will be executed once when the module is
 * loaded. Here we create at thread in which our actual module code
 * will run in.
 */

int init_test(void)
{
	struct task_struct *t;

	printk("HI THERE! Note that the update function in line 333 is currently disabled due to missing float conversion\n");

	t = kthread_create(time_test, NULL, 0, "TEST");
#if 1
#if 0
	/* allocate 2% of the cpu in a RT thread */
	kthread_set_sched_edf(t, 1000*1000, 3*1000, 2*1000);
#else
#if 0
	/* compute the centroid every 100 ms*/
	kthread_set_sched_edf(t, 100*1000, 40*1000, 30*1000);
#endif
#endif
#else
	/* defauilt to RR */
#endif

	/* long duration test, print time once every second,
	 * we only need it to compare to an external clock */
	kthread_set_sched_edf(t, 1000*1000, 200*1000, 100*1000);
	if (kthread_wake_up(t) < 0)
		printk("---- TEST NOT SCHEDULABLE---\n");

	return 0;
}


/**
 * this is our optional exit call, it will be called when the module/module
 * is removed by the kernel
 */

int exit_test(void)
{
	printk("module exit\n");
	/* kthread_destroy() */
	return 0;
}

/* here we declare our init and exit functions */
module_init(init_test);
module_exit(exit_test);
