/*****************************************************************************/
// File: aligned_image_comps.h
// Author: David Taubman
// Last Revised: 13 August, 2007
/*****************************************************************************/
// Copyright 2007, David Taubman, The University of New South Wales (UNSW)
/*****************************************************************************/

#include <assert.h>
#include <string>

/*****************************************************************************/
/* STRUCT                     my_aligned_image_comp                          */
/*****************************************************************************/

struct my_aligned_image_comp {
    // Data members: (these occupy space in the structure's block of memory)
    int width;
    int height;
    int stride;
    int border; // Extra rows/cols to leave around the boundary
    float *handle; // Points to start of allocated memory buffer
    float *buf; // Points to the first real image sample
    // Function members: (these do not occupy any space in memory)
    my_aligned_image_comp()
      { width = height = stride = border = 0;  handle = buf = NULL; }
    ~my_aligned_image_comp()
      { if (handle != NULL) delete[] handle; }
    void init(int height, int width, int border)
      {
        this->width = width;  this->height = height;  this->border = border;
        stride = width + 2*border;
        stride = (stride+3) & ~3; // Make stride a multiple of 4 (16 bytes)
        if (handle != NULL)
          delete[] handle; // Delete mem allocated by any previous `init' call
        handle = new float[stride*(height+2*border)+3]; // Allow for alignment
        buf = handle + (border*stride) + border;
        int existing_alignment = (int)(((__int64) buf) & 15);
        if (existing_alignment != 0)
          { // Address is not a multiple of 16 bytes already
            int byte_offset = 16 - existing_alignment;
            int sample_offset = byte_offset / sizeof(float);
            assert(byte_offset == (sample_offset * sizeof(float)));
            buf += sample_offset;
          }
      }
    void perform_boundary_extension();
       // This function is implemented in "filtering_main.cpp".
    void filter(my_aligned_image_comp *in);
       /* Direct implementation of vertical filtering, mapping `in' to the
          current image component.  This function is implemented in
          "filtering_main.cpp". */
    void vector_filter(my_aligned_image_comp *in);
       /* Vector implementation of vertical filtering, using X86 processor
          intrinsics.  This function is implemented in "vector_filter.cpp". */
    void bilinear_interpolation(my_aligned_image_comp* in);
       /* Using bi-linear interpolation to fill the gaps(missing pixels) 
          after expansion. This function is implemented in aligned_image_comps.cpp. */
    void sinc_interpolation(my_aligned_image_comp* in, int H); // H means the windowed sinc extent
        /* for project1 task2. */
    float* differentiation(my_aligned_image_comp* in, float g, std::string mode); // g means output gain
        /* for project1 task3 and task4. */
    float* derivative_gaussian(my_aligned_image_comp* in, float s, std::string mode); // s means sigma in Gaussin FILTER
        /* for project1 taks6. */
  };
  /* Notes:
       This class is the same as `my_image_comp' from the "filtering_example"
       project, except that it ensures that the first sample of every image
       row has a 16-byte aligned address.  This also means that we can access
       a whole number of 16-byte chunks within each line without crashing
       into the next line, regardless of the original image dimensions.  These
       properties are important for fast vector processing. */