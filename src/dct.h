
#ifndef __DCT_H__
#define __DCT_H__

#include "fft2d/alloc.h"

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

void ddctNxNs(int n, int isgn, double **a);
gfloat weighted_max_dct_correlation(int blocksize, double** data, gfloat edges, gfloat textures);

#endif /* __DCT_H__ */
