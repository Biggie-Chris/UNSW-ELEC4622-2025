/*****************************************************************************/
// File: task4_main.cpp
// Author: Chris
// Last Revised: 2025-08-05
/*****************************************************************************/

#include "io_bmp.h"
#include "image_comps.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <algorithm>

struct KeyPoint { int r, c; float score; };           // simple POD

static const float G[3] = { 0.25f, 0.5f, 0.25f };      // separable Gaussian kernel

/* ---------------- 1-D separable 3-tap Gaussian blur (in-place) ---------- */
static void gaussian_blur(my_image_comp& img)
{
    int W = img.width, H = img.height, S = img.stride;
    float* tmp = new float[H * W];

    /* horizontal pass */
    for (int r = 0;r < H;++r)
        for (int c = 0;c < W;++c) {
            float v = 0.f;
            for (int k = -1;k <= 1;++k) {
                int cc = (c + k < 0) ? 0 : (c + k >= W ? W - 1 : c + k);
                v += G[k + 1] * img.buf[r * S + cc];
            }
            tmp[r * W + c] = v;
        }

    /* vertical pass */
    for (int r = 0;r < H;++r)
        for (int c = 0;c < W;++c) {
            float v = 0.f;
            for (int k = -1;k <= 1;++k) {
                int rr = (r + k < 0) ? 0 : (r + k >= H ? H - 1 : r + k);
                v += G[k + 1] * tmp[rr * W + c];
            }
            img.buf[r * S + c] = v;
        }
    delete[] tmp;
}

/* ---------------------------- main ------------------------------------- */
int main(int argc, char* argv[])
{
    if (argc != 5) {
        fprintf(stderr,
            "Usage: %s <in.bmp> <out.bmp> <NK> <H>\n", argv[0]);
        return -1;
    }
    int NK = std::atoi(argv[3]);
    int H = std::atoi(argv[4]);

    int err_code = 0;
    try
    {
        /* ---- read BMP (gray) ---------------------------------------------- */
        bmp_in inp; int err;
        if ((err = bmp_in__open(&inp, argv[1])) != 0) {
            fprintf(stderr, "Cannot open input file\n"); return -1;
        }
        int W = inp.cols, Hgt = inp.rows;
        if (inp.num_components != 1) {
            fprintf(stderr, "Only 8-bit gray BMP supported\n");return -1;
        }
        my_image_comp img; img.init(Hgt, W, 0);
        io_byte* scan = new io_byte[W];
        for (int r = Hgt - 1; r >= 0; --r) {
            bmp_in__get_line(&inp, scan);
            for (int c = 0;c < W;++c) img.buf[r * img.stride + c] = scan[c];
        }
        bmp_in__close(&inp);

        /* ---- Gaussian blur ------------------------------------------------- */
        gaussian_blur(img);                          /* now holds yσ[n] */

        /* ---- gradient gx,gy ----------------------------------------------- */
        float* gx = new float[Hgt * W];
        float* gy = new float[Hgt * W];
        for (int r = 0;r < Hgt;++r)
            for (int c = 0;c < W;++c) {
                float xm = img.buf[r * img.stride + (c ? c - 1 : 0)];
                float xp = img.buf[r * img.stride + (c + 1 < W ? c + 1 : W - 1)];
                float ym = img.buf[(r ? r - 1 : 0) * img.stride + c];
                float yp = img.buf[((r + 1 < Hgt) ? r + 1 : Hgt - 1) * img.stride + c];
                gx[r * W + c] = 0.5f * (xp - xm);
                gy[r * W + c] = 0.5f * (yp - ym);
            }

        /* ---- K[n]  --------------------------------------------------------- */
        float* K = new float[Hgt * W];
        for (int i = 0;i < Hgt * W;++i) K[i] = 0.f;

        int win = 2 * H + 1;
        for (int r = H;r < Hgt - H;++r)
            for (int c = H;c < W - H;++c) {
                float S11 = 0.f, S12 = 0.f, S22 = 0.f;
                for (int v = -H;v <= H;++v)
                    for (int u = -H;u <= H;++u) {
                        int idx = (r + v) * W + (c + u);
                        float gxv = gx[idx], gyv = gy[idx];
                        S11 += gxv * gxv;
                        S12 += gxv * gyv;
                        S22 += gyv * gyv;
                    }
                float trace = S11 + S22;
                if (trace > 1e-6f) {
                    float det = S11 * S22 - S12 * S12;
                    K[r * W + c] = det / trace;
                }
            }

        /* ---- collect local maxima ----------------------------------------- */
        KeyPoint* all = new KeyPoint[Hgt * W];   /* worst-case size */
        int tot = 0;
        for (int r = H;r < Hgt - H;++r)
            for (int c = H;c < W - H;++c) {
                float v = K[r * W + c];
                if (v <= 0)continue;
                if (v >= K[(r - 1) * W + c] && v >= K[(r + 1) * W + c] &&
                    v >= K[r * W + c - 1] && v >= K[r * W + c + 1])
                {
                    all[tot++] = { r,c,v };
                }
            }

        /* ---- 选择前 NK 个（简单选择排序，不用 STL） ---------------------- */
        if (tot < NK) NK = tot;
        for (int i = 0;i < NK;++i) {
            int maxIdx = i;
            for (int j = i + 1;j < tot;++j)
                if (all[j].score > all[maxIdx].score) maxIdx = j;
            std::swap(all[i], all[maxIdx]);
        }

        /* ---- prepare output RGB buffer ------------------------------------ */
        int strideRGB = W * 3;
        io_byte* rgb = new io_byte[Hgt * strideRGB];

        for (int r = 0;r < Hgt;++r)
            for (int c = 0;c < W;++c) {
                float v = img.buf[r * img.stride + c] * 0.5f;
                int idx = r * strideRGB + c * 3;
                rgb[idx] = rgb[idx + 1] = rgb[idx + 2] = (io_byte)v;
            }

        /* ---- draw 3×3 red blocks ------------------------------------------ */
        for (int k = 0;k < NK;++k) {
            int rr = all[k].r, cc = all[k].c;
            for (int v = -1;v <= 1;++v)
                for (int u = -1;u <= 1;++u) {
                    int r = rr + v, c = cc + u;
                    if (r < 0 || r >= Hgt || c < 0 || c >= W) continue;
                    int idx = r * strideRGB + c * 3;
                    rgb[idx] = 0; rgb[idx + 1] = 0; rgb[idx + 2] = 255;
                }
        }

        /* ---- write BMP ----------------------------------------------------- */
        bmp_out out;
        if ((err = bmp_out__open(&out, argv[2], W, Hgt, 3)) != 0) {
            fprintf(stderr, "Write error\n"); return -1;
        }
        io_byte* lineRGB = new io_byte[strideRGB];
        for (int r = Hgt - 1; r >= 0; --r) {      /* BMP bottom-up */
            std::memcpy(lineRGB, rgb + r * strideRGB, strideRGB);
            bmp_out__put_line(&out, lineRGB);
        }
        bmp_out__close(&out);

        /* ---- cleanup ------------------------------------------------------- */
        delete[] scan;
        delete[] gx; 
        delete[] gy; 
        delete[] K; 
        delete[] all;
        delete[] rgb; 
        delete[] lineRGB;
        return 0;
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