////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2005 Id Software, Inc.
// Copyright(C) 2000 - 2013 Darklegion Development
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
// File name:   r_image.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <renderSystem/r_precompiled.hpp>

static uchar8 s_intensitytable[256];
static uchar8 s_gammatable[256];

sint gl_filter_min = GL_LINEAR_MIPMAP_NEAREST;
sint    gl_filter_max = GL_LINEAR;

#define FILE_HASH_SIZE      1024
static  image_t        *hashTable[FILE_HASH_SIZE];

struct textureMode_t {
    pointer name;
    sint    minimize, maximize;
};

textureMode_t modes[] = {
    {"GL_NEAREST", GL_NEAREST, GL_NEAREST},
    {"GL_LINEAR", GL_LINEAR, GL_LINEAR},
    {"GL_NEAREST_MIPMAP_NEAREST", GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST},
    {"GL_LINEAR_MIPMAP_NEAREST", GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR},
    {"GL_NEAREST_MIPMAP_LINEAR", GL_NEAREST_MIPMAP_LINEAR, GL_NEAREST},
    {"GL_LINEAR_MIPMAP_LINEAR", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR}
};

/*
================
return a hash value for the filename
================
*/
static sint32 generateHashValue(pointer fname) {
    sint        i;
    sint32  hash;
    valueType   letter;

    hash = 0;
    i = 0;

    while(fname[i] != '\0') {
        letter = tolower(fname[i]);

        if(letter == '.') {
            break;    // don't include extension
        }

        if(letter == '\\') {
            letter = '/';    // damn path names
        }

        hash += static_cast<sint32>(letter) * (i + 119);
        i++;
    }

    hash &= (FILE_HASH_SIZE - 1);
    return hash;
}

void GL_TextureMode(const uint &mode, image_t *image) {
    gl_filter_min = modes[mode].minimize;
    gl_filter_max = modes[mode].maximize;

    if(nullptr == image) {
        return;
    }

    if(image->flags & IMGFLAG_MIPMAP) {
        qglTextureParameterfEXT(image->texnum, GL_TEXTURE_2D,
                                GL_TEXTURE_MIN_FILTER, gl_filter_min);
        qglTextureParameterfEXT(image->texnum, GL_TEXTURE_2D,
                                GL_TEXTURE_MAG_FILTER, gl_filter_max);
    } else {
        qglTextureParameterfEXT(image->texnum, GL_TEXTURE_2D,
                                GL_TEXTURE_MIN_FILTER, gl_filter_max);
        qglTextureParameterfEXT(image->texnum, GL_TEXTURE_2D,
                                GL_TEXTURE_MAG_FILTER, gl_filter_max);
    }
}

void GL_TextureMode(const uint &mode) {
    image_t *glt;

    gl_filter_min = modes[mode].minimize;
    gl_filter_max = modes[mode].maximize;

    // change all the existing mipmap texture objects
    for(sint i = 0; i < tr.numImages; i++) {
        glt = tr.images[i];

        if(glt->flags & IMGFLAG_MIPMAP && !(glt->flags & IMGFLAG_CUBEMAP)) {
            qglTextureParameterfEXT(glt->texnum, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                                    gl_filter_min);
            qglTextureParameterfEXT(glt->texnum, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                                    gl_filter_max);
        }
    }
}

/*
===============
GL_TextureMode
===============
*/
void GL_TextureMode(pointer string) {
    uint i;

    for(i = 0 ; i < 6 ; i++) {
        if(!Q_stricmp(modes[i].name, string)) {
            break;
        }
    }

    if(i == 6U) {
        clientRendererSystem->RefPrintf(PRINT_ALL, "bad filter name\n");
        return;
    }

    GL_TextureMode(i);
}

/*
===============
R_SumOfUsedImages
===============
*/
sint R_SumOfUsedImages(void) {
    sint    total;
    sint i;

    total = 0;

    for(i = 0; i < tr.numImages; i++) {
        if(tr.images[i]->frameUsed == tr.frameCount) {
            total += tr.images[i]->uploadWidth * tr.images[i]->uploadHeight;
        }
    }

    return total;
}

/*
===============
R_ImageList_f
===============
*/
void R_ImageList_f(void) {
    sint i;
    sint estTotalSize = 0;

    clientRendererSystem->RefPrintf(PRINT_ALL,
                                    "\n      -w-- -h-- -type-- -size- --name-------\n");

    for(i = 0 ; i < tr.numImages ; i++) {
        image_t *image = tr.images[i];
        pointer format = "????   ";
        pointer sizeSuffix;
        sint estSize;
        sint displaySize;

        estSize = image->uploadHeight * image->uploadWidth;

        switch(image->internalFormat) {
            case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:
                format = "sDXT1  ";
                // 64 bits per 16 pixels, so 4 bits per pixel
                estSize /= 2;
                break;

            case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:
                format = "sDXT5  ";
                // 128 bits per 16 pixels, so 1 byte per pixel
                break;

            case GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB:
                format = "sBPTC  ";
                // 128 bits per 16 pixels, so 1 byte per pixel
                break;

            case GL_COMPRESSED_RG_RGTC2:
                format = "RGTC2  ";
                // 128 bits per 16 pixels, so 1 byte per pixel
                break;

            case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
                format = "DXT1   ";
                // 64 bits per 16 pixels, so 4 bits per pixel
                estSize /= 2;
                break;

            case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
                format = "DXT1a  ";
                // 64 bits per 16 pixels, so 4 bits per pixel
                estSize /= 2;
                break;

            case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
                format = "DXT5   ";
                // 128 bits per 16 pixels, so 1 byte per pixel
                break;

            case GL_COMPRESSED_RGBA_BPTC_UNORM_ARB:
                format = "BPTC   ";
                // 128 bits per 16 pixels, so 1 byte per pixel
                break;

            case GL_RGB4_S3TC:
                format = "S3TC   ";
                // same as DXT1?
                estSize /= 2;
                break;

            case GL_RGBA16F:
                format = "RGBA16F";
                // 8 bytes per pixel
                estSize *= 8;
                break;

            case GL_RGBA16:
                format = "RGBA16 ";
                // 8 bytes per pixel
                estSize *= 8;
                break;

            case GL_RGBA4:
            case GL_RGBA8:
            case GL_RGBA:
                format = "RGBA   ";
                // 4 bytes per pixel
                estSize *= 4;
                break;

            case GL_LUMINANCE8:
            case GL_LUMINANCE16:
            case GL_LUMINANCE:
                format = "L      ";
                // 1 byte per pixel?
                break;

            case GL_RGB5:
            case GL_RGB8:
            case GL_RGB:
                format = "RGB    ";
                // 3 bytes per pixel?
                estSize *= 3;
                break;

            case GL_LUMINANCE8_ALPHA8:
            case GL_LUMINANCE16_ALPHA16:
            case GL_LUMINANCE_ALPHA:
                format = "LA     ";
                // 2 bytes per pixel?
                estSize *= 2;
                break;

            case GL_SRGB:
            case GL_SRGB8:
                format = "sRGB   ";
                // 3 bytes per pixel?
                estSize *= 3;
                break;

            case GL_SRGB_ALPHA:
            case GL_SRGB8_ALPHA8:
                format = "sRGBA  ";
                // 4 bytes per pixel?
                estSize *= 4;
                break;

            case GL_SLUMINANCE:
            case GL_SLUMINANCE8:
                format = "sL     ";
                // 1 byte per pixel?
                break;

            case GL_SLUMINANCE_ALPHA:
            case GL_SLUMINANCE8_ALPHA8:
                format = "sLA    ";
                // 2 byte per pixel?
                estSize *= 2;
                break;

            case GL_DEPTH_COMPONENT16:
                format = "Depth16";
                // 2 bytes per pixel
                estSize *= 2;
                break;

            case GL_DEPTH_COMPONENT24:
                format = "Depth24";
                // 3 bytes per pixel
                estSize *= 3;
                break;

            case GL_DEPTH_COMPONENT:
            case GL_DEPTH_COMPONENT32:
                format = "Depth32";
                // 4 bytes per pixel
                estSize *= 4;
                break;
        }

        // mipmap adds about 50%
        if(image->flags & IMGFLAG_MIPMAP) {
            estSize += estSize / 2;
        }

        sizeSuffix = "b ";
        displaySize = estSize;

        if(displaySize > 1024) {
            displaySize /= 1024;
            sizeSuffix = "kb";
        }

        if(displaySize > 1024) {
            displaySize /= 1024;
            sizeSuffix = "Mb";
        }

        if(displaySize > 1024) {
            displaySize /= 1024;
            sizeSuffix = "Gb";
        }

        clientRendererSystem->RefPrintf(PRINT_ALL, "%4i: %4ix%4i %s %4i%s %s\n", i,
                                        image->uploadWidth, image->uploadHeight, format, displaySize, sizeSuffix,
                                        image->imgName);
        estTotalSize += estSize;
    }

    clientRendererSystem->RefPrintf(PRINT_ALL, " ---------\n");
    clientRendererSystem->RefPrintf(PRINT_ALL, " approx %i bytes\n",
                                    estTotalSize);
    clientRendererSystem->RefPrintf(PRINT_ALL, " %i total images\n\n",
                                    tr.numImages);
}

//=======================================================================

/*
================
ResampleTexture

Used to resample images in a more general than quartering fashion.

This will only be filtered properly if the resampled size
is greater than half the original size.

If a larger shrinking is needed, use the mipmap function
before or after.
================
*/
static void ResampleTexture(uchar8 *in, sint inwidth, sint inheight,
                            uchar8 *out, sint outwidth, sint outheight) {
    sint        i, j;
    uchar8 *inrow, *inrow2;
    sint        frac, fracstep;
    sint        p1[2048], p2[2048];
    uchar8 *pix1, *pix2, *pix3, *pix4;

    if(outwidth > 2048) {
        Com_Error(ERR_DROP, "ResampleTexture: max width");
    }

    fracstep = inwidth * 0x10000 / outwidth;

    frac = fracstep >> 2;

    for(i = 0 ; i < outwidth ; i++) {
        p1[i] = 4 * (frac >> 16);
        frac += fracstep;
    }

    frac = 3 * (fracstep >> 2);

    for(i = 0 ; i < outwidth ; i++) {
        p2[i] = 4 * (frac >> 16);
        frac += fracstep;
    }

    for(i = 0 ; i < outheight ; i++) {
        inrow = in + 4 * inwidth * static_cast<sint>((i + 0.25f) * inheight /
                outheight);
        inrow2 = in + 4 * inwidth * static_cast<sint>((i + 0.75f) * inheight /
                 outheight);

        for(j = 0 ; j < outwidth ; j++) {
            pix1 = inrow + p1[j];
            pix2 = inrow + p2[j];
            pix3 = inrow2 + p1[j];
            pix4 = inrow2 + p2[j];
            *out++ = (pix1[0] + pix2[0] + pix3[0] + pix4[0]) >> 2;
            *out++ = (pix1[1] + pix2[1] + pix3[1] + pix4[1]) >> 2;
            *out++ = (pix1[2] + pix2[2] + pix3[2] + pix4[2]) >> 2;
            *out++ = (pix1[3] + pix2[3] + pix3[3] + pix4[3]) >> 2;
        }
    }
}

static void RGBAtoYCoCgA(const uchar8 *in, uchar8 *out, sint width,
                         sint height) {
    sint x, y;

    for(y = 0; y < height; y++) {
        const uchar8 *inbyte  = in  + y * width * 4;
        uchar8       *outbyte = out + y * width * 4;

        for(x = 0; x < width; x++) {
            uchar8 r, g, b, a, rb2;

            r = *inbyte++;
            g = *inbyte++;
            b = *inbyte++;
            a = *inbyte++;
            rb2 = (r + b) >> 1;

            *outbyte++ = (g + rb2) >> 1;       // Y  =  R/4 + G/2 + B/4
            *outbyte++ = (r - b + 256) >> 1;   // Co =  R/2       - B/2
            *outbyte++ = (g - rb2 + 256) >> 1;   // Cg = -R/4 + G/2 - B/4
            *outbyte++ = a;
        }
    }
}

static void YCoCgAtoRGBA(const uchar8 *in, uchar8 *out, sint width,
                         sint height) {
    sint x, y;

    for(y = 0; y < height; y++) {
        const uchar8 *inbyte  = in  + y * width * 4;
        uchar8       *outbyte = out + y * width * 4;

        for(x = 0; x < width; x++) {
            uchar8 _Y, Co, Cg, a;

            _Y = *inbyte++;
            Co = *inbyte++;
            Cg = *inbyte++;
            a  = *inbyte++;

            *outbyte++ = CLAMP(_Y + Co - Cg,       0, 255);   // R = Y + Co - Cg
            *outbyte++ = CLAMP(_Y      + Cg - 128, 0, 255);   // G = Y + Cg
            *outbyte++ = CLAMP(_Y - Co - Cg + 256, 0, 255);   // B = Y - Co - Cg
            *outbyte++ = a;
        }
    }
}


