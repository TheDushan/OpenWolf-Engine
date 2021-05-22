////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2015 James Canete
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
// File name:   r_image_dds.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <framework/precompiled.hpp>

typedef struct ddsHeader_s {
    uint headerSize;
    uint flags;
    uint height;
    uint width;
    uint pitchOrFirstMipSize;
    uint volumeDepth;
    uint numMips;
    uint reserved1[11];
    uint always_0x00000020;
    uint pixelFormatFlags;
    uint fourCC;
    uint rgbBitCount;
    uint rBitMask;
    uint gBitMask;
    uint bBitMask;
    uint aBitMask;
    uint caps;
    uint caps2;
    uint caps3;
    uint caps4;
    uint reserved2;
}
ddsHeader_t;

// flags:
#define _DDSFLAGS_REQUIRED     0x001007
#define _DDSFLAGS_PITCH        0x8
#define _DDSFLAGS_MIPMAPCOUNT  0x20000
#define _DDSFLAGS_FIRSTMIPSIZE 0x80000
#define _DDSFLAGS_VOLUMEDEPTH  0x800000

// pixelFormatFlags:
#define DDSPF_ALPHAPIXELS 0x1
#define DDSPF_ALPHA       0x2
#define DDSPF_FOURCC      0x4
#define DDSPF_RGB         0x40
#define DDSPF_YUV         0x200
#define DDSPF_LUMINANCE   0x20000

// caps:
#define DDSCAPS_COMPLEX  0x8
#define DDSCAPS_MIPMAP   0x400000
#define DDSCAPS_REQUIRED 0x1000

// caps2:
#define DDSCAPS2_CUBEMAP 0xFE00
#define DDSCAPS2_VOLUME  0x200000

typedef struct ddsHeaderDxt10_s {
    uint dxgiFormat;
    uint dimensions;
    uint miscFlags;
    uint arraySize;
    uint miscFlags2;
}
ddsHeaderDxt10_t;

