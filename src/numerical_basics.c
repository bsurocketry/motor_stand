
/* ported from https://github.com/qeftser/tang */

#include "numerical_basics.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

mat _alloc_mat(uint32_t m, uint32_t n, size_t size) {
   mat matrix = malloc(size);
   ((matbase *)matrix)->rows = m;
   ((matbase *)matrix)->cols = n;
   return matrix;
}

mat matadd(mat l, mat r, mat res) {

   assert(l->rows == r->rows && l->cols == r->cols && "matrix dimensions must match");
   assert(l->rows == res->rows && l->cols == res->cols && "matrix dimensions must match");

   for (uint32_t m = 0; m < l->rows; ++m) {
      for (uint32_t n = 0; n < l->cols; ++n) {

         uint32_t pos = n + m * l->cols;

         res->vals[pos] = l->vals[pos] + r->vals[pos];

      }
   }

   return res;
}

mat matsub(mat l, mat r, mat res) {

   assert(l->rows == r->rows && l->cols == r->cols && "matrix dimensions must match");
   assert(l->rows == res->rows && l->cols == res->cols && "matrix dimensions must match");

   for (uint32_t m = 0; m < l->rows; ++m) {
      for (uint32_t n = 0; n < l->cols; ++n) {

         uint32_t pos = n + m * l->cols;

         res->vals[pos] = l->vals[pos] - r->vals[pos];

      }
   }

   return res;
}

mat mattrn(mat a, mat at) {

   assert(a->rows == at->cols && a->cols == at->rows && "matrix a rows must match at cols and matrix a cols must match at rows");
   assert(a != at && "matrix at and a must be different, see mattrn_inplace for in-place transposition");

   for (uint32_t m = 0; m < a->rows; ++m) {
      for (uint32_t n = 0; n < a->cols; ++n) {

         uint32_t pos_a  = n + m * a->cols;
         uint32_t pos_at = m + n * a->rows;

         at->vals[pos_at] = a->vals[pos_a];

      }
   }

   return at;
}

mat mattrn_inplace(mat a) {

   assert(a->rows == a->cols && "matrix a must be square");

   mat ad = DALLOCMAT(a->rows,a->cols);

   mattrn(a,ad);
   matdup(ad,a);

   free(ad);

   return a;
}

mat matmul(mat l, mat r, mat res) {

   assert(l->cols == r->rows && "matrix l's column count must match matrix r's row count");
   assert(l->rows == res->rows && r->cols == res->cols && "matrix res's columns must match l's columns and it's rows must match r's rows");
   assert(l != res && r != res && "res cannot be l or r");

   for (uint32_t i = 0; i < l->rows; ++i) {
      for (uint32_t j = 0; j < r->cols; ++j) {

         double sum = 0.0;

         for (uint32_t k = 0; k < l->cols; ++k) {

            sum += l->vals[k + i * l->cols] * r->vals[j + k * r->cols];

         }

         res->vals[j + i * r->cols] = sum;

      }
   }

   return res;
}

mat matsca(mat a, mat as, double scalar) {

   assert(a->rows == as->rows && a->cols == as->cols && "matrix dimensions must match");

   for (uint32_t m = 0; m < a->rows; ++m) {
      for (uint32_t n = 0; n < a->cols; ++n) {

         as->vals[n + m * a->cols] = a->vals[n + m * a->cols] * scalar;

      }
   }

   return as;
}

mat matidn(mat a) {

   for (uint32_t m = 0; m < a->rows; ++m) {
      for (uint32_t n = 0; n < a->cols; ++n) {

         if (m == n)
            a->vals[n + m * a->cols] = 1.0;
         else
            a->vals[n + m * a->cols] = 0.0;

      }
   }

   return a;
}

mat matzro(mat a) {

   for (uint32_t m = 0; m < a->rows; ++m) {
      for (uint32_t n = 0; n < a->cols; ++n) {

         a->vals[n + m * a->cols] = 0.0;

      }
   }

   return a;
}

mat matinv(mat a, mat ainv) {

   int not_singular = 0;

   matinv_safe(a,ainv,&not_singular);

   assert(not_singular == 1 && "matrix cannot be singular, see matinv_safe instead");

   return ainv;
}

