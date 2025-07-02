#include "aligned_image_comps.h"
#include <iostream>
#include <emmintrin.h>
#include <cmath>
#include <algorithm>
/* ========================================================================= */
/*                 Implementation of `my_image_comp' functions               */
/* ========================================================================= */

/*****************************************************************************/
/*              my_aligned_image_comp::perform_boundary_extension            */
/*****************************************************************************/
constexpr float pi = 3.1415926F;

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
    const int FILTER_EXTENT = 4;
    const int FILTER_TAPS = (2 * FILTER_EXTENT + 1);

    // Create the vertical filter PSF as a local array on the stack.
    float filter_buf[FILTER_TAPS];
    float* mirror_psf = filter_buf + FILTER_EXTENT; // `mirror_psf' points to the central tap in the filter
    // initialize the kernel
    for (int t = -FILTER_EXTENT; t <= FILTER_EXTENT; t++)
        mirror_psf[t] = 1.0F / FILTER_TAPS;

    // Check for consistent dimensions
    assert(in->border >= FILTER_EXTENT);
    assert((this->height <= in->height) && (this->width <= in->width));

    // Allocate line buffers
    float* line_buffer = new float[width];        // Store vertical result for 1 row
    float* line_buffer2 = new float[width];       // Store horizontal result

    // Combined vertical + horizontal convolution row-by-row
    for (int r = 0; r < height; ++r) {
        // 1. Perform vertical convolution for this row
        for (int c = 0; c < width; ++c) {
            float sum = 0.0F;
            for (int y = -FILTER_EXTENT; y <= FILTER_EXTENT; ++y) {
                float* ip = in->buf + (r + y) * in->stride + c;
                sum += (*ip) * mirror_psf[y];
            }
            line_buffer[c] = sum;  // store vertically filtered value
        }

        // 2. Perform horizontal convolution on this line
        for (int c = 0; c < width; ++c) {
            float sum = 0.0F;
            for (int x = -FILTER_EXTENT; x <= FILTER_EXTENT; ++x) {
                sum += line_buffer[c + x] * mirror_psf[x];
            }
            line_buffer2[c] = sum;  // store horizontally filtered value
        }

        // 3. Write final result to output image
        for (int c = 0; c < width; ++c) {
            buf[r * stride + c] = line_buffer2[c];
        }
    }
    delete[] line_buffer;
    delete[] line_buffer2;
}

void my_aligned_image_comp::vector_filter(my_aligned_image_comp* in)
{
    const int FILTER_EXTENT = 4;
    const int FILTER_TAPS = (2 * FILTER_EXTENT + 1);

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

/*****************************************************************************/
/*                  my_aligned_image_comp::bilinear_interpolation            */
/*****************************************************************************/
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
        int y0 = y;
        if (y < output_height - 2) { y = output_height - 2; }
        float input_y = static_cast<float>(y) / scale; // scale promoted to float implicitly
        int n2 = static_cast<int>(input_y); // vertical index
        float sigma_2 = input_y - n2;

        for (int x = 0; x < output_width; x++) {
            int x0 = x;
            if (x < output_width - 2) { x = output_width - 2; }
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
            x = x0;
        }
        y = y0;
    }
    std::cout << "bilinear interpolation done\n";
}


/*****************************************************************************/
/*                  my_aligned_image_comp::sinc_interpolation                */
/*****************************************************************************/
// define hann_sinc function and then use it for every pixel
inline float hann_sinc(float x, int H) {
    float sinc = (x == 0.0F) ? 1.0F : std::sinf(pi * x) / (pi * x);
    float hann = 0.5F * (1 + cosf((pi * x) / H));
    return sinc * hann;
}

// definition of sinc interpolation
// using raised cosine
void my_aligned_image_comp::sinc_interpolation(my_aligned_image_comp* in, int H) {
    // define scaling factor
    const int scale = 3;

    const int FILTER_EXTENT = H;
    const int FILTER_TAPS = 2 * FILTER_EXTENT + 1;
    float* ip = in->buf;
    float* op = buf;
    int input_stride = in->stride;
    int output_stride = stride;
    int output_height = in->height * scale;
    int output_width = in->width * scale;

    // Check for consistent dimensions
    assert(in->border >= FILTER_EXTENT);

    // store intermediate results
    float* line_buffer = new float[output_width];
    float* line_buffer2 = new float[output_width];

    // 1. Perform the vertical convolution
    // (u , v) represents the inverse mapped coordinates from the scaled image
    // (u0, v0) represents the interger value coordinates in the original image
    for (int y_out = 0; y_out < output_height; y_out++) {
        float v = static_cast<float>(y_out) / scale;
        int v0 = static_cast<int>(v);
        float frac_v = v - v0;

        for (int x_out = 0; x_out < output_width; x_out++) {
            float u = static_cast<float>(x_out) / scale;
            int u0 = static_cast<int>(u);
            float sum = 0.0F;
            float wsum = 0.0F; // For normalization
            for (int y = -FILTER_EXTENT; y <= FILTER_EXTENT; y++) {
                float w = hann_sinc(y - frac_v, H);
                sum += w * ip[(v0 + y) * input_stride + u0];
                wsum += w;
            }
            line_buffer[x_out] = sum / wsum; // Normalization is done here
        }

        // write back data to image
        for (int x_out = 0; x_out < output_width; x_out++) {
            op[y_out * output_stride + x_out] = line_buffer[x_out];
        }
    }

    // 2. Perform the horizontal convolution
    for (int y_out = 0; y_out < output_height; y_out++) {
        float v = static_cast<float>(y_out) / scale;
        int   v0 = static_cast<int>(v);
        float frac_v = v - v0;

        for (int x_out = 0; x_out < output_width; x_out++) {
            float u = static_cast<float>(x_out) / scale;
            int   u0 = static_cast<int>(u);
            float frac_u = u - u0;

            float sum = 0.0F, wsum = 0.0F;
            for (int x = -FILTER_EXTENT; x <= FILTER_EXTENT; x++) {
                float w = hann_sinc(x - frac_u, H);

                /* === 改动①：从垂直结果 op[] 读取，而非 ip[] === */
                //int phase = x_out % scale;                  // 当前列在 3 个子像素中的相位 (0/1/2)
                //int sx = (u0 + x) * scale + phase;       // 保持同相位
                int sx = (u0 + x) * scale;
                /* === 改动②：简单 clamp，防止 sx 越界 === */
                if (sx < 0) { sx = 0; }
                else if (sx >= output_width) { sx = output_width - 1; }
                sum += w * op[y_out * output_stride + sx];
                wsum += w;

            }
            line_buffer2[x_out] = sum / wsum;
        }
        // write back value
        for (int x_out = 0; x_out < output_width; x_out++) {
            op[y_out * output_stride + x_out] = line_buffer2[x_out];
        }
    }

    delete[] line_buffer;
    delete[] line_buffer2;
    std::cout << "sinc interpolation done: 3x, H = " << H << "\n";
}

