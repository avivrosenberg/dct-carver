#ifndef __DCT_H__
#define __DCT_H__

#include "fft2d/alloc.h"

/*	Macros */

#define CENTER_ROW(blocksize) (ROUND(((blocksize)-1)/2))
#define CENTER_COL(blocksize) (ROUND(((blocksize)-1)/2))
#define IS_EDGE_ATOM(blocksize,k1,k2) (((k1)+(k2)) < ((blocksize)-2))

/*  Public functions  */
void dctNxN(int n, double **data, int* ip, double* w);
gfloat weighted_max_dct_correlation(int blocksize, double** data, gfloat edges, gfloat textures);

#endif /* __DCT_H__ */
