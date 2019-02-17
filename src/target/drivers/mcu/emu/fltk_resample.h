#include <FL/Fl_Image.H>
#include <string.h>



// Written (c) by Roman Kantor and Miravex Ltd
// The code is Free software distributed unfer LGPL license with FLTK exceptions, see http://www.fltk.org/COPYING.php

// TWO-dimensional scalling is performend in two steps by rescalling first in one direction
// and then in the other - which at the end has the same effect as direct two-dimensional rescalling...
// All rutines are performed with "integer" types for the speed, precision is not lost because the result is scalled up
// and only after resampling in both directions the values are normalised (scalled back) to the range 0-255.
// The macro rutines themselves are written to be general and can use different types, number of channels,
// sample padding (stride), row padding, can encode/devode channels from the original data etc...



// ONE-DIMENSIONAL ROUTINES

// DOWNSAMPLING (w2<w1):
// Following method seems to work better than classic (bi)linear interpolation as all the original pixels contribute to the
// resulting resampled images with the same weight - that is no pixels are lost or suppressed.
//  Downsampling is performed by "weighted" pixel combinations:
// 1) If whole original pixel lies within the area of a new pixel, its whole walue contributes to that particular new pixel.
// 2) If an original pixel lies on the boundary of two new pixels, its contribution is split to these two pixels with
//    a ratio corresponding to the areas within these pixels. The sum of these two contributions is equal to the whole
//    contribution as in case 1)
// All the contributions are then summed and result in a new pixel - just scalled by normalization factor f = w1.
// This factor should be used later to divide resilting values and scalling back (like to the range 0-255 for 8-bit
// channels for RGB(A) images)


// UPSAMPLING:
// For upsampling linear combination is performed where resulting pixel value is  P = a2 * P1 + a1 * P2
// where contributing factors a2, a1 are proportional to the distances from  the surrounding original pixels.
// The value-scalling (normalizarion) factor (which should be used as a divider for normalisation) is:  f = a2 + a1 = 2 * w1
// (twice the factor for downsampling)

//
// Macro parameters:
// T_SOURCE: type of source data (eg unsigned char)
// T_TARGET: type of ratrget tata (eg unsigned int)
// T_ACU type of intermediate data. This type hould be a type "big enought" fo hold scalled values that it does not overflow after pixel combination (scalling).
//       For instance if T_SOURCE type is 8-bit "unsigned char", T_ACU can be 32-bit "unsigned int" without owerflow for all images with original
//       width w1 < 2^24.  However if you combine  row/collumn resampling, the scale factor is multiplication of both and could easily oferflow 32 bit:
//       (eg scalling down images above 4096x4096 or scalling up images above 2048x2048). For that reason please use 64-bit type for T_ACU like
//       "unsigned long long" during second dimension resampling.
// source - (const) pointer to source data array
// target - pointer to target data array
// source stride - pointer shift between samples
// target_stride - pinter shift between resulting data (normaly for packed data it is 1)
// PREPROCESS - pre-processing sequence (code) for row data stored in "input" variable (of type T_ACU). This is useful eg if multi-channels are
//        encoded within single sample:  For istance if ARGB channels are encoded within single "int" sample getting red channel can be
//        performed by  putting sequence
//             val >>=16; val &= 0xFF;
//        in the place of that parameter.
// POSTPROCESS_STORE - post-processing sequence (code), can be used eg for rescalling back the data or channel encoding. Here symbol "output" is used as variable storing
//        initial value and "tg" is the pointer where the result should be stored. This pointer can already have an initial value
//        for instance for multi-channel encoding and "red channer) you can write something like
//           *tg |= ((output/w1) & 0xFF) << 16;
//        If you do not perform additional rescalling (eb by simple *tg = output;) the value in "output" are scalled by factor f = w1.

#define RESAMPLE_DOWN(T_SOURCE, T_TARGET, T_ACU, w1, w2, source, target, source_stride, target_stride, PREPROCESS, POSTPROCESS_STORE) \
  { \
    const T_SOURCE * src = source; \
    T_TARGET * tg = target; \
    T_ACU pre = 0; \
    T_ACU input; \
    T_ACU output; \
    unsigned rest = w1; \
    for(unsigned int i = w2 - 1; i; i--) { \
      T_ACU full = 0; \
      while(rest>=w2) { \
        input = *src; \
        PREPROCESS \
        src += source_stride; \
        full += input; \
        rest -= w2; \
      } \
      input = *src; \
      PREPROCESS \
      src += source_stride; \
      output = pre + w2 * full + rest * input; \
      POSTPROCESS_STORE \
      tg += target_stride; \
      pre = (w2 - rest) * input; \
      rest += w1 - w2; \
    } \
    output = 0; \
    while(rest>=w2) { \
      input = *src;\
      PREPROCESS \
      output += input;\
      src += source_stride; \
      rest -= w2; \
    } \
    output *= w2; \
    output += pre; \
    POSTPROCESS_STORE \
  }

