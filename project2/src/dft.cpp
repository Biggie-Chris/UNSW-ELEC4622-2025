/*****************************************************************************/
// File: dft.cpp
// Author: David Taubman
// Last Revised: 28 August, 2007
/*****************************************************************************/
// Copyright 2007, David Taubman, The University of New South Wales (UNSW)
/*****************************************************************************/

#define _USE_MATH_DEFINES  // Makes `M_PI' available as a constant.

#include <stdlib.h>
#include <math.h>
#include "dft.h"
#include <stdexcept>
/*****************************************************************************/
/*                            my_direct_dft::init                            */
/*****************************************************************************/

void my_direct_dft::init(int N, bool is_forward)
{
    cleanup(); // Delete any pre-existing buffers.
    this->N = N;
    real_buf = new float[N];
    imag_buf = new float[N];
    real_trig = new double[N]; // Use `double` to load trigonometric tables
    imag_trig = new double[N];
    for (int n = 0; n < N; n++)
    {
        real_trig[n] = cos(n * 2.0 * M_PI / N);
        imag_trig[n] = sin(n * 2.0 * M_PI / N);
        if (is_forward)
            imag_trig[n] = -imag_trig[n]; // If it's forward trasnform, `sin(x)` should be 
    }
}

/*****************************************************************************/
/*                   my_direct_dft::perform_transform                        */
/*****************************************************************************/

void my_direct_dft::perform_transform(float *real, float *imag, int stride)
{
  // First copy the input values to the real and imaginary temporary buffers.
  int n, k;
  float *rp, *ip;
  for (rp=real, ip=imag, n=0; n < N; n++, rp+=stride, ip+=stride)
    { real_buf[n] = *rp;  imag_buf[n] = *ip; }

  // Now compute each output coefficient in turn
  for (rp=real, ip=imag, k=0; k < N; k++, rp+=stride, ip+=stride)
    {
      int index = 0; // This holds n*k mod N; it indexes the trig tables
      double real_sum=0.0, imag_sum=0.0;
      for (n=0; n < N; n++, index+=k)
        {
          if (index >= N)
            index -= N;
          real_sum += real_buf[n]*real_trig[index]
                    - imag_buf[n]*imag_trig[index];
          imag_sum += real_buf[n]*imag_trig[index]
                    + imag_buf[n]*real_trig[index];
        }
      *rp = (float) real_sum;
      *ip = (float) imag_sum;
    }
}

/*****************************************************************************/
/*                   my_direct_dft::perform_fft                              */
/*****************************************************************************/
namespace   // 文件局部辅助实现
{
    // 递归函数：`n` 为当前 DFT 长度（必为 2 的幂）；`N_tot` 为全局 N
    static void fft_rec_eo(float* re, float* im,
        int n,           // 当前块长度  = 2^r_lev
        int N_tot,       // 总长度     = 2^r
        const double* rt,// cos 表     = W_N^k 的实部
        const double* it)// sin 表(正/逆号已在 init 决定)
    {
        if (n == 1) return;                // 递归出口：长度 1

        const int half = n >> 1;
        /*------------------------------------------------------------------*/
        /* 1. 将偶、奇序列“去交织”成连续两块                                */
        /*------------------------------------------------------------------*/
        float* even_re = new float[half];
        float* even_im = new float[half];
        float* odd_re = new float[half];
        float* odd_im = new float[half];

        for (int i = 0; i < half; ++i) {
            even_re[i] = re[2 * i];
            even_im[i] = im[2 * i];
            odd_re[i] = re[2 * i + 1];
            odd_im[i] = im[2 * i + 1];
        }

        /*------------------------------------------------------------------*/
        /* 2. 递归计算偶、奇两块的 DFT                                      */
        /*------------------------------------------------------------------*/
        fft_rec_eo(even_re, even_im, half, N_tot, rt, it);
        fft_rec_eo(odd_re, odd_im, half, N_tot, rt, it);

        /*------------------------------------------------------------------*/
        /* 3. 用旋转因子合并偶/奇结果                                       */
        /*------------------------------------------------------------------*/
        const int step = N_tot / n;        // twiddle 索引步长 (W_N^step)
        for (int k = 0; k < half; ++k)
        {
            int idx = k * step;          // 当前蝶形用到的 W_N^idx
            double wr = rt[idx];
            double wi = it[idx];

            double tr = odd_re[k] * wr - odd_im[k] * wi;
            double ti = odd_re[k] * wi + odd_im[k] * wr;

            re[k] = even_re[k] + (float)tr;
            im[k] = even_im[k] + (float)ti;
            re[k + half] = even_re[k] - (float)tr;
            im[k + half] = even_im[k] - (float)ti;
        }

        delete[] even_re;  delete[] even_im;
        delete[] odd_re;   delete[] odd_im;
    }
} // 匿名命名空间结束


void my_direct_dft::perform_fft(float* real, float* imag, int stride)
{
    /* 0. 若 N 不是 2 的幂，回退到直接 DFT ---------------------------- */
    if ((N & (N - 1)) != 0) {
        perform_transform(real, imag, stride);
        return;
    }

    /* 1. 将带 stride 的输入收拢到连续缓冲区 real_buf / imag_buf ------- */
    for (int i = 0; i < N; ++i) {
        real_buf[i] = real[i * stride];
        imag_buf[i] = imag[i * stride];
    }

    /* 2. 调用递归偶/奇 FFT ------------------------------------------ */
    fft_rec_eo(real_buf, imag_buf, N, N, real_trig, imag_trig);

    /* 3. 写回到调用者的 (stride) 缓冲区 ------------------------------ */
    for (int i = 0; i < N; ++i) {
        real[i * stride] = real_buf[i];
        imag[i * stride] = imag_buf[i];
    }
}