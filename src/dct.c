#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "dct.h"
#include "main.h"
#include "interface.h"

/*	Constants */

/* 	Private functions */

/* Prototypes of functions for fast DCT calculation.
 * These functions are taken from the fft2d package by 
 * Takuya OOURA (email: ooura@kurims.kyoto-u.ac.jp) and have not been changed. 
 * The full package can be obtained at http://www.kurims.kyoto-u.ac.jp/~ooura/fft.html */
void ddct8x8s(int isgn, double **a);
void ddct16x16s(int isgn, double **a);
void ddct2d(int n1, int n2, int isgn, double **a, double *t, int *ip, double *w);
/* End of functions prototypes from the fft2d package */

void dctNxN(int n, double **data, int* ip, double* w) {
    switch(n) {
        case 2:
        case 4:
            ddct2d(n, n, -1, data, NULL, ip, w);
            break;
        case 8:
            ddct8x8s(-1,data);
            break;
        case 16:
            ddct16x16s(-1,data);
            break;
        default:
            error("N (blocksize) wasn't a factor of 2 in dctNxN");
            exit(1);
            break;
    }
}

gfloat weighted_max_dct_correlation(int blocksize, double** dct_data, gfloat edges, gfloat textures) {
    int k1, k2, k1max, k2max;
    double max = 0, currval;
    
    for (k1 = 0; k1 < blocksize; k1++) {
        for (k2 = 0; k2 < blocksize; k2++) {
            currval = ABS(dct_data[k1][k2]);
            if ( (max <= currval) && (k1 || k2) ) {
                    max = currval; 
                    k1max = k1; k2max = k2;
            }
        }
    }
    return (IS_EDGE_ATOM(blocksize,k1max,k2max) ? max*edges : max*textures);
}

