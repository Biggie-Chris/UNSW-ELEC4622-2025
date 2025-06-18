/*****************************************************************************/
// File: filtering_main.cpp
// Author: David Taubman
// Last Revised: 13 August, 2007
/*****************************************************************************/
// Copyright 2007, David Taubman, The University of New South Wales (UNSW)
/*****************************************************************************/
#include <string>
#include <iostream>
#include <algorithm> // std::clamp(<v>, <lo>, <hi>)
#include "io_bmp.h"
#include "image_comps.h"

/* ========================================================================= */
/*                 Implementation of `my_image_comp' functions               */
/* ========================================================================= */

/*****************************************************************************/
/*                  my_image_comp::perform_boundary_extension                */
/*****************************************************************************/

void my_image_comp::perform_boundary_extension(BoundaryExtensionType type)
{
    int r, c;

    switch (type) {
    case BoundaryExtensionType::zero_padding: {
        // First extend upwards
        float* first_line = buf;
        for (r = 1; r <= border; r++)
            for (c = 0; c < width; c++)
                first_line[-r * stride + c] = 0.0F;

        // Now extend downwards
        float* last_line = buf + (height - 1) * stride;
        for (r = 1; r <= border; r++)
            for (c = 0; c < width; c++)
                last_line[r * stride + c] = 0.0F;

        // Now extend all rows to the left and to the right
        float* left_edge = buf - border * stride;
        float* right_edge = left_edge + width - 1;
        for (r = height + 2 * border; r > 0; r--, left_edge += stride, right_edge += stride)
            for (c = 1; c <= border; c++)
            {
                left_edge[-c] = 0.0F;
                right_edge[c] = 0.0F;
            }
        break;
    }
    case BoundaryExtensionType::zero_order_hold: {
        // First extend upwards
        float* first_line = buf;
        for (r = 1; r <= border; r++)
            for (c = 0; c < width; c++)
                first_line[-r * stride + c] = first_line[c];

        // Now extend downwards
        float* last_line = buf + (height - 1) * stride;
        for (r = 1; r <= border; r++)
            for (c = 0; c < width; c++)
                last_line[r * stride + c] = last_line[c];

        // Now extend all rows to the left and to the right
        float* left_edge = buf - border * stride;
        float* right_edge = left_edge + width - 1;
        for (r = height + 2 * border; r > 0; r--, left_edge += stride, right_edge += stride)
            for (c = 1; c <= border; c++)
            {
                left_edge[-c] = left_edge[0];
                right_edge[c] = right_edge[0];
            }
        break;
    }
    case BoundaryExtensionType::symmetric_extension: {
        
        // First extend upwards
        float* first_line = buf;
        for (r = 1; r <= border; r++) {
            float* src_line = buf + r * stride;
            for (c = 0; c < width; c++) {
                first_line[-r * stride + c] = src_line[c];
            }
        }
        
        // Then extend downwards
        float* last_line = buf + (height - 1) * stride;
        for (r = 1; r <= border; r++) {
            float* src_line = last_line - r * stride;
            for (c = 0; c < width; c++) {
                last_line[r * stride + c] = src_line[c];
            }
        }
        
        // Now extend all rows to the left and to the right
        float* left_edge = buf - border * stride;
        float* right_edge = left_edge + width - 1;
        for (r = height + 2 * border; r > 0; r--, left_edge += stride, right_edge += stride) {
            for (c = 1; c <= border; c++) {
                left_edge[-c] = left_edge[c - 1];
                right_edge[c] = right_edge[-(c - 1)];
            }
        }
        break;
    }
    };
    
}


/* ========================================================================= */
/*                              Global Functions                             */
/* ========================================================================= */

/*****************************************************************************/
/*                                apply_filter                               */
/*****************************************************************************/
enum class FilterType {
    mean_avg,
    h1, // unsharp masking filter
    h2,
    h3
};