// uses a sobel filter to change a texture to a normal map
static void RGBAtoNormal(const uchar8 *in, uchar8 *out, sint width,
                         sint height, bool clampToEdge) {
    sint x, y, max;

    // convert to heightmap, storing in alpha
    // same as converting to Y in YCoCg
    max = 1;

    for(y = 0; y < height; y++) {
        const uchar8 *inbyte  = in  + y * width * 4;
        uchar8       *outbyte = out + y * width * 4 + 3;

        for(x = 0; x < width; x++) {
            uchar8 result = (inbyte[0] >> 2) + (inbyte[1] >> 1) + (inbyte[2] >> 2);
            result = result * result / 255; // Make linear
            *outbyte = result;
            max = MAX(max, *outbyte);
            outbyte += 4;
            inbyte  += 4;
        }
    }

    // level out heights
    if(max < 255) {
        for(y = 0; y < height; y++) {
            uchar8 *outbyte = out + y * width * 4 + 3;

            for(x = 0; x < width; x++) {
                *outbyte = *outbyte + (255 - max);
                outbyte += 4;
            }
        }
    }


    // now run sobel filter over height values to generate X and Y
    // then normalize
    for(y = 0; y < height; y++) {
        uchar8 *outbyte = out + y * width * 4;

        for(x = 0; x < width; x++) {
            // 0 1 2
            // 3 4 5
            // 6 7 8

            uchar8 s[9];
            sint x2, y2, i;
            vec3_t normal;

            i = 0;

            for(y2 = -1; y2 <= 1; y2++) {
                sint src_y = y + y2;

                if(clampToEdge) {
                    src_y = CLAMP(src_y, 0, height - 1);
                } else {
                    src_y = (src_y + height) % height;
                }


                for(x2 = -1; x2 <= 1; x2++) {
                    sint src_x = x + x2;

                    if(clampToEdge) {
                        src_x = CLAMP(src_x, 0, width - 1);
                    } else {
                        src_x = (src_x + width) % width;
                    }

                    s[i++] = *(out + (src_y * width + src_x) * 4 + 3);
                }
            }

            normal[0] =        s[0]            -     s[2]
                               + 2 * s[3]            - 2 * s[5]
                               +     s[6]            -     s[8];

            normal[1] =        s[0] + 2 * s[1] +     s[2]

                               -     s[6] - 2 * s[7] -     s[8];

            normal[2] = s[4] * 4;

            if(!VectorNormalize2(normal, normal)) {
                VectorSet(normal, 0, 0, 1);
            }

            *outbyte++ = FloatToOffsetByte(normal[0]);
            *outbyte++ = FloatToOffsetByte(normal[1]);
            *outbyte++ = FloatToOffsetByte(normal[2]);
            outbyte++;
        }
    }
}

#define COPYSAMPLE(a,b) *(uint *)(a) = *(uint *)(b)

// based on Fast Curve Based Interpolation
// from Fast Artifacts-Free Image Interpolation (http://www.andreagiachetti.it/icbi/)
// assumes data has a 2 pixel thick border of clamped or wrapped data
// expects data to be a grid with even (0, 0), (2, 0), (0, 2), (2, 2) etc pixels filled
// only performs FCBI on specified component
static void DoFCBI(uchar8 *in, uchar8 *out, sint width, sint height,
                   sint component) {
    sint x, y;
    uchar8 *outbyte, *inbyte;

    // copy in to out
    for(y = 2; y < height - 2; y += 2) {
        inbyte  = in  + (y * width + 2) * 4 + component;
        outbyte = out + (y * width + 2) * 4 + component;

        for(x = 2; x < width - 2; x += 2) {
            *outbyte = *inbyte;
            outbyte += 8;
            inbyte += 8;
        }
    }

    for(y = 3; y < height - 3; y += 2) {
        // diagonals
        //
        // NWp  - northwest interpolated pixel
        // NEp  - northeast interpolated pixel
        // NWd  - northwest first derivative
        // NEd  - northeast first derivative
        // NWdd - northwest second derivative
        // NEdd - northeast second derivative
        //
        // Uses these samples:
        //
        //         0
        //   - - a - b - -
        //   - - - - - - -
        //   c - d - e - f
        // 0 - - - - - - -
        //   g - h - i - j
        //   - - - - - - -
        //   - - k - l - -
        //
        // x+2 uses these samples:
        //
        //         0
        //   - - - - a - b - -
        //   - - - - - - - - -
        //   - - c - d - e - f
        // 0 - - - - - - - - -
        //   - - g - h - i - j
        //   - - - - - - - - -
        //   - - - - k - l - -
        //
        // so we can reuse 8 of them on next iteration
        //
        // a=b, c=d, d=e, e=f, g=h, h=i, i=j, k=l
        //
        // only b, f, j, and l need to be sampled on next iteration

        uchar8 sa, sb, sc, sd, se, sf, sg, sh, si, sj, sk, sl;
        uchar8 *line1, *line2, *line3, *line4;

        x = 3;

        // optimization one
        //                       SAMPLE2(sa, x-1, y-3);
        //SAMPLE2(sc, x-3, y-1); SAMPLE2(sd, x-1, y-1); SAMPLE2(se, x+1, y-1);
        //SAMPLE2(sg, x-3, y+1); SAMPLE2(sh, x-1, y+1); SAMPLE2(si, x+1, y+1);
        //                       SAMPLE2(sk, x-1, y+3);

        // optimization two
        line1 = in + ((y - 3) * width + (x - 1)) * 4 + component;
        line2 = in + ((y - 1) * width + (x - 3)) * 4 + component;
        line3 = in + ((y + 1) * width + (x - 3)) * 4 + component;
        line4 = in + ((y + 3) * width + (x - 1)) * 4 + component;

        //                                   COPYSAMPLE(sa, line1); line1 += 8;
        //COPYSAMPLE(sc, line2); line2 += 8; COPYSAMPLE(sd, line2); line2 += 8; COPYSAMPLE(se, line2); line2 += 8;
        //COPYSAMPLE(sg, line3); line3 += 8; COPYSAMPLE(sh, line3); line3 += 8; COPYSAMPLE(si, line3); line3 += 8;
        //                                   COPYSAMPLE(sk, line4); line4 += 8;

        sa = *line1;
        line1 += 8;
        sc = *line2;
        line2 += 8;
        sd = *line2;
        line2 += 8;
        se = *line2;
        line2 += 8;
        sg = *line3;
        line3 += 8;
        sh = *line3;
        line3 += 8;
        si = *line3;
        line3 += 8;
        sk = *line4;
        line4 += 8;

        outbyte = out + (y * width + x) * 4 + component;

        for(; x < width - 3; x += 2) {
            sint NWd, NEd, NWp, NEp;

            // original
            //                       SAMPLE2(sa, x-1, y-3); SAMPLE2(sb, x+1, y-3);
            //SAMPLE2(sc, x-3, y-1); SAMPLE2(sd, x-1, y-1); SAMPLE2(se, x+1, y-1); SAMPLE2(sf, x+3, y-1);
            //SAMPLE2(sg, x-3, y+1); SAMPLE2(sh, x-1, y+1); SAMPLE2(si, x+1, y+1); SAMPLE2(sj, x+3, y+1);
            //                       SAMPLE2(sk, x-1, y+3); SAMPLE2(sl, x+1, y+3);

            // optimization one
            //SAMPLE2(sb, x+1, y-3);
            //SAMPLE2(sf, x+3, y-1);
            //SAMPLE2(sj, x+3, y+1);
            //SAMPLE2(sl, x+1, y+3);

            // optimization two
            //COPYSAMPLE(sb, line1); line1 += 8;
            //COPYSAMPLE(sf, line2); line2 += 8;
            //COPYSAMPLE(sj, line3); line3 += 8;
            //COPYSAMPLE(sl, line4); line4 += 8;

            sb = *line1;
            line1 += 8;
            sf = *line2;
            line2 += 8;
            sj = *line3;
            line3 += 8;
            sl = *line4;
            line4 += 8;

            NWp = sd + si;
            NEp = se + sh;
            NWd = abs(sd - si);
            NEd = abs(se - sh);

            if(NWd > 100 || NEd > 100 || abs(NWp - NEp) > 200) {
                if(NWd < NEd) {
                    *outbyte = NWp >> 1;
                } else {
                    *outbyte = NEp >> 1;
                }
            } else {
                sint NWdd, NEdd;

                //NEdd = abs(sg + sd + sb - 3 * (se + sh) + sk + si + sf);
                //NWdd = abs(sa + se + sj - 3 * (sd + si) + sc + sh + sl);
                NEdd = abs(sg + sb - 3 * NEp + sk + sf + NWp);
                NWdd = abs(sa + sj - 3 * NWp + sc + sl + NEp);

                if(NWdd > NEdd) {
                    *outbyte = NWp >> 1;
                } else {
                    *outbyte = NEp >> 1;
                }
            }

            outbyte += 8;

            //                    COPYSAMPLE(sa, sb);
            //COPYSAMPLE(sc, sd); COPYSAMPLE(sd, se); COPYSAMPLE(se, sf);
            //COPYSAMPLE(sg, sh); COPYSAMPLE(sh, si); COPYSAMPLE(si, sj);
            //                    COPYSAMPLE(sk, sl);

            sa = sb;
            sc = sd;
            sd = se;
            se = sf;
            sg = sh;
            sh = si;
            si = sj;
            sk = sl;
        }
    }

    // hack: copy out to in again
    for(y = 3; y < height - 3; y += 2) {
        inbyte = out + (y * width + 3) * 4 + component;
        outbyte = in + (y * width + 3) * 4 + component;

        for(x = 3; x < width - 3; x += 2) {
            *outbyte = *inbyte;
            outbyte += 8;
            inbyte += 8;
        }
    }

    for(y = 2; y < height - 3; y++) {
        // horizontal & vertical
        //
        // hp  - horizontally interpolated pixel
        // vp  - vertically interpolated pixel
        // hd  - horizontal first derivative
        // vd  - vertical first derivative
        // hdd - horizontal second derivative
        // vdd - vertical second derivative
        // Uses these samples:
        //
        //       0
        //   - a - b -
        //   c - d - e
        // 0 - f - g -
        //   h - i - j
        //   - k - l -
        //
        // x+2 uses these samples:
        //
        //       0
        //   - - - a - b -
        //   - - c - d - e
        // 0 - - - f - g -
        //   - - h - i - j
        //   - - - k - l -
        //
        // so we can reuse 7 of them on next iteration
        //
        // a=b, c=d, d=e, f=g, h=i, i=j, k=l
        //
        // only b, e, g, j, and l need to be sampled on next iteration

        uchar8 sa, sb, sc, sd, se, sf, sg, sh, si, sj, sk, sl;
        uchar8 *line1, *line2, *line3, *line4, *line5;

        //x = (y + 1) % 2;
        x = (y + 1) % 2 + 2;

        // optimization one
        //            SAMPLE2(sa, x-1, y-2);
        //SAMPLE2(sc, x-2, y-1); SAMPLE2(sd, x,   y-1);
        //            SAMPLE2(sf, x-1, y  );
        //SAMPLE2(sh, x-2, y+1); SAMPLE2(si, x,   y+1);
        //            SAMPLE2(sk, x-1, y+2);

        line1 = in + ((y - 2) * width + (x - 1)) * 4 + component;
        line2 = in + ((y - 1) * width + (x - 2)) * 4 + component;
        line3 = in + ((y) * width + (x - 1)) * 4 + component;
        line4 = in + ((y + 1) * width + (x - 2)) * 4 + component;
        line5 = in + ((y + 2) * width + (x - 1)) * 4 + component;

        //                 COPYSAMPLE(sa, line1); line1 += 8;
        //COPYSAMPLE(sc, line2); line2 += 8; COPYSAMPLE(sd, line2); line2 += 8;
        //                 COPYSAMPLE(sf, line3); line3 += 8;
        //COPYSAMPLE(sh, line4); line4 += 8; COPYSAMPLE(si, line4); line4 += 8;
        //                 COPYSAMPLE(sk, line5); line5 += 8;

        sa = *line1;
        line1 += 8;
        sc = *line2;
        line2 += 8;
        sd = *line2;
        line2 += 8;
        sf = *line3;
        line3 += 8;
        sh = *line4;
        line4 += 8;
        si = *line4;
        line4 += 8;
        sk = *line5;
        line5 += 8;

        outbyte = out + (y * width + x) * 4 + component;

        for(; x < width - 3; x += 2) {
            sint hd, vd, hp, vp;

            //            SAMPLE2(sa, x-1, y-2); SAMPLE2(sb, x+1, y-2);
            //SAMPLE2(sc, x-2, y-1); SAMPLE2(sd, x,   y-1); SAMPLE2(se, x+2, y-1);
            //            SAMPLE2(sf, x-1, y  ); SAMPLE2(sg, x+1, y  );
            //SAMPLE2(sh, x-2, y+1); SAMPLE2(si, x,   y+1); SAMPLE2(sj, x+2, y+1);
            //            SAMPLE2(sk, x-1, y+2); SAMPLE2(sl, x+1, y+2);

            // optimization one
            //SAMPLE2(sb, x+1, y-2);
            //SAMPLE2(se, x+2, y-1);
            //SAMPLE2(sg, x+1, y  );
            //SAMPLE2(sj, x+2, y+1);
            //SAMPLE2(sl, x+1, y+2);

            //COPYSAMPLE(sb, line1); line1 += 8;
            //COPYSAMPLE(se, line2); line2 += 8;
            //COPYSAMPLE(sg, line3); line3 += 8;
            //COPYSAMPLE(sj, line4); line4 += 8;
            //COPYSAMPLE(sl, line5); line5 += 8;

            sb = *line1;
            line1 += 8;
            se = *line2;
            line2 += 8;
            sg = *line3;
            line3 += 8;
            sj = *line4;
            line4 += 8;
            sl = *line5;
            line5 += 8;

            hp = sf + sg;
            vp = sd + si;
            hd = abs(sf - sg);
            vd = abs(sd - si);

            if(hd > 100 || vd > 100 || abs(hp - vp) > 200) {
                if(hd < vd) {
                    *outbyte = hp >> 1;
                } else {
                    *outbyte = vp >> 1;
                }
            } else {
                sint hdd, vdd;

                //hdd = abs(sc[i] + sd[i] + se[i] - 3 * (sf[i] + sg[i]) + sh[i] + si[i] + sj[i]);
                //vdd = abs(sa[i] + sf[i] + sk[i] - 3 * (sd[i] + si[i]) + sb[i] + sg[i] + sl[i]);

                hdd = abs(sc + se - 3 * hp + sh + sj + vp);
                vdd = abs(sa + sk - 3 * vp + sb + sl + hp);

                if(hdd > vdd) {
                    *outbyte = hp >> 1;
                } else {
                    *outbyte = vp >> 1;
                }
            }

            outbyte += 8;

            //          COPYSAMPLE(sa, sb);
            //COPYSAMPLE(sc, sd); COPYSAMPLE(sd, se);
            //          COPYSAMPLE(sf, sg);
            //COPYSAMPLE(sh, si); COPYSAMPLE(si, sj);
            //          COPYSAMPLE(sk, sl);
            sa = sb;
            sc = sd;
            sd = se;
            sf = sg;
            sh = si;
            si = sj;
            sk = sl;
        }
    }
}

