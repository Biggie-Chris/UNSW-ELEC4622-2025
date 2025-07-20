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
#include <string>

/* ========================================================================= */
/*                              Global Functions                             */
/* ========================================================================= */

/*****************************************************************************/
/*                                    main                                   */
/*****************************************************************************/

int
  main(int argc, char *argv[])
{
  if (argc != 4)
    {
      fprintf(stderr,"Usage: %s <in bmp file> <out bmp file> <L: block size>\n",argv[0]);
      return -1;
    }
  
  int L = std::stoi(argv[3]);

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
        output_comps[n].init(L,L,0); // No extension required
      
      // Allocate storage for DFT buffers (L * L)
      int max_dim = height;
      if (max_dim < width)
        max_dim = width;

      int blocks_row = height / L;
      int blocks_col = width  / L;
      int K = blocks_row * blocks_col;
      if (K == 0) { 
          fprintf(stderr, "Block size L is larger than image size\n"); 
          return -1;
      }

      float *dft_real = new float[L * L];
      float *dft_imag = new float[L * L];
      float *avg_pow = new float[L * L];

      my_direct_dft row_dft, col_dft;
      row_dft.init(L, true);
      col_dft.init(L, true); // initialization done outside the loop

      // Process the image, plane by plane.
      for (n = 0; n < num_comps; n++)
      {
          // First copy all samples to the `dft_real' buffer
          int stride = input_comps[n].stride;

          // Reset avg_pow[] to 0
          for (int r = 0; r < L; r++) {
              for (int c = 0; c < L; c++) {
                  avg_pow[r * L + c] = 0.0F;
              }
          }

          ////////////////////////////////////////////
          /// Process each block (Bartlett Method) ///
          ////////////////////////////////////////////
          for (int br = 0; br < blocks_row; br++) {
              for (int bc = 0; bc < blocks_col; bc++) {
                  // 1. Copy all samples to dft_real buffer
                  for (int r = 0; r < L; r++) {
                      for (int c = 0; c < L; c++) {
                          int src_r = br * L + r;
                          int src_c = bc * L + c;
                          dft_real[r * L + c] = input_comps[n].buf[src_r * stride + src_c];
                          dft_imag[r * L + c] = 0.0F;
                      }
                  }

                  // 2. Perform row and col Fourier Transform
                  for (int r = 0; r < L; r++) {
                      row_dft.perform_transform(dft_real + r * L, dft_imag + r * L, 1);
                  }

                  for (int c = 0; c < L; c++) {
                      col_dft.perform_transform(dft_real + c, dft_imag + c, L);
                  }

                  // 3. Accumulate the Periodogram for each block
                  for (int r = 0; r < L; r++) {
                      for (int c = 0; c < L; c++) {
                          float re = dft_real[r * L + c];
                          float im = dft_imag[r * L + c];

                          avg_pow[r * L + c] += re * re + im * im;
                      }
                  }
              }
          }

          // Do average and log representation and write back to output image
          for (int i = 0; i < L * L; i++) {
              avg_pow[i] = avg_pow[i] / static_cast<float>(K);
              avg_pow[i] = logf(avg_pow[i] + 1.0e-12F); // in case divided by 0
              output_comps[n].buf[i] = avg_pow[i];
          }

          // Normalize the output image so that the maximum value is 255
          // and clip to avoid negative values.
          float max_val = 0.0F;

          // 1. Find the max value
          for (int r = 0; r < L; r++) {
              for (int c = 0; c < L; c++) {
                  float val = output_comps[n].buf[r * L + c];
                  if (val > max_val) {
                      max_val = val;
                  }
              }
          }

          // 2. Calculate Scale factor
          float scale = 1.0F;
          if (max_val > 0.0F) {
              scale = 255.0F / max_val;
          }

          // 3. Normalize each pixel and write back to output
          for (int r = 0; r < L; r++) {
              for (int c = 0; c < L; c++) {
                  float val = output_comps[n].buf[r * L + c];
                  val *= scale;
                  if (val < 0.0F)      val = 0.0F;
                  else if (val > 255.0F) val = 255.0F;
                  output_comps[n].buf[r * L + c] = val;
              }
          }
      }
          

      // Write the image back out again
      bmp_out out;
      if ((err_code = bmp_out__open(&out,argv[2],L,L,num_comps)) != 0)
        throw err_code;
      for (r=L-1; r >= 0; r--)
        { // "r" holds the true row index we are writing, since the image is
          // written upside down in BMP files.
          for (n=0; n < num_comps; n++)
            {
              io_byte *dst = line+n; // Points to first sample of component n
              float *src = output_comps[n].buf + r * output_comps[n].stride;
              for (c=0; c < L; c++, dst+=num_comps)
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
      delete[] avg_pow;
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
