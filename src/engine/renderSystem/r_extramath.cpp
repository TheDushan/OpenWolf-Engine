////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2011 James Canete (use.less01@gmail.com)
// Copyright(C) 2011 - 2021 Dusan Jocic <dusanjocic@msn.com>
//
// This file is part of OpenWolf.
//
// OpenWolf is free software; you can redistribute it
// and / or modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 3 of the License,
// or (at your option) any later version.
//
// OpenWolf is distributed in the hope that it will be
// useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OpenWolf; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110 - 1301  USA
//
// -------------------------------------------------------------------------------------
// File name:   r_extramath.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description: extra math needed by the renderer not in qmath.c
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <renderSystem/r_precompiled.hpp>

// Some matrix helper functions
// FIXME: do these already exist in ioq3 and I don't know about them?

void Mat4Zero(mat4_t out) {
    out[ 0] = 0.0f;
    out[ 4] = 0.0f;
    out[ 8] = 0.0f;
    out[12] = 0.0f;
    out[ 1] = 0.0f;
    out[ 5] = 0.0f;
    out[ 9] = 0.0f;
    out[13] = 0.0f;
    out[ 2] = 0.0f;
    out[ 6] = 0.0f;
    out[10] = 0.0f;
    out[14] = 0.0f;
    out[ 3] = 0.0f;
    out[ 7] = 0.0f;
    out[11] = 0.0f;
    out[15] = 0.0f;
}

void Mat4Identity(mat4_t out) {
    out[ 0] = 1.0f;
    out[ 4] = 0.0f;
    out[ 8] = 0.0f;
    out[12] = 0.0f;
    out[ 1] = 0.0f;
    out[ 5] = 1.0f;
    out[ 9] = 0.0f;
    out[13] = 0.0f;
    out[ 2] = 0.0f;
    out[ 6] = 0.0f;
    out[10] = 1.0f;
    out[14] = 0.0f;
    out[ 3] = 0.0f;
    out[ 7] = 0.0f;
    out[11] = 0.0f;
    out[15] = 1.0f;
}

void Mat4Copy(const mat4_t in, mat4_t out) {
    out[ 0] = in[ 0];
    out[ 4] = in[ 4];
    out[ 8] = in[ 8];
    out[12] = in[12];
    out[ 1] = in[ 1];
    out[ 5] = in[ 5];
    out[ 9] = in[ 9];
    out[13] = in[13];
    out[ 2] = in[ 2];
    out[ 6] = in[ 6];
    out[10] = in[10];
    out[14] = in[14];
    out[ 3] = in[ 3];
    out[ 7] = in[ 7];
    out[11] = in[11];
    out[15] = in[15];
}

void Mat4Multiply(const mat4_t in1, const mat4_t in2, mat4_t out) {
    out[ 0] = in1[ 0] * in2[ 0] + in1[ 4] * in2[ 1] + in1[ 8] * in2[ 2] +
              in1[12] * in2[ 3];
    out[ 1] = in1[ 1] * in2[ 0] + in1[ 5] * in2[ 1] + in1[ 9] * in2[ 2] +
              in1[13] * in2[ 3];
    out[ 2] = in1[ 2] * in2[ 0] + in1[ 6] * in2[ 1] + in1[10] * in2[ 2] +
              in1[14] * in2[ 3];
    out[ 3] = in1[ 3] * in2[ 0] + in1[ 7] * in2[ 1] + in1[11] * in2[ 2] +
              in1[15] * in2[ 3];

    out[ 4] = in1[ 0] * in2[ 4] + in1[ 4] * in2[ 5] + in1[ 8] * in2[ 6] +
              in1[12] * in2[ 7];
    out[ 5] = in1[ 1] * in2[ 4] + in1[ 5] * in2[ 5] + in1[ 9] * in2[ 6] +
              in1[13] * in2[ 7];
    out[ 6] = in1[ 2] * in2[ 4] + in1[ 6] * in2[ 5] + in1[10] * in2[ 6] +
              in1[14] * in2[ 7];
    out[ 7] = in1[ 3] * in2[ 4] + in1[ 7] * in2[ 5] + in1[11] * in2[ 6] +
              in1[15] * in2[ 7];

    out[ 8] = in1[ 0] * in2[ 8] + in1[ 4] * in2[ 9] + in1[ 8] * in2[10] +
              in1[12] * in2[11];
    out[ 9] = in1[ 1] * in2[ 8] + in1[ 5] * in2[ 9] + in1[ 9] * in2[10] +
              in1[13] * in2[11];
    out[10] = in1[ 2] * in2[ 8] + in1[ 6] * in2[ 9] + in1[10] * in2[10] +
              in1[14] * in2[11];
    out[11] = in1[ 3] * in2[ 8] + in1[ 7] * in2[ 9] + in1[11] * in2[10] +
              in1[15] * in2[11];

    out[12] = in1[ 0] * in2[12] + in1[ 4] * in2[13] + in1[ 8] * in2[14] +
              in1[12] * in2[15];
    out[13] = in1[ 1] * in2[12] + in1[ 5] * in2[13] + in1[ 9] * in2[14] +
              in1[13] * in2[15];
    out[14] = in1[ 2] * in2[12] + in1[ 6] * in2[13] + in1[10] * in2[14] +
              in1[14] * in2[15];
    out[15] = in1[ 3] * in2[12] + in1[ 7] * in2[13] + in1[11] * in2[14] +
              in1[15] * in2[15];
}

