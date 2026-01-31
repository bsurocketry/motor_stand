
/* ported from https://github.com/qeftser/tang */

#include "method_of_least_squares.h"
#include <math.h>
#include <stdio.h>

void zero_order_least_squares_filter(double * x, double * y, size_t nmemb, double * a0) {

   *a0 = 0.0;

   for (size_t i = 0; i < nmemb; ++i) {

      *a0 += *y;
      ++y;

   }

   *a0 /= nmemb;
}

void stable_first_order_least_squares(double * x, double * y, size_t nmemb, double * a0, double * a1) {

   mat A = DALLOCMAT(nmemb,2);
   mat xv = DALLOCMAT(2,1);
   mat bv = DALLOCMAT(nmemb,1);

   mat At = DALLOCMAT(2,nmemb);
   mat AAt = DALLOCMAT(nmemb,nmemb);
   mat AAt_inv = DALLOCMAT(nmemb,nmemb);
   mat AAt_invAt = DALLOCMAT(2,nmemb);

   for (size_t i = 0; i < nmemb; ++i) {
      A->vals[i*2] = x[i];
      A->vals[(i*2)+1] = 1;
      bv->vals[i] = y[i];
   }

   mattrn(A,At);

printf("1\n");
   matmul(A,At,AAt);

printf("2\n");
   matinv(AAt,AAt_inv);

printf("3\n");
   matmul(AAt_inv,At,AAt_invAt);

printf("4\n");
   matmul(AAt_invAt,bv,xv);

   *a0 = xv->vals[0];
   *a1 = xv->vals[1];
}

void first_order_least_squares_filter(double * x, double * y, size_t nmemb, double * a0, double * a1) {

   mat eq = DALLOCMAT(2,2);
   mat ac = DALLOCMAT(2,1);
   mat ei = DALLOCMAT(2,2);
   mat co = DALLOCMAT(2,1);

   matzro(eq);
   matzro(ac);

   eq->vals[0] = nmemb;
   for (size_t i = 0; i < nmemb; ++i) {

      eq->vals[1] += *x;
      eq->vals[2] += *x;
      eq->vals[3] += *x * *x;
      ac->vals[0] += *y;
      ac->vals[1] += *y * *x;

      ++x;
      ++y;
   }

   matmul(matinv(eq,ei),ac,co);

   *a0 = co->vals[0];
   *a1 = co->vals[1];

   free(eq);
   free(ac);
   free(ei);
   free(co);
}

void second_order_least_squares_filter(double * x, double * y, size_t nmemb, double * a0, double * a1, double * a2) {

   mat eq = DALLOCMAT(3,3);
   mat ac = DALLOCMAT(3,1);
   mat ei = DALLOCMAT(3,3);
   mat co = DALLOCMAT(3,1);

   matzro(eq);
   matzro(ac);

   eq->vals[0] = nmemb;
   for (size_t i = 0; i < nmemb; ++i) {

      eq->vals[1] += *x;
      eq->vals[2] += *x * *x;
      eq->vals[3] += *x;
      eq->vals[4] += *x * *x;
      eq->vals[5] += *x * *x * *x;
      eq->vals[6] += *x * *x;
      eq->vals[7] += *x * *x * *x;
      eq->vals[8] += *x * *x * *x * *x;
      ac->vals[0] += *y;
      ac->vals[1] += *x * *y;
      ac->vals[2] += *x * *x * *y;

      ++x;
      ++y;
   }

   matmul(matinv(eq,ei),ac,co);

   *a0 = co->vals[0];
   *a1 = co->vals[1];
   *a2 = co->vals[2];

   free(eq);
   free(ac);
   free(ei);
   free(co);
}

void least_squares_filter(double * x, double * y, size_t nmemb, double * a, size_t n) {

   switch (n) {

      case 0:
         zero_order_least_squares_filter(x,y,nmemb,a);
      break;
      case 1:
         first_order_least_squares_filter(x,y,nmemb,a,a+1);
      break;
      case 2:
         second_order_least_squares_filter(x,y,nmemb,a,a+1,a+2);
      break;

         /* this is generic time! use of pow slows things down a bit... */
      default:
         {

            /* the real value of n! :) */
            n += 1;

            mat eq = DALLOCMAT(n,n);
            mat ac = DALLOCMAT(n,1);
            mat ei = DALLOCMAT(n,n);
            mat co = DALLOCMAT(n,1);

            matzro(eq);
            matzro(ac);

            for (size_t i = 0; i < nmemb; ++i) {

               for (size_t j = 0; j < n; ++j) {

                  for (size_t k = 0; k < n; ++k) {

                     eq->vals[k + j * n] += pow(*x,j+k);

                  }

                  ac->vals[j] += *y * pow(*x,j);
               }

               ++x;
               ++y;
            }

            matmul(matinv(eq,ei),ac,co);

            for (size_t i = 0; i < n; ++i)
               a[i] = co->vals[i];

            free(eq);
            free(ac);
            free(ei);
            free(co);
         }
      break;
   }
}