float alpha; // for unsharp mask filter only
void apply_filter(my_image_comp* in, my_image_comp* out, FilterType type)
{
#define FILTER_EXTENT 4
#define FILTER_DIM (2*FILTER_EXTENT+1) // kernel diameter 9
#define FILTER_TAPS (FILTER_DIM*FILTER_DIM) // kernel elements 9 * 9 = 81

    // Create the filter kernel as a local array on the stack, which can accept
    // row and column indices in the range -FILTER_EXTENT to +FILTER_EXTENT.
    float filter_buf[FILTER_TAPS]; // use 1D arry to mimic 2D arry, this is much faster
    float* mirror_psf = filter_buf + (FILTER_DIM * FILTER_EXTENT) + FILTER_EXTENT;
    // `mirror_psf' points to the central tap in the filter
    
    // initialize filters
    int r, c;
    switch (type) {
    case FilterType::mean_avg:
        for (r = -FILTER_EXTENT; r <= FILTER_EXTENT; r++)
            for (c = -FILTER_EXTENT; c <= FILTER_EXTENT; c++)
                mirror_psf[r * FILTER_DIM + c] = 1.0F / FILTER_TAPS; // F means float(4 bytes)
        break;
    case FilterType::h1: {
        // Step 1: initialze to 0
        for (r = -FILTER_EXTENT; r <= FILTER_EXTENT; r++)
            for (c = -FILTER_EXTENT; c <= FILTER_EXTENT; c++)
                mirror_psf[r * FILTER_DIM + c] = 0.0F;

        // Step 2: create a temp h1
        float h1_5x5[5][5] = {
            {0.0F, 1.0F / 3, 1.0F / 2, 1.0F / 3, 0.0F},
            {1.0F / 3, 1.0F / 2, 1.0F, 0.5F, 1.0F / 3},
            {1.0F / 2, 1.0F, 1.0F, 1.0F, 1.0F / 2},
            {1.0F / 3, 1.0F / 2, 1.0F, 1.0F / 2, 1.0F / 3},
            {0.0F, 1.0F / 3, 1.0F / 2, 1.0F / 3, 0.0F}
        };

        // Step 3: normalize h1
        float sum = 0.0F;
        for (int i = 0; i < 5; ++i)
            for (int j = 0; j < 5; ++j)
                sum += h1_5x5[i][j];
        float A = 1.0F / sum;

        // Step 4: make the center of delta[n]
        mirror_psf[0] = 1.0F + alpha;  // mirror_psf[0] is the center (0,0)

        // Step 5: implement: -α * h₁[n] 
        int row_offset = -2;
        int col_offset = -2;
        for (int i = 0; i < 5; ++i)
            for (int j = 0; j < 5; ++j) {
                int r9 = row_offset + i;
                int c9 = col_offset + j;
                mirror_psf[r9 * FILTER_DIM + c9] -= alpha * (A * h1_5x5[i][j]);
            }
        break;
    }
    case FilterType::h2: {
        // first set all elements to 0
        for (r = -FILTER_EXTENT; r <= FILTER_EXTENT; r++)
            for (c = -FILTER_EXTENT; c <= FILTER_EXTENT; c++)
                mirror_psf[r * FILTER_DIM + c] = 0.0F;
        // apply the none zero elements
        filter_buf[0 * FILTER_DIM + 0] = 0.25F;
        filter_buf[0 * FILTER_DIM + 1] = 0.5F;
        filter_buf[0 * FILTER_DIM + 2] = 0.5F;
        filter_buf[0 * FILTER_DIM + 3] = 0.25F;

        filter_buf[1 * FILTER_DIM + 0] = 0.5F;
        filter_buf[1 * FILTER_DIM + 1] = 1.0F;
        filter_buf[1 * FILTER_DIM + 2] = 1.0F;
        filter_buf[1 * FILTER_DIM + 3] = 0.5F;

        filter_buf[2 * FILTER_DIM + 0] = 0.5F;
        filter_buf[2 * FILTER_DIM + 1] = 1.0F;
        filter_buf[2 * FILTER_DIM + 2] = 1.0F;
        filter_buf[2 * FILTER_DIM + 3] = 0.5F;

        filter_buf[3 * FILTER_DIM + 0] = 0.25F;
        filter_buf[3 * FILTER_DIM + 1] = 0.5F;
        filter_buf[3 * FILTER_DIM + 2] = 0.5F;
        filter_buf[3 * FILTER_DIM + 3] = 0.25F;

        // normalnization
        float sum = 0.0F;
        for (int i = 0; i < FILTER_TAPS; i++) {
            sum += filter_buf[i];
        }
        assert(sum > 0.0F && "Filter kernel sum must be positive!");
        float B = 1.0F / sum;

        for (int i = 0; i < FILTER_TAPS; i++) {
            filter_buf[i] *= B;
        }
        break;
    }       
    case FilterType::h3: {
        // first set all elements to 0
        for (r = -FILTER_EXTENT; r <= FILTER_EXTENT; r++)
            for (c = -FILTER_EXTENT; c <= FILTER_EXTENT; c++)
                mirror_psf[r * FILTER_DIM + c] = 0.0F;
        // apply the none zero elements
        filter_buf[5 * FILTER_DIM + 5] = 0.25F;
        filter_buf[5 * FILTER_DIM + 6] = 0.5F;
        filter_buf[5 * FILTER_DIM + 7] = 0.5F;
        filter_buf[5 * FILTER_DIM + 8] = 0.25F;

        filter_buf[6 * FILTER_DIM + 5] = 0.5F;
        filter_buf[6 * FILTER_DIM + 6] = 1.0F;
        filter_buf[6 * FILTER_DIM + 7] = 1.0F;
        filter_buf[6 * FILTER_DIM + 8] = 0.5F;

        filter_buf[7 * FILTER_DIM + 5] = 0.5F;
        filter_buf[7 * FILTER_DIM + 6] = 1.0F;
        filter_buf[7 * FILTER_DIM + 7] = 1.0F;
        filter_buf[7 * FILTER_DIM + 8] = 0.5F;

        filter_buf[8 * FILTER_DIM + 5] = 0.25F;
        filter_buf[8 * FILTER_DIM + 6] = 0.5F;
        filter_buf[8 * FILTER_DIM + 7] = 0.5F;
        filter_buf[8 * FILTER_DIM + 8] = 0.25F;

        // normalnization
        float sum = 0.0F;
        for (int i = 0; i < FILTER_TAPS; i++) {
            sum += filter_buf[i];
        }
        assert(sum > 0.0F && "Filter kernel sum must be positive!");
        float B = 1.0F / sum;

        for (int i = 0; i < FILTER_TAPS; i++) {
            filter_buf[i] *= B;
        }
        break;
    }
    };

    // Check for consistent dimensions
    assert(in->border >= FILTER_EXTENT);
    assert((out->height <= in->height) && (out->width <= in->width));

    // Perform the convolution
    for (r = 0; r < out->height; r++)
        for (c = 0; c < out->width; c++)
        {
            float* ip = in->buf + r * in->stride + c;
            float* op = out->buf + r * out->stride + c;
            float sum = 0.0F;
            for (int y = -FILTER_EXTENT; y <= FILTER_EXTENT; y++)
                for (int x = -FILTER_EXTENT; x <= FILTER_EXTENT; x++) {
                    sum += ip[y * in->stride + x] * mirror_psf[y * FILTER_DIM + x];
                }
            *op = std::clamp(sum, 0.0F, 255.0F);
        }
}