// Similar to FCBI, but throws out the second order derivatives for speed
static void DoFCBIQuick(uchar8 *in, uchar8 *out, sint width, sint height,
                        sint component) {
    sint x, y;
    uchar8 *outbyte, *inbyte;

    // copy in to out
    for(y = 2; y < height - 2; y += 2) {
        inbyte  = in  + (y * width + 2) * 4 + component;
        outbyte = out + (y * width + 2) * 4 + component;

        for(x = 2; x < width - 2; x += 2) {
            *outbyte = *inbyte;
            outbyte += 8;
            inbyte += 8;
        }
    }

    for(y = 3; y < height - 4; y += 2) {
        uchar8 sd, se, sh, si;
        uchar8 *line2, *line3;

        x = 3;

        line2 = in + ((y - 1) * width + (x - 1)) * 4 + component;
        line3 = in + ((y + 1) * width + (x - 1)) * 4 + component;

        sd = *line2;
        line2 += 8;
        sh = *line3;
        line3 += 8;

        outbyte = out + (y * width + x) * 4 + component;

        for(; x < width - 4; x += 2) {
            sint NWd, NEd, NWp, NEp;

            se = *line2;
            line2 += 8;
            si = *line3;
            line3 += 8;

            NWp = sd + si;
            NEp = se + sh;
            NWd = abs(sd - si);
            NEd = abs(se - sh);

            if(NWd < NEd) {
                *outbyte = NWp >> 1;
            } else {
                *outbyte = NEp >> 1;
            }

            outbyte += 8;

            sd = se;
            sh = si;
        }
    }

    // hack: copy out to in again
    for(y = 3; y < height - 3; y += 2) {
        inbyte  = out + (y * width + 3) * 4 + component;
        outbyte = in  + (y * width + 3) * 4 + component;

        for(x = 3; x < width - 3; x += 2) {
            *outbyte = *inbyte;
            outbyte += 8;
            inbyte += 8;
        }
    }

    for(y = 2; y < height - 3; y++) {
        uchar8 sd, sf, sg, si;
        uchar8 *line2, *line3, *line4;

        x = (y + 1) % 2 + 2;

        line2 = in + ((y - 1) * width + (x)) * 4 + component;
        line3 = in + ((y) * width + (x - 1)) * 4 + component;
        line4 = in + ((y + 1) * width + (x)) * 4 + component;

        outbyte = out + (y * width + x) * 4 + component;

        sf = *line3;
        line3 += 8;

        for(; x < width - 3; x += 2) {
            sint hd, vd, hp, vp;

            sd = *line2;
            line2 += 8;
            sg = *line3;
            line3 += 8;
            si = *line4;
            line4 += 8;

            hp = sf + sg;
            vp = sd + si;
            hd = abs(sf - sg);
            vd = abs(sd - si);

            if(hd < vd) {
                *outbyte = hp >> 1;
            } else {
                *outbyte = vp >> 1;
            }

            outbyte += 8;

            sf = sg;
        }
    }
}

// Similar to DoFCBIQuick, but just takes the average instead of checking derivatives
// as well, this operates on all four components
static void DoLinear(uchar8 *in, uchar8 *out, sint width, sint height) {
    sint x, y, i;
    uchar8 *outbyte, *inbyte;

    // copy in to out
    for(y = 2; y < height - 2; y += 2) {
        x = 2;

        inbyte  = in  + (y * width + x) * 4;
        outbyte = out + (y * width + x) * 4;

        for(; x < width - 2; x += 2) {
            COPYSAMPLE(outbyte, inbyte);
            outbyte += 8;
            inbyte += 8;
        }
    }

    for(y = 1; y < height - 1; y += 2) {
        uchar8 sd[4] = {0}, se[4] = {0}, sh[4] = {0}, si[4] = {0};
        uchar8 *line2, *line3;

        x = 1;

        line2 = in + ((y - 1) * width + (x - 1)) * 4;
        line3 = in + ((y + 1) * width + (x - 1)) * 4;

        COPYSAMPLE(sd, line2);
        line2 += 8;
        COPYSAMPLE(sh, line3);
        line3 += 8;

        outbyte = out + (y * width + x) * 4;

        for(; x < width - 1; x += 2) {
            COPYSAMPLE(se, line2);
            line2 += 8;
            COPYSAMPLE(si, line3);
            line3 += 8;

            for(i = 0; i < 4; i++) {
                *outbyte++ = (sd[i] + si[i] + se[i] + sh[i]) >> 2;
            }

            outbyte += 4;

            COPYSAMPLE(sd, se);
            COPYSAMPLE(sh, si);
        }
    }

    // hack: copy out to in again
    for(y = 1; y < height - 1; y += 2) {
        x = 1;

        inbyte  = out + (y * width + x) * 4;
        outbyte = in  + (y * width + x) * 4;

        for(; x < width - 1; x += 2) {
            COPYSAMPLE(outbyte, inbyte);
            outbyte += 8;
            inbyte += 8;
        }
    }

    for(y = 1; y < height - 1; y++) {
        uchar8 sd[4], sf[4], sg[4], si[4];
        uchar8 *line2, *line3, *line4;

        x = y % 2 + 1;

        line2 = in + ((y - 1) * width + (x)) * 4;
        line3 = in + ((y) * width + (x - 1)) * 4;
        line4 = in + ((y + 1) * width + (x)) * 4;

        COPYSAMPLE(sf, line3);
        line3 += 8;

        outbyte = out + (y * width + x) * 4;

        for(; x < width - 1; x += 2) {
            COPYSAMPLE(sd, line2);
            line2 += 8;
            COPYSAMPLE(sg, line3);
            line3 += 8;
            COPYSAMPLE(si, line4);
            line4 += 8;

            for(i = 0; i < 4; i++) {
                *outbyte++ = (sf[i] + sg[i] + sd[i] + si[i]) >> 2;
            }

            outbyte += 4;

            COPYSAMPLE(sf, sg);
        }
    }
}


static void ExpandHalfTextureToGrid(uchar8 *data, sint width,
                                    sint height) {
    sint x, y;

    for(y = height / 2; y > 0; y--) {
        uchar8 *outbyte = data + ((y * 2 - 1) * (width)     - 2) * 4;
        uchar8 *inbyte  = data + (y           * (width / 2) - 1) * 4;

        for(x = width / 2; x > 0; x--) {
            COPYSAMPLE(outbyte, inbyte);

            outbyte -= 8;
            inbyte -= 4;
        }
    }
}

static void FillInNormalizedZ(const uchar8 *in, uchar8 *out, sint width,
                              sint height) {
    sint x, y;

    for(y = 0; y < height; y++) {
        const uchar8 *inbyte  = in  + y * width * 4;
        uchar8       *outbyte = out + y * width * 4;

        for(x = 0; x < width; x++) {
            uchar8 nx, ny, nz, h;
            float32 fnx, fny, fll, fnz;

            nx = *inbyte++;
            ny = *inbyte++;
            inbyte++;
            h  = *inbyte++;

            fnx = OffsetByteToFloat(nx);
            fny = OffsetByteToFloat(ny);
            fll = 1.0f - fnx * fnx - fny * fny;

            if(fll >= 0.0f) {
                fnz = static_cast<float32>(sqrt(fll));
            } else {
                fnz = 0.0f;
            }

            nz = FloatToOffsetByte(fnz);

            *outbyte++ = nx;
            *outbyte++ = ny;
            *outbyte++ = nz;
            *outbyte++ = h;
        }
    }
}


// size must be even
#define WORKBLOCK_SIZE     128
#define WORKBLOCK_BORDER   4
#define WORKBLOCK_REALSIZE (WORKBLOCK_SIZE + WORKBLOCK_BORDER * 2)

// assumes that data has already been expanded into a 2x2 grid
static void FCBIByBlock(uchar8 *data, sint width, sint height,
                        bool clampToEdge, bool normalized) {
    uchar8 workdata[WORKBLOCK_REALSIZE * WORKBLOCK_REALSIZE * 4];
    uchar8 outdata[WORKBLOCK_REALSIZE * WORKBLOCK_REALSIZE * 4];
    uchar8 *inbyte, *outbyte;
    sint x, y;
    sint srcx, srcy;

    ExpandHalfTextureToGrid(data, width, height);

    for(y = 0; y < height; y += WORKBLOCK_SIZE) {
        for(x = 0; x < width; x += WORKBLOCK_SIZE) {
            sint x2, y2;
            sint workwidth, workheight, fullworkwidth, fullworkheight;

            workwidth =  MIN(WORKBLOCK_SIZE, width  - x);
            workheight = MIN(WORKBLOCK_SIZE, height - y);

            fullworkwidth =  workwidth  + WORKBLOCK_BORDER * 2;
            fullworkheight = workheight + WORKBLOCK_BORDER * 2;

            //memset(workdata, 0, WORKBLOCK_REALSIZE * WORKBLOCK_REALSIZE * 4);

            // fill in work block
            for(y2 = 0; y2 < fullworkheight; y2 += 2) {
                srcy = y + y2 - WORKBLOCK_BORDER;

                if(clampToEdge) {
                    srcy = CLAMP(srcy, 0, height - 2);
                } else {
                    srcy = (srcy + height) % height;
                }

                outbyte = workdata + y2   * fullworkwidth * 4;
                inbyte  = data     + srcy * width         * 4;

                for(x2 = 0; x2 < fullworkwidth; x2 += 2) {
                    srcx = x + x2 - WORKBLOCK_BORDER;

                    if(clampToEdge) {
                        srcx = CLAMP(srcx, 0, width - 2);
                    } else {
                        srcx = (srcx + width) % width;
                    }

                    COPYSAMPLE(outbyte, inbyte + srcx * 4);
                    outbyte += 8;
                }
            }

            // submit work block
            DoLinear(workdata, outdata, fullworkwidth, fullworkheight);

            if(!normalized) {
                switch(r_imageUpsampleType->integer) {
                    case 0:
                        break;

                    case 1:
                        DoFCBIQuick(workdata, outdata, fullworkwidth, fullworkheight, 0);
                        break;

                    case 2:
                    default:
                        DoFCBI(workdata, outdata, fullworkwidth, fullworkheight, 0);
                        break;
                }
            } else {
                switch(r_imageUpsampleType->integer) {
                    case 0:
                        break;

                    case 1:
                        DoFCBIQuick(workdata, outdata, fullworkwidth, fullworkheight, 0);
                        DoFCBIQuick(workdata, outdata, fullworkwidth, fullworkheight, 1);
                        break;

                    case 2:
                    default:
                        DoFCBI(workdata, outdata, fullworkwidth, fullworkheight, 0);
                        DoFCBI(workdata, outdata, fullworkwidth, fullworkheight, 1);
                        break;
                }
            }

            // copy back work block
            for(y2 = 0; y2 < workheight; y2++) {
                inbyte = outdata + ((y2 + WORKBLOCK_BORDER) * fullworkwidth +
                                    WORKBLOCK_BORDER) * 4;
                outbyte = data + ((y + y2)                * width         + x)
                          * 4;

                for(x2 = 0; x2 < workwidth; x2++) {
                    COPYSAMPLE(outbyte, inbyte);
                    outbyte += 4;
                    inbyte += 4;
                }
            }
        }
    }
}
#undef COPYSAMPLE

/*
================
R_LightScaleTexture

Scale up the pixel values in a texture to increase the
lighting range
================
*/
void R_LightScaleTexture(uchar8 *in, sint inwidth, sint inheight,
                         bool only_gamma) {
    if(only_gamma) {
        return;
    } else {
        sint    i, c;
        uchar8 *p;

        p = in;

        c = inwidth * inheight;

        for(i = 0; i < c; i++, p += 4) {
            p[0] = s_intensitytable[p[0]];
            p[1] = s_intensitytable[p[1]];
            p[2] = s_intensitytable[p[2]];
        }
    }
}

/*
================
R_MipMapsRGB

Operates in place, quartering the size of the texture
Colors are gamma correct
================
*/
static void R_MipMapsRGB(uchar8 *in, sint inWidth, sint inHeight) {
    sint x, y, c, stride;
    const uchar8 *in2;
    float32 total;
    static float32 downmipSrgbLookup[256];
    static sint downmipSrgbLookupSet = 0;
    uchar8 *out = in;

    if(!downmipSrgbLookupSet) {
        for(x = 0; x < 256; x++) {
            downmipSrgbLookup[x] = powf(x / 255.0f, 2.2f) * 0.25f;
        }

        downmipSrgbLookupSet = 1;
    }

    if(inWidth == 1 && inHeight == 1) {
        return;
    }

    if(inWidth == 1 || inHeight == 1) {
        for(x = (inWidth * inHeight) >> 1; x; x--) {
            for(c = 3; c; c--, in++) {
                total  = (downmipSrgbLookup[*(in)] + downmipSrgbLookup[*(in + 4)]) * 2.0f;

                *out++ = static_cast<uchar8>(powf(total, 1.0f / 2.2f) * 255.0f);
            }

            *out++ = (*(in) + * (in + 4)) >> 1;
            in += 5;
        }

        return;
    }

    stride = inWidth * 4;
    inWidth >>= 1;
    inHeight >>= 1;

    in2 = in + stride;

    for(y = inHeight; y; y--, in += stride, in2 += stride) {
        for(x = inWidth; x; x--) {
            for(c = 3; c; c--, in++, in2++) {
                total = downmipSrgbLookup[*(in)]  + downmipSrgbLookup[*(in + 4)]
                        + downmipSrgbLookup[*(in2)] + downmipSrgbLookup[*(in2 + 4)];

                *out++ = static_cast<uchar8>(powf(total, 1.0f / 2.2f) * 255.0f);
            }

            *out++ = (*(in) + * (in + 4) + * (in2) + * (in2 + 4)) >> 2;
            in += 5, in2 += 5;
        }
    }
}


