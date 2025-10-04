
/* ported from https://github.com/qeftser/tang */

/*
 * This file contains code for all functions and concepts introduced in the first
 * chapter that could be readily constructed. This includes a small matrix library
 * that encompasses all of the presented matrix functions, as well as 
 * random number generators for floating-point and gaussian distributions. 
 *
 * // TODO(qeftser):
 * Runge-Kutta and Euler integration are not included as I am not sure the common
 * form that these methods will take at this point. This header will be updated
 * to include them as I go further into the text.                                */

#ifndef __TANG_NUMERICAL_BASICS__

#define __TANG_NUMERICAL_BASICS__
#include <stdint.h>
#include <stdlib.h>

typedef struct MAT0X0 {
   uint32_t rows;
   uint32_t cols;
   double vals[];
} matbase, MB;

typedef matbase * mat;

#define DEFMAT(m,n)              \
   typedef struct MAT##m##X##n { \
      uint32_t rows;             \
      uint32_t cols;             \
      double   vals[m][n];       \
   } mat##m##x##n;               \
   size_t mat##m##x##n##_size = sizeof(struct MAT##m##X##n)

#define MAT(m,n) \
{ .rows = m, .cols = n }

#define MATSIZE(m,n) \
   mat##m##x##n##_size

#define ALLOCMAT(m, n) \
   _alloc_mat((m),(n),MATSIZE(m,n))

#define DALLOCMAT(m, n) \
   _alloc_mat((m),(n),sizeof(uint32_t)*2 + sizeof(double)*m*n)

typedef struct MATDECOMP {
   mat m;
   int * indx;
   double d;
} matdecomp;

/* allocate a new matrix with m rows and n columns
 * that has size size. Do not use this function 
 * directly, instead call ALLOCMAT, which will
 * fill the size of the matrix automatically.    */
mat _alloc_mat(uint32_t m, uint32_t n, size_t size);

/* perform the LU decomposition on the given matrix
 * a, storing the result in lu. Return 1 if the
 * decomposition succeeded, and 0 if it failed.
 *
 * indx should be a vector of length == a->rows
 * it will be stored with the lu row permutations
 *
 * if zero is returned the values in lu are undefined.
 *
 * LU decomposition code is from pg. 52 of numerical
 * recipies, third edition.
 *
 * it is safe for a and lu->mat to be the same matrix
 */
int _lu_decomp_mat(mat a, struct MATDECOMP * lu);

/* solve a system of linear equations of the
 * form A * x = b, where lu is the LU decomposition
 * of A. We are finding x here, so x is modified
 * to return the answer.                          
 *
 * x should be a vector of length lu->m->rows
 * b should eb a vector of length lu->m->rows
 *
 * LU decomposition code is from pg. 53 of numerical
 * recipies, third edition.   */
void _solve_mat(matdecomp * lu, double * x, double * b);

/* add two matrices together, storing the
 * result in res, and returning res as an
 * output.                                
 *
 * l and r can safely be used as res */
mat matadd(mat l, mat r, mat res);

/* subtract matrix r from matrix l, storing the
 * result in res and returning res.            
 *
 * l and r can safely be used as res */
mat matsub(mat l, mat r, mat res);

/* transpose the given matrix a, storing the result
 * in at and returning at.
 *
 * a cannot be used as at, see mattrn_inplace      */
mat mattrn(mat a, mat at);

/* transpose the given matrix a, storing the result
 * in a and returning a.                           */
mat mattrn_inplace(mat a);

/* multiply the matrices l and r, storing the
 * result in res and returning res.          
 *
 * res cannot be l or r. l and r can be the same. */
mat matmul(mat l, mat r, mat res);

/* scale the matrix a by scalar, storing the
 * result in as and returning as.         
 *
 * a can safely be used as as */
mat matsca(mat a, mat as, double scalar);

/* set the given matrix a to the identity matrix */
mat matidn(mat a);

/* set the given matrix a to a matrix of all zeros */
mat matzro(mat a);

/* determine the inverse of the given matrix a, storing
 * the result in ainv. Return ainv. Standard solutions are used 
 * for matrices of size 1, 2, and 3, and the LU decomposition
 * is used for matrices of a larger size. 
 *
 * LU decomposition code is from pg. 52-54 of numerical
 * recipies, third edition.
 *
 * a cannot be ainv, see matinv_inplace */
mat matinv(mat a, mat ainv);

/* determine the inverse of the given matrix a,
 * storing the result in a and returning a.   */
mat matinv_inplace(mat a);

/* determine the inverse of the given matrix a,
 * storing the result in ainv. If the matrix is
 * singular, flag is set to zero, otherwise flag
 * is set to 1. Return ainv on success and NULL
 * if the matrix is singular. If NULL is returned
 * the state of ainv is undefined.               */
mat matinv_safe(mat a, mat ainv, int * flag);

/* duplicate the matrix a in ad, returning ad.
 *
 * a and ad can be the same, if your weird like that... */
mat matdup(mat a, mat ad);

/* against my better judgement, a function that returns
 * the determinent of the given matrix. performs LU
 * decomposition for matrices larger than 3 
 *
 * LU decomposition code is from pg. 52-54 of numerical
 * recipies, third edition.   */
double matdet(mat a);

/* print the given matrix, returning the matrix passed in. */
mat matshw(mat a);

/* produce a random floating point value in the range [0.0 - 1.0] */
double frand();

/* produce a random number from the gaussian distribution with mean
 * and standard deviation as the values given (sigma is std. dev.) 
 *
 * method is box-muller transform as shown in numerical recipies
 * third edition pg. 365                                        */
double grand(double mu, double sigma);

/* generate a random value from a distribution of white noise
 * with psd as the power spectral density and h as the time
 * step that the simulation is running at. */
double wnoise(double psd, double h);

/* helper macro for seeding with true randomness */
#ifdef _WIN32
#include <windows.h>
#include <bcrypt.h>
#define SRAND()                                              \
do {                                                         \
   uint32_t retval;                                          \
   BCryptGenRandom(NULL, (uint8_t *)&retval, sizeof(retval), \
                   BCRYPT_USE_SYSTEM_PREFERRED_RNG);         \
   srand(retval);                                            \
} while (0)
#else
#define SRAND() \
   srand(arc4random())
#endif

#endif
