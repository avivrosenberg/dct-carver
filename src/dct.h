
#ifndef __DCT_H__
#define __DCT_H__

/*	Comstants */

#define BLOCK_SIZE 8
#define CENTER_ROW (ROUND((BLOCK_SIZE-1)/2))
#define CENTER_COL (ROUND((BLOCK_SIZE-1)/2))
#define NUM_DCT_ATOMS (BLOCK_SIZE*BLOCK_SIZE)
#define C_INV_SQRT_BLOCKSIZE (1.0/sqrt(BLOCK_SIZE))
//0.353553390593 /* 1/sqrt(8) */
#define C_SQRT_2_INV_BLOCKSIZE (sqrt(2.0/BLOCK_SIZE))
//0.5 /* sqrt(2/8) */

#define DOUBLE2GUCHAR(d,min,max) (ROUND(255*(((d) - (min))/((max)-(min)))))
#define GUCHAR2DOUBLE(g) (((gdouble)g)/255.0)
//#define ATOMDB_INDEX(k1,k2) (BLOCK_SIZE*(k1)+(k2)-((k1)+1)*(k1)/2)
#define ATOMDB_INDEX(k1,k2) (BLOCK_SIZE*(k1)+(k2))

/* 	Defintions */

typedef struct {
	gdouble** matrix;
	//gboolean transpose;
} DCTAtom;

typedef gdouble*** DCTAtomDB;

/*  Public functions  */

/* Initialize the Global database with the atoms */
void init_dctatomdb(DCTAtomDB* db);

void atomdb_free(DCTAtomDB dctAtomDB) ;

DCTAtom get_atom (DCTAtomDB dctAtomDB, gint k1, gint k2);

#endif /* __DCT_H__ */
