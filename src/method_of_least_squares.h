
/* ported from https://github.com/qeftser/tang */

/* contains the least-squares filters presented in chapter
 * two of the text. All of these methods are rolled into
 * a single callable function, which is pretty convinient,
 * but also have their own individual functions that can be
 * used. Timestep and measurement data are fed in as x and y,
 * and the coefficents are returned as output               */

#ifndef __METHOD_OF_LEAST_SQUARES__

#define __METHOD_OF_LEAST_SQUARES__
#include "numerical_basics.h"

/* compute the zero order least squares fit of the given data. x is an array of sampling
 * times, y is an array of measurements that corresponds to each sampling time, and nmemb
 * is the number of measurements collected, the length of each array. 
 *
 * result is returned in a0
 */
void zero_order_least_squares_filter(double * x, double * y, size_t nmemb, double * a0);

/* compute the first order least squares fit of the given data. x is an array of sampling
 * times, y is an array of measurements that corresponds to each sampling time, and nmemb
 * is the number of measurements collected, the length of each array.
 *
 * result is returned in a0 and a1
 */
void first_order_least_squares_filter(double * x, double * y, size_t nmemb, double * a0, double * a1);

/* compute the second order least squares fit of the given data. x is an array of sampling
 * times, y is an array of measurements that corresponds to each sampling time, and nmemb
 * is the number of measurements collected, the length of each array.
 *
 * results are returned in a0, a1, and a2
 */
void second_order_least_squares_filter(double * x, double * y, size_t nmemb, double * a0, double * a1, double * a2);

/* compute the n order least squares fit of the given data. x is an array of sampling
 * times, y is an array of measurements that corresponds to each sampling time, and nmemb
 * is the number of measurements collected, the length of each array.
 *
 * n is the degree of polynomial to fit with. Results are returned in a, which should be an 
 * array of length n+1, where each element corresponds to the given coefficient of the
 * polynomial.
 */
void least_squares_filter(double * x, double * y, size_t nmemb, double * a, size_t n);

#endif
