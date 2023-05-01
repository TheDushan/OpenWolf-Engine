////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2005 Id Software, Inc.
// Copyright(C) 2000 - 2013 Darklegion Development
// Copyright(C) 2011 - 2023 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   r_image_tga.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0,
//              AppleClang 9.0.0.9000039
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <renderSystem/r_precompiled.hpp>

/*
========================================================================

TGA files are used for 24/32 bit images

========================================================================
*/

void R_LoadTGA(pointer name, uchar8 **pic, sint *width, sint *height) {
    uint    columns, rows, numPixels;
    uchar8 *pixbuf;
    sint        row, column;
    uchar8 *buf_p;
    uchar8 *end;
    union {
        uchar8 *b;
        void *v;
    } buffer;
    TargaHeader targa_header;
    uchar8     *targa_rgba;
    sint length;

    *pic = nullptr;

    if(width) {
        *width = 0;
    }

    if(height) {
        *height = 0;
    }

    //
    // load the file
    //
    length = fileSystem->ReadFile(const_cast< valueType * >(name), &buffer.v);

    if(!buffer.b || length < 0) {
        return;
    }

    if(length < 18) {
        fileSystem->FreeFile(buffer.v);
        common->Error(ERR_DROP, "LoadTGA: header too short (%s)", name);
    }

    buf_p = buffer.b;
    end = buffer.b + length;

    targa_header.id_length = buf_p[0];
    targa_header.colormap_type = buf_p[1];
    targa_header.image_type = buf_p[2];

    memcpy(&targa_header.colormap_index, &buf_p[3], 2);
    memcpy(&targa_header.colormap_length, &buf_p[5], 2);
    targa_header.colormap_size = buf_p[7];
    memcpy(&targa_header.x_origin, &buf_p[8], 2);
    memcpy(&targa_header.y_origin, &buf_p[10], 2);
    memcpy(&targa_header.width, &buf_p[12], 2);
    memcpy(&targa_header.height, &buf_p[14], 2);
    targa_header.pixel_size = buf_p[16];
    targa_header.attributes = buf_p[17];

    targa_header.colormap_index = LittleShort(targa_header.colormap_index);
    targa_header.colormap_length = LittleShort(targa_header.colormap_length);
    targa_header.x_origin = LittleShort(targa_header.x_origin);
    targa_header.y_origin = LittleShort(targa_header.y_origin);
    targa_header.width = LittleShort(targa_header.width);
    targa_header.height = LittleShort(targa_header.height);

    buf_p += 18;

    if(targa_header.image_type != 2
            && targa_header.image_type != 10
            && targa_header.image_type != 3) {
        common->Error(ERR_DROP,
                      "LoadTGA: Only type 2 (RGB), 3 (gray), and 10 (RGB) TGA images supported");
    }

    if(targa_header.colormap_type != 0) {
        common->Error(ERR_DROP, "LoadTGA: colormaps not supported");
    }

    if((targa_header.pixel_size != 32 && targa_header.pixel_size != 24) &&
            targa_header.image_type != 3) {
        common->Error(ERR_DROP,
                      "LoadTGA: Only 32 or 24 bit images supported (no colormaps)");
    }

    columns = targa_header.width;
    rows = targa_header.height;
    numPixels = columns * rows * 4;

    if(!columns || !rows || numPixels > 0x7FFFFFFF ||
            numPixels / columns / 4 != rows) {
        common->Error(ERR_DROP, "LoadTGA: %s has an invalid image size", name);
    }


    targa_rgba = static_cast<uchar8 *>(clientRendererSystem->RefMalloc(
                                           numPixels));

    if(targa_header.id_length != 0) {
        if(buf_p + targa_header.id_length > end) {
            memorySystem->Free(targa_rgba);
            fileSystem->FreeFile(buffer.v);
            common->Error(ERR_DROP, "LoadTGA: header too short (%s)", name);
        }

        buf_p += targa_header.id_length;  // skip TARGA image comment
    }

    if(targa_header.image_type == 2 || targa_header.image_type == 3) {
        if(buf_p + columns * rows * targa_header.pixel_size / 8 > end) {
            memorySystem->Free(targa_rgba);
            fileSystem->FreeFile(buffer.v);
            common->Error(ERR_DROP, "LoadTGA: file truncated (%s)", name);
        }

        // Uncompressed RGB or gray scale image
        for(row = rows - 1; row >= 0; row--) {
            pixbuf = targa_rgba + row * columns * 4;

            for(column = 0; column < columns; column++) {
                uchar8 red, green, blue, alphabyte;

                switch(targa_header.pixel_size) {

                    case 8:
                        blue = *buf_p++;
                        green = blue;
                        red = blue;
                        *pixbuf++ = red;
                        *pixbuf++ = green;
                        *pixbuf++ = blue;
                        *pixbuf++ = 255;
                        break;

                    case 24:
                        blue = *buf_p++;
                        green = *buf_p++;
                        red = *buf_p++;
                        *pixbuf++ = red;
                        *pixbuf++ = green;
                        *pixbuf++ = blue;
                        *pixbuf++ = 255;
                        break;

                    case 32:
                        blue = *buf_p++;
                        green = *buf_p++;
                        red = *buf_p++;
                        alphabyte = *buf_p++;
                        *pixbuf++ = red;
                        *pixbuf++ = green;
                        *pixbuf++ = blue;
                        *pixbuf++ = alphabyte;
                        break;

                    default:
                        common->Error(ERR_DROP, "LoadTGA: illegal pixel_size '%d' in file '%s'",
                                      targa_header.pixel_size, name);
                        break;
                }
            }
        }
    } else if(targa_header.image_type == 10) { // Runlength encoded RGB images
        uchar8 red, green, blue, alphabyte, packetHeader, packetSize, j;

        for(row = rows - 1; row >= 0; row--) {
            pixbuf = targa_rgba + row * columns * 4;

            for(column = 0; column < columns;) {
                if(buf_p + 1 > end) {
                    common->Error(ERR_DROP, "LoadTGA: file truncated (%s)", name);
                }

                packetHeader = *buf_p++;
                packetSize = 1 + (packetHeader & 0x7f);

                if(packetHeader & 0x80) {         // run-length packet
                    if(buf_p + targa_header.pixel_size / 8 > end) {
                        common->Error(ERR_DROP, "LoadTGA: file truncated (%s)", name);
                    }

                    switch(targa_header.pixel_size) {
                        case 24:
                            blue = *buf_p++;
                            green = *buf_p++;
                            red = *buf_p++;
                            alphabyte = 255;
                            break;

                        case 32:
                            blue = *buf_p++;
                            green = *buf_p++;
                            red = *buf_p++;
                            alphabyte = *buf_p++;
                            break;

                        default:
                            common->Error(ERR_DROP, "LoadTGA: illegal pixel_size '%d' in file '%s'",
                                          targa_header.pixel_size, name);
                            break;
                    }

                    for(j = 0; j < packetSize; j++) {
                        *pixbuf++ = red;
                        *pixbuf++ = green;
                        *pixbuf++ = blue;
                        *pixbuf++ = alphabyte;
                        column++;

                        if(column == columns) { // run spans across rows
                            column = 0;

                            if(row > 0) {
                                row--;
                            } else {
                                goto breakOut;
                            }

                            pixbuf = targa_rgba + row * columns * 4;
                        }
                    }
                } else {                          // non run-length packet

                    if(buf_p + targa_header.pixel_size / 8 * packetSize > end) {
                        common->Error(ERR_DROP, "LoadTGA: file truncated (%s)", name);
                    }

                    for(j = 0; j < packetSize; j++) {
                        switch(targa_header.pixel_size) {
                            case 24:
                                blue = *buf_p++;
                                green = *buf_p++;
                                red = *buf_p++;
                                *pixbuf++ = red;
                                *pixbuf++ = green;
                                *pixbuf++ = blue;
                                *pixbuf++ = 255;
                                break;

                            case 32:
                                blue = *buf_p++;
                                green = *buf_p++;
                                red = *buf_p++;
                                alphabyte = *buf_p++;
                                *pixbuf++ = red;
                                *pixbuf++ = green;
                                *pixbuf++ = blue;
                                *pixbuf++ = alphabyte;
                                break;

                            default:
                                common->Error(ERR_DROP, "LoadTGA: illegal pixel_size '%d' in file '%s'",
                                              targa_header.pixel_size, name);
                                break;
                        }

                        column++;

                        if(column == columns) { // pixel packet run spans across rows
                            column = 0;

                            if(row > 0) {
                                row--;
                            } else {
                                goto breakOut;
                            }

                            pixbuf = targa_rgba + row * columns * 4;
                        }
                    }
                }
            }

breakOut:
            ;
        }
    }

#if 1

    // TTimo: this is the chunk of code to ensure a behavior that meets TGA specs
    // bit 5 set => top-down
    if(targa_header.attributes & 0x20) {
        uchar8 *flip = static_cast<uchar8 *>(malloc(columns * 4));
        uchar8 *src, *dst;

        for(row = 0; row < rows / 2; row++) {
            src = targa_rgba + row * 4 * columns;
            dst = targa_rgba + (rows - row - 1) * 4 * columns;

            memcpy(flip, src, columns * 4);
            memcpy(src, dst, columns * 4);
            memcpy(dst, flip, columns * 4);
        }

        free(flip);
    }

#else

    // instead we just print a warning
    if(targa_header.attributes & 0x20) {
        clientRendererSystem->RefPrintf(PRINT_WARNING,
                                        "WARNING: '%s' TGA file header declares top-down image, ignoring\n", name);
    }

#endif

    if(width) {
        *width = columns;
    }

    if(height) {
        *height = rows;
    }

    *pic = targa_rgba;

    fileSystem->FreeFile(buffer.v);
}
