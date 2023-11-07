/*
 * MIT License
 *
 * Copyright (c) 2020-2021 Jaime Ita Passos
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef _LZML_H_
#define _LZML_H_

#include <math.h>

void lzml_vector3_add(float a[3], float b[3]);
void lzml_vector3_add_by_scalar(float vec[3], float scalar);
void lzml_vector3_subtract(float a[3], float b[3]);
void lzml_vector3_subtract_by_scalar(float vec[3], float scalar);
void lzml_vector3_multiply(float a[3], float b[3]);
void lzml_vector3_multiply_by_scalar(float vec[3], float scalar);
#define lzml_vector3_scale lzml_vector3_multiply_by_scalar
void lzml_vector3_divide(float a[3], float b[3]);
void lzml_vector3_divide_by_scalar(float vec[3], float scalar);
void lzml_vector3_add_into(float dest[3], float a[3], float b[3]);
void lzml_vector3_add_by_scalar_into(float dest[3], float vec[3], float scalar);
void lzml_vector3_subtract_into(float dest[3], float a[3], float b[3]);
void lzml_vector3_subtract_by_scalar_into(float dest[3], float vec[3], float scalar);
void lzml_vector3_multiply_into(float dest[3], float a[3], float b[3]);
void lzml_vector3_multiply_by_scalar_into(float dest[3], float vec[3], float scalar);
#define lzml_vector3_scale_into lzml_vector3_multiply_by_scalar_into
void lzml_vector3_divide_into(float dest[3], float a[3], float b[3]);
void lzml_vector3_divide_by_scalar_into(float dest[3], float vec[3], float scalar);
float lzml_vector3_dot_product(float a[3], float b[3]);
float lzml_vector3_magnitude(float vec[3]);
void lzml_vector3_normalize(float vec[3]);
void lzml_vector3_normalize_into(float dest[3], float vec[3]);
void lzml_vector3_copy(float dest[3], float src[3]);

void lzml_vector4_add(float a[4], float b[4]);
void lzml_vector4_add_by_scalar(float vec[4], float scalar);
void lzml_vector4_subtract(float a[4], float b[4]);
void lzml_vector4_subtract_by_scalar(float vec[4], float scalar);
void lzml_vector4_multiply(float a[4], float b[4]);
void lzml_vector4_multiply_by_scalar(float vec[4], float scalar);
#define lzml_vector4_scale lzml_vector4_multiply_by_scalar
void lzml_vector4_divide(float a[4], float b[4]);
void lzml_vector4_divide_by_scalar(float vec[4], float scalar);
void lzml_vector4_add_into(float dest[4], float a[4], float b[4]);
void lzml_vector4_add_by_scalar_into(float dest[4], float vec[4], float scalar);
void lzml_vector4_subtract_into(float dest[4], float a[4], float b[4]);
void lzml_vector4_subtract_by_scalar_into(float dest[4], float vec[4], float scalar);
void lzml_vector4_multiply_into(float dest[4], float a[4], float b[4]);
void lzml_vector4_multiply_by_scalar_into(float dest[4], float vec[4], float scalar);
#define lzml_vector4_scale_into lzml_vector4_multiply_by_scalar_into
void lzml_vector4_divide_into(float dest[4], float a[4], float b[4]);
void lzml_vector4_divide_by_scalar_into(float dest[4], float vec[4], float scalar);
float lzml_vector4_dot_product(float a[4], float b[4]);
float lzml_vector4_magnitude(float vec[4]);
void lzml_vector4_normalize(float vec[4]);
void lzml_vector4_normalize_into(float dest[4], float vec[4]);
void lzml_vector4_copy(float dest[4], float src[4]);

static float lzml_zeromatrix[4][4] = {
    {0.0f, 0.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.0f, 0.0f}};

static float lzml_identitymatrix[4][4] = {
    {1.0f, 0.0f, 0.0f, 0.0f},
    {0.0f, 1.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 1.0f, 0.0f},
    {0.0f, 0.0f, 0.0f, 1.0f}};

void lzml_matrix4_copy(float dest[4][4], float src[4][4]);
void lzml_matrix4_clear(float mat[4][4]);
void lzml_matrix4_identity(float mat[4][4]);
void lzml_matrix4_multiply(float a[4][4], float b[4][4]);
void lzml_matrix4_multiply_into(float dest[4][4], float a[4][4], float b[4][4]);
void lzml_matrix4_translate(float mat[4][4], float vec[3]);
void lzml_matrix4_translate_x(float mat[4][4], float x);
void lzml_matrix4_translate_y(float mat[4][4], float y);
void lzml_matrix4_translate_z(float mat[4][4], float z);
void lzml_matrix4_rotate_x(float mat[4][4], float angle);
void lzml_matrix4_rotate_y(float mat[4][4], float angle);
void lzml_matrix4_rotate_z(float mat[4][4], float angle);
void lzml_matrix4_rotate_by_vector(float mat[4][4], float vec[3], float angle);
void lzml_matrix4_scale(float mat[4][4], float vec[3]);
void lzml_matrix4_rotation(float rot[4][4], float vec[3], float angle);
void lzml_matrix4_perspective(float mat[4][4], float fovy, float aspect_ratio, float near_clip, float far_clip);

/**
    == VECTOR3 OPERATIONS ==
**/