// dxgiFormat
// from http://msdn.microsoft.com/en-us/library/windows/desktop/bb173059%28v=vs.85%29.aspx
enum DXGI_FORMAT {
    DXGI_FORMAT_UNKNOWN = 0,
    DXGI_FORMAT_R32G32B32A32_TYPELESS = 1,
    DXGI_FORMAT_R32G32B32A32_FLOAT = 2,
    DXGI_FORMAT_R32G32B32A32_UINT = 3,
    DXGI_FORMAT_R32G32B32A32_SINT = 4,
    DXGI_FORMAT_R32G32B32_TYPELESS = 5,
    DXGI_FORMAT_R32G32B32_FLOAT = 6,
    DXGI_FORMAT_R32G32B32_UINT = 7,
    DXGI_FORMAT_R32G32B32_SINT = 8,
    DXGI_FORMAT_R16G16B16A16_TYPELESS = 9,
    DXGI_FORMAT_R16G16B16A16_FLOAT = 10,
    DXGI_FORMAT_R16G16B16A16_UNORM = 11,
    DXGI_FORMAT_R16G16B16A16_UINT = 12,
    DXGI_FORMAT_R16G16B16A16_SNORM = 13,
    DXGI_FORMAT_R16G16B16A16_SINT = 14,
    DXGI_FORMAT_R32G32_TYPELESS = 15,
    DXGI_FORMAT_R32G32_FLOAT = 16,
    DXGI_FORMAT_R32G32_UINT = 17,
    DXGI_FORMAT_R32G32_SINT = 18,
    DXGI_FORMAT_R32G8X24_TYPELESS = 19,
    DXGI_FORMAT_D32_FLOAT_S8X24_UINT = 20,
    DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS = 21,
    DXGI_FORMAT_X32_TYPELESS_G8X24_UINT = 22,
    DXGI_FORMAT_R10G10B10A2_TYPELESS = 23,
    DXGI_FORMAT_R10G10B10A2_UNORM = 24,
    DXGI_FORMAT_R10G10B10A2_UINT = 25,
    DXGI_FORMAT_R11G11B10_FLOAT = 26,
    DXGI_FORMAT_R8G8B8A8_TYPELESS = 27,
    DXGI_FORMAT_R8G8B8A8_UNORM = 28,
    DXGI_FORMAT_R8G8B8A8_UNORM_SRGB = 29,
    DXGI_FORMAT_R8G8B8A8_UINT = 30,
    DXGI_FORMAT_R8G8B8A8_SNORM = 31,
    DXGI_FORMAT_R8G8B8A8_SINT = 32,
    DXGI_FORMAT_R16G16_TYPELESS = 33,
    DXGI_FORMAT_R16G16_FLOAT = 34,
    DXGI_FORMAT_R16G16_UNORM = 35,
    DXGI_FORMAT_R16G16_UINT = 36,
    DXGI_FORMAT_R16G16_SNORM = 37,
    DXGI_FORMAT_R16G16_SINT = 38,
    DXGI_FORMAT_R32_TYPELESS = 39,
    DXGI_FORMAT_D32_FLOAT = 40,
    DXGI_FORMAT_R32_FLOAT = 41,
    DXGI_FORMAT_R32_UINT = 42,
    DXGI_FORMAT_R32_SINT = 43,
    DXGI_FORMAT_R24G8_TYPELESS = 44,
    DXGI_FORMAT_D24_UNORM_S8_UINT = 45,
    DXGI_FORMAT_R24_UNORM_X8_TYPELESS = 46,
    DXGI_FORMAT_X24_TYPELESS_G8_UINT = 47,
    DXGI_FORMAT_R8G8_TYPELESS = 48,
    DXGI_FORMAT_R8G8_UNORM = 49,
    DXGI_FORMAT_R8G8_UINT = 50,
    DXGI_FORMAT_R8G8_SNORM = 51,
    DXGI_FORMAT_R8G8_SINT = 52,
    DXGI_FORMAT_R16_TYPELESS = 53,
    DXGI_FORMAT_R16_FLOAT = 54,
    DXGI_FORMAT_D16_UNORM = 55,
    DXGI_FORMAT_R16_UNORM = 56,
    DXGI_FORMAT_R16_UINT = 57,
    DXGI_FORMAT_R16_SNORM = 58,
    DXGI_FORMAT_R16_SINT = 59,
    DXGI_FORMAT_R8_TYPELESS = 60,
    DXGI_FORMAT_R8_UNORM = 61,
    DXGI_FORMAT_R8_UINT = 62,
    DXGI_FORMAT_R8_SNORM = 63,
    DXGI_FORMAT_R8_SINT = 64,
    DXGI_FORMAT_A8_UNORM = 65,
    DXGI_FORMAT_R1_UNORM = 66,
    DXGI_FORMAT_R9G9B9E5_SHAREDEXP = 67,
    DXGI_FORMAT_R8G8_B8G8_UNORM = 68,
    DXGI_FORMAT_G8R8_G8B8_UNORM = 69,
    DXGI_FORMAT_BC1_TYPELESS = 70,
    DXGI_FORMAT_BC1_UNORM = 71,
    DXGI_FORMAT_BC1_UNORM_SRGB = 72,
    DXGI_FORMAT_BC2_TYPELESS = 73,
    DXGI_FORMAT_BC2_UNORM = 74,
    DXGI_FORMAT_BC2_UNORM_SRGB = 75,
    DXGI_FORMAT_BC3_TYPELESS = 76,
    DXGI_FORMAT_BC3_UNORM = 77,
    DXGI_FORMAT_BC3_UNORM_SRGB = 78,
    DXGI_FORMAT_BC4_TYPELESS = 79,
    DXGI_FORMAT_BC4_UNORM = 80,
    DXGI_FORMAT_BC4_SNORM = 81,
    DXGI_FORMAT_BC5_TYPELESS = 82,
    DXGI_FORMAT_BC5_UNORM = 83,
    DXGI_FORMAT_BC5_SNORM = 84,
    DXGI_FORMAT_B5G6R5_UNORM = 85,
    DXGI_FORMAT_B5G5R5A1_UNORM = 86,
    DXGI_FORMAT_B8G8R8A8_UNORM = 87,
    DXGI_FORMAT_B8G8R8X8_UNORM = 88,
    DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM = 89,
    DXGI_FORMAT_B8G8R8A8_TYPELESS = 90,
    DXGI_FORMAT_B8G8R8A8_UNORM_SRGB = 91,
    DXGI_FORMAT_B8G8R8X8_TYPELESS = 92,
    DXGI_FORMAT_B8G8R8X8_UNORM_SRGB = 93,
    DXGI_FORMAT_BC6H_TYPELESS = 94,
    DXGI_FORMAT_BC6H_UF16 = 95,
    DXGI_FORMAT_BC6H_SF16 = 96,
    DXGI_FORMAT_BC7_TYPELESS = 97,
    DXGI_FORMAT_BC7_UNORM = 98,
    DXGI_FORMAT_BC7_UNORM_SRGB = 99,
    DXGI_FORMAT_AYUV = 100,
    DXGI_FORMAT_Y410 = 101,
    DXGI_FORMAT_Y416 = 102,
    DXGI_FORMAT_NV12 = 103,
    DXGI_FORMAT_P010 = 104,
    DXGI_FORMAT_P016 = 105,
    DXGI_FORMAT_420_OPAQUE = 106,
    DXGI_FORMAT_YUY2 = 107,
    DXGI_FORMAT_Y210 = 108,
    DXGI_FORMAT_Y216 = 109,
    DXGI_FORMAT_NV11 = 110,
    DXGI_FORMAT_AI44 = 111,
    DXGI_FORMAT_IA44 = 112,
    DXGI_FORMAT_P8 = 113,
    DXGI_FORMAT_A8P8 = 114,
    DXGI_FORMAT_B4G4R4A4_UNORM = 115,
    DXGI_FORMAT_FORCE_UINT = 0xffffffffUL
};