/*****************************************************************************/
/*                  my_aligned_image_comp::differentiation                   */
/*****************************************************************************/
float* my_aligned_image_comp::differentiation(my_aligned_image_comp* in, float g, std::string mode) {
    const int FILTER_EXTENT = 1;
    const int FILTER_TAPS = (2 * FILTER_EXTENT + 1);

    // Create the vertical filter PSF as a local array on the stack.
    float filter_buf[FILTER_TAPS] = { -0.5F, 0.0F, 0.5F };
    float* mirror_psf = filter_buf + FILTER_EXTENT; // `mirror_psf' points to the central tap in the filter

    // Check for consistent dimensions
    assert(in->border >= FILTER_EXTENT);
    //assert((this->height <= in->height) && (this->width <= in->width));

    // Allocate magnitute buffers and rgb buffer    
    float* rgb_buffer = new float[height * width * 3];
    float* magnitude = new float[height * width];

    std::cout << "begin filtering...\n";
    // Combined vertical + horizontal convolution row-by-row
    for (int r = 0; r < height; ++r) {
        // processing each pixel
        for (int c = 0; c < width; ++c) {
            float sum_fx = 0.0F;
            float sum_fy = 0.0F;

            // index helper
            int base = (r * width + c) * 3;

            // 1. vertical filtering
            for (int y = -FILTER_EXTENT; y <= FILTER_EXTENT; ++y) {
                float* ip = in->buf + (r + y) * in->stride + c;
                sum_fy += (*ip) * mirror_psf[y];
            }

            // 2. horizontal filtering
            for (int x = -FILTER_EXTENT; x <= FILTER_EXTENT; ++x) {
                float* ip = in->buf + r * in->stride + (c + x);
                sum_fx += (*ip) * mirror_psf[x];
            }

            // 3. calculate magnitute and angle terms
            float m = sqrtf(sum_fy * sum_fy + sum_fx * sum_fx);
            float theta = atan2(sum_fy, sum_fx);

            // 4. calculat hue
            float hue;
            if (theta >= 0) { hue = 3.0F / pi * theta; }
            else { hue = 3 / pi * (theta + 2 * pi); }

            // 5. convert to RGB
            float C = std::min(float(255), g * m);
            float X = C * (1 - std::fabsf(fmodf(hue, 2.0F) - 1));

            float R, G, B;
            if (hue >= 0 && hue < 1) { R = C; G = X; B = 0.0F; }
            else if (hue >= 1 && hue < 2) { R = X; G = C; B = 0.0F; }
            else if (hue >= 2 && hue < 3) { R = 0.0F; G = C; B = X; }
            else if (hue >= 3 && hue < 4) { R = 0.0F; G = X; B = C; }
            else if (hue >= 4 && hue < 5) { R = X; G = 0.0F; B = C; }
            else { R = C; G = 0.0F; B = X; }

            // BMP file stores RGB values in B->G->R order
            rgb_buffer[base + 0] = B;
            rgb_buffer[base + 1] = G;
            rgb_buffer[base + 2] = R;

            magnitude[r * width + c] = m;
        }
    }

    if (mode == "on") {
        std::cout << "differentiation done: g = " << g << "\n";
        delete[] magnitude;
        return rgb_buffer;
    }
    else if (mode == "off") {
        for (int r = 1; r < height - 1; ++r) {
            for (int c = 1; c < width - 1; ++c) {
                int idx = r * width + c;
                float m = magnitude[idx];
                float m_up = magnitude[(r - 1) * width + c];
                float m_down = magnitude[(r + 1) * width + c];
                float m_left = magnitude[r * width + (c - 1)];
                float m_right = magnitude[r * width + (c + 1)];

                if (!(m > m_up && m > m_down && m > m_left && m > m_right)) {
                    int base = idx * 3;
                    rgb_buffer[base + 0] = 0.0F;
                    rgb_buffer[base + 1] = 0.0F;
                    rgb_buffer[base + 2] = 0.0F;
                }
            }
        }
        std::cout << "differentiation done: g = " << g << "\n";
        delete[] magnitude;
        return rgb_buffer;
    }
    else {
        std::cout << "Invalid mode type\n";
        return nullptr;
    }
}