/** Adds a vector by another.
  *
  * \param a The first vector.
  * \param b The second vector.
  */
void lzml_vector3_add(float a[3], float b[3])
{
    lzml_vector3_add_into(a, a, b);
}

/** Adds a vector by a scalar.
  *
  * \param vec The destination vector.
  * \param scalar The scalar.
  */
void lzml_vector3_add_by_scalar(float vec[3], float scalar)
{
    lzml_vector3_add_by_scalar_into(vec, vec, scalar);
}

/** Subtracts a vector by another.
  *
  * \param a The first vector.
  * \param b The second vector.
  */
void lzml_vector3_subtract(float a[3], float b[3])
{
    lzml_vector3_subtract_into(a, a, b);
}

/** Subtracts a vector by a scalar.
  *
  * \param vec The destination vector.
  * \param scalar The scalar.
  */
void lzml_vector3_subtract_by_scalar(float vec[3], float scalar)
{
    lzml_vector3_subtract_by_scalar_into(vec, vec, scalar);
}

/** Multiplies a vector by another.
  *
  * \param a The first vector.
  * \param b The second vector.
  */
void lzml_vector3_multiply(float a[3], float b[3])
{
    lzml_vector3_multiply_into(a, a, b);
}

/** Multiplies a vector by a scalar.
  *
  * \param vec The destination vector.
  * \param scalar The scalar.
  */
void lzml_vector3_multiply_by_scalar(float vec[3], float scalar)
{
    lzml_vector3_multiply_by_scalar_into(vec, vec, scalar);
}

/** Divides a vector by another.
  *
  * \param a The first vector.
  * \param b The second vector.
  */
void lzml_vector3_divide(float a[3], float b[3])
{
    lzml_vector3_divide_into(a, a, b);
}

/** Divides a vector by a scalar.
  *
  * \param vec The destination vector.
  * \param scalar The scalar.
  */
void lzml_vector3_divide_by_scalar(float vec[3], float scalar)
{
    lzml_vector3_divide_by_scalar_into(vec, vec, scalar);
}

/** Adds a vector by another, and stores the result in dest.
  *
  * \param dest The destination vector.
  * \param a The first vector.
  * \param b The second vector.
  */
void lzml_vector3_add_into(float dest[3], float a[3], float b[3])
{
    dest[0] = a[0] + b[0];
    dest[1] = a[1] + b[1];
    dest[2] = a[2] + b[2];
}

/** Adds a vector by a scalar, and stores the result in dest.
  *
  * \param dest The destination vector.
  * \param vec The vector.
  * \param scalar The scalar.
  */