#define EncodeFourCC(x) ((((uint)((x)[0]))      ) | \
                         (((uint)((x)[1])) << 8 ) | \
                         (((uint)((x)[2])) << 16) | \
                         (((uint)((x)[3])) << 24) )


void R_LoadDDS(pointer filename, uchar8 **pic, sint *width, sint *height,
               uint *picFormat, sint *numMips) {
    union {
        uchar8 *b;
        void *v;
    } buffer;
    sint len;
    ddsHeader_t *ddsHeader = nullptr;
    ddsHeaderDxt10_t *ddsHeaderDxt10 = nullptr;
    uchar8 *data;

    if(!picFormat) {
        clientRendererSystem->RefPrintf(PRINT_ERROR,
                                        "R_LoadDDS() called without picFormat parameter!");
        return;
    }

    if(width) {
        *width = 0;
    }

    if(height) {
        *height = 0;
    }

    if(picFormat) {
        *picFormat = GL_RGBA8;
    }

    if(numMips) {
        *numMips = 1;
    }

    *pic = nullptr;

    //
    // load the file
    //
    len = fileSystem->ReadFile(const_cast< valueType * >(filename), &buffer.v);

    if(!buffer.b || len < 0) {
        return;
    }

    //
    // reject files that are too small to hold even a header
    //
    if(len < 4 + sizeof(*ddsHeader)) {
        clientRendererSystem->RefPrintf(PRINT_ALL,
                                        "File %s is too small to be a DDS file.\n",
                                        filename);
        fileSystem->FreeFile(buffer.v);
        return;
    }

    //
    // reject files that don't start with "DDS "
    //
    if(*(reinterpret_cast<uint *>((buffer.b))) != EncodeFourCC("DDS ")) {
        clientRendererSystem->RefPrintf(PRINT_ALL, "File %s is not a DDS file.\n",
                                        filename);
        fileSystem->FreeFile(buffer.v);
        return;
    }

    //
    // parse header and dx10 header if available
    //
    ddsHeader = (ddsHeader_t *)(buffer.b + 4);

    if((ddsHeader->pixelFormatFlags & DDSPF_FOURCC) &&
            ddsHeader->fourCC == EncodeFourCC("DX10")) {
        if(len < 4 + sizeof(*ddsHeader) + sizeof(*ddsHeaderDxt10)) {
            clientRendererSystem->RefPrintf(PRINT_ALL,
                                            "File %s indicates a DX10 header it is too small to contain.\n", filename);
            fileSystem->FreeFile(buffer.v);
            return;
        }

        ddsHeaderDxt10 = (ddsHeaderDxt10_t *)(buffer.b + 4 + sizeof(ddsHeader_t));
        data = buffer.b + 4 + sizeof(*ddsHeader) + sizeof(*ddsHeaderDxt10);
        len -= 4 + sizeof(*ddsHeader) + sizeof(*ddsHeaderDxt10);
    } else {
        data = buffer.b + 4 + sizeof(*ddsHeader);
        len -= 4 + sizeof(*ddsHeader);
    }

    if(width) {
        *width = ddsHeader->width;
    }

    if(height) {
        *height = ddsHeader->height;
    }

    if(numMips) {
        if(ddsHeader->flags & _DDSFLAGS_MIPMAPCOUNT) {
            *numMips = ddsHeader->numMips;
        } else {
            *numMips = 1;
        }
    }

    // FIXME: handle cube map
    //if ((ddsHeader->caps2 & DDSCAPS2_CUBEMAP) == DDSCAPS2_CUBEMAP)

    //
    // Convert DXGI format/FourCC into OpenGL format
    //
    if(ddsHeaderDxt10) {
        switch(ddsHeaderDxt10->dxgiFormat) {
            case DXGI_FORMAT_BC1_TYPELESS:
            case DXGI_FORMAT_BC1_UNORM:
                // FIXME: check for GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
                *picFormat = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
                break;

            case DXGI_FORMAT_BC1_UNORM_SRGB:
                // FIXME: check for GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT
                *picFormat = GL_COMPRESSED_SRGB_S3TC_DXT1_EXT;
                break;

            case DXGI_FORMAT_BC2_TYPELESS:
            case DXGI_FORMAT_BC2_UNORM:
                *picFormat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
                break;

            case DXGI_FORMAT_BC2_UNORM_SRGB:
                *picFormat = GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT;
                break;

            case DXGI_FORMAT_BC3_TYPELESS:
            case DXGI_FORMAT_BC3_UNORM:
                *picFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
                break;

            case DXGI_FORMAT_BC3_UNORM_SRGB:
                *picFormat = GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT;
                break;

            case DXGI_FORMAT_BC4_TYPELESS:
            case DXGI_FORMAT_BC4_UNORM:
                *picFormat = GL_COMPRESSED_RED_RGTC1;
                break;

            case DXGI_FORMAT_BC4_SNORM:
                *picFormat = GL_COMPRESSED_SIGNED_RED_RGTC1;
                break;

            case DXGI_FORMAT_BC5_TYPELESS:
            case DXGI_FORMAT_BC5_UNORM:
                *picFormat = GL_COMPRESSED_RG_RGTC2;
                break;

            case DXGI_FORMAT_BC5_SNORM:
                *picFormat = GL_COMPRESSED_SIGNED_RG_RGTC2;
                break;

            case DXGI_FORMAT_BC6H_TYPELESS:
            case DXGI_FORMAT_BC6H_UF16:
                *picFormat = GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_ARB;
                break;

            case DXGI_FORMAT_BC6H_SF16:
                *picFormat = GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT_ARB;
                break;

            case DXGI_FORMAT_BC7_TYPELESS:
            case DXGI_FORMAT_BC7_UNORM:
                *picFormat = GL_COMPRESSED_RGBA_BPTC_UNORM_ARB;
                break;

            case DXGI_FORMAT_BC7_UNORM_SRGB:
                *picFormat = GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB;
                break;

            case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
                *picFormat = GL_SRGB8_ALPHA8_EXT;
                break;

            case DXGI_FORMAT_R8G8B8A8_UNORM:
            case DXGI_FORMAT_R8G8B8A8_SNORM:
                *picFormat = GL_RGBA8;
                break;

            default:
                clientRendererSystem->RefPrintf(PRINT_ALL,
                                                "DDS File %s has unsupported DXGI format %d.",
                                                filename, ddsHeaderDxt10->dxgiFormat);
                fileSystem->FreeFile(buffer.v);
                return;
                break;
        }
    } else {
        if(ddsHeader->pixelFormatFlags & DDSPF_FOURCC) {
            if(ddsHeader->fourCC == EncodeFourCC("DXT1")) {
                *picFormat = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
            } else if(ddsHeader->fourCC == EncodeFourCC("DXT2")) {
                *picFormat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
            } else if(ddsHeader->fourCC == EncodeFourCC("DXT3")) {
                *picFormat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
            } else if(ddsHeader->fourCC == EncodeFourCC("DXT4")) {
                *picFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
            } else if(ddsHeader->fourCC == EncodeFourCC("DXT5")) {
                *picFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
            } else if(ddsHeader->fourCC == EncodeFourCC("ATI1")) {
                *picFormat = GL_COMPRESSED_RED_RGTC1;
            } else if(ddsHeader->fourCC == EncodeFourCC("BC4U")) {
                *picFormat = GL_COMPRESSED_RED_RGTC1;
            } else if(ddsHeader->fourCC == EncodeFourCC("BC4S")) {
                *picFormat = GL_COMPRESSED_SIGNED_RED_RGTC1;
            } else if(ddsHeader->fourCC == EncodeFourCC("ATI2")) {
                *picFormat = GL_COMPRESSED_RG_RGTC2;
            } else if(ddsHeader->fourCC == EncodeFourCC("BC5U")) {
                *picFormat = GL_COMPRESSED_RG_RGTC2;
            } else if(ddsHeader->fourCC == EncodeFourCC("BC5S")) {
                *picFormat = GL_COMPRESSED_SIGNED_RG_RGTC2;
            } else {
                clientRendererSystem->RefPrintf(PRINT_ALL,
                                                "DDS File %s has unsupported FourCC.", filename);
                fileSystem->FreeFile(buffer.v);
                return;
            }
        } else if(ddsHeader->pixelFormatFlags == (DDSPF_RGB | DDSPF_ALPHAPIXELS)
                  && ddsHeader->rgbBitCount == 32
                  && ddsHeader->rBitMask == 0x000000ff
                  && ddsHeader->gBitMask == 0x0000ff00
                  && ddsHeader->bBitMask == 0x00ff0000
                  && ddsHeader->aBitMask == 0xff000000) {
            *picFormat = GL_RGBA8;
        } else {
            clientRendererSystem->RefPrintf(PRINT_ALL,
                                            "DDS File %s has unsupported RGBA format.",
                                            filename);
            fileSystem->FreeFile(buffer.v);
            return;
        }
    }

    *pic = static_cast<uchar8 *>(clientRendererSystem->RefMalloc(len));
    ::memcpy(*pic, data, len);

    fileSystem->FreeFile(buffer.v);
}

