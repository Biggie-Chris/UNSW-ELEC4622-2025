/* File: texture_fft_task6.cpp
 * Author: Chris 
 * Date:   2025-07-23
 */
#include "io_bmp.h"
#include "aligned_image_comps.h"
#include "dft.h"

#include <algorithm>   // std::clamp
#include <chrono>
#include <cmath>       // cosf, logf
#include <iostream>
#include <string>      // stoi, stof

 /*****************************************************************************/
 /*                                Tool functions                             */
 /*****************************************************************************/
constexpr float pi = 3.1415926F;

inline float hann_window(int x, int N)
{
    return 0.5F * (1.0F - cosf(2.0F * pi * x / (N - 1)));
}

// -------------------------------- fftshift ---------------------------------
static void fftshift(my_aligned_image_comp& dst_comp,
    const float* src, int N)
{
    const int half = N >> 1;
    float* dst_buf = dst_comp.buf;
    const int stride = dst_comp.stride;

    for (int r = 0; r < N; ++r)
    {
        int rr = (r + half) % N;         
        for (int c = 0; c < N; ++c)
        {
            int cc = (c + half) % N;      
            dst_buf[r * stride + c] = src[rr * N + cc];
        }
    }
}

/*****************************************************************************/
/*                                    main                                   */
/*****************************************************************************/
int main(int argc, char* argv[])
{
    if (argc != 5)
    {
        fprintf(stderr,
            "Usage: %s <in bmp file> <out bmp file> <N> <alpha>\n",
            argv[0]);
        return -1;
    }
    /*---------------- Parsing command line parameters --------------------*/
    const int   N = std::stoi(argv[3]);
    const float alpha = std::stof(argv[4]);

    if ((N & 1) != 0) { fprintf(stderr, "N must be even\n");   return -1; }
    if (alpha <= 0.0F) { fprintf(stderr, "alpha > 0 required\n");return -1; }

    int err_code = 0;
    try
    {
        /*---------------------------- Read BMP ----------------------------*/
        bmp_in in;
        if ((err_code = bmp_in__open(&in, argv[1])) != 0) throw err_code;

        const int width = in.cols;
        const int height = in.rows;
        const int num_comps = in.num_components;     

       
        auto* input_comps = new my_aligned_image_comp[num_comps];
        for (int n = 0; n < num_comps; ++n)
            input_comps[n].init(height, width, N);

        io_byte* line = new io_byte[width * num_comps];
        for (int r = height - 1; r >= 0; --r)         
        {
            if ((err_code = bmp_in__get_line(&in, line)) != 0) throw err_code;
            for (int n = 0; n < num_comps; ++n)
            {
                io_byte* src = line + n;
                float* dst = input_comps[n].buf + r * input_comps[n].stride;
                for (int c = 0; c < width; ++c, src += num_comps)
                    dst[c] = static_cast<float>(*src);
            }
        }
        bmp_in__close(&in);

        // Symmetric extension
        for (int n = 0; n < num_comps; ++n)
            input_comps[n].perform_boundary_extension();

        /*---------------------------Output buffer ---------------------------*/
        my_aligned_image_comp out_comps[3];
        for (auto& comp : out_comps)
            comp.init(height, width, 0);

        /*------------------------ Temp buffer --------------------------*/
        my_aligned_image_comp patch_comp;
        patch_comp.init(N, N, 0);

        auto* dft_real = new float[N * N];
        auto* dft_imag = new float[N * N];
        auto* tmp_buf = new float[N * N];

       // fft initialization
        my_direct_dft row_dft;  row_dft.init(N, true);
        my_direct_dft col_dft;  col_dft.init(N, true);

        /*======================   Main Process Stage   =====================*/
        for (int rowP = 0; rowP < height; ++rowP)
        {
            for (int colP = 0; colP < width; ++colP)
            {
                //-------------------- (a) copy patch + Hann -----------------
                float sum = 0.f;
                for (int r = 0; r < N; ++r)
                {
                    for (int c = 0; c < N; ++c)
                    {
                        float val = input_comps[0].buf[(rowP + r) *
                            input_comps[0].stride + (colP + c)];
                        patch_comp.buf[r * patch_comp.stride + c] = val;
                        sum += val;
                    }
                }
                const float mean = sum / (N * N);
                for (int r = 0; r < N; ++r)
                {
                    float wr = hann_window(r, N);
                    for (int c = 0; c < N; ++c)
                    {
                        float wc = hann_window(c, N);
                        float& v = patch_comp.buf[r * patch_comp.stride + c];
                        v = (v - mean) * wr * wc;
                    }
                }

                //----------------------- (b) 2-D DFT -------------------------
                for (int r = 0; r < N; ++r)            // Copy to dft_real
                {
                    const float* src_row = patch_comp.buf + r * patch_comp.stride;
                    float* re_row = dft_real + r * N;
                    float* im_row = dft_imag + r * N;
                    for (int c = 0; c < N; ++c) { re_row[c] = src_row[c]; im_row[c] = 0.f; }
                }
                for (int r = 0; r < N; ++r) row_dft.perform_fft(dft_real + r * N,
                    dft_imag + r * N, 1);
                for (int c = 0; c < N; ++c) col_dft.perform_fft(dft_real + c,
                    dft_imag + c, N);

                //------------------- (c) Periodogram + shift -----------------
                for (int i = 0; i < N * N; ++i)
                {
                    float re = dft_real[i], im = dft_imag[i];
                    tmp_buf[i] = (re * re + im * im) / (N * N);   // ¦£_p[k]
                }
                fftshift(patch_comp, tmp_buf, N);                // ¦£_p[k] -> patch_comp
                for (int i = 0; i < N * N; ++i)
                    patch_comp.buf[i] = logf(1.f + patch_comp.buf[i]);

                //------------------- (d) Average 3 rings ---------------------------
                float sumR = 0.f, sumG = 0.f, sumB = 0.f;
                int   cntR = 0, cntG = 0, cntB = 0;

                const int halfN = N >> 1;
                const int N2 = N >> 1, N4 = N >> 2, N8 = N >> 3, N16 = N >> 4;

                for (int r = 0; r < N; ++r)
                {
                    int k1 = r - halfN, ak1 = std::abs(k1);
                    float* row = patch_comp.buf + r * patch_comp.stride;
                    for (int c = 0; c < N; ++c)
                    {
                        int k2 = c - halfN, ak2 = std::abs(k2);
                        float v = alpha * row[c];          // ¦Á¡¤¦£_p[k]

                        if (ak1 > N8 && ak1 <= N4 && ak2 > N8 && ak2 <= N4)
                        {
                            sumR += v; ++cntR;
                        }
                        else if (ak1 > N4 && ak1 <= N2 && ak2 > N4 && ak2 <= N2)
                        {
                            sumG += v; ++cntG;
                        }
                        else if (ak1 > N16 && ak1 <= N8 && ak2 > N16 && ak2 <= N8)
                        {
                            sumB += v; ++cntB;
                        }
                    }
                }
                float R = cntR ? sumR / cntR : 0.f;
                float G = cntG ? sumG / cntG : 0.f;
                float B = cntB ? sumB / cntB : 0.f;

                //---------------------- (e) Wirte to ouput buffer -----------------------
                out_comps[0].buf[rowP * out_comps[0].stride + colP] = R;
                out_comps[1].buf[rowP * out_comps[1].stride + colP] = G;
                out_comps[2].buf[rowP * out_comps[2].stride + colP] = B;
            }
        }

        /*--------------------------- Write to BMP file--------------------------*/
        io_byte* out_line = new io_byte[width * 3];
        bmp_out  out;
        if ((err_code = bmp_out__open(&out, argv[2], width, height, 3)) != 0)
            throw err_code;

        for (int r = height - 1; r >= 0; --r)
        {
            io_byte* dst = out_line;
            float* srcR = out_comps[0].buf + r * out_comps[0].stride;
            float* srcG = out_comps[1].buf + r * out_comps[1].stride;
            float* srcB = out_comps[2].buf + r * out_comps[2].stride;

            for (int c = 0; c < width; ++c)
            {
                
                *dst++ = static_cast<io_byte>(std::clamp(srcB[c] + 0.5f, 0.f, 255.f)); // B
                *dst++ = static_cast<io_byte>(std::clamp(srcG[c] + 0.5f, 0.f, 255.f)); // G
                *dst++ = static_cast<io_byte>(std::clamp(srcR[c] + 0.5f, 0.f, 255.f)); // R
            }
            bmp_out__put_line(&out, out_line);
        }
        bmp_out__close(&out);

        /*-----------------------------Clean up ----------------------------*/
        delete[] line;         delete[] out_line;
        delete[] input_comps;  delete[] dft_real;
        delete[] dft_imag;     delete[] tmp_buf;
    }
    catch (int exc)
    {
        if (exc == IO_ERR_NO_FILE)      fprintf(stderr, "File open error.\n");
        else if (exc == IO_ERR_FILE_HEADER)  fprintf(stderr, "Bad BMP header.\n");
        else if (exc == IO_ERR_UNSUPPORTED)  fprintf(stderr, "Unsupported BMP.\n");
        else if (exc == IO_ERR_FILE_TRUNC)   fprintf(stderr, "File truncated.\n");
        else if (exc == IO_ERR_FILE_NOT_OPEN)fprintf(stderr, "File not open.\n");
        return -1;
    }
    return 0;
}