void lzml_vector3_add_by_scalar_into(float dest[3], float vec[3], float scalar)
{
    dest[0] = vec[0] + scalar;
    dest[1] = vec[1] + scalar;
    dest[2] = vec[2] + scalar;
}

/** Subtracts a vector by another, and stores the result in dest.
  *
  * \param dest The destination vector.
  * \param a The first vector.
  * \param b The second vector.
  */
void lzml_vector3_subtract_into(float dest[3], float a[3], float b[3])
{
    dest[0] = a[0] - b[0];
    dest[1] = a[1] - b[1];
    dest[2] = a[2] - b[2];
}

/** Subtracts a vector by a scalar, and stores the result in dest.
  *
  * \param dest The destination vector.
  * \param vec The vector.
  * \param scalar The scalar.
  */
void lzml_vector3_subtract_by_scalar_into(float dest[3], float vec[3], float scalar)
{
    dest[0] = vec[0] - scalar;
    dest[1] = vec[1] - scalar;
    dest[2] = vec[2] - scalar;
}

/** Multiplies a vector by another, and stores the result in dest.
  *
  * \param dest The destination vector.
  * \param a The first vector.
  * \param b The second vector.
  */
void lzml_vector3_multiply_into(float dest[3], float a[3], float b[3])
{
    dest[0] = a[0] * b[0];
    dest[1] = a[1] * b[1];
    dest[2] = a[2] * b[2];
}

/** Multiplies a vector by a scalar, and stores the result in dest.
  *
  * \param dest The destination vector.
  * \param vec The vector.
  * \param scalar The scalar.
  */
void lzml_vector3_multiply_by_scalar_into(float dest[3], float vec[3], float scalar)
{
    dest[0] = vec[0] * scalar;
    dest[1] = vec[1] * scalar;
    dest[2] = vec[2] * scalar;
}

/** Divides a vector by another, and stores the result in dest.
  *
  * \param dest The destination vector.
  * \param a The first vector.
  * \param b The second vector.
  */
void lzml_vector3_divide_into(float dest[3], float a[3], float b[3])
{
    dest[0] = a[0] / b[0];
    dest[1] = a[1] / b[1];
    dest[2] = a[2] / b[2];
}

/** Divides a vector by a scalar, and stores the result in dest.
  *
  * \param dest The destination vector.
  * \param vec The vector.
  * \param scalar The scalar.
  */
void lzml_vector3_divide_by_scalar_into(float dest[3], float vec[3], float scalar)
{
    lzml_vector3_multiply_by_scalar_into(dest, vec, (1.0f / scalar));
}

/** Calculates a dot product.
  *
  * \param a The first vector.
  * \param b The second vector.
  */
float lzml_vector3_dot_product(float a[3], float b[3])
{
    return (a[0] * b[0]) + (a[1] * b[1]) + (a[2] * b[2]);
}

/** Calculates the magnitude of a vector.
  *
  * \param vec The vector.
  */
float lzml_vector3_magnitude(float vec[3])
{
    return sqrtf(lzml_vector3_dot_product(vec, vec));
}

/** Normalizes a vector.
  *
  * \param vec The vector to be normalized.
  */
void lzml_vector3_normalize(float vec[3])
{
    lzml_vector3_normalize_into(vec, vec);
}

/** Normalizes a vector, and stores the result in dest.
  *
  * \param dest The destination vector.
  * \param vec The vector to be normalized.
  */
void lzml_vector3_normalize_into(float dest[3], float vec[3])
{
    float normal = lzml_vector3_magnitude(vec);

    if (fpclassify(normal) == FP_ZERO)
        dest[0] = dest[1] = dest[2] = 0.0f;
    else
        lzml_vector3_multiply_by_scalar_into(dest, vec, (1.0f / normal));
}

/** Copies a vector into another.
  *
  * \param dest The destination vector.
  * \param src The source vector.
  */
void lzml_vector3_copy(float dest[3], float src[3])
{
    dest[0] = src[0];
    dest[1] = src[1];
    dest[2] = src[2];
}

/**
    == VECTOR4 OPERATIONS ==
**/