void Mat4Transform(const mat4_t in1, const vec4_t in2, vec4_t out) {
    out[ 0] = in1[ 0] * in2[ 0] + in1[ 4] * in2[ 1] + in1[ 8] * in2[ 2] +
              in1[12] * in2[ 3];
    out[ 1] = in1[ 1] * in2[ 0] + in1[ 5] * in2[ 1] + in1[ 9] * in2[ 2] +
              in1[13] * in2[ 3];
    out[ 2] = in1[ 2] * in2[ 0] + in1[ 6] * in2[ 1] + in1[10] * in2[ 2] +
              in1[14] * in2[ 3];
    out[ 3] = in1[ 3] * in2[ 0] + in1[ 7] * in2[ 1] + in1[11] * in2[ 2] +
              in1[15] * in2[ 3];
}

bool Mat4Compare(const mat4_t a, const mat4_t b) {
    return !(a[ 0] != b[ 0] || a[ 4] != b[ 4] || a[ 8] != b[ 8] ||
             a[12] != b[12] ||
             a[ 1] != b[ 1] || a[ 5] != b[ 5] || a[ 9] != b[ 9] || a[13] != b[13] ||
             a[ 2] != b[ 2] || a[ 6] != b[ 6] || a[10] != b[10] || a[14] != b[14] ||
             a[ 3] != b[ 3] || a[ 7] != b[ 7] || a[11] != b[11] || a[15] != b[15]);
}

void Mat4Dump(const mat4_t in) {
    clientRendererSystem->RefPrintf(PRINT_ALL, "%3.5f %3.5f %3.5f %3.5f\n",
                                    in[ 0], in[ 4],
                                    in[ 8], in[12]);
    clientRendererSystem->RefPrintf(PRINT_ALL, "%3.5f %3.5f %3.5f %3.5f\n",
                                    in[ 1], in[ 5],
                                    in[ 9], in[13]);
    clientRendererSystem->RefPrintf(PRINT_ALL, "%3.5f %3.5f %3.5f %3.5f\n",
                                    in[ 2], in[ 6],
                                    in[10], in[14]);
    clientRendererSystem->RefPrintf(PRINT_ALL, "%3.5f %3.5f %3.5f %3.5f\n",
                                    in[ 3], in[ 7],
                                    in[11], in[15]);
}

void Mat4Translation(vec3_t vec, mat4_t out) {
    out[ 0] = 1.0f;
    out[ 4] = 0.0f;
    out[ 8] = 0.0f;
    out[12] = vec[0];
    out[ 1] = 0.0f;
    out[ 5] = 1.0f;
    out[ 9] = 0.0f;
    out[13] = vec[1];
    out[ 2] = 0.0f;
    out[ 6] = 0.0f;
    out[10] = 1.0f;
    out[14] = vec[2];
    out[ 3] = 0.0f;
    out[ 7] = 0.0f;
    out[11] = 0.0f;
    out[15] = 1.0f;
}

void Mat4Ortho(float32 left, float32 right, float32 bottom, float32 top,
               float32 znear, float32 zfar, mat4_t out) {
    out[ 0] = 2.0f / (right - left);
    out[ 4] = 0.0f;
    out[ 8] = 0.0f;
    out[12] = -(right + left) / (right - left);
    out[ 1] = 0.0f;
    out[ 5] = 2.0f / (top - bottom);
    out[ 9] = 0.0f;
    out[13] = -(top + bottom) / (top - bottom);
    out[ 2] = 0.0f;
    out[ 6] = 0.0f;
    out[10] = 2.0f / (zfar - znear);
    out[14] = -(zfar + znear) / (zfar - znear);
    out[ 3] = 0.0f;
    out[ 7] = 0.0f;
    out[11] = 0.0f;
    out[15] = 1.0f;
}

void Mat4View(vec3_t axes[3], vec3_t origin, mat4_t out) {
    out[0]  = axes[0][0];
    out[1]  = axes[1][0];
    out[2]  = axes[2][0];
    out[3]  = 0;

    out[4]  = axes[0][1];
    out[5]  = axes[1][1];
    out[6]  = axes[2][1];
    out[7]  = 0;

    out[8]  = axes[0][2];
    out[9]  = axes[1][2];
    out[10] = axes[2][2];
    out[11] = 0;

    out[12] = -DotProduct(origin, axes[0]);
    out[13] = -DotProduct(origin, axes[1]);
    out[14] = -DotProduct(origin, axes[2]);
    out[15] = 1;
}

