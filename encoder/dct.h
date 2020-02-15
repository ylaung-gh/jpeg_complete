// -*- c++ -*-
// ---------------------------------------------
// File: Head file for Wavelet Transform Class
// Author: Jianfei Cai
// Date: 1/20/99
// Note: -- 9/7 filter
// ---------------------------------------------
#ifndef DCT_H
#define DCT_H

#include <stdio.h>
#include <math.h>
#include <stdlib.h>


void ddct8x8s(int isgn, double **a);
void ddct16x16s(int isgn, double **a);


#define DataType double

//void dct(float **image, int img_dim, int dct_dim, int isgn);
void dct(float *image, int img_dim, int dct_dim, int isgn); // for image
void dct_1d(int N, DataType *bin, DataType *bout);
void idct_1d(int N, DataType *bin, DataType *bout);
void dct_2d(int isgn, int blk_dim, DataType **blk_buf);	// for a block
#endif
