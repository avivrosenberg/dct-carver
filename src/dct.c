#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "dct.h"
#include "main.h"


/*	Constants */

/* 	Private functions */

/* calculate the value of the DCT atom k1,k2 at location j1,j2 */
inline gdouble
dct_atom_at(gint k1, gint k2, gint j1, gint j2) {
	gdouble a1 = ((k1==0) ? C_INV_SQRT_BLOCKSIZE : C_SQRT_2_INV_BLOCKSIZE);
	gdouble a2 = ((k2==0) ? C_INV_SQRT_BLOCKSIZE : C_SQRT_2_INV_BLOCKSIZE);
	return (a1*a2*cos(M_PI*((gdouble)((2*j1+1)*k1))/((gdouble)(2*BLOCK_SIZE)))*
				  cos(M_PI*((gdouble)((2*j2+1)*k2))/((gdouble)(2*BLOCK_SIZE))));
}

/* returns a BLOCK_SIZExBLOCK_SIZE DCT atom, indexed k1,k2*/
gdouble**
dct_atom(gint k1, gint k2) {
	gdouble** atom = g_new(gdouble*, BLOCK_SIZE);
	gint j1,j2;
	
	for(j1 = 0; j1 < BLOCK_SIZE; j1++) {
		atom[j1] = g_new(gdouble, BLOCK_SIZE);
		for(j2 = 0; j2 < BLOCK_SIZE; j2++) {
			atom[j1][j2] = dct_atom_at(k1,k2,j1,j2);
		}
	}
	return atom;
	
}/* 	Public function */

/* Initialize the Global database with the atoms */
void
init_dctatomdb(DCTAtomDB* db) {
	gint k1,k2;
	*db = g_new(gdouble**, NUM_DCT_ATOMS);
	
	for(k1 = 0; k1 < BLOCK_SIZE; k1++) {
		for(k2 = 0; k2 < BLOCK_SIZE; k2++) {
		(*db)[ATOMDB_INDEX(k1,k2)] = dct_atom(k1,k2);
		}
	}
}



void atomdb_free(DCTAtomDB dctAtomDB) {
	gint k1,k2,j1;
	for(k1=0; k1 < BLOCK_SIZE; k1++) {
		for(k1=0; k1 < BLOCK_SIZE; k1++) {
			for(j1=0; j1 < BLOCK_SIZE; j1++) {
				g_free(dctAtomDB[ATOMDB_INDEX(k1,k2)][j1]);
			}
			g_free(dctAtomDB[ATOMDB_INDEX(k1,k2)]);
		}
	}
	g_free(dctAtomDB);
}

DCTAtom
get_atom (DCTAtomDB dctAtomDB, gint k1, gint k2)
{
	DCTAtom retval;
	//retval.transpose = FALSE;
	/*
	if (k1 < k2){
		gint tmp = k1;
		k1 = k2;
		k2 = tmp;
		retval.transpose = TRUE;
	} 
	*/
	retval.matrix = dctAtomDB[ATOMDB_INDEX(k1,k2)];
	return retval;
}