void Mat4SimpleInverse(const mat4_t in, mat4_t out) {
    vec3_t v;
    float32 invSqrLen;

    VectorCopy(in + 0, v);
    invSqrLen = 1.0f / DotProduct(v, v);
    VectorScale(v, invSqrLen, v);
    out[ 0] = v[0];
    out[ 4] = v[1];
    out[ 8] = v[2];
    out[12] = -DotProduct(v, &in[12]);

    VectorCopy(in + 4, v);
    invSqrLen = 1.0f / DotProduct(v, v);
    VectorScale(v, invSqrLen, v);
    out[ 1] = v[0];
    out[ 5] = v[1];
    out[ 9] = v[2];
    out[13] = -DotProduct(v, &in[12]);

    VectorCopy(in + 8, v);
    invSqrLen = 1.0f / DotProduct(v, v);
    VectorScale(v, invSqrLen, v);
    out[ 2] = v[0];
    out[ 6] = v[1];
    out[10] = v[2];
    out[14] = -DotProduct(v, &in[12]);

    out[ 3] = 0.0f;
    out[ 7] = 0.0f;
    out[11] = 0.0f;
    out[15] = 1.0f;
}

void VectorLerp(vec3_t a, vec3_t b, float32 lerp, vec3_t c) {
    c[0] = a[0] * (1.0f - lerp) + b[0] * lerp;
    c[1] = a[1] * (1.0f - lerp) + b[1] * lerp;
    c[2] = a[2] * (1.0f - lerp) + b[2] * lerp;
}

bool SpheresIntersect(vec3_t origin1, float32 radius1, vec3_t origin2,
                      float32 radius2) {
    float32 radiusSum = radius1 + radius2;
    vec3_t diff;

    VectorSubtract(origin1, origin2, diff);

    if(DotProduct(diff, diff) <= radiusSum * radiusSum) {
        return true;
    }

    return false;
}

void BoundingSphereOfSpheres(vec3_t origin1, float32 radius1,
                             vec3_t origin2, float32 radius2, vec3_t origin3, float32 *radius3) {
    vec3_t diff;

    VectorScale(origin1, 0.5f, origin3);
    VectorMA(origin3, 0.5f, origin2, origin3);

    VectorSubtract(origin1, origin2, diff);
    *radius3 = VectorLength(diff) * 0.5f + MAX(radius1, radius2);
}

sint NextPowerOfTwo(sint in) {
    sint out;

    for(out = 1; out < in; out <<= 1)
        ;

    return out;
}

union f32_u {
    float32 f;
    uint ui;
    struct {
        uint fraction: 23;
        uint exponent: 8;
        uint sign: 1;
    } pack;
};

union f16_u {
    uchar16 ui;
    struct {
        uint fraction: 10;
        uint exponent: 5;
        uint sign: 1;
    } pack;
};

uchar16 FloatToHalf(float32 in) {
    union f32_u f32;
    union f16_u f16;

    f32.f = in;

    f16.pack.exponent = CLAMP(static_cast<sint>(f32.pack.exponent) - 112, 0,
                              31);
    f16.pack.fraction = f32.pack.fraction >> 13;
    f16.pack.sign     = f32.pack.sign;

    return f16.ui;
}

float32 HalfToFloat(uchar16 in) {
    union f32_u f32;
    union f16_u f16;

    f16.ui = in;

    f32.pack.exponent = static_cast<sint>(f16.pack.exponent) + 112;
    f32.pack.fraction = f16.pack.fraction << 13;
    f32.pack.sign = f16.pack.sign;

    return f32.f;
}

uint ReverseBits(uint v) {
    v = ((v >> 1) & 0x55555555) | ((v & 0x55555555) << 1);
    v = ((v >> 2) & 0x33333333) | ((v & 0x33333333) << 2);
    v = ((v >> 4) & 0x0F0F0F0F) | ((v & 0x0F0F0F0F) << 4);
    v = ((v >> 8) & 0x00FF00FF) | ((v & 0x00FF00FF) << 8);
    v = (v >> 16) | (v << 16);
    return v;
}

float32 GSmithCorrelated(float32 roughness, float32 ndotv, float32 ndotl) {
    float32 m2 = roughness * roughness;
    float32 visV = ndotl * sqrt(ndotv * (ndotv - ndotv * m2) + m2);
    float32 visL = ndotv * sqrt(ndotl * (ndotl - ndotl * m2) + m2);
    return 0.5f / (visV + visL);
}