/*****************************************************************************/
/*                                    main                                   */
/*****************************************************************************/

int
main(int argc, char* argv[])
{
    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s <in bmp file> <out bmp file> <alpha>\n", argv[0]);
        return -1;
    }
    // alpha for h1 filter
    if (argc == 4) {
        try {
            alpha = std::stof(argv[3]);
        }
        catch (std::exception& e) {
            std::cout << "Invalid alpha value: " << e.what() << std::endl;
        }
    }

    int err_code = 0;
    try {
        // Read the input image
        bmp_in in;
        if ((err_code = bmp_in__open(&in, argv[1])) != 0)
            throw err_code;

        int width = in.cols, height = in.rows;
        int n, num_comps = in.num_components;
        my_image_comp* input_comps = new my_image_comp[num_comps];
        for (n = 0; n < num_comps; n++)
            input_comps[n].init(height, width, 4); // Leave a border of 4

        int r; // Declare row index
        io_byte* line = new io_byte[width * num_comps];
        for (r = height - 1; r >= 0; r--)
        { // "r" holds the true row index we are reading, since the image is
          // stored upside down in the BMP file.
            if ((err_code = bmp_in__get_line(&in, line)) != 0)
                throw err_code;
            for (n = 0; n < num_comps; n++)
            {
                io_byte* src = line + n; // Points to first sample of component n
                float* dst = input_comps[n].buf + r * input_comps[n].stride;
                for (int c = 0; c < width; c++, src += num_comps)
                    dst[c] = (float)*src; // The cast to type "float" is not
                // strictly required here, since bytes can always be
                // converted to floats without any loss of information.
            }
        }
        bmp_in__close(&in);

        // Allocate storage for the filtered output
        my_image_comp* output_comps = new my_image_comp[num_comps];
        for (n = 0; n < num_comps; n++)
            output_comps[n].init(height, width, 0); // Don't need a border for output

        // Process the image, all in floating point (easy)
        for (n = 0; n < num_comps; n++)
            input_comps[n].perform_boundary_extension(BoundaryExtensionType::symmetric_extension);
        for (n = 0; n < num_comps; n++)
            apply_filter(input_comps + n, output_comps + n, FilterType::h1);

        // Write the image back out again
        bmp_out out;
        if ((err_code = bmp_out__open(&out, argv[2], width, height, num_comps)) != 0)
            throw err_code;
        for (r = height - 1; r >= 0; r--)
        { // "r" holds the true row index we are writing, since the image is
          // written upside down in BMP files.
            for (n = 0; n < num_comps; n++)
            {
                io_byte* dst = line + n; // Points to first sample of component n
                float* src = output_comps[n].buf + r * output_comps[n].stride;
                for (int c = 0; c < width; c++, dst += num_comps)
                    //*dst = (io_byte)src[c];
                    *dst = static_cast<io_byte>(src[c] + 0.5F); // src[i] + 0.5F : do rouding first
                // The cast to type "io_byte" is
                // required here, since floats cannot generally be
                // converted to bytes without loss of information.  The
                // compiler will warn you of this if you remove the cast.
                // There is in fact not the best way to do the
                // conversion.  You should fix it up in the lab.
            }
            bmp_out__put_line(&out, line);
        }
        bmp_out__close(&out);
        delete[] line;
        delete[] input_comps;
        delete[] output_comps;
    }
    catch (int exc) {
        if (exc == IO_ERR_NO_FILE)
            fprintf(stderr, "Cannot open supplied input or output file.\n");
        else if (exc == IO_ERR_FILE_HEADER)
            fprintf(stderr, "Error encountered while parsing BMP file header.\n");
        else if (exc == IO_ERR_UNSUPPORTED)
            fprintf(stderr, "Input uses an unsupported BMP file format.\n  Current "
                "simple example supports only 8-bit and 24-bit data.\n");
        else if (exc == IO_ERR_FILE_TRUNC)
            fprintf(stderr, "Input or output file truncated unexpectedly.\n");
        else if (exc == IO_ERR_FILE_NOT_OPEN)
            fprintf(stderr, "Trying to access a file which is not open!(?)\n");
        return -1;
    }
    return 0;
}

/*
to do:
    1. fix the write_back process(clipping)---ok
    2. fix io_byte type casting---ok
    3. add boundary extenison methods (usually 3 ways to do it) ---ok
*/