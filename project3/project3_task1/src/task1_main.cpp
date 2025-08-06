/*****************************************************************************/
// File: task1_main.cpp
// Author: Chris
// Last Revised: 2025-08-05
/*****************************************************************************/

#include "io_bmp.h"
#include "image_comps.h"
#include "motion.h"
#include <algorithm>
#include <string>

#define MAX_KEYPOINTS 10000
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
              int start_row, int start_col, int block_width, int block_height, int S)
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
  for (vec.y=-S; vec.y <= S; vec.y++) // Most pixel motion is within the range of 8 units(pixles)
    for (vec.x=-S; vec.x <= S; vec.x++)
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

/* ========================================================================= */
/*                              Global Functions                             */
/* ========================================================================= */

/*****************************************************************************/
/*                                    main                                   */
/*****************************************************************************/

int
  main(int argc, char *argv[])
{
  if (argc != 6)
    {
      fprintf(stderr,
              "Usage: %s <bmp frame 1> <bmp frame 2> <block size> <search range> <keypoints spacing>\n", 
              argv[0]);
      return -1;
    }

  uint8_t H = std::stoi(argv[3]);
  uint8_t S = std::stoi(argv[4]);
  int delta = std::stoi(argv[5]); // keypoints spacing
  
  int block_width = 2 * H + 1;
  int block_height = 2 * H + 1;

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
      io_byte *line = new io_byte[width]; 
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

      // Step 1: Find defined keypoints and motion vectors
      mvector motion_vectors[MAX_KEYPOINTS];
      int count = 0;

      int start_y = H + S;
      int start_x = H + S;
      int end_y = height - H - S;
      int end_x = width - H - S;

      for (int y = start_y; y <= end_y; y += delta) {
          for (int x = start_x; x <= end_x; x += delta) {

              mvector vec = find_motion(&mono[0], &mono[1],
                  y - H, x - H,
                  block_width, block_height, S);
              motion_vectors[count++] = vec;
          }
      }

      // Step 2: Get global motion vector
      double sum_x = 0, sum_y = 0;
      for (int i = 0; i < count; i++)
      {
          sum_x += motion_vectors[i].x;
          sum_y += motion_vectors[i].y;
      }

      mvector global_vec;
      global_vec.x = static_cast<int>(round(sum_x / count));
      global_vec.y = static_cast<int>(round(sum_y / count));

      printf("Global motion vector = (%d, %d)\n", global_vec.x, global_vec.y);

      //// Step 3: Do motion compensation
      //motion_comp(&mono[0], &output, global_vec, 0, 0, width, height);

      //// Compute `MSE` between tgt_frame and compensated then print the value out
      //double mse = 0.0;
      //int total_pixels = height * width;
      //double total_err = 0;

      //for (r = 0; r < height; r++) {
      //    int* ref_line = mono[1].buf + r * mono[1].stride;
      //    int* comp_line = output.buf + r * output.stride;

      //    for (c = 0; c < width; c++) {
      //        int diff = ref_line[c] - comp_line[c];
      //        total_err += diff * diff;
      //    }
      //}

      //mse = static_cast<double>(total_err / total_pixels);
      //printf("MSE between target frame and compensated frame is: %.2f\n", mse);


      // Write the motion compensated image out
      //bmp_out out;
      //if ((err_code = bmp_out__open(&out,argv[3],width,height,1)) != 0)
      //  throw err_code;
      //for (r=height-1; r >= 0; r--)
      //  { // "r" holds the true row index we are writing, since the image is
      //    // written upside down in BMP files.
      //    io_byte *dst = line; // Points to first sample of component n
      //    int* src = output.buf + r * width; 
      //    for (int c = 0; c < width; c++, dst++)
      //        *dst = static_cast<io_byte>(std::clamp(src[c], 0, 255));
      //    bmp_out__put_line(&out,line);
      //  }
      //bmp_out__close(&out);
      delete[] line;
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