// This should be somewhat faster than above (if (w1%w2)==0) but for this routine the samples are scalled only by factor f = w1/w2
// where w1 is orihinal size and w2 is the new size
// WARNING: this was not yet properly unit-tested, thease do  do if you want to include this and uncoment all the code marked as FL_RESAMPLE_EXPERIMENTAL

#define RESAMPLE_DOWN_DIVABLE(T_SOURCE, T_TARGET, T_ACU, w1, w2, source, target, source_stride, target_stride, PREPROCESS, POSTPROCESS_STORE) \
  { \
    const T_SOURCE * src = source; \
    T_TARGET * tg = target; \
    unsigned int pp  = w1 / w2 - 1; \
    for(unsigned int i = w2; i; i--){ \
      T_ACU input = *src; \
      src += source_stride; \
      PREPROCESS \
      T_ACU output = input; \
      for(unsigned int j = pp; j; j--){ \
        input = *src; \
        src += source_stride; \
        PREPROCESS \
        output += input; \
      } \
      POSTPROCESS_STORE \
      tg += target_stride; \
    } \
  }


// Resampling by linear interpolation
// This is similar to RESAMPLE_DOWN however this time the result is scalled by factor  f = 2 * w2  - unless modified by POSTPROCESS_STORE sequence.
#define RESAMPLE_UP(T_SOURCE, T_TARGET, T_ACU, w1, w2, source, target, source_stride, target_stride, PREPROCESS, POSTPROCESS_STORE) \
  { \
    unsigned  W2 = w2 * 2; \
    unsigned W1 = w1 * 2; \
    const T_SOURCE * src = source; \
    T_TARGET * tg = target; \
    T_ACU input = *src; \
    PREPROCESS \
    T_ACU output = input * W2; \
    T_ACU tmp = output; \
    unsigned i = 0; \
    unsigned mod = w1; \
    while(mod<=w2){ \
      POSTPROCESS_STORE \
      output = tmp; \
      mod += W1; \
      tg += target_stride; \
      i++; \
    } \
    mod -= w2; \
    T_ACU pre_input = input; \
    src += source_stride; \
    input = *src; \
    PREPROCESS \
    for(unsigned no = w2 - 2 * i; no; no--) { \
      if(mod>=W2){ \
        mod -=W2; \
        pre_input = input; \
        src += source_stride;\
        input = *src; \
        PREPROCESS \
      } \
      T_ACU output = input * mod + (W2 - mod) * pre_input; \
      POSTPROCESS_STORE \
      tg += target_stride; \
      mod += W1;\
    } \
    output = W2 * input; \
    tmp = output; \
    while(i){\
      POSTPROCESS_STORE \
      tg += target_stride; \
      output = tmp; \
      i--; \
    } \
 }