static void R_MipMapNormalHeight(const uchar8 *in, uchar8 *out, sint width,
                                 sint height, bool swizzle) {
    sint        i, j;
    sint        row;
    sint sx = swizzle ? 3 : 0;
    sint sa = swizzle ? 0 : 3;

    if(width == 1 && height == 1) {
        return;
    }

    row = width * 4;
    width >>= 1;
    height >>= 1;

    for(i = 0 ; i < height ; i++, in += row) {
        for(j = 0 ; j < width ; j++, out += 4, in += 8) {
            vec3_t v;

            v[0] =  OffsetByteToFloat(in[sx      ]);
            v[1] =  OffsetByteToFloat(in[       1]);
            v[2] =  OffsetByteToFloat(in[       2]);

            v[0] += OffsetByteToFloat(in[sx    + 4]);
            v[1] += OffsetByteToFloat(in[       5]);
            v[2] += OffsetByteToFloat(in[       6]);

            v[0] += OffsetByteToFloat(in[sx + row  ]);
            v[1] += OffsetByteToFloat(in[   row + 1]);
            v[2] += OffsetByteToFloat(in[   row + 2]);

            v[0] += OffsetByteToFloat(in[sx + row + 4]);
            v[1] += OffsetByteToFloat(in[   row + 5]);
            v[2] += OffsetByteToFloat(in[   row + 6]);

            VectorNormalizeFast(v);

            //v[0] *= 0.25f;
            //v[1] *= 0.25f;
            //v[2] = 1.0f - v[0] * v[0] - v[1] * v[1];
            //v[2] = sqrt(MAX(v[2], 0.0f));

            out[sx] = FloatToOffsetByte(v[0]);
            out[1 ] = FloatToOffsetByte(v[1]);
            out[2 ] = FloatToOffsetByte(v[2]);
            out[sa] = MAX(MAX(in[sa], in[sa + 4]), MAX(in[sa + row],
                          in[sa + row + 4]));
        }
    }
}


/*
==================
R_BlendOverTexture

Apply a color blend over a set of pixels
==================
*/
static void R_BlendOverTexture(uchar8 *data, sint pixelCount,
                               uchar8 blend[4]) {
    sint        i;
    sint        inverseAlpha;
    sint        premult[3];

    inverseAlpha = 255 - blend[3];
    premult[0] = blend[0] * blend[3];
    premult[1] = blend[1] * blend[3];
    premult[2] = blend[2] * blend[3];

    for(i = 0 ; i < pixelCount ; i++, data += 4) {
        data[0] = (data[0] * inverseAlpha + premult[0]) >> 9;
        data[1] = (data[1] * inverseAlpha + premult[1]) >> 9;
        data[2] = (data[2] * inverseAlpha + premult[2]) >> 9;
    }
}

uchar8  mipBlendColors[16][4] = {
    {0, 0, 0, 0},
    {255, 0, 0, 128},
    {0, 255, 0, 128},
    {0, 0, 255, 128},
    {255, 0, 0, 128},
    {0, 255, 0, 128},
    {0, 0, 255, 128},
    {255, 0, 0, 128},
    {0, 255, 0, 128},
    {0, 0, 255, 128},
    {255, 0, 0, 128},
    {0, 255, 0, 128},
    {0, 0, 255, 128},
    {255, 0, 0, 128},
    {0, 255, 0, 128},
    {0, 0, 255, 128},
};

static void RawImage_SwizzleRA(uchar8 *data, sint width, sint height) {
    sint i;
    uchar8 *ptr = data, swap;

    for(i = 0; i < width * height; i++, ptr += 4) {
        // swap red and alpha
        swap = ptr[0];
        ptr[0] = ptr[3];
        ptr[3] = swap;
    }
}


/*
===============
RawImage_ScaleToPower2

===============
*/
static bool RawImage_ScaleToPower2(uchar8 **data, sint *inout_width,
                                   sint *inout_height, imgType_t type, sint/*imgFlags_t*/ flags,
                                   uchar8 **resampledBuffer) {
    sint width =         *inout_width;
    sint height =        *inout_height;
    sint scaled_width;
    sint scaled_height;
    bool picmip = flags & IMGFLAG_PICMIP;
    bool mipmap = flags & IMGFLAG_MIPMAP;
    bool clampToEdge = flags & IMGFLAG_CLAMPTOEDGE;
    bool scaled;

    //
    // convert to exact power of 2 sizes
    //
    if(!mipmap) {
        scaled_width = width;
        scaled_height = height;
    } else {
        scaled_width = NextPowerOfTwo(width);
        scaled_height = NextPowerOfTwo(height);
    }

    if(r_roundImagesDown->integer && scaled_width > width) {
        scaled_width >>= 1;
    }

    if(r_roundImagesDown->integer && scaled_height > height) {
        scaled_height >>= 1;
    }

    if(picmip && data && resampledBuffer && r_imageUpsample->integer &&
            scaled_width < r_imageUpsampleMaxSize->integer &&
            scaled_height < r_imageUpsampleMaxSize->integer) {
        sint finalwidth, finalheight;
        //sint startTime, endTime;

        //startTime = clientRendererSystem->ScaledMilliseconds();

        finalwidth = scaled_width << r_imageUpsample->integer;
        finalheight = scaled_height << r_imageUpsample->integer;

        while(finalwidth > r_imageUpsampleMaxSize->integer
                || finalheight > r_imageUpsampleMaxSize->integer) {
            finalwidth >>= 1;
            finalheight >>= 1;
        }

        while(finalwidth > glConfig.maxTextureSize
                || finalheight > glConfig.maxTextureSize) {
            finalwidth >>= 1;
            finalheight >>= 1;
        }

        *resampledBuffer = static_cast<uchar8 *>(memorySystem->AllocateTempMemory(
                               finalwidth * finalheight * 4));

        if(scaled_width != width || scaled_height != height) {
            ResampleTexture(*data, width, height, *resampledBuffer, scaled_width,
                            scaled_height);
        } else {
            ::memcpy(*resampledBuffer, *data, width * height * 4);
        }

        if(type == IMGTYPE_COLORALPHA) {
            RGBAtoYCoCgA(*resampledBuffer, *resampledBuffer, scaled_width,
                         scaled_height);
        }

        while(scaled_width < finalwidth || scaled_height < finalheight) {
            scaled_width <<= 1;
            scaled_height <<= 1;

            FCBIByBlock(*resampledBuffer, scaled_width, scaled_height, clampToEdge,
                        (type == IMGTYPE_NORMAL || type == IMGTYPE_NORMALHEIGHT));
        }

        if(type == IMGTYPE_COLORALPHA) {
            YCoCgAtoRGBA(*resampledBuffer, *resampledBuffer, scaled_width,
                         scaled_height);
        } else if(type == IMGTYPE_NORMAL || type == IMGTYPE_NORMALHEIGHT) {
            FillInNormalizedZ(*resampledBuffer, *resampledBuffer, scaled_width,
                              scaled_height);
        }

        //endTime = clientRendererSystem->ScaledMilliseconds();

        //clientRendererSystem->RefPrintf(PRINT_ALL, "upsampled %dx%d to %dx%d in %dms\n", width, height, scaled_width, scaled_height, endTime - startTime);

        *data = *resampledBuffer;
    } else if(scaled_width != width || scaled_height != height) {
        if(data && resampledBuffer) {
            *resampledBuffer = static_cast<uchar8 *>(memorySystem->AllocateTempMemory(
                                   scaled_width * scaled_height * 4));
            ResampleTexture(*data, width, height, *resampledBuffer, scaled_width,
                            scaled_height);
            *data = *resampledBuffer;
        }
    }

    width  = scaled_width;
    height = scaled_height;

    //
    // perform optional picmip operation
    //
    if(picmip) {
        scaled_width >>= r_picmip->integer;
        scaled_height >>= r_picmip->integer;
    }

    //
    // clamp to the current upper OpenGL limit
    // scale both axis down equally so we don't have to
    // deal with a half mip resampling
    //
    while(scaled_width > glConfig.maxTextureSize
            || scaled_height > glConfig.maxTextureSize) {
        scaled_width >>= 1;
        scaled_height >>= 1;
    }

    //
    // clamp to minimum size
    //
    scaled_width  = MAX(1, scaled_width);
    scaled_height = MAX(1, scaled_height);

    scaled = (width != scaled_width) || (height != scaled_height);

    //
    // rescale texture to new size using existing mipmap functions
    //
    if(data) {
        while(width > scaled_width || height > scaled_height) {
            if(type == IMGTYPE_NORMAL || type == IMGTYPE_NORMALHEIGHT) {
                R_MipMapNormalHeight(*data, *data, width, height, false);
            } else {
                R_MipMapsRGB(*data, width, height);
            }

            width  = MAX(1, width >> 1);
            height = MAX(1, height >> 1);
        }
    }

    *inout_width  = width;
    *inout_height = height;

    return scaled;
}


static bool RawImage_HasAlpha(const uchar8 *scan, sint numPixels) {
    sint i;

    if(!scan) {
        return true;
    }

    for(i = 0; i < numPixels; i++) {
        if(scan[i * 4 + 3] != 255) {
            return true;
        }
    }

    return false;
}

static uint RawImage_GetFormat(const uchar8 *data, sint numPixels,
                               uint picFormat, bool lightMap, imgType_t type, sint/*imgFlags_t*/ flags) {
    sint samples = 3;
    uint internalFormat = GL_RGB;
    bool forceNoCompression = (flags & IMGFLAG_NO_COMPRESSION);
    bool normalmap = (type == IMGTYPE_NORMAL || type == IMGTYPE_NORMALHEIGHT);

    if(picFormat != GL_RGBA8) {
        return picFormat;
    }

    if(normalmap) {
        if((type == IMGTYPE_NORMALHEIGHT) && RawImage_HasAlpha(data, numPixels) &&
                r_parallaxMapping->integer) {
            if(!forceNoCompression && glRefConfig.textureCompression & TCR_BPTC) {
                internalFormat = GL_COMPRESSED_RGBA_BPTC_UNORM_ARB;
            } else if(!forceNoCompression &&
                      glConfig.textureCompression == TC_S3TC_ARB) {
                internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
            } else if(r_texturebits->integer == 16) {
                internalFormat = GL_RGBA4;
            } else if(r_texturebits->integer == 32) {
                internalFormat = GL_RGBA8;
            } else {
                internalFormat = GL_RGBA;
            }
        } else {
            if(!forceNoCompression && glRefConfig.textureCompression & TCR_RGTC) {
                internalFormat = GL_COMPRESSED_RG_RGTC2;
            } else if(!forceNoCompression &&
                      glRefConfig.textureCompression & TCR_BPTC) {
                internalFormat = GL_COMPRESSED_RGBA_BPTC_UNORM_ARB;
            } else if(!forceNoCompression &&
                      glConfig.textureCompression == TC_S3TC_ARB) {
                internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
            } else if(r_texturebits->integer == 16) {
                internalFormat = GL_RGB5;
            } else if(r_texturebits->integer == 32) {
                internalFormat = GL_RGB8;
            } else {
                internalFormat = GL_RGB;
            }
        }
    } else if(lightMap) {
        if(r_greyscale->integer) {
            internalFormat = GL_LUMINANCE;
        } else {
            internalFormat = GL_RGBA;
        }
    } else {
        if(RawImage_HasAlpha(data, numPixels)) {
            samples = 4;
        }

        // select proper internal format
        if(samples == 3) {
            if(r_greyscale->integer) {
                if(r_texturebits->integer == 16) {
                    internalFormat = GL_LUMINANCE8;
                } else if(r_texturebits->integer == 32) {
                    internalFormat = GL_LUMINANCE16;
                } else {
                    internalFormat = GL_LUMINANCE;
                }
            } else {
                if(!forceNoCompression && (glRefConfig.textureCompression & TCR_BPTC)) {
                    internalFormat = GL_COMPRESSED_RGBA_BPTC_UNORM_ARB;
                } else if(!forceNoCompression &&
                          glConfig.textureCompression == TC_S3TC_ARB) {
                    internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
                } else if(!forceNoCompression && glConfig.textureCompression == TC_S3TC) {
                    internalFormat = GL_RGB4_S3TC;
                } else if(r_texturebits->integer == 16) {
                    internalFormat = GL_RGB5;
                } else if(r_texturebits->integer == 32) {
                    internalFormat = GL_RGB8;
                } else {
                    internalFormat = GL_RGB;
                }
            }
        } else if(samples == 4) {
            if(r_greyscale->integer) {
                if(r_texturebits->integer == 16) {
                    internalFormat = GL_LUMINANCE8_ALPHA8;
                } else if(r_texturebits->integer == 32) {
                    internalFormat = GL_LUMINANCE16_ALPHA16;
                } else {
                    internalFormat = GL_LUMINANCE_ALPHA;
                }
            } else {
                if(!forceNoCompression && (glRefConfig.textureCompression & TCR_BPTC)) {
                    internalFormat = GL_COMPRESSED_RGBA_BPTC_UNORM_ARB;
                } else if(!forceNoCompression &&
                          glConfig.textureCompression == TC_S3TC_ARB) {
                    internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
                } else if(r_texturebits->integer == 16) {
                    internalFormat = GL_RGBA4;
                } else if(r_texturebits->integer == 32) {
                    internalFormat = GL_RGBA8;
                } else {
                    internalFormat = GL_RGBA;
                }
            }
        }
    }

    return internalFormat;
}

