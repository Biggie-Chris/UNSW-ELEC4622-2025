/* File: fft_main.cpp
*  Author: Chris
*  Date: 2025-07-23
*/
#include "io_bmp.h"
#include "aligned_image_comps.h"
#include "dft.h"
#include <iostream>
#include <chrono>
#include <algorithm>  // std::clamp
#include <string>     // stoi, stof
#include <cmath>      // cosf, logf               

/*****************************************************************************/
/*                                Tool functions                             */
/*****************************************************************************/
constexpr float pi = 3.1415926F;
inline float hann_window(int x, int N)
{
    return 0.5F * (1.0F - cosf(2.0F * pi * x / (N - 1)));
}

static void fftshift(my_aligned_image_comp& dst_comp, const float* src, int N)
{
    const int half = N >> 1;
    float* dst_buf = dst_comp.buf;
    const int stride = dst_comp.stride;

    for (int r = 0; r < N; ++r)
    {
        int rr = (r + half) & (N - 1);
        for (int c = 0; c < N; ++c)
        {
            int cc = (c + half) & (N - 1);
            dst_buf[r * stride + c] = src[rr * N + cc];
        }
    }
}

/*****************************************************************************/
/*                                    main                                   */
/*****************************************************************************/
int main(int argc, char* argv[])
{
    if (argc != 7)
    {
        fprintf(stderr,
            "Usage: %s <in bmp file> <out bmp file> <N> <p1> <p2> <alpha>\n",
            argv[0]);
        return -1;
    }

    int  N = std::stoi(argv[3]);
    if ((N & 1) == 1)
    {
        fprintf(stderr, "N must be even number\n");
        return -1;
    }

    float alpha = std::stof(argv[6]);
    if (alpha <= 0.0F)
    {
        fprintf(stderr, "alpha must be positive\n");
        return -1;
    }

    int err_code = 0;
    try
    {

        bmp_in in;
        if ((err_code = bmp_in__open(&in, argv[1])) != 0)
            throw err_code;

        const int width = in.cols;
        const int height = in.rows;
        const int num_comps = in.num_components;

        my_aligned_image_comp* input_comps = new my_aligned_image_comp[num_comps];
        for (int n = 0; n < num_comps; ++n)
            input_comps[n].init(height, width, N);  // leaver a border of N

        io_byte* line = new io_byte[width * num_comps];
        for (int r = height - 1; r >= 0; --r)
        {                                          // BMP row order is bottom up
            if ((err_code = bmp_in__get_line(&in, line)) != 0)
                throw err_code;
            for (int n = 0; n < num_comps; ++n)
            {
                io_byte* src = line + n;
                float* dst = input_comps[n].buf + r * input_comps[n].stride;
                for (int c = 0; c < width; ++c, src += num_comps)
                    dst[c] = static_cast<float>(*src);
            }
        }
        bmp_in__close(&in);

        /*------------------------------------------------------------------*/
        /*                Calculate the coordinates of `p`                  */
        /*------------------------------------------------------------------*/
        const int p1 = static_cast<int>(std::stof(argv[4]));
        const int p2 = static_cast<int>(std::stof(argv[5]));
        if (p1 < 0 || p2 < 0 || p1 >= width || p2 >= height)
        {
            fprintf(stderr, "P should be within the input image range\n");
            return -1;
        }

        /*------------------------------------------------------------------*/
        /*                Boudary Extension                                 */
        /*------------------------------------------------------------------*/
        for (int n = 0; n < num_comps; ++n)
            input_comps[n].perform_boundary_extension();  // symmetric extension

        /*------------------------------------------------------------------*/
        /*                Extract N¡ÁN and apply Hann-window                 */
        /*------------------------------------------------------------------*/
        my_aligned_image_comp* output_comp = new my_aligned_image_comp;
        output_comp->init(N, N, 0);  // only need one 

        float sum = 0.0F;
        for (int r = 0; r < N; ++r)
        {
            for (int c = 0; c < N; ++c)
            {
                float val = input_comps[0].buf[(p2 + r) * input_comps[0].stride + (p1 + c)];
                output_comp->buf[r * output_comp->stride + c] = val;
                sum += val;
            }
        }

        const float mean = sum / (N * N);

        for (int r = 0; r < N; ++r)
        {
            float w_r = hann_window(r, N);
            for (int c = 0; c < N; ++c)
            {
                float w_c = hann_window(c, N);
                float& val =
                    output_comp->buf[r * output_comp->stride + c];
                val = (val - mean) * w_r * w_c;
            }
        }

        /*------------------------------------------------------------------*/
        /*                2-D DFT                                            */
        /*------------------------------------------------------------------*/
        float* dft_real = new float[N * N];
        float* dft_imag = new float[N * N];

        /* 1. Copy imagedata to dft_real£¬dft_imag set to 0 */
        for (int r = 0; r < N; ++r)
        {
            const float* src_row = output_comp->buf + r * output_comp->stride;
            float* re_row = dft_real + r * N;
            float* im_row = dft_imag + r * N;
            for (int c = 0; c < N; ++c)
            {
                re_row[c] = src_row[c];
                im_row[c] = 0.0F;
            }
        }

        // begin timer
        auto start_time = std::chrono::high_resolution_clock::now();

        /* 2. Row DFT */
        my_direct_dft row_dft;
        row_dft.init(N, true);
        for (int r = 0; r < N; ++r)
            row_dft.perform_fft(dft_real + r * N, dft_imag + r * N, 1);

        /* 3. Col DFT */
        my_direct_dft col_dft;
        col_dft.init(N, true);
        for (int c = 0; c < N; ++c)
            col_dft.perform_fft(dft_real + c, dft_imag + c, N);

        // end timer
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
        std::cout << "Processing time is: " << duration << " ms" << std::endl;

        /* 4. Calculate Periodogram£¬and apply fftshift */
        float* temp_buf = new float[N * N];
        for (int i = 0; i < N * N; ++i)
        {
            float re = dft_real[i];
            float im = dft_imag[i];
            temp_buf[i] = (re * re + im * im) / (N * N);  // periodogram
        }

        fftshift(*output_comp, temp_buf, N);

        for (int i = 0; i < N * N; ++i)
            output_comp->buf[i] = logf(1.0F + output_comp->buf[i]);


        /*------------------------------------------------------------------*/
        /*                write_out bmp                                     */
        /*------------------------------------------------------------------*/
        io_byte* output_line = new io_byte[N];
        bmp_out  out;
        if ((err_code = bmp_out__open(&out, argv[2], N, N, 1)) != 0)
            throw err_code;

        for (int r = N - 1; r >= 0; --r)
        {  // BMP row is bottom up
            io_byte* dst = output_line;
            float* src = output_comp->buf + r * output_comp->stride;
            for (int c = 0; c < N; c++, dst++) {
                src[c] = src[c] * alpha;
                *dst = static_cast<io_byte>(std::clamp(src[c] + 0.5F, 0.0F, 255.0F));
            }
            bmp_out__put_line(&out, output_line);
        }
        bmp_out__close(&out);

        /*------------------------------------------------------------------*/
        /*                        clean up                                  */
        /*------------------------------------------------------------------*/
        delete[] line;
        delete[] input_comps;
        delete output_comp;
        delete[] dft_real;
        delete[] dft_imag;
        delete[] temp_buf;
        delete[] output_line;
    }
    catch (int exc)
    {
        if (exc == IO_ERR_NO_FILE)
            fprintf(stderr, "Cannot open supplied input or output file.\n");
        else if (exc == IO_ERR_FILE_HEADER)
            fprintf(stderr, "Error encountered while parsing BMP file header.\n");
        else if (exc == IO_ERR_UNSUPPORTED)
            fprintf(stderr,
                "Input uses an unsupported BMP file format.\n  Current "
                "simple example supports only 8-bit and 24-bit data.\n");
        else if (exc == IO_ERR_FILE_TRUNC)
            fprintf(stderr, "Input or output file truncated unexpectedly.\n");
        else if (exc == IO_ERR_FILE_NOT_OPEN)
            fprintf(stderr, "Trying to access a file which is not open!(?)\n");
        return -1;
    }

    return 0;
}
