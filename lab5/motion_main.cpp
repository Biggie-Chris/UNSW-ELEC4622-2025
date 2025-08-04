/*****************************************************************************/
// File: motion_main.cpp
// Author: David Taubman
// Last Revised: 30 September, 2007
/*****************************************************************************/
// Copyright 2007, David Taubman, The University of New South Wales (UNSW)
/*****************************************************************************/

#include "io_bmp.h"
#include "image_comps.h"
#include "motion.h"
#include <algorithm>

/* ========================================================================= */
/*                 Implementation of `my_image_comp' functions               */
/* ========================================================================= */

/*****************************************************************************/
/*                  my_image_comp::perform_boundary_extension                */
/*****************************************************************************/

void my_image_comp::perform_boundary_extension()
{
  int r, c;

  // First extend upwards
  int *first_line = buf;
  for (r=1; r <= border; r++)
    for (c=0; c < width; c++)
      first_line[-r*stride+c] = first_line[c];

  // Now extend downwards
  int *last_line = buf+(height-1)*stride;
  for (r=1; r <= border; r++)
    for (c=0; c < width; c++)
      last_line[r*stride+c] = last_line[c];

  // Now extend all rows to the left and to the right
  int *left_edge = buf-border*stride;
  int *right_edge = left_edge + width - 1;
  for (r=height+2*border; r > 0; r--, left_edge+=stride, right_edge+=stride)
    for (c=1; c <= border; c++)
      {
        left_edge[-c] = left_edge[0];
        right_edge[c] = right_edge[0];
      }
}

/*****************************************************************************/
/* STATIC                         find_motion                                */
/*****************************************************************************/

static mvector
  find_motion(my_image_comp *ref, my_image_comp *tgt,
              int start_row, int start_col, int block_width, int block_height)
  /* This function finds the motion vector which best describes the motion
     between the `ref' and `tgt' frames, over a specified block in the
     `tgt' frame.  Specifically, the block in the `tgt' frame commences
     at the coordinates given by `start_row' and `start_col' and extends
     over `block_width' columns and `block_height' rows.  The function finds
     the translational offset (the returned vector) which describes the
     best matching block of the same size in the `ref' frame, where
     the "best match" is interpreted as the one which minimizes the sum of
     absolute differences (SAD) metric. */
{
  mvector vec, best_vec;
  int sad, best_sad=256*block_width*block_height;
  for (vec.y=-8; vec.y <= 8; vec.y++) // Most pixel motion is within the range of 8 units(pixles)
    for (vec.x=-8; vec.x <= 8; vec.x++)
      {
        int ref_row = start_row-vec.y;
        int ref_col = start_col-vec.x;
        if ((ref_row < 0) || (ref_col < 0) ||
            ((ref_row+block_height) > ref->height) ||
            ((ref_col+block_width) > ref->width))
          continue; // Translated block not containe within reference frame
        int r, c;
        int *rp = ref->buf + ref_row*ref->stride + ref_col;
        int *tp = tgt->buf + start_row*tgt->stride + start_col;
        for (sad=0, r=block_height; r > 0; r--,
             rp+=ref->stride, tp+=tgt->stride)
          for (c=0; c < block_width; c++)
            {
              int diff = tp[c] - rp[c];
              sad += (diff < 0)?(-diff):diff;
            }
        if (sad < best_sad)
          {
            best_sad = sad;
            best_vec = vec;
          }
      }

  return best_vec;
}

/*****************************************************************************/
/* STATIC                         motion_comp                                */
/*****************************************************************************/

static void
  motion_comp(my_image_comp *ref, my_image_comp *tgt, mvector vec,
              int start_row, int start_col, int block_width, int block_height)
  /* This function transfers data from the `ref' frame to a block within the
     `tgt' frame, thereby realizing motion compensation.  The motion in
     question has already been found by `find_motion' and is captured by
     the `vec' argument.  The block in the `tgt' frame commences
     at the coordinates given by `start_row' and `start_col' and extends
     for `block_width' columns and `block_height' rows. */
{
  int r, c;
  int ref_row = start_row - vec.y;
  int ref_col = start_col - vec.x;
  int *rp = ref->buf + ref_row*ref->stride + ref_col;
  int *tp = tgt->buf + start_row*tgt->stride + start_col;
  for (r=block_height; r > 0; r--,
       rp+=ref->stride, tp+=tgt->stride)
    for (c=0; c < block_width; c++)
      tp[c] = rp[c];
}