mat _matinv_inplace(mat a) {

   mat ad = DALLOCMAT(a->rows,a->cols);

   matinv(a,ad);
   matdup(ad,a);

   free(ad);

   return a;
}

mat matinv_safe(mat _a, mat ainv, int * flag) {

   *flag = 0;

   assert(_a->rows == ainv->rows && _a->cols == ainv->cols && "matrix dimensions must match");
   assert(_a != ainv && "matrix a and ainv cannot be the same, see matinv_inplace instead");
   assert(_a->rows == _a->cols && "matrix must be square");

   switch (_a->rows) {
      case 1:
         if (_a->vals[0] == 0.0)
            return NULL;
         ainv->vals[0] = 1.0 / _a->vals[0];
      break;

      case 2:
      {

         double div = _a->vals[0] * _a->vals[3] - _a->vals[1] * _a->vals[2];
         if (div == 0.0)
            return NULL;

         ainv->vals[0] =  _a->vals[3] / div;
         ainv->vals[1] = -_a->vals[1] / div;
         ainv->vals[2] = -_a->vals[2] / div;
         ainv->vals[3] =  _a->vals[0] / div;

      }
      break;
      
      case 3:
      {

         double a = _a->vals[0],
                b = _a->vals[1],
                c = _a->vals[2],
                d = _a->vals[3],
                e = _a->vals[4],
                f = _a->vals[5],
                g = _a->vals[6],
                h = _a->vals[7],
                i = _a->vals[8];

         double div = a*e*i + b*f*g + c*d*h - c*e*g - b*d*i - a*f*h;
         if (div == 0.0)
            return NULL;

         ainv->vals[0] = (e*i - f*h) / div;
         ainv->vals[1] = (c*h - b*i) / div;
         ainv->vals[2] = (b*f - e*c) / div;
         ainv->vals[3] = (g*f - d*i) / div;
         ainv->vals[4] = (a*i - g*c) / div;
         ainv->vals[5] = (d*c - a*f) / div;
         ainv->vals[6] = (d*h - g*e) / div;
         ainv->vals[7] = (g*b - a*h) / div;
         ainv->vals[8] = (a*e - b*d) / div;

      }
      break;

      default:
      {

         mat lu = DALLOCMAT(_a->rows,_a->cols);
         matdecomp dcmp;
         dcmp.m = lu;
         dcmp.indx = malloc(sizeof(int)*_a->rows);

         *flag = _lu_decomp_mat(_a,&dcmp);

         if (*flag == 0) {
            free(lu);
            free(dcmp.indx);
            return NULL;
         }

         matidn(ainv);

         double * xx = malloc(sizeof(double)*_a->rows);
         for (uint32_t j = 0; j < _a->rows; ++j) {
            for (uint32_t i = 0; i < _a->rows; ++i)
               xx[i] = ainv->vals[j + i * _a->rows];
            _solve_mat(&dcmp,xx,xx);
            for (uint32_t i = 0; i < _a->rows; ++i)
               ainv->vals[j + i * _a->rows] = xx[i];
         }

         free(lu);
         free(dcmp.indx);
         free(xx);
      }
      break;

   }

   *flag = 1;
   return ainv;
}

int _lu_decomp_mat(mat a, matdecomp * lu) {

   assert(a->rows == a->cols && "matrix must be square");

   if (a != lu->m) {

      assert(a->rows == lu->m->rows && a->cols == lu->m->cols && "matrix dimensions must match");

      matdup(a,lu->m);

   }

   uint32_t i, imax, j, k;
   double big, temp;
   lu->d = 1.0;

   double * vv = malloc(sizeof(double)*a->rows);

   for (i = 0; i < a->rows; ++i) {
      big = 0.0;

      for (j = 0; j < a->rows; ++j) 
         if ((temp = fabs(lu->m->vals[j + i * a->rows])) > big)
            big = temp;

      if (big == 0.0) {
         free(vv);
         return 0;
      }

      vv[i] = 1.0 / big;
   }

   for (k = 0; k < a->rows; ++k) {
      big = 0.0;

      imax = k;
      for (i = k; i < a->rows; ++i) {
         temp = vv[i] * fabs(lu->m->vals[k + i * a->rows]);
         if (temp > big) {
            big = temp;
            imax = i;
         }
      }

      if (k != imax) {
         for (j = 0; j < a->rows; ++j) {
            temp = lu->m->vals[j + imax * a->rows];
            lu->m->vals[j + imax * a->rows] = lu->m->vals[j + k * a->rows];
            lu->m->vals[j + k * a->rows] = temp;
         }

         lu->d = -lu->d;
         vv[imax] = vv[k];
      }

      lu->indx[k] = imax;

      if (lu->m->vals[k + k * a->rows] == 0.0) {
         free(vv);
         return 0;
      }

      for (i = k + 1; i < a->rows; ++i) {
         temp = lu->m->vals[k + i * a->rows] /= lu->m->vals[k + k * a->rows];
         for (j = k + 1; j < a->rows; ++j)
            lu->m->vals[j + i * a->rows] -= temp * lu->m->vals[j + k * a->rows];
      }

   }

   free(vv);

   return 1;
}