/** Adds a vector by another.
  *
  * \param a The first vector.
  * \param b The second vector.
  */
void lzml_vector4_add(float a[4], float b[4])
{
    lzml_vector4_add_into(a, a, b);
}

/** Adds a vector by a scalar.
  *
  * \param vec The destination vector.
  * \param scalar The scalar.
  */
void lzml_vector4_add_by_scalar(float vec[4], float scalar)
{
    lzml_vector4_add_by_scalar_into(vec, vec, scalar);
}

/** Subtracts a vector by another.
  *
  * \param a The first vector.
  * \param b The second vector.
  */
void lzml_vector4_subtract(float a[4], float b[4])
{
    lzml_vector4_subtract_into(a, a, b);
}

/** Subtracts a vector by a scalar.
  *
  * \param vec The destination vector.
  * \param scalar The scalar.
  */
void lzml_vector4_subtract_by_scalar(float vec[4], float scalar)
{
    lzml_vector4_subtract_by_scalar_into(vec, vec, scalar);
}

/** Multiplies a vector by another.
  *
  * \param a The first vector.
  * \param b The second vector.
  */
void lzml_vector4_multiply(float a[4], float b[4])
{
    lzml_vector4_multiply_into(a, a, b);
}

/** Multiplies a vector by a scalar.
  *
  * \param vec The destination vector.
  * \param scalar The scalar.
  */
void lzml_vector4_multiply_by_scalar(float vec[4], float scalar)
{
    lzml_vector4_multiply_by_scalar_into(vec, vec, scalar);
}

/** Divides a vector by another.
  *
  * \param a The first vector.
  * \param b The second vector.
  */
void lzml_vector4_divide(float a[4], float b[4])
{
    lzml_vector4_divide_into(a, a, b);
}

/** Divides a vector by a scalar.
  *
  * \param vec The destination vector.
  * \param scalar The scalar.
  */
void lzml_vector4_divide_by_scalar(float vec[4], float scalar)
{
    lzml_vector4_multiply_by_scalar_into(vec, vec, scalar);
}

/** Adds a vector by another, and stores the result in dest.
  *
  * \param dest The destination vector.
  * \param a The first vector.
  * \param b The second vector.
  */
void lzml_vector4_add_into(float dest[4], float a[4], float b[4])
{
    dest[0] = a[0] + b[0];
    dest[1] = a[1] + b[1];
    dest[2] = a[2] + b[2];
    dest[3] = a[3] + b[3];
}

/** Adds a vector by a scalar, and stores the result in dest.
  *
  * \param dest The destination vector.
  * \param vec The vector.
  * \param scalar The scalar.
  */
void lzml_vector4_add_by_scalar_into(float dest[4], float vec[4], float scalar)
{
    dest[0] = vec[0] + scalar;
    dest[1] = vec[1] + scalar;
    dest[2] = vec[2] + scalar;
    dest[3] = vec[3] + scalar;
}

/** Subtracts a vector by another, and stores the result in dest.
  *
  * \param dest The destination vector.
  * \param a The first vector.
  * \param b The second vector.
  */
void lzml_vector4_subtract_into(float dest[4], float a[4], float b[4])
{
    dest[0] = a[0] - b[0];
    dest[1] = a[1] - b[1];
    dest[2] = a[2] - b[2];
    dest[3] = a[3] - b[3];
}

/** Subtracts a vector by a scalar, and stores the result in dest.
  *
  * \param dest The destination vector.
  * \param vec The vector.
  * \param scalar The scalar.
  */
void lzml_vector4_subtract_by_scalar_into(float dest[4], float vec[4], float scalar)
{
    dest[0] = vec[0] - scalar;
    dest[1] = vec[1] - scalar;
    dest[2] = vec[2] - scalar;
    dest[3] = vec[3] - scalar;
}

/** Multiplies a vector by another, and stores the result in dest.
  *
  * \param dest The destination vector.
  * \param a The first vector.
  * \param b The second vector.
  */
