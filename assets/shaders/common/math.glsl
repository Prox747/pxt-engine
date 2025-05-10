#ifndef _MATH_
#define _MATH_

#define PI 3.14159265359

#define FLT_EPSILON 1e-5

#define saturate(x) clamp(x, 0.0, 1.0)


/**
 * Computes x^2 as a single multiplication.
 */
float pow2(float x) {
    return x * x;
}


/**
 * Computes x^5 using only multiply operations.
 */
float pow5(float x) {
    float x2 = x * x;
    return x2 * x2 * x;
}

#endif