static void CompressMonoBlock(uchar8 outdata[8], const uchar8 indata[16]) {
    sint hi, lo, diff, bias, outbyte, shift, i;
    uchar8 *p = outdata;

    hi = lo = indata[0];

    for(i = 1; i < 16; i++) {
        hi = MAX(indata[i], hi);
        lo = MIN(indata[i], lo);
    }

    *p++ = hi;
    *p++ = lo;

    diff = hi - lo;

    if(diff == 0) {
        outbyte = (hi == 255) ? 255 : 0;

        for(i = 0; i < 6; i++) {
            *p++ = outbyte;
        }

        return;
    }

    bias = diff / 2 - lo * 7;
    outbyte = shift = 0;

    for(i = 0; i < 16; i++) {
        const uchar8 fixIndex[8] = { 1, 7, 6, 5, 4, 3, 2, 0 };
        uchar8 index = fixIndex[(indata[i] * 7 + bias) / diff];

        outbyte |= index << shift;
        shift += 3;

        if(shift >= 8) {
            *p++ = outbyte & 0xff;
            shift -= 8;
            outbyte >>= 8;
        }
    }
}

static void RawImage_UploadToRgtc2Texture(uint texture, sint miplevel,
        sint x, sint y, sint width, sint height, uchar8 *data) {
    sint wBlocks, hBlocks, iy, ix, size;
    uchar8 *compressedData, *p = nullptr;

    wBlocks = (width + 3) / 4;
    hBlocks = (height + 3) / 4;
    size = wBlocks * hBlocks * 16;

    p = compressedData = static_cast<uchar8 *>
                         (memorySystem->AllocateTempMemory(size));

    for(iy = 0; iy < height; iy += 4) {
        sint oh = MIN(4, height - iy);

        for(ix = 0; ix < width; ix += 4) {
            uchar8 workingData[16];
            sint component;

            sint ow = MIN(4, width - ix);

            for(component = 0; component < 2; component++) {
                sint ox, oy;

                for(oy = 0; oy < oh; oy++)
                    for(ox = 0; ox < ow; ox++) {
                        workingData[oy * 4 + ox] = data[((iy + oy) * width + ix + ox) * 4 +
                                                        component];
                    }

                // dupe data to fill
                for(oy = 0; oy < 4; oy++)
                    for(ox = (oy < oh) ? ow : 0; ox < 4; ox++) {
                        workingData[oy * 4 + ox] = workingData[(oy % oh) * 4 + ox % ow];
                    }

                CompressMonoBlock(p, workingData);
                p += 8;
            }
        }
    }

    // FIXME: Won't work for x/y that aren't multiples of 4.
    qglCompressedTextureSubImage2DEXT(texture, GL_TEXTURE_2D, miplevel, x, y,
                                      width, height, GL_COMPRESSED_RG_RGTC2, size, compressedData);

    memorySystem->FreeTempMemory(compressedData);
}

static sint CalculateMipSize(sint width, sint height, uint picFormat) {
    sint numBlocks = ((width + 3) / 4) * ((height + 3) / 4);
    sint numPixels = width * height;

    switch(picFormat) {
        case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
        case GL_COMPRESSED_SRGB_S3TC_DXT1_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
        case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:
        case GL_COMPRESSED_RED_RGTC1:
        case GL_COMPRESSED_SIGNED_RED_RGTC1:
            return numBlocks * 8;

        case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
        case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
        case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:
        case GL_COMPRESSED_RG_RGTC2:
        case GL_COMPRESSED_SIGNED_RG_RGTC2:
        case GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_ARB:
        case GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT_ARB:
        case GL_COMPRESSED_RGBA_BPTC_UNORM_ARB:
        case GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB:
            return numBlocks * 16;

        case GL_RGBA8:
        case GL_SRGB8_ALPHA8_EXT:
            return numPixels * 4;

        case GL_RGBA16:
            return numPixels * 8;

        default:
            clientRendererSystem->RefPrintf(PRINT_ALL,
                                            "Unsupported texture format %08x\n", picFormat);
            return 0;
    }

    return 0;
}


static uint PixelDataFormatFromInternalFormat(uint internalFormat) {
    switch(internalFormat) {
        case GL_DEPTH_COMPONENT:
        case GL_DEPTH_COMPONENT16_ARB:
        case GL_DEPTH_COMPONENT24_ARB:
        case GL_DEPTH_COMPONENT32_ARB:
            return GL_DEPTH_COMPONENT;

        default:
            return GL_RGBA;
            break;
    }
}

static void RawImage_UploadTexture(uint texture, uchar8 *data, sint x,
                                   sint y, sint width, sint height, uint target, uint picFormat,
                                   sint numMips, uint internalFormat, imgType_t type,
                                   sint/*imgFlags_t*/ flags, bool subtexture) {
    uint dataFormat, dataType;
    sint size, miplevel;
    bool rgtc = internalFormat == GL_COMPRESSED_RG_RGTC2;
    bool rgba8 = picFormat == GL_RGBA8 || picFormat == GL_SRGB8_ALPHA8_EXT;
    bool rgba = rgba8 || picFormat == GL_RGBA16;
    bool mipmap = !!(flags & IMGFLAG_MIPMAP);
    bool lastMip = false;

    dataFormat = PixelDataFormatFromInternalFormat(internalFormat);
    dataType = picFormat == GL_RGBA16 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_BYTE;

    miplevel = 0;

    do {
        lastMip = (width == 1 && height == 1) || !mipmap;
        size = CalculateMipSize(width, height, picFormat);

        if(!rgba) {
            qglCompressedTextureSubImage2DEXT(texture, target, miplevel, x, y, width,
                                              height, picFormat, size, data);
        } else {
            if(rgba8 && miplevel != 0 && r_colorMipLevels->integer) {
                R_BlendOverTexture(static_cast<uchar8 *>(data), width * height,
                                   mipBlendColors[miplevel]);
            }

            if(rgba8 && rgtc) {
                RawImage_UploadToRgtc2Texture(texture, miplevel, x, y, width, height,
                                              data);
            } else {
                qglTextureSubImage2DEXT(texture, target, miplevel, x, y, width, height,
                                        dataFormat, dataType, data);
            }
        }

        if(!lastMip && numMips < 2) {
            if(glRefConfig.framebufferObject) {
                qglGenerateTextureMipmapEXT(texture, target);
                break;
            } else if(rgba8) {
                if(type == IMGTYPE_NORMAL || type == IMGTYPE_NORMALHEIGHT) {
                    R_MipMapNormalHeight(data, data, width, height,
                                         glRefConfig.swizzleNormalmap);
                } else {
                    R_MipMapsRGB(data, width, height);
                }
            }
        }

        x >>= 1;
        y >>= 1;
        width = MAX(1, width >> 1);
        height = MAX(1, height >> 1);
        miplevel++;

        if(numMips > 1) {
            data += size;
            numMips--;
        }
    } while(!lastMip);
}


/*
===============
Upload32

===============
*/
static void Upload32(uchar8 *data, sint x, sint y, sint width, sint height,
                     uint picFormat, sint numMips, image_t *image, bool scaled) {
    sint i, c;
    uchar8 *scan;

    imgType_t type = image->type;
    sint/*imgFlags_t*/ flags = image->flags;
    uint internalFormat = image->internalFormat;
    bool rgba8 = picFormat == GL_RGBA8 || picFormat == GL_SRGB8_ALPHA8_EXT;
    bool mipmap = !!(flags & IMGFLAG_MIPMAP) && (rgba8 || numMips > 1);
    bool cubemap = !!(flags & IMGFLAG_CUBEMAP);

    // These operations cannot be performed on non-rgba8 images.
    if(rgba8 && !cubemap) {
        c = width * height;
        scan = data;

        if(type == IMGTYPE_COLORALPHA) {
            if(r_greyscale->integer) {
                for(i = 0; i < c; i++) {
                    uchar8 luma = LUMA(scan[i * 4], scan[i * 4 + 1], scan[i * 4 + 2]);
                    scan[i * 4] = luma;
                    scan[i * 4 + 1] = luma;
                    scan[i * 4 + 2] = luma;
                }
            } else if(r_greyscale->value) {
                for(i = 0; i < c; i++) {
                    float32 luma = LUMA(scan[i * 4], scan[i * 4 + 1], scan[i * 4 + 2]);
                    scan[i * 4] = LERP(scan[i * 4], luma, r_greyscale->value);
                    scan[i * 4 + 1] = LERP(scan[i * 4 + 1], luma, r_greyscale->value);
                    scan[i * 4 + 2] = LERP(scan[i * 4 + 2], luma, r_greyscale->value);
                }
            }

            // This corresponds to what the OpenGL1 renderer does.
            if(!(flags & IMGFLAG_NOLIGHTSCALE) && (scaled || mipmap)) {
                R_LightScaleTexture(data, width, height, !mipmap);
            }
        }

        if(glRefConfig.swizzleNormalmap && (type == IMGTYPE_NORMAL ||
                                            type == IMGTYPE_NORMALHEIGHT)) {
            RawImage_SwizzleRA(data, width, height);
        }
    }

    if(cubemap) {
        for(i = 0; i < 6; i++) {
            sint w2 = width, h2 = height;
            RawImage_UploadTexture(image->texnum, data, x, y, width, height,
                                   GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, picFormat, numMips, internalFormat,
                                   type, flags, false);

            for(c = numMips; c; c--) {
                data += CalculateMipSize(w2, h2, picFormat);
                w2 = MAX(1, w2 >> 1);
                h2 = MAX(1, h2 >> 1);
            }
        }
    } else {
        RawImage_UploadTexture(image->texnum, data, x, y, width, height,
                               GL_TEXTURE_2D, picFormat, numMips, internalFormat, type, flags, false);
    }

    GL_CheckErrors();
}