void lzml_vector4_multiply_into(float dest[4], float a[4], float b[4])
{
    dest[0] = a[0] * b[0];
    dest[1] = a[1] * b[1];
    dest[2] = a[2] * b[2];
    dest[3] = a[3] * b[3];
}

/** Multiplies a vector by a scalar, and stores the result in dest.
  *
  * \param dest The destination vector.
  * \param vec The vector.
  * \param scalar The scalar.
  */
void lzml_vector4_multiply_by_scalar_into(float dest[4], float vec[4], float scalar)
{
    dest[0] = vec[0] * scalar;
    dest[1] = vec[1] * scalar;
    dest[2] = vec[2] * scalar;
    dest[3] = vec[3] * scalar;
}

/** Divides a vector by another, and stores the result in dest.
  *
  * \param dest The destination vector.
  * \param a The first vector.
  * \param b The second vector.
  */
void lzml_vector4_divide_into(float dest[4], float a[4], float b[4])
{
    dest[0] = a[0] / b[0];
    dest[1] = a[1] / b[1];
    dest[2] = a[2] / b[2];
    dest[3] = a[3] / b[3];
}

/** Divides a vector by a scalar, and stores the result in dest.
  *
  * \param dest The destination vector.
  * \param vec The vector.
  * \param scalar The scalar.
  */
void lzml_vector4_divide_by_scalar_into(float dest[4], float vec[4], float scalar)
{
    lzml_vector4_multiply_by_scalar_into(dest, vec, (1.0f / scalar));
}

/** Calculates a dot product.
  *
  * \param a The first vector.
  * \param b The second vector.
  */
float lzml_vector4_dot_product(float a[4], float b[4])
{
    return (a[0] * b[0]) + (a[1] * b[1]) + (a[2] * b[2]) + (a[3] * b[3]);
}

/** Calculates the magnitude of a vector.
  *
  * \param vec The vector.
  */
float lzml_vector4_magnitude(float vec[4])
{
    return sqrtf(lzml_vector4_dot_product(vec, vec));
}

/** Normalizes a vector.
  *
  * \param vec The vector to be normalized.
  */
void lzml_vector4_normalize(float vec[4])
{
    lzml_vector4_normalize_into(vec, vec);
}

/** Normalizes a vector, and stores the result in dest.
  *
  * \param dest The destination vector.
  * \param vec The vector to be normalized.
  */
void lzml_vector4_normalize_into(float dest[4], float vec[4])
{
    float normal = lzml_vector4_magnitude(vec);

    if (fpclassify(normal) == FP_ZERO)
        dest[0] = dest[1] = dest[2] = dest[3] = 0.0f;
    else
        lzml_vector4_multiply_by_scalar_into(dest, vec, (1.0f / normal));
}

/** Copies a vector into another.
  *
  * \param dest The destination vector.
  * \param src The source vector.
  */
void lzml_vector4_copy(float dest[4], float src[4])
{
    dest[0] = src[0];
    dest[1] = src[1];
    dest[2] = src[2];
    dest[3] = src[3];
}

/**
    == MATRIX OPERATIONS ==
**/

/** Copies a matrix into another.
  *
  * \param dest The destination matrix.
  * \param src The source matrix.
  */
void lzml_matrix4_copy(float dest[4][4], float src[4][4])
{
    dest[0][0] = src[0][0];
    dest[0][1] = src[0][1];
    dest[0][2] = src[0][2];
    dest[0][3] = src[0][3];
    dest[1][0] = src[1][0];
    dest[1][1] = src[1][1];
    dest[1][2] = src[1][2];
    dest[1][3] = src[1][3];
    dest[2][0] = src[2][0];
    dest[2][1] = src[2][1];
    dest[2][2] = src[2][2];
    dest[2][3] = src[2][3];
    dest[3][0] = src[3][0];
    dest[3][1] = src[3][1];
    dest[3][2] = src[3][2];
    dest[3][3] = src[3][3];
}

/** Clears a matrix.
  *
  * \param mat The matrix to be cleared.
  */
