
#ifndef __DCT_H__
#define __DCT_H__

#include "alloc.h"

/*	Comstants */

#define CENTER_ROW(blocksize) (ROUND(((blocksize)-1)/2))
#define CENTER_COL(blocksize) (ROUND(((blocksize)-1)/2))
#define NUM_DCT_ATOMS(blocksize) ((blocksize)*(blocksize))
#define ATOMDB_INDEX(blocksize,k1,k2) ((blocksize)*(k1) + (k2))
#define IS_EDGE_ATOM(blocksize,k1,k2) (((k1)+(k2)) < ((blocksize)-2))


/* 	Defintions */

typedef struct {
    gdouble** matrix;
    //gboolean transpose;
} DCTAtom;

typedef gdouble*** DCTAtomsMatrix;

typedef struct {
    DCTAtomsMatrix 	db;
    gint			blocksize;
} DCTAtomDB;



/*  Public functions  */

/* Initialize the Global database with the atoms */
void init_dctatomdb(DCTAtomDB* _db, gint _blocksize);
void atomdb_free(DCTAtomDB _db) ;
DCTAtom get_atom(DCTAtomDB dctAtomDB, gint k1, gint k2);

void ddct8x8s(int isgn, double **a);
void ddct16x16s(int isgn, double **a);
void ddct2d(int n1, int n2, int isgn, double **a, double *t, int *ip, double *w);
void ddctNxNs(int n, int isgn, double **a);
gfloat weighted_max_dct_correlation(int blocksize, double** data, gfloat edges, gfloat textures);

#endif /* __DCT_H__ */