/*****************************************************************************/
/* STATIC                         draw_motion_line                           */
/*****************************************************************************/
static void 
draw_motion_line(float* rgb_buf, int stride, int width, int height,
    int cx, int cy, int vx, int vy) 
{
    int x0 = cx, y0 = cy;
    int x1 = cx + vx, y1 = cy + vy;

    if (abs(vy) > abs(vx)) {
        // Vertical scan (y is the main axis)
        for (int y = std::min(y0, y1); y <= std::max(y0, y1); ++y) {
            float t = float(y - y0) / float(y1 - y0);
            int x = x0 + int(t * (x1 - x0) + 0.5f);
            if (x >= 0 && x < width && y >= 0 && y < height)
                rgb_buf[y * stride + 3 * x + 1] = 0; // set green to 0
        }
    }
    else {
        // Horizontal scan (x is the main axis)
        for (int x = std::min(x0, x1); x <= std::max(x0, x1); ++x) {
            float t = float(x - x0) / float(x1 - x0);
            int y = y0 + int(t * (y1 - y0) + 0.5f);
            if (x >= 0 && x < width && y >= 0 && y < height)
                rgb_buf[y * stride + 3 * x + 1] = 0; // set green to 0
        }
    }
}


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
      fprintf(stderr,
              "Usage: %s <bmp frame 1> <bmp frame 2> <bmp MC out>\n",
              argv[0]);
      return -1;
    }

  int err_code=0;
  try {
      // Read the input image
      bmp_in in[2];
      if ((err_code = bmp_in__open(&in[0],argv[1])) != 0)
        throw err_code;
      if ((err_code = bmp_in__open(&in[1],argv[2])) != 0)
        throw err_code;

      int width = in[0].cols, height = in[0].rows;
      if ((width != in[1].cols) || (height != in[1].rows))
        {
          fprintf(stderr,"The two input frames have different dimensions.\n");
          return -1;
        }
      my_image_comp mono[2];
      mono[0].init(height,width,4); // Leave a border of 4 (in case needed)
      mono[1].init(height,width,4); // Leave a border of 4 (in case needed)
      
      int n, r, c;
      int num_comps = in[0].num_components;
      io_byte *line = new io_byte[width*3]; // RGB output
      for (n=0; n < 2; n++)
        {
          for (r=height-1; r >= 0; r--)
            { // "r" holds the true row index we are reading, since the image
              // is stored upside down in the BMP file.
              if ((err_code = bmp_in__get_line(&(in[n]),line)) != 0)
                throw err_code;
              io_byte *src = line; // Points to first sample of component n
              int *dst = mono[n].buf + r * mono[n].stride;
              for (c=0; c < width; c++, src+=num_comps)
                dst[c] = *src;
            }
          bmp_in__close(&(in[n]));
        }

      // Allocate storage for the motion compensated output
      my_image_comp output;
      output.init(height,width,0); // Don't need a border for output
      // Create a rgb_buffer prepared for rgb vector drawing
      float* rgb_buf = new float[height * width * 3];

      /* Set background color */
      for (r = 0; r < height; r++) {
          for (c = 0; c < width; c++) {
              int idx = (r * width + c) * 3;
              int stride = mono[1].stride;
              float val = mono[1].buf[r * stride + c];
              val = val / 2.0f + 128;

              rgb_buf[idx + 0] = val;
              rgb_buf[idx + 1] = val;
              rgb_buf[idx + 2] = val;
          }
      }

      // Now perform simple motion estimation and compensation
      int nominal_block_width = 32;
      int nominal_block_height = 32;
      int block_width, block_height;
      for (r=0; r < height; r+=block_height)
        {
          block_height = nominal_block_height;
          if ((r+block_height) > height)
            block_height = height-r;
          for (c=0; c < width; c+=block_width)
            {
              block_width = nominal_block_width;
              if ((c+block_width) > width)
                block_width = width-c;
              // Step 1: Motion Estimation
              mvector vec = find_motion(&(mono[0]),&(mono[1]),
                                        r,c,block_width,block_height);
              // Step 2: Motion Compensation
              motion_comp(&(mono[0]),&output,vec,
                          r,c,block_width,block_height);
              // Step 3: draw motion vector line on rgb_buf
              int cx = c + block_width / 2;
              int cy = r + block_height / 2;
              draw_motion_line(rgb_buf, width * 3, width, height,
                  cx, cy, vec.x, vec.y);
            }
        }

      // Compute `MSE` between tgt_frame and compensated then print the value out
      double mse = 0.0;
      int total_pixels = height * width;
      double total_err = 0;

      for (r = 0; r < height; r++) {
          int* ref_line = mono[1].buf + r * mono[1].stride;
          int* comp_line = output.buf + r * output.stride;

          for (c = 0; c < width; c++) {
              int diff = ref_line[c] - comp_line[c];
              total_err += diff * diff;
          }
      }

      mse = static_cast<double>(total_err / total_pixels);
      printf("MSE between target frame and compensated frame is: %.2f\n", mse);


      // Write the motion compensated image out
      bmp_out out;
      if ((err_code = bmp_out__open(&out,argv[3],width,height,3)) != 0)
        throw err_code;
      for (r=height-1; r >= 0; r--)
        { // "r" holds the true row index we are writing, since the image is
          // written upside down in BMP files.
          io_byte *dst = line; // Points to first sample of component n
          float* src = rgb_buf + r * width * 3; 
          for (int c = 0; c < width * 3; c++, dst++)
              *dst = static_cast<io_byte>(std::clamp(src[c] + 0.5f, 0.0f, 255.0f));  
          bmp_out__put_line(&out,line);
        }
      bmp_out__close(&out);
      delete[] line;
      delete[] rgb_buf;
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