void lzml_matrix4_clear(float mat[4][4])
{
    lzml_matrix4_copy(mat, lzml_zeromatrix);
}

/** Makes an identity matrix.
  *
  * \param mat The destination matrix.
  */
void lzml_matrix4_identity(float mat[4][4])
{
    lzml_matrix4_copy(mat, lzml_identitymatrix);
}

/** Multiplies a matrix by another, and stores the result into the first matrix.
  *
  * \param a The first matrix.
  * \param b The second matrix.
  */
void lzml_matrix4_multiply(float a[4][4], float b[4][4])
{
    float mul[4][4];
    lzml_matrix4_multiply_into(mul, a, b);
    lzml_matrix4_copy(a, mul);
}

/** Multiplies a matrix by another, and stores the result into the destination.
  *
  * \param dest The destination matrix.
  * \param a The first matrix.
  * \param b The second matrix.
  */
void lzml_matrix4_multiply_into(float dest[4][4], float a[4][4], float b[4][4])
{
#define MULTMAT(i, j) dest[i][j] = (a[0][j] * b[i][0]) + (a[1][j] * b[i][1]) + (a[2][j] * b[i][2]) + (a[3][j] * b[i][3]);
    MULTMAT(0, 0)
    MULTMAT(0, 1)
    MULTMAT(0, 2)
    MULTMAT(0, 3)
    MULTMAT(1, 0)
    MULTMAT(1, 1)
    MULTMAT(1, 2)
    MULTMAT(1, 3)
    MULTMAT(2, 0)
    MULTMAT(2, 1)
    MULTMAT(2, 2)
    MULTMAT(2, 3)
    MULTMAT(3, 0)
    MULTMAT(3, 1)
    MULTMAT(3, 2)
    MULTMAT(3, 3)
#undef MULTMAT
}

/** Translates a matrix in the X axis.
  *
  * \param mat The matrix.
  * \param x The X translation.
  */
void lzml_matrix4_translate_x(float mat[4][4], float x)
{
    float vec[4];
    lzml_vector4_multiply_by_scalar_into(vec, mat[0], x);
    lzml_vector4_add(mat[3], vec);
}

/** Translates a matrix in the Y axis.
  *
  * \param mat The matrix.
  * \param y The Y translation.
  */
void lzml_matrix4_translate_y(float mat[4][4], float y)
{
    float vec[4];
    lzml_vector4_multiply_by_scalar_into(vec, mat[1], y);
    lzml_vector4_add(mat[3], vec);
}

/** Translates a matrix in the Z axis.
  *
  * \param mat The matrix.
  * \param z The Y translation.
  */
void lzml_matrix4_translate_z(float mat[4][4], float z)
{
    float vec[4];
    lzml_vector4_multiply_by_scalar_into(vec, mat[2], z);
    lzml_vector4_add(mat[3], vec);
}

/** Translates a matrix by a vector.
  *
  * \param mat The matrix.
  * \param vec The translation vector.
  */
void lzml_matrix4_translate(float mat[4][4], float vec[3])
{
    lzml_matrix4_translate_x(mat, vec[0]);
    lzml_matrix4_translate_y(mat, vec[1]);
    lzml_matrix4_translate_z(mat, vec[2]);
}

/** Rotates a matrix in the X axis.
  *
  * \param mat The matrix.
  * \param angle The rotation angle.
  */
void lzml_matrix4_rotate_x(float mat[4][4], float angle)
{
    float sine = sinf(angle);
    float cosine = cosf(angle);

    float rot[4][4];
    lzml_matrix4_identity(rot);

    rot[1][1] = cosine;
    rot[1][2] = sine;
    rot[2][1] = -sine;
    rot[2][2] = cosine;

    lzml_matrix4_multiply(mat, rot);
}

/** Rotates a matrix in the Y axis.
  *
  * \param mat The matrix.
  * \param angle The rotation angle.
  */
