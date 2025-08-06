/*****************************************************************************/
// File: task5_main.cpp
// Author: Chris
// Last Revised: 2025-08-06
/*****************************************************************************/

#include "io_bmp.h"
#include "image_comps.h"
#include "aligned_image_comps.h"
#include "motion.h"          
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <algorithm>         


#define MAX_KEYPOINTS 20000          /* 角点最大缓存 */


struct KeyPoint { int r, c; float score; };

/* ------------------------------------------------------------------------- */
/*                    生成 3-tap 高斯核 (可调 σ)                              */
/* ------------------------------------------------------------------------- */
static void make_gaussian_kernel(float sigma, float G[3])
{ /* 离散样点位于  -1,0,+1 ；再归一化 */
    if (sigma < 0.05f) sigma = 0.05f;
    float a = std::exp(-0.5f / (sigma * sigma));  
    G[0] = G[2] = a;
    G[1] = 1.0f;
    float sum = G[0] + G[1] + G[2];
    G[0] /= sum;  G[1] /= sum;  G[2] /= sum;
}

/* ------------------------------------------------------------------------- */
/*                  1-D separable 3-tap 高斯模糊  (in-place)                  */
/* ------------------------------------------------------------------------- */
static void gaussian_blur(my_aligned_image_comp& img, const float G[3])
{
    int W = img.width, H = img.height, S = img.stride;
    float* tmp = new float[W * H];

    /* --- 横向 --- */
    for (int r = 0; r < H; ++r)
        for (int c = 0; c < W; ++c)
        {
            float v = 0.0f;
            for (int k = -1; k <= 1; ++k)
            {
                int cc = (c + k < 0) ? 0 : ((c + k >= W) ? W - 1 : c + k);
                v += G[k + 1] * img.buf[r * S + cc];
            }
            tmp[r * W + c] = v;
        }

    /* --- 纵向 --- */
    for (int r = 0; r < H; ++r)
        for (int c = 0; c < W; ++c)
        {
            float v = 0.0f;
            for (int k = -1; k <= 1; ++k)
            {
                int rr = (r + k < 0) ? 0 : ((r + k >= H) ? H - 1 : r + k);
                v += G[k + 1] * tmp[rr * W + c];
            }
            img.buf[r * S + c] = v;
        }
    delete[] tmp;
}

/* ------------------------------------------------------------------------- */
/*                在 (x,y) 浮点坐标取得双线性像素值                          */
/* ------------------------------------------------------------------------- */
static inline float get_bilinear_pixel(const my_aligned_image_comp* src,
    float x, float y)
{
    /* 边界截断 */
    if (x < 0.0f) x = 0.0f;
    if (y < 0.0f) y = 0.0f;
    if (x > src->width - 1.001f)  x = (float)(src->width - 1.001f);
    if (y > src->height - 1.001f) y = (float)(src->height - 1.001f);

    int x0 = (int)floorf(x), x1 = x0 + 1;
    int y0 = (int)floorf(y), y1 = y0 + 1;
    float dx = x - x0, dy = y - y0;

    float* buf = src->buf;  int S = src->stride;
    float p00 = buf[y0 * S + x0];
    float p10 = buf[y0 * S + x1];
    float p01 = buf[y1 * S + x0];
    float p11 = buf[y1 * S + x1];

    return (1 - dx) * (1 - dy) * p00 +
        dx * (1 - dy) * p10 +
        (1 - dx) * dy * p01 +
        dx * dy * p11;
}

/* ------------------------------------------------------------------------- */
/*                    Task 2 的 find_motion / motion_comp                    */
/* ------------------------------------------------------------------------- */
static mvector
find_motion(my_aligned_image_comp* ref, my_aligned_image_comp* tgt,
    int start_row, int start_col,
    int blk_w, int blk_h, int S)
{
    mvector vec{}, best_vec{};
    int best_sad = 256 * blk_w * blk_h;

    for (vec.y = -S; vec.y <= S; ++vec.y)
        for (vec.x = -S; vec.x <= S; ++vec.x)
        {
            int rr = start_row - vec.y;
            int cc = start_col - vec.x;
            if (rr < 0 || cc < 0 || (rr + blk_h) > ref->height || (cc + blk_w) > ref->width)
                continue;                    /* 越界则跳过 */

            float* rp = ref->buf + rr * ref->stride + cc;
            float* tp = tgt->buf + start_row * tgt->stride + start_col;
            int sad = 0;
            for (int r = 0; r < blk_h; ++r, rp += ref->stride, tp += tgt->stride)
                for (int c = 0; c < blk_w; ++c)
                {
                    int diff = (int)(tp[c] - rp[c]);
                    sad += (diff < 0) ? -diff : diff;
                }

            if (sad < best_sad) { best_sad = sad; best_vec = vec; }
        }
    return best_vec;
}

