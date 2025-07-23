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
namespace
{
    // Recursive FFT function (Radix-2, Decimation-in-Time)
    // Arguments:
    // - re, im: pointers to the real and imaginary parts of the signal (length n)
    // - n: current FFT size (must be a power of 2)
    // - N_tot: total size of the transform (used for twiddle factor indexing)
    // - rt, it: precomputed cosine (real) and sine (imaginary) twiddle tables of size N_tot
    static void fft_rec_eo(float* re, float* im,
        int n,
        int N_tot,
        const double* rt, // cos values: cos(2¦Ðk/N)
        const double* it) // sin values: sin(2¦Ðk/N)
    {
        if (n == 1) return; // Base case: length 1 DFT is trivial

        const int half = n >> 1;

        // Step 1: De-interleave the input into even and odd parts
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

        // Step 2: Recursively compute FFTs of even and odd parts
        fft_rec_eo(even_re, even_im, half, N_tot, rt, it);
        fft_rec_eo(odd_re, odd_im, half, N_tot, rt, it);

        // Step 3: Combine even and odd FFT results using twiddle factors
        const int step = N_tot / n; // Twiddle factor stride: W_N^step
        for (int k = 0; k < half; ++k)
        {
            int idx = k * step;    // Current twiddle index

            double wr = rt[idx];   // twiddle real part: cos
            double wi = it[idx];   // twiddle imag part: sin

            // Complex multiplication: T = W_N^k * FFT_odd[k]
            double tr = odd_re[k] * wr - odd_im[k] * wi;
            double ti = odd_re[k] * wi + odd_im[k] * wr;

            // Combine FFT_even[k] + T, FFT_even[k] - T
            re[k] = even_re[k] + static_cast<float>(tr);
            im[k] = even_im[k] + static_cast<float>(ti);
            re[k + half] = even_re[k] - static_cast<float>(tr);
            im[k + half] = even_im[k] - static_cast<float>(ti);
        }

        delete[] even_re;
        delete[] even_im;
        delete[] odd_re;
        delete[] odd_im;
    }
}

// Dispatch method: chooses between FFT and direct DFT
void my_direct_dft::perform_fft(float* real, float* imag, int stride)
{
    // Step 0: Fallback to direct DFT if N is not a power of 2
    if ((N & (N - 1)) != 0) {
        perform_transform(real, imag, stride);
        return;
    }

    // Step 1: Gather input from strided layout into temporary contiguous buffers
    for (int i = 0; i < N; ++i) {
        real_buf[i] = real[i * stride];
        imag_buf[i] = imag[i * stride];
    }

    // Step 2: Call recursive FFT
    fft_rec_eo(real_buf, imag_buf, N, N, real_trig, imag_trig);

    // Step 3: Scatter results back into caller's strided buffer
    for (int i = 0; i < N; ++i) {
        real[i * stride] = real_buf[i];
        imag[i * stride] = imag_buf[i];
    }
}