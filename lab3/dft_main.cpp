/*****************************************************************************/
// File: dft_main.cpp
// Author: David Taubman
// Last Revised: 28 August, 2007
/*****************************************************************************/
// Copyright 2007, David Taubman, The University of New South Wales (UNSW)
/*****************************************************************************/

#include <math.h>
#include "io_bmp.h"
#include "image_comps.h"
#include "dft.h"


/* ========================================================================= */
/*                              Global Functions                             */
/* ========================================================================= */

/*****************************************************************************/
/*                                    main                                   */
/*****************************************************************************/

int
  main(int argc, char *argv[])
{
  if (argc != 3)
    {
      fprintf(stderr,"Usage: %s <in bmp file> <out bmp file>\n",argv[0]);
      return -1;
    }

  int err_code=0;
  try {
      // Read the input image
      bmp_in in;
      if ((err_code = bmp_in__open(&in,argv[1])) != 0)
        throw err_code;

      int width = in.cols, height = in.rows;
      int n, num_comps = in.num_components;
      my_image_comp *input_comps = new my_image_comp[num_comps];
      for (n=0; n < num_comps; n++)
        input_comps[n].init(height,width,0); // No boundary extension required
      
      int r, c; // Declare row index
      io_byte *line = new io_byte[width*num_comps];
      for (r=height-1; r >= 0; r--)
        { // "r" holds the true row index we are reading, since the image is
          // stored upside down in the BMP file.
          if ((err_code = bmp_in__get_line(&in,line)) != 0)
            throw err_code;
          for (n=0; n < num_comps; n++)
            {
              io_byte *src = line+n; // Points to first sample of component n
              float *dst = input_comps[n].buf + r * input_comps[n].stride;
              for (c=0; c < width; c++, src+=num_comps)
                dst[c] = (float) *src; // The cast to type "float" is not
                      // strictly required here, since bytes can always be
                      // converted to floats without any loss of information.
            }
        }
      bmp_in__close(&in);

      // Allocate storage for the output image
      my_image_comp *output_comps = new my_image_comp[num_comps];
      for (n=0; n < num_comps; n++)
        output_comps[n].init(height,width,0); // No extension required

      // Allocate storage for DFT buffers
      int max_dim = height;
      if (max_dim < width)
        max_dim = width;
      float *dft_real = new float[height*width];
      float *dft_imag = new float[height*width];

      // Process the image, plane by plane.
      for (n=0; n < num_comps; n++)
        {
          // First copy all samples to the `dft_real' buffer
          int stride = input_comps[n].stride;
          for (r=0; r < height; r++)
            for (c=0; c < width; c++)
              {
                dft_real[r*width+c] = input_comps[n].buf[r*stride+c];
                dft_imag[r*width+c] = 0.0F;
              }

          // Next, perform the 2D DFT

               // Put your code here


          // Write DFT magnitudes to the output image, possibly using a log
          // function to see the values more clearly.

               // Put your code here


          // Normalize the output image so that the maximum value is 255
          // and clip to avoid negative values.
          float max_val = 0.0F;
          for (r=0; r < height; r++)
            for (c=0; c < width; c++)
              {
                float val = output_comps[n].buf[r*stride+c];
                if (val > max_val)
                  max_val = val;
              }
          float scale = 1.0F;
          if (max_val > 0.0F)
            scale = 255.0F / max_val;
          for (r=0; r < height; r++)
            for (c=0; c < width; c++)
              output_comps[n].buf[r*stride+c] *= scale;
        }

      // Write the image back out again
      bmp_out out;
      if ((err_code = bmp_out__open(&out,argv[2],width,height,num_comps)) != 0)
        throw err_code;
      for (r=height-1; r >= 0; r--)
        { // "r" holds the true row index we are writing, since the image is
          // written upside down in BMP files.
          for (n=0; n < num_comps; n++)
            {
              io_byte *dst = line+n; // Points to first sample of component n
              float *src = output_comps[n].buf + r * output_comps[n].stride;
              for (c=0; c < width; c++, dst+=num_comps)
                *dst = (io_byte) src[c];
            }
          bmp_out__put_line(&out,line);
        }
      bmp_out__close(&out);
      delete[] line;
      delete[] input_comps;
      delete[] output_comps;
      delete[] dft_real;
      delete[] dft_imag;
    }
  catch (int exc) {
      if (exc == IO_ERR_NO_FILE)
        fprintf(stderr,"Cannot open supplied input or output file.\n");
      else if (exc == IO_ERR_FILE_HEADER)
        fprintf(stderr,"Error encountered while parsing BMP file header.\n");
      else if (exc == IO_ERR_UNSUPPORTED)
        fprintf(stderr,"Input uses an unsupported BMP file format.\n  Current "
                "simple example supports only 8-bit and 24-bit data.\n");
      else if (exc == IO_ERR_FILE_TRUNC)
        fprintf(stderr,"Input or output file truncated unexpectedly.\n");
      else if (exc == IO_ERR_FILE_NOT_OPEN)
        fprintf(stderr,"Trying to access a file which is not open!(?)\n");
      return -1;
    }
  return 0;
}