/* ------------------------------------------------------------------------- */
/*                                   main                                    */
/* ------------------------------------------------------------------------- */
int main(int argc, char* argv[])
{
    if (argc != 8)
    {
        fprintf(stderr,
            "Usage: %s <bmp frame 1> <bmp frame 2> <output bmp> "
            "<H> <S> <NK> <sigma>\n", argv[0]);
        return -1;
    }

    /* ------------------ 解析命令行 ------------------ */
    const char* fname_in0 = argv[1];
    const char* fname_in1 = argv[2];
    const char* fname_out = argv[3];
    int   H = std::atoi(argv[4]);    /* 块半径          */
    int   S = std::atoi(argv[5]);    /* 搜索半径        */
    int   NK = std::atoi(argv[6]);    /* 角点上限        */
    float sigma = (float)std::atof(argv[7]);

    if (H < 1 || S < 0 || NK < 1 || NK > MAX_KEYPOINTS)
    {
        fprintf(stderr, "Parameter out of range.\n");
        return -1;
    }

    const int blk_w = 2 * H + 1;
    const int blk_h = 2 * H + 1;

    int err_code = 0;
    try {
        /* --------------- 读两帧 BMP ----------------- */
        bmp_in in[2];
        if ((err_code = bmp_in__open(&in[0], fname_in0)) != 0) throw err_code;
        if ((err_code = bmp_in__open(&in[1], fname_in1)) != 0) throw err_code;

        if (in[0].cols != in[1].cols || in[0].rows != in[1].rows)
        {
            fprintf(stderr, "The two frames have different dimensions.\n");
            return -1;
        }
        if (in[0].num_components != 1 || in[1].num_components != 1)
        {
            fprintf(stderr, "Only 8-bit grayscale BMP supported.\n");
            return -1;
        }

        int width = in[0].cols;
        int height = in[0].rows;

        my_aligned_image_comp mono[2];
        mono[0].init(height, width, S);   /* 给足边界，便于搜索 */
        mono[1].init(height, width, S);

        io_byte* line = new io_byte[width];
        for (int n = 0; n < 2; ++n)
        {
            for (int r = height - 1; r >= 0; --r)          /* BMP 底->顶 */
            {
                if ((err_code = bmp_in__get_line(&in[n], line)) != 0) throw err_code;
                float* dst = mono[n].buf + r * mono[n].stride;
                for (int c = 0; c < width; ++c) dst[c] = line[c];
            }
            bmp_in__close(&in[n]);
        }
        mono[0].perform_boundary_extension();
        mono[1].perform_boundary_extension();

        /* =====================================================================
         *                       Step 1 : Harris 角点检测                       *
         * ===================================================================*/
        my_aligned_image_comp tgt_smooth;
        tgt_smooth.init(height, width, 0);
        std::memcpy(tgt_smooth.buf, mono[1].buf, sizeof(float) * height * width);

        float G[3];              /* 3-tap 高斯核 */
        make_gaussian_kernel(sigma, G);
        gaussian_blur(tgt_smooth, G);

        float* gx = new float[width * height];
        float* gy = new float[width * height];
        float* K = new float[width * height];
        for (int r = 0; r < height; ++r)
            for (int c = 0; c < width; ++c)
            {
                float xm = tgt_smooth.buf[r * tgt_smooth.stride + (c ? c - 1 : 0)];
                float xp = tgt_smooth.buf[r * tgt_smooth.stride + (c + 1 < width ? c + 1 : width - 1)];
                float ym = tgt_smooth.buf[(r ? r - 1 : 0) * tgt_smooth.stride + c];
                float yp = tgt_smooth.buf[(r + 1 < height ? r + 1 : height - 1) * tgt_smooth.stride + c];
                gx[r * width + c] = 0.5f * (xp - xm);
                gy[r * width + c] = 0.5f * (yp - ym);
                K[r * width + c] = 0.0f;     /* 清零 */
            }

        const int win = 2 * H + 1;
        int border = H + S;             /* 必须离边界 ≥H+S 像素 */
        for (int r = border; r < height - border; ++r)
            for (int c = border; c < width - border; ++c)
            {
                float S11 = 0, S22 = 0, S12 = 0;
                for (int v = -H; v <= H; ++v)
                    for (int u = -H; u <= H; ++u)
                    {
                        int idx = (r + v) * width + (c + u);
                        float gxv = gx[idx], gyv = gy[idx];
                        S11 += gxv * gxv;
                        S22 += gyv * gyv;
                        S12 += gxv * gyv;
                    }
                float tr = S11 + S22;
                if (tr > 1e-6f)
                {
                    float det = S11 * S22 - S12 * S12;
                    K[r * width + c] = det / tr;          /* Harris 响应 */
                }
            }

        /* -------------------- 取局部最大值 -------------------- */
        KeyPoint* cand = new KeyPoint[width * height]; /* 最坏情况 */
        int tot = 0;
        for (int r = border; r < height - border; ++r)
            for (int c = border; c < width - border; ++c)
            {
                float v = K[r * width + c];
                if (v <= 0) continue;
                if (v >= K[(r - 1) * width + c] && v >= K[(r + 1) * width + c] &&
                    v >= K[r * width + c - 1] && v >= K[r * width + c + 1])
                    cand[tot++] = { r,c,v };
            }

        if (tot == 0) { fprintf(stderr, "No corner found!\n"); return -1; }
        if (tot < NK) NK = tot;

        /* ----------- 选得分最高的前 NK 个 (选择排序) ----------- */
        for (int i = 0; i < NK; ++i)
        {
            int best = i;
            for (int j = i + 1; j < tot; ++j)
                if (cand[j].score > cand[best].score) best = j;
            std::swap(cand[i], cand[best]);
        }

        /* =====================================================================
         *                       Step 2 : 每角点块匹配                          *
         * ===================================================================*/
        mvector* motion = new mvector[NK];
        for (int k = 0; k < NK; ++k)
        {
            int y = cand[k].r;
            int x = cand[k].c;
            motion[k] = find_motion(&mono[0], &mono[1],
                y - H, x - H, blk_w, blk_h, S);
        }

        /* ------------------ Step 3 : 求全局向量 ------------------ */
        double sum_x = 0, sum_y = 0;
        for (int i = 0; i < NK; ++i) { sum_x += motion[i].x; sum_y += motion[i].y; }
        mvector gvec;
        gvec.x = (int)std::round(sum_x / NK);
        gvec.y = (int)std::round(sum_y / NK);
        printf("Global motion = (%d,%d)  |  corners used = %d\n", gvec.x, gvec.y, NK);

        /* =====================================================================
         *               Step 4 : 全局平移补偿 + 计算 MSE                       *
         * ===================================================================*/
        my_aligned_image_comp compensated;
        compensated.init(height, width, 0);

        float vx = (float)gvec.x;
        float vy = (float)gvec.y;

        for (int r = 0; r < height; ++r)
            for (int c = 0; c < width; ++c)
                compensated.buf[r * compensated.stride + c] =
                get_bilinear_pixel(&mono[0], c - vx, r - vy);

        double mse_acc = 0.0;
        for (int r = 0; r < height; ++r)
        {
            float* tp = mono[1].buf + r * mono[1].stride;
            float* cp = compensated.buf + r * compensated.stride;
            for (int c = 0; c < width; ++c)
            {
                float diff = tp[c] - cp[c];
                mse_acc += diff * diff;
            }
        }
        double mse = mse_acc / (width * height);
        printf("Global-motion-compensated MSE = %.3f\n", mse);

        /* ------------------ Step 5 : 写出补偿后帧 ------------------ */
        bmp_out out;
        if ((err_code = bmp_out__open(&out, fname_out, width, height, 1)) != 0)
            throw err_code;

        for (int r = height - 1; r >= 0; --r)
        {
            for (int c = 0; c < width; ++c)
            {
                float val = compensated.buf[r * compensated.stride + c];
                line[c] = (io_byte)std::clamp(val + 0.5f, 0.0f, 255.0f);
            }
            bmp_out__put_line(&out, line);
        }
        bmp_out__close(&out);

        /* ------------------ Clean up ------------------ */
        delete[] line;
        delete[] gx; delete[] gy; delete[] K;
        delete[] cand; delete[] motion;
    }
    catch (int exc)
    {
        if (exc == IO_ERR_NO_FILE)
            fprintf(stderr, "Cannot open file.\n");
        else if (exc == IO_ERR_FILE_HEADER)
            fprintf(stderr, "BMP header parsing error.\n");
        else if (exc == IO_ERR_UNSUPPORTED)
            fprintf(stderr, "Unsupported BMP format (need 8-bit gray).\n");
        else if (exc == IO_ERR_FILE_TRUNC)
            fprintf(stderr, "File truncated unexpectedly.\n");
        else if (exc == IO_ERR_FILE_NOT_OPEN)
            fprintf(stderr, "Trying to access a file which is not open.\n");
        return -1;
    }
    return 0;
}