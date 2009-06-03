#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "dct.h"
#include "main.h"


/*	Constants */

/* 	Private functions */

/* calculate the value of the DCT atom k1,k2 at location j1,j2 */
inline gdouble
dct_atom_at(gint blocksize, gint k1, gint k2, gint j1, gint j2) {

    gdouble c1 = (1.0 / sqrt(blocksize));
    gdouble c2 = (sqrt(2.0 / blocksize));
    gdouble a1 = ((k1 == 0) ? c1 : c2);
    gdouble a2 = ((k2 == 0) ? c1 : c2);
    return (a1*a2*cos(M_PI*((gdouble)((2*j1 + 1)*k1)) / ((gdouble)(2*blocksize)))*
            cos(M_PI*((gdouble)((2*j2 + 1)*k2)) / ((gdouble)(2*blocksize))));
}

/* returns a BLOCK_SIZExBLOCK_SIZE DCT atom, indexed k1,k2*/
gdouble**
dct_atom(gint blocksize, gint k1, gint k2) {
    gdouble** atom = g_new(gdouble*, blocksize);
    gint j1, j2;

    for (j1 = 0; j1 < blocksize; j1++) {
        atom[j1] = g_new(gdouble, blocksize);

        for (j2 = 0; j2 < blocksize; j2++) {
            atom[j1][j2] = dct_atom_at(blocksize, k1, k2, j1, j2);
        }
    }

    return atom;
}

void
init_dctatoms_matrix(gint blocksize, DCTAtomsMatrix* db) {
    gint k1, k2;
    *db = g_new(gdouble**, NUM_DCT_ATOMS(blocksize));

    for (k1 = 0; k1 < blocksize; k1++) {
        for (k2 = 0; k2 < blocksize; k2++) {
            (*db)[ATOMDB_INDEX(blocksize,k1,k2)] = dct_atom(blocksize, k1, k2);
        }
    }
}

void atomdb_free_matrix(gint blocksize, DCTAtomsMatrix dctAtomsMatrix) {
    gint k1, k2, j1;

    for (k1 = 0; k1 < blocksize; k1++) {
        for (k2 = 0; k2 < blocksize; k2++) {
            for (j1 = 0; j1 < blocksize; j1++) {
                g_free(dctAtomsMatrix[ATOMDB_INDEX(blocksize,k1,k2)][j1]);
            }

            g_free(dctAtomsMatrix[ATOMDB_INDEX(blocksize,k1,k2)]);
        }
    }

    g_free(dctAtomsMatrix);
}

/* 	Public functions */

/* Initialize the Global database with the atoms */

void
init_dctatomdb(DCTAtomDB* _db, gint _blocksize) {
    _db->blocksize = _blocksize;
    init_dctatoms_matrix(_blocksize, &(_db->db));
}

void
atomdb_free(DCTAtomDB _db) {
    atomdb_free_matrix(_db.blocksize, _db.db);
}

DCTAtom
get_atom(DCTAtomDB dctAtomDB, gint k1, gint k2) {
    DCTAtom retval;
    retval.matrix = dctAtomDB.db[ATOMDB_INDEX(dctAtomDB.blocksize,k1,k2)];
    return retval;
}

//void ddct8x8s(int isgn, double **a);
//void ddct16x16s(int isgn, double **a);
//void ddct2d(int n1, int n2, int isgn, double **a, double *t, int *ip, double *w);

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
