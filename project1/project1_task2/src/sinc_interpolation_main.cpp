/*****************************************************************************/
// File: vertical_filtering_main.cpp
// Author: David Taubman
// Last Revised: 13 August, 2007
/*****************************************************************************/
// Copyright 2007, David Taubman, The University of New South Wales (UNSW)
/*****************************************************************************/

#include "io_bmp.h"
#include "aligned_image_comps.h"
#include <iostream>
#include <chrono>
#include <algorithm> // std::clamp
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

  //// begin timer
  //auto start_time = std::chrono::high_resolution_clock::now();
  int err_code=0;
  try {
      // Read the input image
      bmp_in in;
      if ((err_code = bmp_in__open(&in,argv[1])) != 0) // 8 bit per pixel: grey image, 24 bit per pixel: RGB image
        throw err_code;

      int width = in.cols, height = in.rows;
      int n, num_comps = in.num_components;
      my_aligned_image_comp *input_comps =
        new my_aligned_image_comp[num_comps];
      for (n=0; n < num_comps; n++)
        input_comps[n].init(height,width,4); // Leave a border of 4
      
      int r; // Declare row index
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
              for (int c=0; c < width; c++, src+=num_comps)
                dst[c] = static_cast<float>(*src); // The cast to type "float" is not
                      // strictly required here, since bytes can always be
                      // converted to floats without any loss of information.
            }
        }
      bmp_in__close(&in);

      // boudary extension for input
      for (n = 0; n < num_comps; n++) {
          input_comps[n].perform_boundary_extension();
      }

      // Allocate storage for the filtered output
      my_aligned_image_comp *output_comps = new my_aligned_image_comp;
      output_comps->init(height * 3, width * 3, 0); // only need one component for grey image output
                                                    // scaling by 3, Don't need a border for output
    
      // Process the image, all in floating point (easy)
      if (num_comps == 1) {
          output_comps->bilinear_interpolation(input_comps); // grey image input
      }
      else if (num_comps == 3) {
          output_comps->bilinear_interpolation(input_comps + 1); // rgb image input
      }

      io_byte* output_line = new io_byte[width * 3];
      // Write the image back out again
      bmp_out out;
      if ((err_code = bmp_out__open(&out, argv[2], width * 3, height * 3, 1)) != 0) 
        throw err_code;
      for (r=height * 3 -1; r >= 0; r--)
        { // "r" holds the true row index we are writing, since the image is
          // written upside down in BMP files.
            io_byte *dst = output_line; 
            float *src = output_comps->buf + r * output_comps->stride;
            for (int c = 0; c < width * 3; c++, dst++) {
                *dst = static_cast<io_byte>(std::clamp(src[c] + 0.5F, 0.0F, 255.0F)); // The cast to type "io_byte" is
                // required here, since floats cannot generally be
                // converted to bytes without loss of information.  The
                // compiler will warn you of this if you remove the cast.
                // There is in fact not the best way to do the
                // conversion.  You should fix it up in the lab. -- done
            }
          bmp_out__put_line(&out,output_line);
        }
      bmp_out__close(&out);
      delete[] line;
      delete[] input_comps;
      delete output_comps;
      delete[] output_line;
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
  //// end timer
  //auto end_time = std::chrono::high_resolution_clock::now();
  //auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
  //std::cout << "Processing time is: " << duration << " ms" << std::endl;

  return 0;
}
