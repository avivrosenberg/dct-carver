
#ifndef __DCT_H__
#define __DCT_H__

/*	Comstants */

#define CENTER_ROW(blocksize) (ROUND(((blocksize)-1)/2))
#define CENTER_COL(blocksize) (ROUND(((blocksize)-1)/2))
#define NUM_DCT_ATOMS(blocksize) ((blocksize)*(blocksize))
#define ATOMDB_INDEX(blocksize,k1,k2) ((blocksize)*(k1) + (k2))

#define DOUBLE2GUCHAR(d,min,max) (ROUND(255*(((d) - (min))/((max)-(min)))))
#define GUCHAR2DOUBLE(g) (((gdouble)g)/255.0)

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

DCTAtom get_atom (DCTAtomDB dctAtomDB, gint k1, gint k2);

#endif /* __DCT_H__ */
