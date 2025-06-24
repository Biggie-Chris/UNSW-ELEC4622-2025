#include "aligned_image_comps.h"
#include <iostream>
#include <emmintrin.h>
/* ========================================================================= */
/*                 Implementation of `my_image_comp' functions               */
/* ========================================================================= */

/*****************************************************************************/
/*              my_aligned_image_comp::perform_boundary_extension            */
/*****************************************************************************/

void my_aligned_image_comp::perform_boundary_extension()
{
    int r, c;

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
}

/*****************************************************************************/
/*                        my_aligned_image_comp::filter                      */
/*****************************************************************************/

void my_aligned_image_comp::filter(my_aligned_image_comp* in)
{
#define FILTER_EXTENT 4
#define FILTER_TAPS (2*FILTER_EXTENT+1)

    // Create the vertical filter PSF as a local array on the stack.
    float filter_buf[FILTER_TAPS];
    float* mirror_psf = filter_buf + FILTER_EXTENT;
    // `mirror_psf' points to the central tap in the filter
    for (int t = -FILTER_EXTENT; t <= FILTER_EXTENT; t++)
        mirror_psf[t] = 1.0F / FILTER_TAPS;

    // Check for consistent dimensions
    assert(in->border >= FILTER_EXTENT);
    assert((this->height <= in->height) && (this->width <= in->width));

    // store intermediate results
    float* line_buffer = new float[width];

    // Perform the vertical convolution
    for (int r = 0; r < height; r++) {
        for (int c = 0; c < width; c++) {
            float* ip = in->buf + r * in->stride + c;
            float sum = 0.0F;
            for (int y = -FILTER_EXTENT; y <= FILTER_EXTENT; y++) {
                sum += ip[y * in->stride] * mirror_psf[y];
            }
            line_buffer[c] = sum;
        }
        // write back buffer data to image
        for (int c = 0; c < width; c++) {
            buf[r * stride + c] = line_buffer[c];
        }
    }

    // Perform the horizontal convolution
    for (int r = 0; r < height; r++) {
        for (int c = 0; c < width; c++) {
            float* ip = in->buf + r * in->stride + c;
            float* op = buf + r * stride + c;
            float sum = 0.0F;
            for (int x = -FILTER_EXTENT; x <= FILTER_EXTENT; x++) {
                sum += ip[x] * mirror_psf[x];
            }
            *op = sum;
        }
    }
}

void my_aligned_image_comp::vector_filter(my_aligned_image_comp* in)
{
#define FILTER_EXTENT 4
#define FILTER_TAPS (2*FILTER_EXTENT+1)

    // Create the vertical filter PSF as a local array on the stack.
    __m128 filter_buf[FILTER_TAPS];
    __m128* mirror_psf = filter_buf + FILTER_EXTENT;
    // `mirror_psf' points to the central tap in the filter
    for (int t = -FILTER_EXTENT; t <= FILTER_EXTENT; t++)
        mirror_psf[t] = _mm_set1_ps(1.0F / FILTER_TAPS);

    // Check for consistent dimensions
    assert(in->border >= FILTER_EXTENT);
    assert((this->height <= in->height) && (this->width <= in->width));
    assert(((stride & 3) == 0) && ((in->stride & 3) == 0));
    int vec_stride_in = in->stride / 4;
    int vec_stride_out = this->stride / 4;
    int vec_width_out = (this->width + 3) / 4; // Big enough to cover the width

    // Do the filtering
    __m128* line_out = (__m128*) buf;
    __m128* line_in = (__m128*)(in->buf);
    for (int r = 0; r < height; r++,
        line_out += vec_stride_out, line_in += vec_stride_in)
        for (int c = 0; c < vec_width_out; c++)
        {
            __m128* ip = (line_in + c) - vec_stride_in * FILTER_EXTENT;
            __m128 sum = _mm_setzero_ps();
            for (int y = -FILTER_EXTENT; y <= FILTER_EXTENT; y++, ip += vec_stride_in)
                sum = _mm_add_ps(sum, _mm_mul_ps(mirror_psf[y], *ip));
            line_out[c] = sum;
        }
}

void my_aligned_image_comp::bilinear_interpolation(my_aligned_image_comp* in) {
    // define scaling factor
    const int scale = 3;

    float* ip = in->buf;
    float* op = buf;

    int input_stride = in->stride;
    int output_stride = stride;
    int output_height = in->height * 3;
    int output_width = in->width * 3;

    for (int y = 0; y < output_height; y++) {
        float input_y = static_cast<float>(y) / scale; // scale promoted to float implicitly
        int n2 = static_cast<int>(input_y); // vertical index
        float sigma_2 = input_y - n2;

        for (int x = 0; x < output_width; x++) {
            float input_x = static_cast<float>(x) / scale;
            int n1 = static_cast<int>(input_x); // horizontal index
            float sigma_1 = input_x - n1;

            // find 4 nearby pixel values
            float top_left = ip[n2 * input_stride + n1];
            float top_right = ip[n2 * input_stride + (n1 + 1)];
            float bottom_left = ip[(n2 + 1) * input_stride + n1];
            float bottom_right = ip[(n2 + 1) * input_stride + (n1 + 1)];

            // do bilinear calculation
            float interpolated =
                (1 - sigma_2) * ((1 - sigma_1) * top_left + sigma_1 * top_right) +
                sigma_2 * ((1 - sigma_1) * bottom_left + sigma_1 * bottom_right);

            op[y * output_stride + x] = interpolated;
            
        }
    }
    std::cout << "bilinear done\n";
}