
#ifndef __DCT_H__
#define __DCT_H__

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

#endif /* __DCT_H__ */