void R_SaveDDS(pointer filename, uchar8 *pic, sint width, sint height,
               sint depth) {
    uchar8 *data;
    ddsHeader_t *ddsHeader;
    sint picSize, size;

    if(!depth) {
        depth = 1;
    }

    picSize = width * height * depth * 4;
    size = 4 + sizeof(*ddsHeader) + picSize;
    data = static_cast<uchar8 *>(clientRendererSystem->RefMalloc(size));

    data[0] = 'D';
    data[1] = 'D';
    data[2] = 'S';
    data[3] = ' ';

    ddsHeader = (ddsHeader_t *)(data + 4);
    memset(ddsHeader, 0, sizeof(ddsHeader_t));

    ddsHeader->headerSize = 0x7c;
    ddsHeader->flags = _DDSFLAGS_REQUIRED;
    ddsHeader->height = height;
    ddsHeader->width = width;
    ddsHeader->always_0x00000020 = 0x00000020;
    ddsHeader->caps = DDSCAPS_COMPLEX | DDSCAPS_REQUIRED;

    if(depth == 6) {
        ddsHeader->caps2 = DDSCAPS2_CUBEMAP;
    }

    ddsHeader->pixelFormatFlags = DDSPF_RGB | DDSPF_ALPHAPIXELS;
    ddsHeader->rgbBitCount = 32;
    ddsHeader->rBitMask = 0x000000ff;
    ddsHeader->gBitMask = 0x0000ff00;
    ddsHeader->bBitMask = 0x00ff0000;
    ddsHeader->aBitMask = 0xff000000;

    ::memcpy(data + 4 + sizeof(*ddsHeader), pic, picSize);

    fileSystem->WriteFile(filename, data, size);

    Z_Free(data);
}