/*
================
R_CreateImage2

This is the only way any image_t are created
================
*/
image_t *R_CreateImage2(pointer name, uchar8 *pic, sint width, sint height,
                        uint picFormat, sint numMips, imgType_t type, sint/*imgFlags_t*/ flags,
                        sint internalFormat) {
    uchar8 *resampledBuffer = nullptr;
    image_t *image = nullptr;
    bool isLightmap = false, scaled = false;
    sint32 hash;
    sint glWrapClampMode, mipWidth, mipHeight, miplevel;
    bool rgba8 = picFormat == GL_RGBA8 || picFormat == GL_SRGB8_ALPHA8_EXT;
    bool mipmap = !!(flags & IMGFLAG_MIPMAP);
    bool cubemap = !!(flags & IMGFLAG_CUBEMAP);
    bool picmip = !!(flags & IMGFLAG_PICMIP);
    bool lastMip;
    uint textureTarget = cubemap ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;
    uint dataFormat;

    if(strlen(name) >= MAX_QPATH) {
        Com_Error(ERR_DROP, "R_CreateImage: \"%s\" is too sint32", name);
    }

    if(!strncmp(name, "*lightmap", 9)) {
        isLightmap = true;
    }

    if(tr.numImages == MAX_DRAWIMAGES) {
        Com_Error(ERR_DROP, "R_CreateImage: MAX_DRAWIMAGES hit");
        return nullptr;
    }

    image = tr.images[tr.numImages] = reinterpret_cast<image_t *>
                                      (memorySystem->Alloc(
                                           sizeof(image_t), h_low));
    qglGenTextures(1, &image->texnum);
    tr.numImages++;

    image->type = type;
    image->flags = flags;

    Q_strcpy_s(image->imgName, name);

    image->width = width;
    image->height = height;

    if(flags & IMGFLAG_CLAMPTOEDGE) {
        glWrapClampMode = GL_CLAMP_TO_EDGE;
    } else {
        glWrapClampMode = GL_REPEAT;
    }

    if(!internalFormat) {
        internalFormat = RawImage_GetFormat(pic, width * height, picFormat,
                                            isLightmap, image->type, image->flags);
    }

    image->internalFormat = internalFormat;

    // lightmaps are always allocated on TMU 1
    if(isLightmap) {
        image->TMU = 1;
    } else {
        image->TMU = 0;
    }

    // Possibly scale image before uploading.
    // if not rgba8 and uploading an image, skip picmips.
    if(!cubemap) {
        if(rgba8) {
            scaled = RawImage_ScaleToPower2(&pic, &width, &height, type, flags,
                                            &resampledBuffer);
        } else if(pic && picmip) {
            for(miplevel = r_picmip->integer; miplevel > 0 &&
                    numMips > 1; miplevel--, numMips--) {
                sint size = CalculateMipSize(width, height, picFormat);
                width = MAX(1, width >> 1);
                height = MAX(1, height >> 1);
                pic += size;
            }
        }
    }

    image->uploadWidth = width;
    image->uploadHeight = height;

    sint format = GL_BGRA;

    if(internalFormat == GL_DEPTH_COMPONENT24) {
        format = GL_DEPTH_COMPONENT;
    }

    // Allocate texture storage so we don't have to worry about it later.
    dataFormat = PixelDataFormatFromInternalFormat(internalFormat);
    mipWidth = width;
    mipHeight = height;
    miplevel = 0;

    do {
        lastMip = !mipmap || (mipWidth == 1 && mipHeight == 1);

        if(cubemap) {
            sint i;

            for(i = 0; i < 6; i++) {
                qglTextureImage2DEXT(image->texnum, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                                     miplevel, internalFormat, mipWidth, mipHeight, 0, dataFormat,
                                     GL_UNSIGNED_BYTE, nullptr);
            }
        } else {
            qglTextureImage2DEXT(image->texnum, GL_TEXTURE_2D, miplevel,
                                 internalFormat, mipWidth, mipHeight, 0, format, GL_UNSIGNED_BYTE, nullptr);
        }

        mipWidth  = MAX(1, mipWidth >> 1);
        mipHeight = MAX(1, mipHeight >> 1);
        miplevel++;
    } while(!lastMip);

    // Upload data.
    if(pic) {
        Upload32(pic, 0, 0, width, height, picFormat, numMips, image, scaled);
    }

    if(resampledBuffer != nullptr) {
        memorySystem->FreeTempMemory(resampledBuffer);
    }

    // Set all necessary texture parameters.
    qglTextureParameterfEXT(image->texnum, textureTarget, GL_TEXTURE_WRAP_S,
                            glWrapClampMode);
    qglTextureParameterfEXT(image->texnum, textureTarget, GL_TEXTURE_WRAP_T,
                            glWrapClampMode);

    if(cubemap) {
        qglTextureParameteriEXT(image->texnum, textureTarget, GL_TEXTURE_WRAP_R,
                                glWrapClampMode);
    }

    if(glConfig.textureFilterAnisotropic && !cubemap)
        qglTextureParameteriEXT(image->texnum, textureTarget,
                                GL_TEXTURE_MAX_ANISOTROPY_EXT,
                                mipmap ? static_cast<sint>(Com_Clamp(1, glConfig.maxAnisotropy,
                                        r_ext_max_anisotropy->integer)) : 1);

    switch(internalFormat) {
        case GL_DEPTH_COMPONENT:
        case GL_DEPTH_COMPONENT16:
        case GL_DEPTH_COMPONENT24:
        case GL_DEPTH_COMPONENT32:
            // Fix for sampling depth buffer on old nVidia cards.
            // from http://www.idevgames.com/forums/thread-4141-post-34844.html#pid34844
            qglTextureParameterfEXT(image->texnum, textureTarget,
                                    GL_DEPTH_TEXTURE_MODE, GL_LUMINANCE);
            qglTextureParameterfEXT(image->texnum, textureTarget,
                                    GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            qglTextureParameterfEXT(image->texnum, textureTarget,
                                    GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            break;

        default:
            qglTextureParameterfEXT(image->texnum, textureTarget,
                                    GL_TEXTURE_MIN_FILTER, mipmap ? gl_filter_min : GL_LINEAR);
            qglTextureParameterfEXT(image->texnum, textureTarget,
                                    GL_TEXTURE_MAG_FILTER, mipmap ? gl_filter_max : GL_LINEAR);
            break;
    }

    GL_CheckErrors();

    hash = generateHashValue(name);
    image->next = hashTable[hash];
    hashTable[hash] = image;

    return image;
}


/*
================
R_CreateImage

Wrapper for R_CreateImage2(), for the old parameters.
================
*/
image_t *R_CreateImage(pointer name, uchar8 *pic, sint width, sint height,
                       imgType_t type, sint/*imgFlags_t*/ flags, sint internalFormat) {
    return R_CreateImage2(name, pic, width, height, GL_RGBA8, 0, type, flags,
                          internalFormat);
}


void R_UpdateSubImage(image_t *image, uchar8 *pic, sint x, sint y,
                      sint width, sint height, uint picFormat) {
    Upload32(pic, x, y, width, height, picFormat, 0, image, false);
}

//===================================================================

// Prototype for dds loader function which isn't common to both renderers
void R_LoadDDS(pointer filename, uchar8 **pic, sint *width, sint *height,
               uint *picFormat, sint *numMips);

typedef struct {
    pointer ext;
    void (*ImageLoader)(pointer, uchar8 **, sint *, sint *);
} imageExtToLoaderMap_t;

// Note that the ordering indicates the order of preference used
// when there are multiple images of different formats available
static imageExtToLoaderMap_t imageLoaders[ ] = {
    { "png",  R_LoadPNG },
    { "tga",  R_LoadTGA },
    { "jpg",  R_LoadJPG },
    { "jpeg", R_LoadJPG }
};

static sint numImageLoaders = ARRAY_LEN(imageLoaders);

/*
=================
R_LoadImage

Loads any of the supported image types into a cannonical
32 bit format.
=================
*/
void R_LoadImage(pointer name, uchar8 **pic, sint *width, sint *height,
                 uint *picFormat, sint *numMips) {
    bool orgNameFailed = false;
    sint orgLoader = -1;
    sint i;
    valueType localName[ MAX_QPATH ];
    pointer ext;
    pointer altName;

    *pic = nullptr;
    *width = 0;
    *height = 0;
    *picFormat = GL_RGBA8;
    *numMips = 0;

    Q_strncpyz(localName, name, MAX_QPATH);

    ext = COM_GetExtension(localName);

    // If compressed textures are enabled, try loading a DDS first, it'll load fastest
    if(r_ext_compressed_textures->integer) {
        valueType ddsName[MAX_QPATH];

        COM_StripExtension3(name, ddsName, MAX_QPATH);
        Q_strcat(ddsName, MAX_QPATH, ".dds");

        R_LoadDDS(ddsName, pic, width, height, picFormat, numMips);

        // If loaded, we're done.
        if(*pic) {
            return;
        }
    }

    if(*ext) {
        // Look for the correct loader and use it
        for(i = 0; i < numImageLoaders; i++) {
            if(!Q_stricmp(ext, imageLoaders[ i ].ext)) {
                // Load
                imageLoaders[ i ].ImageLoader(localName, pic, width, height);
                break;
            }
        }

        // A loader was found
        if(i < numImageLoaders) {
            if(*pic == nullptr) {
                // Loader failed, most likely because the file isn't there;
                // try again without the extension
                orgNameFailed = true;
                orgLoader = i;
                COM_StripExtension3(name, localName, MAX_QPATH);
            } else {
                // Something loaded
                return;
            }
        }
    }

    // Try and find a suitable match using all
    // the image formats supported
    for(i = 0; i < numImageLoaders; i++) {
        if(i == orgLoader) {
            continue;
        }

        altName = va("%s.%s", localName, imageLoaders[ i ].ext);

        // Load
        imageLoaders[ i ].ImageLoader(altName, pic, width, height);

        if(*pic) {
            if(orgNameFailed) {
#ifdef _DEBUG
                clientRendererSystem->RefPrintf(PRINT_DEVELOPER,
                                                "WARNING: %s not present, using %s instead\n", name, altName);
#endif
            }

            break;
        }
    }
}


/*
===============
R_FindImageFile

Finds or loads the given image.
Returns nullptr if it fails, not a default image.
==============
*/
image_t    *R_FindImageFile(pointer name, imgType_t type,
                            sint/*imgFlags_t*/ flags) {
    image_t    *image;
    sint        width, height;
    uchar8 *pic;
    uint  picFormat;
    sint picNumMips;
    sint32  hash;
    sint/*imgFlags_t*/ checkFlagsTrue, checkFlagsFalse;

    if(!name) {
        return nullptr;
    }

    hash = generateHashValue(name);

    //
    // see if the image is already loaded
    //
    for(image = hashTable[hash]; image; image = image->next) {
        if(!strcmp(name, image->imgName)) {
            // the white image can be used with any set of parms, but other mismatches are errors
            if(strcmp(name, "*white")) {
                if(image->flags != flags) {
                    clientRendererSystem->RefPrintf(PRINT_DEVELOPER,
                                                    "WARNING: reused image %s with mixed flags (%i vs %i)\n", name,
                                                    image->flags, flags);
                }
            }

            return image;
        }
    }

    //
    // load the pic from disk
    //
    R_LoadImage(name, &pic, &width, &height, &picFormat, &picNumMips);

    if(pic == nullptr) {
        clientRendererSystem->RefPrintf(PRINT_DEVELOPER,
                                        "R_FindImageFile(%s, %i, %i) failed\n", name,
                                        type, flags);
        return nullptr;
    }

    checkFlagsTrue = IMGFLAG_PICMIP | IMGFLAG_MIPMAP | IMGFLAG_GENNORMALMAP;
    checkFlagsFalse = IMGFLAG_CUBEMAP;

    if(r_normalMapping->integer && (picFormat == GL_RGBA8) &&
            (type == IMGTYPE_COLORALPHA) &&
            ((flags & checkFlagsTrue) == checkFlagsTrue) &&
            !(flags & checkFlagsFalse)) {
        valueType normalName[MAX_QPATH];
        image_t *normalImage;
        sint normalWidth, normalHeight;
        sint/*imgFlags_t*/ normalFlags;

        normalFlags = (flags & ~IMGFLAG_GENNORMALMAP) | IMGFLAG_NOLIGHTSCALE;

        COM_StripExtension3(name, normalName, MAX_QPATH);
        Q_strcat(normalName, MAX_QPATH, "_n");

        // find normalmap in case it's there
        normalImage = R_FindImageFile(normalName, IMGTYPE_NORMAL, normalFlags);

        // if not, generate it
        if(normalImage == nullptr) {
            uchar8 *normalPic;
            sint x, y;

            normalWidth = width;
            normalHeight = height;
            normalPic = static_cast<uchar8 *>(clientRendererSystem->RefMalloc(
                                                  width * height * 4));
            RGBAtoNormal(pic, normalPic, width, height, flags & IMGFLAG_CLAMPTOEDGE);

#if 1
            // Brighten up the original image to work with the normal map
            RGBAtoYCoCgA(pic, pic, width, height);

            for(y = 0; y < height; y++) {
                uchar8 *picbyte  = pic       + y * width * 4;
                uchar8 *normbyte = normalPic + y * width * 4;

                for(x = 0; x < width; x++) {
                    sint div = MAX(normbyte[2] - 127, 16);
                    picbyte[0] = CLAMP(picbyte[0] * 128 / div, 0, 255);
                    picbyte  += 4;
                    normbyte += 4;
                }
            }

            YCoCgAtoRGBA(pic, pic, width, height);
#else
            // Blur original image's luma to work with the normal map
            {
                uchar8 *blurPic;

                RGBAtoYCoCgA(pic, pic, width, height);
                blurPic = clientRendererSystem->RefMalloc(width * height);

                for(y = 1; y < height - 1; y++) {
                    uchar8 *picbyte  = pic     + y * width * 4;
                    uchar8 *blurbyte = blurPic + y * width;

                    picbyte += 4;
                    blurbyte += 1;

                    for(x = 1; x < width - 1; x++) {
                        sint result;

                        result = *(picbyte - (width + 1) * 4) + *(picbyte - width * 4) + *
                                 (picbyte - (width - 1) * 4) +
                                 *(picbyte -          1  * 4) + *(picbyte) + *(picbyte +          1  * 4) +
                                 *(picbyte + (width - 1) * 4) + *(picbyte + width * 4) + *(picbyte +
                                         (width + 1) * 4);

                        result /= 9;

                        *blurbyte = result;
                        picbyte += 4;
                        blurbyte += 1;
                    }
                }

                // FIXME: do borders

                for(y = 1; y < height - 1; y++) {
                    uchar8 *picbyte  = pic     + y * width * 4;
                    uchar8 *blurbyte = blurPic + y * width;

                    picbyte += 4;
                    blurbyte += 1;

                    for(x = 1; x < width - 1; x++) {
                        picbyte[0] = *blurbyte;
                        picbyte += 4;
                        blurbyte += 1;
                    }
                }

                memorySystem->Free(blurPic);

                YCoCgAtoRGBA(pic, pic, width, height);
            }
#endif

            R_CreateImage(normalName, normalPic, normalWidth, normalHeight,
                          IMGTYPE_NORMAL, normalFlags, 0);
            memorySystem->Free(normalPic);
        }
    }

    // force mipmaps off if image is compressed but doesn't have enough mips
    if((flags & IMGFLAG_MIPMAP) && picFormat != GL_RGBA8 &&
            picFormat != GL_SRGB8_ALPHA8_EXT) {
        sint wh = MAX(width, height);
        sint neededMips = 0;

        while(wh) {
            neededMips++;
            wh >>= 1;
        }

        if(neededMips > picNumMips) {
            flags &= ~IMGFLAG_MIPMAP;
        }
    }

    image = R_CreateImage2(const_cast< valueType * >(name), pic, width, height,
                           picFormat, picNumMips, type, flags, 0);
    memorySystem->Free(pic);
    return image;
}


/*
================
R_CreateDlightImage
================
*/
#define DLIGHT_SIZE 16
static void R_CreateDlightImage(void) {
    sint        x, y;
    uchar8  data[DLIGHT_SIZE][DLIGHT_SIZE][4];
    sint        b;

    // make a centered inverse-square falloff blob for dynamic lighting
    for(x = 0 ; x < DLIGHT_SIZE ; x++) {
        for(y = 0 ; y < DLIGHT_SIZE ; y++) {
            float32 d;

            d = (DLIGHT_SIZE / 2 - 0.5f - x) * (DLIGHT_SIZE / 2 - 0.5f - x) +
                (DLIGHT_SIZE / 2 - 0.5f - y) * (DLIGHT_SIZE / 2 - 0.5f - y);
            b = 4000 / d;

            if(b > 255) {
                b = 255;
            } else if(b < 75) {
                b = 0;
            }

            data[y][x][0] =
                data[y][x][1] =
                    data[y][x][2] = b;
            data[y][x][3] = 255;
        }
    }

    tr.dlightImage = R_CreateImage("*dlight", reinterpret_cast<uchar8 *>(data),
                                   DLIGHT_SIZE, DLIGHT_SIZE, IMGTYPE_COLORALPHA, IMGFLAG_CLAMPTOEDGE, 0);
}


/*
=================
R_InitFogTable
=================
*/
void R_InitFogTable(void) {
    sint        i;
    float32 d;
    float32 exp;

    exp = 0.5;

    for(i = 0 ; i < FOG_TABLE_SIZE ; i++) {
        d = pow(static_cast<float32>(i) / (FOG_TABLE_SIZE - 1), exp);

        tr.fogTable[i] = d;
    }
}

/*
================
R_FogFactor

Returns a 0.0 to 1.0 fog density value
This is called for each texel of the fog texture on startup
and for each vertex of transparent shaders in fog dynamically
================
*/
float32 R_FogFactor(float32 s, float32 t) {
    float32 d;

    s -= 1.0f / 512;

    if(s < 0) {
        return 0;
    }

    if(t < 1.0f / 32.0f) {
        return 0;
    }

    if(t < 31.0f / 32.0f) {
        s *= (t - 1.0f / 32.0f) / (30.0f / 32.0f);
    }

    // we need to leave a lot of clamp range
    s *= 8;

    if(s > 1.0) {
        s = 1.0;
    }

    d = tr.fogTable[static_cast<sint>(s * (FOG_TABLE_SIZE - 1)) ];

    return d;
}

/*
================
R_CreateFogImage
================
*/
#define FOG_S   256
#define FOG_T   32
static void R_CreateFogImage(void) {
    sint        x, y;
    uchar8 *data = nullptr;
    float32 d;

    data = static_cast<uchar8 *>(memorySystem->AllocateTempMemory(
                                     FOG_S * FOG_T * 4));

    // S is distance, T is depth
    for(x = 0 ; x < FOG_S ; x++) {
        for(y = 0 ; y < FOG_T ; y++) {
            d = R_FogFactor((x + 0.5f) / FOG_S, (y + 0.5f) / FOG_T);

            data[(y * FOG_S + x) * 4 + 0] =
                data[(y * FOG_S + x) * 4 + 1] =
                    data[(y * FOG_S + x) * 4 + 2] = 255;
            data[(y * FOG_S + x) * 4 + 3] = 255 * d;
        }
    }

    tr.fogImage = R_CreateImage("*fog", reinterpret_cast<uchar8 *>(data),
                                FOG_S, FOG_T, IMGTYPE_COLORALPHA, IMGFLAG_CLAMPTOEDGE, 0);
    memorySystem->FreeTempMemory(data);
}

/*
================
R_CreateEnvBrdfLUT
from https://github.com/knarkowicz/IntegrateDFG
================
*/
#define LUT_WIDTH   128
#define LUT_HEIGHT  128
static void R_CreateEnvBrdfLUT(void) {
    if(!r_cubeMapping->integer) {
        return;
    }

    sint        x, y;
    uchar16    data[LUT_WIDTH][LUT_HEIGHT][4];

    float32 const MATH_PI = 3.14159f;
    uint const sampleNum = 1024;

    for(uint y = 0; y < LUT_HEIGHT; ++y) {
        float32 const ndotv = (y + 0.5f) / LUT_HEIGHT;
        float32 const vy = 0.0f;
        float32 const vz = ndotv;

        for(uint x = 0; x < LUT_WIDTH; ++x) {
            float32 const roughness = (x + 0.5f) / LUT_WIDTH;
            float32 const m = roughness * roughness;

            float32 const vx = sqrtf(1.0f - ndotv * ndotv);
            float32 const m2 = m * m;

            float32 scale = 0.0f;
            float32 bias = 0.0f;

            for(uint i = 0; i < sampleNum; ++i) {
                float32 const e1 = static_cast<float32>(i) / sampleNum;
                float32 const e2 = static_cast<float32>(static_cast<float64>(ReverseBits(
                        i)) / static_cast<float64>(0x100000000LL));

                float32 const phi = 2.0f * MATH_PI * e1;
                float32 const cosPhi = cosf(phi);
                float32 const sinPhi = sinf(phi);
                float32 const cosTheta = sqrtf((1.0f - e2) / (1.0f +
                                               (m2 - 1.0f) * e2));
                float32 const sinTheta = sqrtf(1.0f - cosTheta * cosTheta);

                float32 const hx = sinTheta * cosf(phi);
                float32 const hy = sinTheta * sinf(phi);
                float32 const hz = cosTheta;

                float32 const vdh = vx * hx + vy * hy + vz * hz;
                float32 const lx = 2.0f * vdh * hx - vx;
                float32 const ly = 2.0f * vdh * hy - vy;
                float32 const lz = 2.0f * vdh * hz - vz;

                float32 const ndotl = MAX(lz, 0.0f);
                float32 const ndoth = MAX(hz, 0.0f);
                float32 const vdoth = MAX(vdh, 0.0f);

                if(ndotl > 0.0f) {
                    float32 const visibility = GSmithCorrelated(roughness, ndotv, ndotl);
                    float32 const ndotlVisPDF = ndotl * visibility * (4.0f * vdoth / ndoth);
                    float32 const fresnel = powf(1.0f - vdoth, 5.0f);

                    scale += ndotlVisPDF * (1.0f - fresnel);
                    bias += ndotlVisPDF * fresnel;
                }
            }

            scale /= sampleNum;
            bias /= sampleNum;

            data[y][x][0] = FloatToHalf(scale);
            data[y][x][1] = FloatToHalf(bias);
            data[y][x][2] = 0.0f;
            data[y][x][3] = 0.0f;
        }
    }

    tr.envBrdfImage = R_CreateImage("*envBrdfLUT",
                                    reinterpret_cast<uchar8 *>(data), 128, 128, IMGTYPE_COLORALPHA,
                                    IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE, GL_RGBA16F);
    return;
}

static sint Hex(valueType c) {
    if(c >= '0' && c <= '9') {
        return c - '0';
    }

    if(c >= 'A' && c <= 'F') {
        return 10 + c - 'A';
    }

    if(c >= 'a' && c <= 'f') {
        return 10 + c - 'a';
    }

    return -1;
}

/*
==================
R_BuildDefaultImage
Create solid color texture from following input formats (hex):
#rgb
#rrggbb
==================
*/
#define DEFAULT_SIZE 16
static bool R_BuildDefaultImage(pointer format) {
    uchar8 data[DEFAULT_SIZE][DEFAULT_SIZE][4];
    uchar8 color[4];
    sint i, len, hex[6];
    sint x, y;

    if(*format++ != '#') {
        return false;
    }

    len = static_cast<sint>(::strlen(format));

    if(len <= 0 || len > 6) {
        return false;
    }

    for(i = 0; i < len; i++) {
        hex[i] = Hex(format[i]);

        if(hex[i] == -1) {
            return false;
        }
    }

    switch(len) {
        case 3: // #rgb
            color[0] = hex[0] << 4 | hex[0];
            color[1] = hex[1] << 4 | hex[1];
            color[2] = hex[2] << 4 | hex[2];
            color[3] = 255;
            break;

        case 6: // #rrggbb
            color[0] = hex[0] << 4 | hex[1];
            color[1] = hex[2] << 4 | hex[3];
            color[2] = hex[4] << 4 | hex[5];
            color[3] = 255;
            break;

        default: // unsupported format
            return false;
    }

    for(y = 0; y < DEFAULT_SIZE; y++) {
        for(x = 0; x < DEFAULT_SIZE; x++) {
            data[x][y][0] = color[0];
            data[x][y][1] = color[1];
            data[x][y][2] = color[2];
            data[x][y][3] = color[3];
        }
    }

    tr.defaultImage = R_CreateImage("*default",
                                    reinterpret_cast<uchar8 *>(data), DEFAULT_SIZE, DEFAULT_SIZE,
                                    IMGTYPE_COLORALPHA, IMGFLAG_MIPMAP, 0);

    return true;
}

/*
==================
R_CreateDefaultImage
==================
*/
#define DEFAULT_IMG_SIZE 128
static void R_CreateDefaultImage(void) {
    sint x, flags = IMGFLAG_MIPMAP | IMGFLAG_PICMIP;
    uchar8 data[DEFAULT_IMG_SIZE][DEFAULT_IMG_SIZE][4];

    if(r_defaultImage->string[0]) {
        // build from format
        if(R_BuildDefaultImage(r_defaultImage->string)) {
            return;
        }

        // load from external file
        tr.defaultImage = R_FindImageFile(r_defaultImage->string,
                                          IMGTYPE_COLORALPHA, flags);

        if(tr.defaultImage) {
            return;
        }
    }

    // the default image will be a box, to allow you to see the mapping coordinates
    memset(data, 32, sizeof(data));

    for(x = 0; x < DEFAULT_SIZE; x++) {
        data[0][x][0] =
            data[0][x][1] =
                data[0][x][2] =
                    data[0][x][3] = 255;

        data[x][0][0] =
            data[x][0][1] =
                data[x][0][2] =
                    data[x][0][3] = 255;

        data[DEFAULT_SIZE - 1][x][0] =
            data[DEFAULT_SIZE - 1][x][1] =
                data[DEFAULT_SIZE - 1][x][2] =
                    data[DEFAULT_SIZE - 1][x][3] = 255;

        data[x][DEFAULT_SIZE - 1][0] =
            data[x][DEFAULT_SIZE - 1][1] =
                data[x][DEFAULT_SIZE - 1][2] =
                    data[x][DEFAULT_SIZE - 1][3] = 255;
    }

    tr.defaultImage = R_CreateImage("*default",
                                    reinterpret_cast<uchar8 *>(data), DEFAULT_IMG_SIZE, DEFAULT_IMG_SIZE,
                                    IMGTYPE_COLORALPHA, IMGFLAG_MIPMAP, 0);
}


/*
==================
R_CreateBuiltinImages
==================
*/
void R_CreateBuiltinImages(void) {
    sint        x, y;
    uchar8  data[DEFAULT_SIZE][DEFAULT_SIZE][4];

    R_CreateDefaultImage();

    // we use a solid white image instead of disabling texturing
    ::memset(data, 255, sizeof(data));
    tr.whiteImage = R_CreateImage("*white", reinterpret_cast<uchar8 *>(data),
                                  8, 8, IMGTYPE_COLORALPHA, IMGFLAG_NONE, 0);

    if(r_dlightMode->integer >= 2) {
        for(x = 0; x < MAX_DLIGHTS; x++) {
            tr.shadowCubemaps[x] = R_CreateImage(va("*shadowcubemap%i", x), nullptr,
                                                 PSHADOW_MAP_SIZE, PSHADOW_MAP_SIZE, IMGTYPE_COLORALPHA,
                                                 IMGFLAG_CLAMPTOEDGE | IMGFLAG_CUBEMAP, 0);
        }
    }

    // with overbright bits active, we need an image which is some fraction of full color,
    // for default lightmaps, etc
    for(x = 0 ; x < DEFAULT_SIZE ; x++) {
        for(y = 0 ; y < DEFAULT_SIZE ; y++) {
            data[y][x][0] =
                data[y][x][1] =
                    data[y][x][2] = tr.identityLightByte;
            data[y][x][3] = 255;
        }
    }

    tr.identityLightImage = R_CreateImage("*identityLight",
                                          reinterpret_cast<uchar8 *>(data), 8, 8, IMGTYPE_COLORALPHA, IMGFLAG_NONE,
                                          0);


    for(x = 0; x < 32; x++) {
        // scratchimage is usually used for cinematic drawing
        tr.scratchImage[x] = R_CreateImage("*scratch",
                                           reinterpret_cast<uchar8 *>(data), DEFAULT_SIZE, DEFAULT_SIZE,
                                           IMGTYPE_COLORALPHA, IMGFLAG_PICMIP | IMGFLAG_CLAMPTOEDGE, 0);
    }

    R_CreateDlightImage();
    R_CreateFogImage();
    R_CreateEnvBrdfLUT();

    if(glRefConfig.framebufferObject) {
        sint width, height, hdrFormat, rgbFormat;

        width = glConfig.vidWidth;
        height = glConfig.vidHeight;

        hdrFormat = GL_RGBA8;

        if(r_hdr->integer && glRefConfig.textureFloat) {
            hdrFormat = GL_RGBA16F;
        }

        rgbFormat = GL_RGBA8;

        tr.renderImage = R_CreateImage("_render", nullptr, width, height,
                                       IMGTYPE_COLORALPHA, IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE,
                                       hdrFormat);

        tr.glowImage = R_CreateImage("*glow", nullptr, width, height,
                                     IMGTYPE_COLORALPHA, IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE,
                                     hdrFormat);
        tr.glowImageScaled[0] = R_CreateImage("*glowScaled0", nullptr, width / 2,
                                              height / 2, IMGTYPE_COLORALPHA,
                                              IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE, hdrFormat);
        tr.glowImageScaled[1] = R_CreateImage("*glowScaled1", nullptr, width / 4,
                                              height / 4, IMGTYPE_COLORALPHA,
                                              IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE, hdrFormat);
        tr.glowImageScaled[2] = R_CreateImage("*glowScaled2a", nullptr, width / 8,
                                              height / 8, IMGTYPE_COLORALPHA,
                                              IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE, hdrFormat);
        tr.glowImageScaled[3] = R_CreateImage("*glowScaled2b", nullptr, width / 8,
                                              height / 8, IMGTYPE_COLORALPHA,
                                              IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE, hdrFormat);

        tr.normalDetailedImage = R_CreateImage("*normaldetailed", nullptr, width,
                                               height, IMGTYPE_COLORALPHA, IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE,
                                               hdrFormat);

        if(r_shadowBlur->integer) {
            tr.screenScratchImage = R_CreateImage("screenScratch", nullptr, width,
                                                  height, IMGTYPE_COLORALPHA, IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE,
                                                  rgbFormat);
        }

        tr.hdrDepthImage = R_CreateImage("*hdrDepth", nullptr, width, height,
                                         IMGTYPE_COLORALPHA, IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE, GL_R32F);


        if(r_drawSunRays->integer) {
            tr.sunRaysImage = R_CreateImage("*sunRays", nullptr, width, height,
                                            IMGTYPE_COLORALPHA, IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE,
                                            rgbFormat);
        }

        tr.renderDepthImage  = R_CreateImage("*renderdepth",  nullptr, width,
                                             height, IMGTYPE_COLORALPHA, IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE,
                                             GL_DEPTH_COMPONENT24_ARB);
        tr.textureDepthImage = R_CreateImage("*texturedepth", nullptr,
                                             PSHADOW_MAP_SIZE, PSHADOW_MAP_SIZE, IMGTYPE_COLORALPHA,
                                             IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE, GL_DEPTH_COMPONENT24_ARB);

        tr.genericFBOImage = R_CreateImage("_generic", nullptr, width, height,
                                           IMGTYPE_COLORALPHA, IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE,
                                           hdrFormat);
        tr.genericFBO2Image = R_CreateImage("_generic2", nullptr, width, height,
                                            IMGTYPE_COLORALPHA, IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE,
                                            hdrFormat);

        tr.bloomRenderFBOImage[0] = R_CreateImage("_bloom0", nullptr, width / 2,
                                    height / 2, IMGTYPE_COLORALPHA,
                                    IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE, hdrFormat);
        tr.bloomRenderFBOImage[1] = R_CreateImage("_bloom1", nullptr, width / 2,
                                    height / 2, IMGTYPE_COLORALPHA,
                                    IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE, hdrFormat);
        tr.bloomRenderFBOImage[2] = R_CreateImage("_bloom2", nullptr, width,
                                    height, IMGTYPE_COLORALPHA, IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE,
                                    hdrFormat);


        tr.anamorphicRenderFBOImage[0] = R_CreateImage("_anamorphic0", nullptr,
                                         width / 8, height / 8, IMGTYPE_COLORALPHA,
                                         IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE, hdrFormat);
        tr.anamorphicRenderFBOImage[1] = R_CreateImage("_anamorphic1", nullptr,
                                         width / 8, height / 8, IMGTYPE_COLORALPHA,
                                         IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE, hdrFormat);
        tr.anamorphicRenderFBOImage[2] = R_CreateImage("_anamorphic2", nullptr,
                                         width, height, IMGTYPE_COLORALPHA,
                                         IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE, hdrFormat);

        {
            uchar8 *p;

            data[0][0][0] = 0;
            data[0][0][1] = 0.45f * 255;
            data[0][0][2] = 255;
            data[0][0][3] = 255;
            p = reinterpret_cast<uchar8 *>(data);

            tr.calcLevelsImage =   R_CreateImage("*calcLevels",    p, 1, 1,
                                                 IMGTYPE_COLORALPHA, IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE,
                                                 hdrFormat);
            tr.targetLevelsImage = R_CreateImage("*targetLevels",  p, 1, 1,
                                                 IMGTYPE_COLORALPHA, IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE,
                                                 hdrFormat);
            tr.fixedLevelsImage =  R_CreateImage("*fixedLevels",   p, 1, 1,
                                                 IMGTYPE_COLORALPHA, IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE,
                                                 hdrFormat);
        }

        for(x = 0; x < 2; x++) {
            tr.textureScratchImage[x] = R_CreateImage(va("*textureScratch%d", x),
                                        nullptr, 256, 256, IMGTYPE_COLORALPHA,
                                        IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE, GL_RGBA8);
        }

        for(x = 0; x < 2; x++) {
            tr.quarterImage[x] = R_CreateImage(va("*quarter%d", x), nullptr, width / 2,
                                               height / 2, IMGTYPE_COLORALPHA,
                                               IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE, GL_RGBA8);
        }

        if(r_ssao->integer) {
            tr.screenSsaoImage = R_CreateImage("*screenSsao", nullptr, width / 2,
                                               height / 2, IMGTYPE_COLORALPHA,
                                               IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE, GL_RGBA8);
        }

        for(x = 0; x < MAX_DRAWN_PSHADOWS; x++) {
            tr.pshadowMaps[x] = R_CreateImage(va("*shadowmap%i", x), nullptr,
                                              PSHADOW_MAP_SIZE, PSHADOW_MAP_SIZE, IMGTYPE_COLORALPHA,
                                              IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE, GL_DEPTH_COMPONENT24);
            //qglTextureParameterfEXT(tr.pshadowMaps[x]->texnum, GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
            //qglTextureParameterfEXT(tr.pshadowMaps[x]->texnum, GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
        }

        if(r_sunlightMode->integer) {
            for(x = 0; x < 4; x++) {
                tr.sunShadowDepthImage[x] = R_CreateImage(va("*sunshadowdepth%i", x),
                                            nullptr, r_shadowMapSize->integer, r_shadowMapSize->integer,
                                            IMGTYPE_COLORALPHA, IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE,
                                            GL_DEPTH_COMPONENT24_ARB);
                qglTextureParameterfEXT(tr.sunShadowDepthImage[x]->texnum, GL_TEXTURE_2D,
                                        GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
                qglTextureParameterfEXT(tr.sunShadowDepthImage[x]->texnum, GL_TEXTURE_2D,
                                        GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
            }

            tr.screenShadowImage = R_CreateImage("*screenShadow", nullptr, width,
                                                 height, IMGTYPE_COLORALPHA, IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE,
                                                 GL_RGBA8);
        }

        if(r_cubeMapping->integer) {
            tr.renderCubeImage = R_CreateImage("*renderCube", nullptr,
                                               r_cubemapSize->integer, r_cubemapSize->integer, IMGTYPE_COLORALPHA,
                                               IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE | IMGFLAG_MIPMAP |
                                               IMGFLAG_CUBEMAP, hdrFormat);
        }

        tr.prefilterEnvMapImage = R_CreateImage("*prefilterEnvMap", nullptr,
                                                r_cubemapSize->integer / 2, r_cubemapSize->integer / 2, IMGTYPE_COLORALPHA,
                                                IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE, hdrFormat);

    }
}


/*
===============
R_SetColorMappings
===============
*/
void R_SetColorMappings(void) {
    sint        i, j;

    // setup the overbright lighting
    tr.overbrightBits = r_overBrightBits->integer;

    // allow 2 overbright bits
    if(tr.overbrightBits > 2) {
        tr.overbrightBits = 2;
    } else if(tr.overbrightBits < 0) {
        tr.overbrightBits = 0;
    }

    // don't allow more overbright bits than map overbright bits
    if(tr.overbrightBits > r_mapOverBrightBits->integer) {
        tr.overbrightBits = r_mapOverBrightBits->integer;
    }

    tr.identityLight = 1.0f / (1 << tr.overbrightBits);
    tr.identityLightByte = 255 * tr.identityLight;


    if(r_intensity->value <= 1) {
        cvarSystem->Set("r_intensity", "1");
    }

    if(r_gamma->value < 0.5f) {
        cvarSystem->Set("r_gamma", "0.5");
    } else if(r_gamma->value > 3.0f) {
        cvarSystem->Set("r_gamma", "3.0");
    }

    for(i = 0 ; i < 256 ; i++) {
        j = i * r_intensity->value;

        if(j > 255) {
            j = 255;
        }

        s_intensitytable[i] = j;
    }
}

/*
===============
R_InitImages
===============
*/
void    R_InitImages(void) {
    ::memset(hashTable, 0, sizeof(hashTable));
    // build brightness translation tables
    R_SetColorMappings();

    // create default texture and white texture
    R_CreateBuiltinImages();
}

/*
===============
R_DeleteTextures
===============
*/
void R_DeleteTextures(void) {
    sint        i;

    for(i = 0; i < tr.numImages ; i++) {
        qglDeleteTextures(1, &tr.images[i]->texnum);
    }

    ::memset(tr.images, 0, sizeof(tr.images));

    tr.numImages = 0;

    GL_BindNullTextures();
}

/*
============================================================================

SKINS

============================================================================
*/

/*
==================
CommaParse

This is unfortunate, but the skin files aren't
compatable with our normal parsing rules.
==================
*/
static pointer CommaParse(valueType **data_p) {
    sint c = 0, len;
    valueType *data;
    static  valueType com_token[MAX_TOKEN_CHARS];

    data = *data_p;
    len = 0;
    com_token[0] = 0;

    // make sure incoming data is valid
    if(!data) {
        *data_p = nullptr;
        return com_token;
    }

    while(1) {
        // skip whitespace
        while((c = *data) <= ' ') {
            if(!c) {
                break;
            }

            data++;
        }


        c = *data;

        // skip double slash comments
        if(c == '/' && data[1] == '/') {
            data += 2;

            while(*data && *data != '\n') {
                data++;
            }
        }
        // skip /* */ comments
        else if(c == '/' && data[1] == '*') {
            data += 2;

            while(*data && (*data != '*' || data[1] != '/')) {
                data++;
            }

            if(*data) {
                data += 2;
            }
        } else {
            break;
        }
    }

    if(c == 0) {
        return "";
    }

    // handle quoted strings
    if(c == '\"') {
        data++;

        while(1) {
            c = *data++;

            if(c == '\"' || !c) {
                com_token[len] = 0;
                *data_p = const_cast< valueType * >(data);
                return com_token;
            }

            if(len < MAX_TOKEN_CHARS - 1) {
                com_token[len] = c;
                len++;
            }
        }
    }

    // parse a regular word
    do {
        if(len < MAX_TOKEN_CHARS - 1) {
            com_token[len] = c;
            len++;
        }

        data++;
        c = *data;
    } while(c > 32 && c != ',');

    com_token[len] = 0;

    *data_p = const_cast< valueType * >(data);
    return com_token;
}


/*
===============
idRenderSystemLocal::RegisterSkin
===============
*/
qhandle_t idRenderSystemLocal::RegisterSkin(pointer name) {
    skinSurface_t parseSurfaces[MAX_SKIN_SURFACES];
    qhandle_t   hSkin;
    skin_t     *skin;
    skinSurface_t  *surf;
    union {
        valueType *c;
        void *v;
    } text;
    valueType      *text_p;
    pointer token;
    valueType       surfName[MAX_QPATH];

    if(!name || !name[0]) {
        clientRendererSystem->RefPrintf(PRINT_DEVELOPER,
                                        "Empty name passed to idRenderSystemLocal::RegisterSkin\n");
        return 0;
    }

    if(strlen(name) >= MAX_QPATH) {
        clientRendererSystem->RefPrintf(PRINT_DEVELOPER,
                                        "Skin name exceeds MAX_QPATH\n");
        return 0;
    }


    // see if the skin is already loaded
    for(hSkin = 1; hSkin < tr.numSkins ; hSkin++) {
        skin = tr.skins[hSkin];

        if(!Q_stricmp(skin->name, name)) {
            if(skin->numSurfaces == 0) {
                return 0;       // default skin
            }

            return hSkin;
        }
    }

    // allocate a new skin
    if(tr.numSkins == MAX_SKINS) {
        clientRendererSystem->RefPrintf(PRINT_WARNING,
                                        "WARNING: idRenderSystemLocal::RegisterSkin( '%s' ) MAX_SKINS hit\n",
                                        name);
        return 0;
    }

    tr.numSkins++;
    skin = reinterpret_cast<skin_t *>(memorySystem->Alloc(sizeof(skin_t),
                                      h_low));
    tr.skins[hSkin] = skin;
    Q_strncpyz(skin->name, name, sizeof(skin->name));
    skin->numSurfaces = 0;

    R_IssuePendingRenderCommands();

    // If not a .skin file, load as a single shader
    if(strcmp(name + strlen(name) - 5, ".skin")) {
        skin->numSurfaces = 1;
        skin->surfaces = reinterpret_cast<skinSurface_t *>(memorySystem->Alloc(
                             sizeof(
                                 skinSurface_t), h_low));
        skin->surfaces[0].shader = R_FindShader(name, LIGHTMAP_NONE, true);
        return hSkin;
    }

    // load and parse the skin file
    fileSystem->ReadFile(name, &text.v);

    if(!text.c) {
        return 0;
    }

    sint totalSurfaces = 0;
    text_p = text.c;

    while(text_p && *text_p) {
        // get surface name
        token = CommaParse(&text_p);
        Q_strncpyz(surfName, token, sizeof(surfName));

        if(!token[0]) {
            break;
        }

        // lowercase the surface name so skin compares are faster
        Q_strlwr(surfName);

        if(*text_p == ',') {
            text_p++;
        }

        if(strstr(token, "tag_")) {
            continue;
        }

        // parse the shader name
        token = CommaParse(&text_p);


        if(skin->numSurfaces >= MD3_MAX_SURFACES) {
            clientRendererSystem->RefPrintf(PRINT_WARNING,
                                            "WARNING: Ignoring surfaces in '%s', the max is %d surfaces!\n", name,
                                            MD3_MAX_SURFACES);
            break;
        }

        if(skin->numSurfaces < MAX_SKIN_SURFACES) {
            surf = &parseSurfaces[skin->numSurfaces];
            Q_strncpyz(surf->name, surfName, sizeof(surf->name));
            surf->shader = R_FindShader(token, LIGHTMAP_NONE, true);
            skin->numSurfaces++;
        }

        totalSurfaces++;
    }

    fileSystem->FreeFile(text.v);

    if(totalSurfaces > MAX_SKIN_SURFACES) {
        clientRendererSystem->RefPrintf(PRINT_WARNING,
                                        "WARNING: Ignoring excess surfaces (found %d, max is %d) in skin '%s'!\n",
                                        totalSurfaces, MAX_SKIN_SURFACES, name);
    }

    // never let a skin have 0 shaders
    if(skin->numSurfaces == 0) {
        return 0;       // use default skin
    }

    // copy surfaces to skin
    skin->surfaces = reinterpret_cast<skinSurface_t *>(memorySystem->Alloc(
                         skin->numSurfaces * sizeof(skinSurface_t), h_low));
    memcpy(skin->surfaces, parseSurfaces,
           skin->numSurfaces * sizeof(skinSurface_t));

    return hSkin;
}


/*
===============
R_InitSkins
===============
*/
void    R_InitSkins(void) {
    skin_t *skin = nullptr;

    tr.numSkins = 1;

    // make the default skin have all default shaders
    skin = tr.skins[0] = (skin_t *)memorySystem->Alloc(sizeof(skin_t), h_low);
    Q_strncpyz(skin->name, "<default skin>", sizeof(skin->name));
    skin->numSurfaces = 1;
    skin->surfaces = (skinSurface_t *)memorySystem->Alloc(sizeof(
                         skinSurface_t), h_low);
    skin->surfaces[0].shader = tr.defaultShader;
}

/*
===============
R_GetSkinByHandle
===============
*/
skin_t *R_GetSkinByHandle(qhandle_t hSkin) {
    if(hSkin < 1 || hSkin >= tr.numSkins) {
        return tr.skins[0];
    }

    return tr.skins[ hSkin ];
}

/*
===============
R_SkinList_f
===============
*/
void    R_SkinList_f(void) {
    sint            i, j;
    skin_t     *skin;

    clientRendererSystem->RefPrintf(PRINT_ALL, "------------------\n");

    for(i = 0 ; i < tr.numSkins ; i++) {
        skin = tr.skins[i];

        clientRendererSystem->RefPrintf(PRINT_ALL, "%3i:%s (%d surfaces)\n", i,
                                        skin->name,
                                        skin->numSurfaces);

        for(j = 0 ; j < skin->numSurfaces ; j++) {
            clientRendererSystem->RefPrintf(PRINT_ALL, "       %s = %s\n",
                                            skin->surfaces[j].name,
                                            skin->surfaces[j].shader->name);
        }
    }

    clientRendererSystem->RefPrintf(PRINT_ALL, "------------------\n");
}