void lzml_matrix4_rotate_y(float mat[4][4], float angle)
{
    float sine = sinf(angle);
    float cosine = cosf(angle);

    float rot[4][4];
    lzml_matrix4_identity(rot);

    rot[0][0] = cosine;
    rot[0][2] = -sine;
    rot[2][0] = sine;
    rot[2][2] = cosine;

    lzml_matrix4_multiply(mat, rot);
}

/** Rotates a matrix in the Z axis.
  *
  * \param mat The matrix.
  * \param angle The rotation angle.
  */
void lzml_matrix4_rotate_z(float mat[4][4], float angle)
{
    float sine = sinf(angle);
    float cosine = cosf(angle);

    float rot[4][4];
    lzml_matrix4_identity(rot);

    rot[0][0] = cosine;
    rot[0][1] = sine;
    rot[1][0] = -sine;
    rot[1][1] = cosine;

    lzml_matrix4_multiply(mat, rot);
}

/** Rotates a matrix by an axis.
  *
  * \param mat The matrix.
  * \param vec The rotation vector.
  * \param angle The rotation angle.
  */
void lzml_matrix4_rotate_by_vector(float mat[4][4], float vec[3], float angle)
{
    float rot[4][4];
    lzml_matrix4_rotation(rot, vec, angle);
    lzml_matrix4_multiply(mat, rot);
}

/** Scales a matrix by a vector.
  *
  * \param mat The matrix.
  * \param vec The scaling vector.
  */
void lzml_matrix4_scale(float mat[4][4], float vec[3])
{
    lzml_vector4_multiply_by_scalar(mat[0], vec[0]);
    lzml_vector4_multiply_by_scalar(mat[1], vec[1]);
    lzml_vector4_multiply_by_scalar(mat[2], vec[2]);
}

/** Creates a rotation matrix.
  *
  * \param mat The destination rotation matrix.
  * \param vec The rotation axis.
  * \param angle The rotation angle.
  */
void lzml_matrix4_rotation(float rot[4][4], float vec[3], float angle)
{
    float sine = sinf(angle);
    float cosine = cosf(angle);
    float normalized[3];
    float x, y, z;

    lzml_vector3_normalize_into(normalized, vec);
    x = normalized[0];
    y = normalized[1];
    z = -normalized[2];

    rot[0][0] = cosine + x * x * (1.0f - cosine);
    rot[0][1] = x * y * (1.0f - cosine) - z * sine;
    rot[0][2] = x * z * (1.0f - cosine) + y * sine;
    rot[0][3] = 0.0f;

    rot[1][0] = y * x * (1.0f - cosine) + z * sine;
    rot[1][1] = cosine + y * y * (1.0f - cosine);
    rot[1][2] = y * z * (1.0f - cosine) - x * sine;
    rot[1][3] = 0.0f;

    rot[2][0] = z * x * (1.0f - cosine) - y * sine;
    rot[2][1] = z * y * (1.0f - cosine) + x * sine;
    rot[2][2] = cosine + z * z * (1.0f - cosine);
    rot[2][3] = 0.0f;

    rot[3][0] = 0.0f;
    rot[3][1] = 0.0f;
    rot[3][2] = 0.0f;
    rot[3][3] = 1.0f;
}

/** Makes a perspective matrix.
  *
  * \param mat The destination matrix.
  * \param fovy The field of view value, in radians.
  * \param aspect_ratio The aspect ratio.
  * \param near The near clipping plane value.
  * \param far The far clipping plane value.
  */
void lzml_matrix4_perspective(float mat[4][4], float fovy, float aspect_ratio, float near_clip, float far_clip)
{
    float tangent = 1.0f / tanf(fovy * 0.5f);
    float delta = (far_clip - near_clip);

    if (fpclassify(aspect_ratio) == FP_ZERO)
        return;

    lzml_matrix4_copy(mat, lzml_zeromatrix);

    mat[0][0] = tangent / aspect_ratio;
    mat[1][1] = tangent;
    mat[2][2] = -(far_clip / delta);
    mat[2][3] = -1.0f;
    mat[3][2] = -((far_clip * near_clip) / delta);
}

#endif