void _solve_mat(matdecomp * lu, double * x, double * b) {

   int32_t i, ii = 0, ip, j;
   double sum;

   for (i = 0; i < lu->m->rows; ++i) x[i] = b[i];
   for (i = 0; i < lu->m->rows; ++i) {

      ip = lu->indx[i];
      sum = x[ip];
      x[ip] = x[i];

      if (ii != 0)
         for (j = ii - 1; j < i; ++j)
            sum -= lu->m->vals[j + i * lu->m->rows] * x[j];
      else if (sum != 0.0)
         ii = i + 1;
      x[i] = sum;
   }

   for (i = lu->m->rows - 1; i >= 0; --i) {
      sum = x[i];
      for (j = i + 1; j < lu->m->rows; ++j)
         sum -= lu->m->vals[j + i * lu->m->rows] * x[j];
      x[i] = sum / lu->m->vals[i + i * lu->m->rows];
   }
}

mat matdup(mat a, mat ad) {

   assert(a->rows == ad->rows && a->cols == ad->cols && "matrix dimensions must match");

   for (uint32_t m = 0; m < a->rows; ++m) {
      for (uint32_t n = 0; n < a->cols; ++n) {

         uint32_t pos = n + m * a->cols;

         ad->vals[pos] = a->vals[pos];

      }
   }

   return ad;
}

double matdet(mat a) {

   if (a->rows != a->cols)
      return 0.0;
   
   switch (a->rows) {
      case 0:
         return 0.0;

      case 1:
         return a->vals[0];

      case 2:
         return a->vals[0] * a->vals[3] - a->vals[1] * a->vals[2];

      case 3:
         {
            double aa = a->vals[0],
                   b  = a->vals[1],
                   c  = a->vals[2],
                   d  = a->vals[3],
                   e  = a->vals[4],
                   f  = a->vals[5],
                   g  = a->vals[6],
                   h  = a->vals[7],
                   i  = a->vals[8];

            return aa*e*i + b*f*g + c*d*h - c*e*g - b*d*i - aa*f*h;
         }

      default:
         break;
   }

   mat lu = DALLOCMAT(a->rows,a->cols);
   matdecomp dcmp;
   dcmp.m = lu;
   dcmp.indx = malloc(sizeof(double)*a->rows);

   if (_lu_decomp_mat(a,&dcmp) == 0) {
      free(lu);
      free(dcmp.indx);
      return 0.0;
   }

   double dd = dcmp.d;
   for (uint32_t i = 0; i < a->rows; ++i)
      dd *= lu->vals[i + i * a->rows];

   free(lu);
   free(dcmp.indx);

   return dd;
}

mat matshw(mat a) {

   for (uint32_t m = 0; m < a->rows; ++m) {

      printf("[ ");

      for (uint32_t n = 0; n < a->cols; ++n) {

         printf("%+010e ",a->vals[n + m * a->cols]);

      }

      printf("]\n");
   }

   return a;
}

double frand() {
   return ((double)rand() / RAND_MAX);
}

double grand(double mu, double sigma) {
   double v1, v2, rsq;
   do {
      v1 = 2.0 * frand() - 1.0;
      v2 = 2.0 * frand() - 1.0;
      rsq = v1 * v1 + v2 * v2;
   } while (rsq >= 1.0 || rsq == 0.0);

   return mu + sigma * v2 * sqrt(-2.0 * log(rsq) / rsq);
}

double wnoise(double psd, double h) {
   return grand(0.0,sqrt(psd / h));
}