namespace { // anonymous namespace


unsigned get_resample_scale(unsigned w1, unsigned w2) {
  if(w2<w1) {
#ifdef FL_RESAMPLE_EXPERIMENTAL
    if(!(w1%w2)) return w1 /w2;
#endif
    return w1;
  }
  return 2 * w2;
}

// resampling fron "unsigned char" to intermediate "unsigned int" array. The function returns the scalling factor.
unsigned resample(unsigned w1, unsigned w2, const unsigned char * source, unsigned * target, int source_stride, int target_stride, unsigned sets, int source_set_stride, int target_set_stride) {
  if(w2<w1) {
#ifdef FL_RESAMPLE_EXPERIMENTAL
    if(!(w1%w2)) {
      for(unsigned j = 0; j<sets; j++) {
        const unsigned char * sou= source + j * source_set_stride;
        unsigned * tar = target + j * target_set_stride;
        RESAMPLE_DOWN_DIVABLE(unsigned char, unsigned, unsigned, w1, w2, sou, tar, source_stride, target_stride, , *tg = output;)
        return w1 / w2;
      }
    }
#endif
    for(unsigned j = 0; j<sets; j++) {
      const unsigned char * sou= source + j * source_set_stride;
      unsigned * tar = target + j * target_set_stride;
      RESAMPLE_DOWN(unsigned char, unsigned, unsigned, w1, w2, sou, tar, source_stride, target_stride, , *tg = output;)
    }
    return w1;
  }
  for(unsigned j = 0; j<sets; j++) {
    const unsigned char * sou= source + j * source_set_stride;
    unsigned * tar = target + j * target_set_stride;
    RESAMPLE_UP(unsigned char, unsigned, unsigned, w1, w2, sou, tar, source_stride, target_stride, , *tg = output;)
  }
  return 2 * w2;
}

// resampling from intermediate "unsigned" to final "unsigned char" with scalling
void resample_scale(unsigned w1, unsigned w2, const unsigned * source, unsigned char * target, int source_stride, int target_stride, unsigned sets, int source_set_stride, int target_set_stride, unsigned long long scale) {
  unsigned long long sc_add = scale / 2;
  if(w2<w1) {
#ifdef FL_RESAMPLE_EXPERIMENTAL
    if(!(w1%w2)) {
      for(unsigned j = 0; j<sets; j++) {
        const unsigned * sou= source + j * source_set_stride;
        unsigned char * tar = target + j * target_set_stride;
        RESAMPLE_DOWN_DIVABLE(unsigned, unsigned char, unsigned, w1, w2, sou, tar, source_stride, target_stride, , *tg = (output + sc_add)/scale;)
        return;
      }
    }
#endif
    for(unsigned j = 0; j<sets; j++) {
      const unsigned * sou= source + j * source_set_stride;
      unsigned char * tar = target + j * target_set_stride;
      RESAMPLE_DOWN(unsigned , unsigned char, unsigned long long, w1, w2, sou, tar, source_stride, target_stride, , *tg = (output + sc_add)/scale;)
    }
    return;
  }
  for(unsigned j = 0; j<sets; j++) {
    const unsigned * sou= source + j * source_set_stride;
    unsigned char * tar = target + j * target_set_stride;
    RESAMPLE_UP(unsigned, unsigned char, unsigned long long, w1, w2, sou, tar, source_stride, target_stride, , *tg = (output + sc_add)/scale;)
  }
}

// direct resampling  "unsigned char" to  "unsigned char" with scalling
void resample_scale(unsigned w1, unsigned w2, const unsigned char * source, unsigned char * target, int source_stride, int target_stride, unsigned sets, int source_set_stride, int target_set_stride, unsigned scale) {
  unsigned sc_add = scale / 2;
  if(w2<w1) {
#ifdef FL_RESAMPLE_EXPERIMENTAL
    if(!(w1%w2)) {
      for(unsigned j = 0; j<sets; j++) {
        const unsigned char * sou= source + j * source_set_stride;
        unsigned char * tar = target + j * target_set_stride;
        RESAMPLE_DOWN_DIVABLE(unsigned char , unsigned char, unsigned, w1, w2, sou, tar, source_stride, target_stride, , *tg = (output + sc_add)/scale;)
        return;
      }
#endif
      for(unsigned j = 0; j<sets; j++) {
        const unsigned char * sou= source + j * source_set_stride;
        unsigned char * tar = target + j * target_set_stride;
        RESAMPLE_DOWN(unsigned char , unsigned char, unsigned, w1, w2, sou, tar, source_stride, target_stride, , *tg = (output + sc_add)/scale;)
      }
      return;
    }
    for(unsigned j = 0; j<sets; j++) {
      const unsigned char * sou= source + j * source_set_stride;
      unsigned char * tar = target + j * target_set_stride;
      RESAMPLE_UP(unsigned char , unsigned char, unsigned, w1, w2, sou, tar, source_stride, target_stride, , *tg = (output + sc_add)/scale;)
    }
  }



// Finaly this is synthesis of the above functions to use optimal tesizing approach:
// Parameters:
// w2, h2 - new (resampled) image size
// source - pointer to original image data
// w1, h1 - size of the original image
// no_ccgannels - number of adjacent channels (1 for mono images, 3 for RGB and 4 for RGBA or ARGB)
// pixel_stride - poinder shift from pixel to pixel for the original image. If 0, "packed" data are assumed and pixel_size equals to no_channels
// row_stride - pointer shift between rows for the original image. If 0, row_stride = w1 * pixel_stride.
// target - poinrer to array where resampled image data is stored. If 0, a new array is allocated with sufficient size.
// target_pixel_stride - poinder shift from pixel to pixel for the resampled image. If 0, "packed" data are assumed and pixel_size equals to no_channels
// target_row_stride - pointer shift between rows for the resampled image. If 0, target_row_stride = w2 * pixel_stride.
// The function returns the "target" parameter or pointer to newly allocated data (if target==0).

  unsigned char * resample(int w2, int h2, const unsigned char * const source, int w1, int h1, int no_channels, int pixel_stride = 0, int row_stride = 0, unsigned char * target = 0, int target_pixel_stride = 0, int target_row_stride = 0) {
    if(!pixel_stride) pixel_stride = no_channels;
    if(!row_stride) row_stride = pixel_stride * w1;
    const unsigned char * src = source;


    if (pixel_stride <0) {
      src += pixel_stride * (w1-1);
      pixel_stride = -pixel_stride;
    }

    if (row_stride <0) {
      src +=  row_stride * (h1-1);
      row_stride = -row_stride;
    }

    if(!target_pixel_stride) target_pixel_stride = no_channels;
    if(!target_row_stride) target_row_stride = target_pixel_stride * w2;


    int size = target_row_stride *h2;
    if(!target)
      target = new unsigned char[size];

    if(w1==w2 && h1==h2) { // direct copy
      if(target_pixel_stride==pixel_stride) {
        if(target_row_stride==row_stride)
          memcpy(target, src, size * sizeof(unsigned char));
        else for(int i = 0; i<h1; i++)
            memcpy(target + i * target_row_stride , src + row_stride , row_stride * sizeof(unsigned char));
      } else {
        for(int j = 0; j<h1; j++) {
          const unsigned char * s = src + j * row_stride;
          unsigned char * t = target + j * target_row_stride;
          int s_addition = pixel_stride - no_channels;
          int t_addition = target_pixel_stride - no_channels;
          for(int i = 0; i<w1; i++) {
            for(int c = no_channels; c; c--)
              *t++ = *s++;
            s += s_addition;
            t += t_addition;
          }
        }

      }
      return target;
    }

    if(w1==w2) { // no need for intermediate data, we resample only collumns
      unsigned scale = get_resample_scale(h1, h2);
      for(int ch = 0; ch< no_channels; ch++)
        resample_scale(h1, h2, source + ch, target + ch, row_stride, target_row_stride, w1, pixel_stride, target_pixel_stride, scale);
      return target;
    }

    if(h1==h2) { // no need for intermediate data, we resample only rows
      unsigned scale = get_resample_scale(w1, w2);
      for(int ch = 0; ch< no_channels; ch++)
        resample_scale(w1, w2, source + ch, target + ch, pixel_stride, target_pixel_stride, h1, row_stride, target_row_stride, scale);
      return target;
    }

    // General rescalling with intermediate "unsigned int" data array.
    // First we try to find optimal approach to minimize intermediate data size:

    int s = w2 * h1;
    bool rows_first = 1;
    {
      int s2 = w1 * h2;
      if(s2<s) {
        s = s2;
        rows_first = 0;
      }
    }

    unsigned * buffer = new unsigned[s]; // intermediate buffer
    unsigned long long scale = ((unsigned long long)(get_resample_scale(w1, w2))) * ((unsigned long long)(get_resample_scale(h1, h2)));

    if(rows_first) {
      for(int ch = 0; ch< no_channels; ch++) {
        resample(w1, w2, source + ch, buffer, pixel_stride, 1, h1, row_stride, w2);
        resample_scale(h1, h2, buffer, target + ch, w2, target_row_stride, w2, 1, target_pixel_stride, scale);
      }
    } else {
      for(int ch = 0; ch< no_channels; ch++) {
        resample(h1, h2, source + ch, buffer, row_stride, w1, w1, pixel_stride, 1);
        resample_scale(w1, w2, buffer, target + ch, 1, target_pixel_stride, h2, w1, target_row_stride, scale);
      }
    }
    delete[] buffer;
    return target;

  }

}  // end of anonymous namespace

void pixel_mult(unsigned char *dest, const unsigned char *src, unsigned x, unsigned y, unsigned scale_x, unsigned scale_y, unsigned bytes_per_pixel) {
    for (unsigned i = 0; i < y; i++) {
        for (unsigned sy = 0; sy < scale_y; sy++) {
            for (unsigned j = 0; j < x; j++) {
                for (unsigned sx = 0; sx < scale_x; sx++) {
                    for (unsigned c = 0; c < bytes_per_pixel; c++) {
                        *dest++ = src[(i*x+j)*bytes_per_pixel+c];
                    }
                }
            }
        }
    }
}
// This is  a simple function to replace Fl_RGB_Image::copy().
// Note a clumsy hack to detect Fl_RGB_IMAGE (or subclass) using count(0 and d() methods
// (RTTI users can modify this using dynamic_cast< >() function)
// If it is Fl_Bitmap or Fl_Pixmap original resampling methods are used.
/*
Fl_Image * fl_copy_image(Fl_Image * im, int w2, int h2) {
  if(im->count()!=1 || !im->d()) // birmap or pixmap
    return im->copy(w2, h2);

  int ld = im->ld();
  int w1 = im->w();
  int pixel_stride = im->d();
  int row_stride = ld? ld : pixel_stride * w1 ;

  const unsigned char * data = (const unsigned char *)(*(im->data()));
  unsigned char * array = resample(w2, h2, data, w1, im->h(), pixel_stride, pixel_stride, row_stride);
  Fl_RGB_Image * ni =   new Fl_RGB_Image(array, w2, h2, pixel_stride);
  ni->alloc_array = 1;
  return ni;

}
*/
