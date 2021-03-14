////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2007, 2008 Joerg Dietrich
// Copyright(C) 2000 - 2013 Darklegion Development
// Copyright(C) 2015 - 2018 GrangerHub
// Copyright(C) 2015 - 2021 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   r_image_png.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <framework/precompiled.hpp>

// we could limit the png size to a lower value here
#ifndef INT_MAX
#define INT_MAX 0x1fffffff
#endif

/*
=================
PNG LOADING
=================
*/

/*
 *  Quake 3 image format : RGBA
 */

#define Q3IMAGE_BYTESPERPIXEL (4)

/*
 *  PNG specifications
 */

/*
 *  The first 8 Bytes of every PNG-File are a fixed signature
 *  to identify the file as a PNG.
 */

#define PNG_Signature "\x89\x50\x4E\x47\xD\xA\x1A\xA"
#define PNG_Signature_Size (8)

/*
 *  After the signature diverse chunks follow.
 *  A chunk consists of a header and if Length
 *  is bigger than 0 a body and a CRC of the body follow.
 */

struct PNG_ChunkHeader
{
    uint Length;
    uint Type;
};

#define PNG_ChunkHeader_Size (8)

typedef uint PNG_ChunkCRC;

#define PNG_ChunkCRC_Size (4)

/*
 *  We use the following ChunkTypes.
 *  All others are ignored.
 */

#define MAKE_CHUNKTYPE(a,b,c,d) (((a) << 24) | ((b) << 16) | ((c) << 8) | ((d)))

#define PNG_ChunkType_IHDR MAKE_CHUNKTYPE('I', 'H', 'D', 'R')
#define PNG_ChunkType_PLTE MAKE_CHUNKTYPE('P', 'L', 'T', 'E')
#define PNG_ChunkType_IDAT MAKE_CHUNKTYPE('I', 'D', 'A', 'T')
#define PNG_ChunkType_IEND MAKE_CHUNKTYPE('I', 'E', 'N', 'D')
#define PNG_ChunkType_tRNS MAKE_CHUNKTYPE('t', 'R', 'N', 'S')

/*
 *  Per specification the first chunk after the signature SHALL be IHDR.
 */

struct PNG_Chunk_IHDR
{
    uint Width;
    uint Height;
    uchar8  BitDepth;
    uchar8  ColourType;
    uchar8  CompressionMethod;
    uchar8  FilterMethod;
    uchar8  InterlaceMethod;
};

#define PNG_Chunk_IHDR_Size (13)

/*
 *  ColourTypes
 */

#define PNG_ColourType_Grey      (0)
#define PNG_ColourType_True      (2)
#define PNG_ColourType_Indexed   (3)
#define PNG_ColourType_GreyAlpha (4)
#define PNG_ColourType_TrueAlpha (6)

/*
 *  number of colour components
 *
 *  Grey      : 1 grey
 *  True      : 1 R, 1 G, 1 B
 *  Indexed   : 1 index
 *  GreyAlpha : 1 grey, 1 alpha
 *  TrueAlpha : 1 R, 1 G, 1 B, 1 alpha
 */

#define PNG_NumColourComponents_Grey      (1)
#define PNG_NumColourComponents_True      (3)
#define PNG_NumColourComponents_Indexed   (1)
#define PNG_NumColourComponents_GreyAlpha (2)
#define PNG_NumColourComponents_TrueAlpha (4)

/*
 *  For the different ColourTypes
 *  different BitDepths are specified.
 */

#define PNG_BitDepth_1  ( 1)
#define PNG_BitDepth_2  ( 2)
#define PNG_BitDepth_4  ( 4)
#define PNG_BitDepth_8  ( 8)
#define PNG_BitDepth_16 (16)

/*
 *  Only one valid CompressionMethod is standardized.
 */

#define PNG_CompressionMethod_0 (0)

/*
 *  Only one valid FilterMethod is currently standardized.
 */

#define PNG_FilterMethod_0 (0)

/*
 *  This FilterMethod defines 5 FilterTypes
 */

#define PNG_FilterType_None    (0)
#define PNG_FilterType_Sub     (1)
#define PNG_FilterType_Up      (2)
#define PNG_FilterType_Average (3)
#define PNG_FilterType_Paeth   (4)

/*
 *  Two InterlaceMethods are standardized :
 *  0 - NonInterlaced
 *  1 - Interlaced
 */

#define PNG_InterlaceMethod_NonInterlaced (0)
#define PNG_InterlaceMethod_Interlaced    (1)

/*
 *  The Adam7 interlace method uses 7 passes.
 */

#define PNG_Adam7_NumPasses (7)

/*
 *  The compressed data starts with a header ...
 */

struct PNG_ZlibHeader
{
    uchar8 CompressionMethod;
    uchar8 Flags;
};

#define PNG_ZlibHeader_Size (2)

/*
 *  ... and is followed by a check value
 */

#define PNG_ZlibCheckValue_Size (4)

/*
 *  Some support functions for buffered files follow.
 */

/*
 *  buffered file representation
 */

struct BufferedFile
{
    uchar8* Buffer;
    sint   Length;
    uchar8* Ptr;
    sint   BytesLeft;
};

/*
 *  Read a file into a buffer.
 */

static struct BufferedFile* ReadBufferedFile( pointer name )
{
    struct BufferedFile* BF;
    union
    {
        uchar8* b;
        void* v;
    } buffer;
    
    /*
     *  input verification
     */
    
    if( !name )
    {
        return( nullptr );
    }
    
    /*
     *  Allocate control struct.
     */
    
    BF = ( struct BufferedFile* )CL_RefMalloc( sizeof( struct BufferedFile ) );
    if( !BF )
    {
        return( nullptr );
    }
    
    /*
     *  Initialize the structs components.
     */
    
    BF->Length    = 0;
    BF->Buffer    = nullptr;
    BF->Ptr       = nullptr;
    BF->BytesLeft = 0;
    
    /*
     *  Read the file.
     */
    
    BF->Length = fileSystem->ReadFile( const_cast< valueType* >( name ), &buffer.v );
    BF->Buffer = buffer.b;
    
    /*
     *  Did we get it? Is it big enough?
     */
    
    if( !( BF->Buffer && ( BF->Length > 0 ) ) )
    {
        Z_Free( BF );
        
        return( nullptr );
    }
    
    /*
     *  Set the pointers and counters.
     */
    
    BF->Ptr       = BF->Buffer;
    BF->BytesLeft = BF->Length;
    
    return( BF );
}

/*
 *  Close a buffered file.
 */

static void CloseBufferedFile( struct BufferedFile* BF )
{
    if( BF )
    {
        if( BF->Buffer )
        {
            fileSystem->FreeFile( BF->Buffer );
        }
        
        Z_Free( BF );
    }
}

/*
 *  Get a pointer to the requested bytes.
 */

static void* BufferedFileRead( struct BufferedFile* BF, uint Length )
{
    void* RetVal;
    
    /*
     *  input verification
     */
    
    if( !( BF && Length ) )
    {
        return( nullptr );
    }
    
    /*
     *  not enough bytes left
     */
    
    if( Length > BF->BytesLeft )
    {
        return( nullptr );
    }
    
    /*
     *  the pointer to the requested data
     */
    
    RetVal = BF->Ptr;
    
    /*
     *  Raise the pointer and counter.
     */
    
    BF->Ptr       += Length;
    BF->BytesLeft -= Length;
    
    return( RetVal );
}

/*
 *  Rewind the buffer.
 */

static bool BufferedFileRewind( struct BufferedFile* BF, uint Offset )
{
    uint BytesRead;
    
    /*
     *  input verification
     */
    
    if( !BF )
    {
        return( false );
    }
    
    /*
     *  special trick to rewind to the beginning of the buffer
     */
    
    if( Offset == static_cast<uint>( -1 ) )
    {
        BF->Ptr       = BF->Buffer;
        BF->BytesLeft = BF->Length;
        
        return( true );
    }
    
    /*
     *  How many bytes do we have already read?
     */
    
    BytesRead = BF->Ptr - BF->Buffer;
    
    /*
     *  We can only rewind to the beginning of the BufferedFile.
     */
    
    if( Offset > BytesRead )
    {
        return( false );
    }
    
    /*
     *  lower the pointer and counter.
     */
    
    BF->Ptr       -= Offset;
    BF->BytesLeft += Offset;
    
    return( true );
}

/*
 *  Skip some bytes.
 */

static bool BufferedFileSkip( struct BufferedFile* BF, uint Offset )
{
    /*
     *  input verification
     */
    
    if( !BF )
    {
        return( false );
    }
    
    /*
     *  We can only skip to the end of the BufferedFile.
     */
    
    if( Offset > BF->BytesLeft )
    {
        return( false );
    }
    
    /*
     *  lower the pointer and counter.
     */
    
    BF->Ptr       += Offset;
    BF->BytesLeft -= Offset;
    
    return( true );
}

/*
 *  Find a chunk
 */

static bool FindChunk( struct BufferedFile* BF, uint ChunkType )
{
    struct PNG_ChunkHeader* CH;
    
    uint Length;
    uint Type;
    
    /*
     *  input verification
     */
    
    if( !BF )
    {
        return( false );
    }
    
    /*
     *  cycle trough the chunks
     */
    
    while( true )
    {
        /*
         *  Read the chunk-header.
         */
        
        CH = ( struct PNG_ChunkHeader* )BufferedFileRead( BF, PNG_ChunkHeader_Size );
        if( !CH )
        {
            return( false );
        }
        
        /*
         *  Do not swap the original types
         *  they might be needed later.
         */
        
        Length = BigLong( CH->Length );
        Type   = BigLong( CH->Type );
        
        /*
         *  We found it!
         */
        
        if( Type == ChunkType )
        {
            /*
             *  Rewind to the start of the chunk.
             */
            
            BufferedFileRewind( BF, PNG_ChunkHeader_Size );
            
            break;
        }
        else
        {
            /*
             *  Skip the rest of the chunk.
             */
            
            if( Length )
            {
                if( !BufferedFileSkip( BF, Length + PNG_ChunkCRC_Size ) )
                {
                    return( false );
                }
            }
        }
    }
    
    return( true );
}

/*
 *  Decompress all IDATs
 */

static uint DecompressIDATs( struct BufferedFile* BF, uchar8** Buffer )
{
    uchar8*  DecompressedData;
    uint  DecompressedDataLength;
    
    uchar8*  CompressedData;
    uchar8*  CompressedDataPtr;
    uint  CompressedDataLength;
    
    struct PNG_ChunkHeader* CH;
    
    uint Length;
    uint Type;
    
    sint BytesToRewind;
    
    sint   puffResult;
    uchar8*  puffDest;
    uint  puffDestLen;
    uchar8*  puffSrc;
    uint  puffSrcLen;
    
    /*
     *  input verification
     */
    
    if( !( BF && Buffer ) )
    {
        return( -1 );
    }
    
    /*
     *  some zeroing
     */
    
    DecompressedData = nullptr;
    *Buffer = DecompressedData;
    
    CompressedData = nullptr;
    CompressedDataLength = 0;
    
    BytesToRewind = 0;
    
    /*
     *  Find the first IDAT chunk.
     */
    
    if( !FindChunk( BF, PNG_ChunkType_IDAT ) )
    {
        return( -1 );
    }
    
    /*
     *  Count the size of the uncompressed data
     */
    
    while( true )
    {
        /*
         *  Read chunk header
         */
        
        CH = ( struct PNG_ChunkHeader* )BufferedFileRead( BF, PNG_ChunkHeader_Size );
        if( !CH )
        {
            /*
             *  Rewind to the start of this adventure
             *  and return unsuccessfull
             */
            
            BufferedFileRewind( BF, BytesToRewind );
            
            return( -1 );
        }
        
        /*
         *  Length and Type of chunk
         */
        
        Length = BigLong( CH->Length );
        Type   = BigLong( CH->Type );
        
        /*
         *  We have reached the end of the IDAT chunks
         */
        
        if( !( Type == PNG_ChunkType_IDAT ) )
        {
            BufferedFileRewind( BF, PNG_ChunkHeader_Size );
            
            break;
        }
        
        /*
         *  Add chunk header to count.
         */
        
        BytesToRewind += PNG_ChunkHeader_Size;
        
        /*
         *  Skip to next chunk
         */
        
        if( Length )
        {
            if( !BufferedFileSkip( BF, Length + PNG_ChunkCRC_Size ) )
            {
                BufferedFileRewind( BF, BytesToRewind );
                
                return( -1 );
            }
            
            BytesToRewind += Length + PNG_ChunkCRC_Size;
            CompressedDataLength += Length;
        }
    }
    
    BufferedFileRewind( BF, BytesToRewind );
    
    CompressedData = static_cast<uchar8*>( CL_RefMalloc( CompressedDataLength ) );
    if( !CompressedData )
    {
        return( -1 );
    }
    
    CompressedDataPtr = CompressedData;
    
    /*
     *  Collect the compressed Data
     */
    
    while( true )
    {
        /*
         *  Read chunk header
         */
        
        CH = ( struct PNG_ChunkHeader* )BufferedFileRead( BF, PNG_ChunkHeader_Size );
        if( !CH )
        {
            Z_Free( CompressedData );
            
            return( -1 );
        }
        
        /*
         *  Length and Type of chunk
         */
        
        Length = BigLong( CH->Length );
        Type   = BigLong( CH->Type );
        
        /*
         *  We have reached the end of the IDAT chunks
         */
        
        if( !( Type == PNG_ChunkType_IDAT ) )
        {
            BufferedFileRewind( BF, PNG_ChunkHeader_Size );
            
            break;
        }
        
        /*
         *  Copy the Data
         */
        
        if( Length )
        {
            uchar8* OrigCompressedData;
            
            OrigCompressedData = static_cast<uchar8*>( BufferedFileRead( BF, Length ) );
            if( !OrigCompressedData )
            {
                Z_Free( CompressedData );
                
                return( -1 );
            }
            
            if( !BufferedFileSkip( BF, PNG_ChunkCRC_Size ) )
            {
                Z_Free( CompressedData );
                
                return( -1 );
            }
            
            memcpy( CompressedDataPtr, OrigCompressedData, Length );
            CompressedDataPtr += Length;
        }
    }
    
    /*
     *  Let puff() calculate the decompressed data length.
     */
    
    puffDest    = nullptr;
    puffDestLen = 0;
    
    /*
     *  The zlib header and checkvalue don't belong to the compressed data.
     */
    
    puffSrc    = CompressedData + PNG_ZlibHeader_Size;
    puffSrcLen = CompressedDataLength - PNG_ZlibHeader_Size - PNG_ZlibCheckValue_Size;
    
    /*
     *  first puff() to calculate the size of the uncompressed data
     */
    
    puffResult = puff( puffDest, &puffDestLen, puffSrc, &puffSrcLen );
    if( !( ( puffResult == 0 ) && ( puffDestLen > 0 ) ) )
    {
        Z_Free( CompressedData );
        
        return( -1 );
    }
    
    /*
     *  Allocate the buffer for the uncompressed data.
     */
    
    DecompressedData = static_cast<uchar8*>( CL_RefMalloc( puffDestLen ) );
    if( !DecompressedData )
    {
        Z_Free( CompressedData );
        
        return( -1 );
    }
    
    /*
     *  Set the input again in case something was changed by the last puff() .
     */
    
    puffDest   = DecompressedData;
    puffSrc    = CompressedData + PNG_ZlibHeader_Size;
    puffSrcLen = CompressedDataLength - PNG_ZlibHeader_Size - PNG_ZlibCheckValue_Size;
    
    /*
     *  decompression puff()
     */
    
    puffResult = puff( puffDest, &puffDestLen, puffSrc, &puffSrcLen );
    
    /*
     *  The compressed data is not needed anymore.
     */
    
    Z_Free( CompressedData );
    
    /*
     *  Check if the last puff() was successfull.
     */
    
    if( !( ( puffResult == 0 ) && ( puffDestLen > 0 ) ) )
    {
        Z_Free( DecompressedData );
        
        return( -1 );
    }
    
    /*
     *  Set the output of this function.
     */
    
    DecompressedDataLength = puffDestLen;
    *Buffer = DecompressedData;
    
    return( DecompressedDataLength );
}

/*
 *  the Paeth predictor
 */

static uchar8 PredictPaeth( uchar8 a, uchar8 b, uchar8 c )
{
    /*
     *  a == Left
     *  b == Up
     *  c == UpLeft
     */
    
    uchar8 Pr;
    sint p;
    sint pa, pb, pc;
    
    p  = ( static_cast<sint>( a ) ) + ( static_cast<sint>( b ) ) - ( static_cast<sint>( c ) );
    pa = abs( p - ( static_cast<sint>( a ) ) );
    pb = abs( p - ( static_cast<sint>( b ) ) );
    pc = abs( p - ( static_cast<sint>( c ) ) );
    
    if( ( pa <= pb ) && ( pa <= pc ) )
    {
        Pr = a;
    }
    else if( pb <= pc )
    {
        Pr = b;
    }
    else
    {
        Pr = c;
    }
    
    return( Pr );
    
}

/*
 *  Reverse the filters.
 */

static bool UnfilterImage( uchar8*  DecompressedData,
                           uint  ImageHeight,
                           uint  BytesPerScanline,
                           uint  BytesPerPixel )
{
    uchar8*   DecompPtr;
    uchar8   FilterType;
    uchar8*  PixelLeft, *PixelUp, *PixelUpLeft;
    uint  w, h, p;
    
    /*
     *  some zeros for the filters
     */
    
    uchar8 Zeros[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    
    /*
     *  input verification
     */
    
    if( !( DecompressedData && BytesPerPixel ) )
    {
        return( false );
    }
    
    /*
     *  ImageHeight and BytesPerScanline can be zero in small interlaced images.
     */
    
    if( ( !ImageHeight ) || ( !BytesPerScanline ) )
    {
        return( true );
    }
    
    /*
     *  Set the pointer to the start of the decompressed Data.
     */
    
    DecompPtr = DecompressedData;
    
    /*
     *  Un-filtering is done in place.
     */
    
    /*
     *  Go trough all scanlines.
     */
    
    for( h = 0; h < ImageHeight; h++ )
    {
        /*
         *  Every scanline starts with a FilterType byte.
         */
        
        FilterType = *DecompPtr;
        DecompPtr++;
        
        /*
         *  Left pixel of the first byte in a scanline is zero.
         */
        
        PixelLeft = Zeros;
        
        /*
         *  Set PixelUp to previous line only if we are on the second line or above.
         *
         *  Plus one byte for the FilterType
         */
        
        if( h > 0 )
        {
            PixelUp = DecompPtr - ( BytesPerScanline + 1 );
        }
        else
        {
            PixelUp = Zeros;
        }
        
        /*
         * The pixel left to the first pixel of the previous scanline is zero too.
         */
        
        PixelUpLeft = Zeros;
        
        /*
         *  Cycle trough all pixels of the scanline.
         */
        
        for( w = 0; w < ( BytesPerScanline / BytesPerPixel ); w++ )
        {
            /*
             *  Cycle trough the bytes of the pixel.
             */
            
            for( p = 0; p < BytesPerPixel; p++ )
            {
                switch( FilterType )
                {
                    case PNG_FilterType_None :
                    {
                        /*
                         *  The byte is unfiltered.
                         */
                        
                        break;
                    }
                    
                    case PNG_FilterType_Sub :
                    {
                        DecompPtr[p] += PixelLeft[p];
                        
                        break;
                    }
                    
                    case PNG_FilterType_Up :
                    {
                        DecompPtr[p] += PixelUp[p];
                        
                        break;
                    }
                    
                    case PNG_FilterType_Average :
                    {
                        DecompPtr[p] += ( static_cast<uchar8>( ( ( static_cast<uchar16>( PixelLeft[p] ) ) + ( static_cast<uchar16>( PixelUp[p] ) ) ) / 2 ) );
                        
                        break;
                    }
                    
                    case PNG_FilterType_Paeth :
                    {
                        DecompPtr[p] += PredictPaeth( PixelLeft[p], PixelUp[p], PixelUpLeft[p] );
                        
                        break;
                    }
                    
                    default :
                    {
                        return( false );
                    }
                }
            }
            
            PixelLeft = DecompPtr;
            
            /*
             *  We only have an upleft pixel if we are on the second line or above.
             */
            
            if( h > 0 )
            {
                PixelUpLeft = DecompPtr - ( BytesPerScanline + 1 );
            }
            
            /*
             *  Skip to the next pixel.
             */
            
            DecompPtr += BytesPerPixel;
            
            /*
             *  We only have a previous line if we are on the second line and above.
             */
            
            if( h > 0 )
            {
                PixelUp = DecompPtr - ( BytesPerScanline + 1 );
            }
        }
    }
    
    return( true );
}

/*
 *  Convert a raw input pixel to Quake 3 RGA format.
 */

static bool ConvertPixel( struct PNG_Chunk_IHDR* IHDR,
                          uchar8*                  OutPtr,
                          uchar8*               DecompPtr,
                          bool               HasTransparentColour,
                          uchar8*               TransparentColour,
                          uchar8*               OutPal )
{
    /*
     *  input verification
     */
    
    if( !( IHDR && OutPtr && DecompPtr && TransparentColour && OutPal ) )
    {
        return( false );
    }
    
    switch( IHDR->ColourType )
    {
        case PNG_ColourType_Grey :
        {
            switch( IHDR->BitDepth )
            {
                case PNG_BitDepth_1 :
                case PNG_BitDepth_2 :
                case PNG_BitDepth_4 :
                {
                    uchar8 Step;
                    uchar8 GreyValue;
                    
                    Step = 0xFF / ( ( 1 << IHDR->BitDepth ) - 1 );
                    
                    GreyValue = DecompPtr[0] * Step;
                    
                    OutPtr[0] = GreyValue;
                    OutPtr[1] = GreyValue;
                    OutPtr[2] = GreyValue;
                    OutPtr[3] = 0xFF;
                    
                    /*
                     *  Grey supports full transparency for one specified colour
                     */
                    
                    if( HasTransparentColour )
                    {
                        if( TransparentColour[1] == DecompPtr[0] )
                        {
                            OutPtr[3] = 0x00;
                        }
                    }
                    
                    
                    break;
                }
                
                case PNG_BitDepth_8 :
                case PNG_BitDepth_16 :
                {
                    OutPtr[0] = DecompPtr[0];
                    OutPtr[1] = DecompPtr[0];
                    OutPtr[2] = DecompPtr[0];
                    OutPtr[3] = 0xFF;
                    
                    /*
                     *  Grey supports full transparency for one specified colour
                     */
                    
                    if( HasTransparentColour )
                    {
                        if( IHDR->BitDepth == PNG_BitDepth_8 )
                        {
                            if( TransparentColour[1] == DecompPtr[0] )
                            {
                                OutPtr[3] = 0x00;
                            }
                        }
                        else
                        {
                            if( ( TransparentColour[0] == DecompPtr[0] ) && ( TransparentColour[1] == DecompPtr[1] ) )
                            {
                                OutPtr[3] = 0x00;
                            }
                        }
                    }
                    
                    break;
                }
                
                default :
                {
                    return( false );
                }
            }
            
            break;
        }
        
        case PNG_ColourType_True :
        {
            switch( IHDR->BitDepth )
            {
                case PNG_BitDepth_8 :
                {
                    OutPtr[0] = DecompPtr[0];
                    OutPtr[1] = DecompPtr[1];
                    OutPtr[2] = DecompPtr[2];
                    OutPtr[3] = 0xFF;
                    
                    /*
                     *  True supports full transparency for one specified colour
                     */
                    
                    if( HasTransparentColour )
                    {
                        if( ( TransparentColour[1] == DecompPtr[0] ) &&
                                ( TransparentColour[3] == DecompPtr[1] ) &&
                                ( TransparentColour[5] == DecompPtr[2] ) )
                        {
                            OutPtr[3] = 0x00;
                        }
                    }
                    
                    break;
                }
                
                case PNG_BitDepth_16 :
                {
                    /*
                     *  We use only the upper byte.
                     */
                    
                    OutPtr[0] = DecompPtr[0];
                    OutPtr[1] = DecompPtr[2];
                    OutPtr[2] = DecompPtr[4];
                    OutPtr[3] = 0xFF;
                    
                    /*
                     *  True supports full transparency for one specified colour
                     */
                    
                    if( HasTransparentColour )
                    {
                        if( ( TransparentColour[0] == DecompPtr[0] ) && ( TransparentColour[1] == DecompPtr[1] ) &&
                                ( TransparentColour[2] == DecompPtr[2] ) && ( TransparentColour[3] == DecompPtr[3] ) &&
                                ( TransparentColour[4] == DecompPtr[4] ) && ( TransparentColour[5] == DecompPtr[5] ) )
                        {
                            OutPtr[3] = 0x00;
                        }
                    }
                    
                    break;
                }
                
                default :
                {
                    return( false );
                }
            }
            
            break;
        }
        
        case PNG_ColourType_Indexed :
        {
            OutPtr[0] = OutPal[DecompPtr[0] * Q3IMAGE_BYTESPERPIXEL + 0];
            OutPtr[1] = OutPal[DecompPtr[0] * Q3IMAGE_BYTESPERPIXEL + 1];
            OutPtr[2] = OutPal[DecompPtr[0] * Q3IMAGE_BYTESPERPIXEL + 2];
            OutPtr[3] = OutPal[DecompPtr[0] * Q3IMAGE_BYTESPERPIXEL + 3];
            
            break;
        }
        
        case PNG_ColourType_GreyAlpha :
        {
            switch( IHDR->BitDepth )
            {
                case PNG_BitDepth_8 :
                {
                    OutPtr[0] = DecompPtr[0];
                    OutPtr[1] = DecompPtr[0];
                    OutPtr[2] = DecompPtr[0];
                    OutPtr[3] = DecompPtr[1];
                    
                    break;
                }
                
                case PNG_BitDepth_16 :
                {
                    /*
                     *  We use only the upper byte.
                     */
                    
                    OutPtr[0] = DecompPtr[0];
                    OutPtr[1] = DecompPtr[0];
                    OutPtr[2] = DecompPtr[0];
                    OutPtr[3] = DecompPtr[2];
                    
                    break;
                }
                
                default :
                {
                    return( false );
                }
            }
            
            break;
        }
        
        case PNG_ColourType_TrueAlpha :
        {
            switch( IHDR->BitDepth )
            {
                case PNG_BitDepth_8 :
                {
                    OutPtr[0] = DecompPtr[0];
                    OutPtr[1] = DecompPtr[1];
                    OutPtr[2] = DecompPtr[2];
                    OutPtr[3] = DecompPtr[3];
                    
                    break;
                }
                
                case PNG_BitDepth_16 :
                {
                    /*
                     *  We use only the upper byte.
                     */
                    
                    OutPtr[0] = DecompPtr[0];
                    OutPtr[1] = DecompPtr[2];
                    OutPtr[2] = DecompPtr[4];
                    OutPtr[3] = DecompPtr[6];
                    
                    break;
                }
                
                default :
                {
                    return( false );
                }
            }
            
            break;
        }
        
        default :
        {
            return( false );
        }
    }
    
    return( true );
}


/*
 *  Decode a non-interlaced image.
 */

static bool DecodeImageNonInterlaced( struct PNG_Chunk_IHDR* IHDR,
                                      uchar8*                  OutBuffer,
                                      uchar8*               DecompressedData,
                                      uint               DecompressedDataLength,
                                      bool               HasTransparentColour,
                                      uchar8*               TransparentColour,
                                      uchar8*               OutPal )
{
    uint IHDR_Width;
    uint IHDR_Height;
    uint BytesPerScanline, BytesPerPixel, PixelsPerByte;
    uint  w, h, p;
    uchar8* OutPtr;
    uchar8* DecompPtr;
    
    /*
     *  input verification
     */
    
    if( !( IHDR && OutBuffer && DecompressedData && DecompressedDataLength && TransparentColour && OutPal ) )
    {
        return( false );
    }
    
    /*
     *  byte swapping
     */
    
    IHDR_Width  = BigLong( IHDR->Width );
    IHDR_Height = BigLong( IHDR->Height );
    
    /*
     *  information for un-filtering
     */
    
    switch( IHDR->ColourType )
    {
        case PNG_ColourType_Grey :
        {
            switch( IHDR->BitDepth )
            {
                case PNG_BitDepth_1 :
                case PNG_BitDepth_2 :
                case PNG_BitDepth_4 :
                {
                    BytesPerPixel    = 1;
                    PixelsPerByte    = 8 / IHDR->BitDepth;
                    
                    break;
                }
                
                case PNG_BitDepth_8  :
                case PNG_BitDepth_16 :
                {
                    BytesPerPixel    = ( IHDR->BitDepth / 8 ) * PNG_NumColourComponents_Grey;
                    PixelsPerByte    = 1;
                    
                    break;
                }
                
                default :
                {
                    return( false );
                }
            }
            
            break;
        }
        
        case PNG_ColourType_True :
        {
            switch( IHDR->BitDepth )
            {
                case PNG_BitDepth_8  :
                case PNG_BitDepth_16 :
                {
                    BytesPerPixel    = ( IHDR->BitDepth / 8 ) * PNG_NumColourComponents_True;
                    PixelsPerByte    = 1;
                    
                    break;
                }
                
                default :
                {
                    return( false );
                }
            }
            
            break;
        }
        
        case PNG_ColourType_Indexed :
        {
            switch( IHDR->BitDepth )
            {
                case PNG_BitDepth_1 :
                case PNG_BitDepth_2 :
                case PNG_BitDepth_4 :
                {
                    BytesPerPixel    = 1;
                    PixelsPerByte    = 8 / IHDR->BitDepth;
                    
                    break;
                }
                
                case PNG_BitDepth_8 :
                {
                    BytesPerPixel    = PNG_NumColourComponents_Indexed;
                    PixelsPerByte    = 1;
                    
                    break;
                }
                
                default :
                {
                    return( false );
                }
            }
            
            break;
        }
        
        case PNG_ColourType_GreyAlpha :
        {
            switch( IHDR->BitDepth )
            {
                case PNG_BitDepth_8 :
                case PNG_BitDepth_16 :
                {
                    BytesPerPixel    = ( IHDR->BitDepth / 8 ) * PNG_NumColourComponents_GreyAlpha;
                    PixelsPerByte    = 1;
                    
                    break;
                }
                
                default :
                {
                    return( false );
                }
            }
            
            break;
        }
        
        case PNG_ColourType_TrueAlpha :
        {
            switch( IHDR->BitDepth )
            {
                case PNG_BitDepth_8 :
                case PNG_BitDepth_16 :
                {
                    BytesPerPixel    = ( IHDR->BitDepth / 8 ) * PNG_NumColourComponents_TrueAlpha;
                    PixelsPerByte    = 1;
                    
                    break;
                }
                
                default :
                {
                    return( false );
                }
            }
            
            break;
        }
        
        default :
        {
            return( false );
        }
    }
    
    /*
     *  Calculate the size of one scanline
     */
    
    BytesPerScanline = ( IHDR_Width * BytesPerPixel + ( PixelsPerByte - 1 ) ) / PixelsPerByte;
    
    /*
     *  Check if we have enough data for the whole image.
     */
    
    if( !( DecompressedDataLength == ( ( BytesPerScanline + 1 ) * IHDR_Height ) ) )
    {
        return( false );
    }
    
    /*
     *  Unfilter the image.
     */
    
    if( !UnfilterImage( DecompressedData, IHDR_Height, BytesPerScanline, BytesPerPixel ) )
    {
        return( false );
    }
    
    /*
     *  Set the working pointers to the beginning of the buffers.
     */
    
    OutPtr = OutBuffer;
    DecompPtr = DecompressedData;
    
    /*
     *  Create the output image.
     */
    
    for( h = 0; h < IHDR_Height; h++ )
    {
        /*
         *  Count the pixels on the scanline for those multipixel bytes
         */
        
        uint CurrPixel;
        
        /*
         *  skip FilterType
         */
        
        DecompPtr++;
        
        /*
         *  Reset the pixel count.
         */
        
        CurrPixel = 0;
        
        for( w = 0; w < ( BytesPerScanline / BytesPerPixel ); w++ )
        {
            if( PixelsPerByte > 1 )
            {
                uchar8  Mask;
                uint Shift;
                uchar8  SinglePixel;
                
                for( p = 0; p < PixelsPerByte; p++ )
                {
                    if( CurrPixel < IHDR_Width )
                    {
                        Mask  = ( 1 << IHDR->BitDepth ) - 1;
                        Shift = ( PixelsPerByte - 1 - p ) * IHDR->BitDepth;
                        
                        SinglePixel = ( ( DecompPtr[0] & ( Mask << Shift ) ) >> Shift );
                        
                        if( !ConvertPixel( IHDR, OutPtr, &SinglePixel, HasTransparentColour, TransparentColour, OutPal ) )
                        {
                            return( false );
                        }
                        
                        OutPtr += Q3IMAGE_BYTESPERPIXEL;
                        CurrPixel++;
                    }
                }
                
            }
            else
            {
                if( !ConvertPixel( IHDR, OutPtr, DecompPtr, HasTransparentColour, TransparentColour, OutPal ) )
                {
                    return( false );
                }
                
                
                OutPtr += Q3IMAGE_BYTESPERPIXEL;
            }
            
            DecompPtr += BytesPerPixel;
        }
    }
    
    return( true );
}

/*
 *  Decode an interlaced image.
 */

static bool DecodeImageInterlaced( struct PNG_Chunk_IHDR* IHDR,
                                   uchar8*                  OutBuffer,
                                   uchar8*               DecompressedData,
                                   uint               DecompressedDataLength,
                                   bool               HasTransparentColour,
                                   uchar8*               TransparentColour,
                                   uchar8*               OutPal )
{
    uint IHDR_Width;
    uint IHDR_Height;
    uint BytesPerScanline[PNG_Adam7_NumPasses], BytesPerPixel, PixelsPerByte;
    uint PassWidth[PNG_Adam7_NumPasses], PassHeight[PNG_Adam7_NumPasses];
    uint WSkip[PNG_Adam7_NumPasses], WOffset[PNG_Adam7_NumPasses], HSkip[PNG_Adam7_NumPasses], HOffset[PNG_Adam7_NumPasses];
    uint w, h, p, a;
    uchar8* OutPtr;
    uchar8* DecompPtr;
    uint TargetLength;
    
    /*
     *  input verification
     */
    
    if( !( IHDR && OutBuffer && DecompressedData && DecompressedDataLength && TransparentColour && OutPal ) )
    {
        return( false );
    }
    
    /*
     *  byte swapping
     */
    
    IHDR_Width  = BigLong( IHDR->Width );
    IHDR_Height = BigLong( IHDR->Height );
    
    /*
     *  Skip and Offset for the passes.
     */
    
    WSkip[0]   = 8;
    WOffset[0] = 0;
    HSkip[0]   = 8;
    HOffset[0] = 0;
    
    WSkip[1]   = 8;
    WOffset[1] = 4;
    HSkip[1]   = 8;
    HOffset[1] = 0;
    
    WSkip[2]   = 4;
    WOffset[2] = 0;
    HSkip[2]   = 8;
    HOffset[2] = 4;
    
    WSkip[3]   = 4;
    WOffset[3] = 2;
    HSkip[3]   = 4;
    HOffset[3] = 0;
    
    WSkip[4]   = 2;
    WOffset[4] = 0;
    HSkip[4]   = 4;
    HOffset[4] = 2;
    
    WSkip[5]   = 2;
    WOffset[5] = 1;
    HSkip[5]   = 2;
    HOffset[5] = 0;
    
    WSkip[6]   = 1;
    WOffset[6] = 0;
    HSkip[6]   = 2;
    HOffset[6] = 1;
    
    /*
     *  Calculate the sizes of the passes.
     */
    
    PassWidth[0]  = ( IHDR_Width  + 7 ) / 8;
    PassHeight[0] = ( IHDR_Height + 7 ) / 8;
    
    PassWidth[1]  = ( IHDR_Width  + 3 ) / 8;
    PassHeight[1] = ( IHDR_Height + 7 ) / 8;
    
    PassWidth[2]  = ( IHDR_Width  + 3 ) / 4;
    PassHeight[2] = ( IHDR_Height + 3 ) / 8;
    
    PassWidth[3]  = ( IHDR_Width  + 1 ) / 4;
    PassHeight[3] = ( IHDR_Height + 3 ) / 4;
    
    PassWidth[4]  = ( IHDR_Width  + 1 ) / 2;
    PassHeight[4] = ( IHDR_Height + 1 ) / 4;
    
    PassWidth[5]  = ( IHDR_Width  + 0 ) / 2;
    PassHeight[5] = ( IHDR_Height + 1 ) / 2;
    
    PassWidth[6]  = ( IHDR_Width  + 0 ) / 1;
    PassHeight[6] = ( IHDR_Height + 0 ) / 2;
    
    /*
     *  information for un-filtering
     */
    
    switch( IHDR->ColourType )
    {
        case PNG_ColourType_Grey :
        {
            switch( IHDR->BitDepth )
            {
                case PNG_BitDepth_1 :
                case PNG_BitDepth_2 :
                case PNG_BitDepth_4 :
                {
                    BytesPerPixel    = 1;
                    PixelsPerByte    = 8 / IHDR->BitDepth;
                    
                    break;
                }
                
                case PNG_BitDepth_8  :
                case PNG_BitDepth_16 :
                {
                    BytesPerPixel    = ( IHDR->BitDepth / 8 ) * PNG_NumColourComponents_Grey;
                    PixelsPerByte    = 1;
                    
                    break;
                }
                
                default :
                {
                    return( false );
                }
            }
            
            break;
        }
        
        case PNG_ColourType_True :
        {
            switch( IHDR->BitDepth )
            {
                case PNG_BitDepth_8  :
                case PNG_BitDepth_16 :
                {
                    BytesPerPixel    = ( IHDR->BitDepth / 8 ) * PNG_NumColourComponents_True;
                    PixelsPerByte    = 1;
                    
                    break;
                }
                
                default :
                {
                    return( false );
                }
            }
            
            break;
        }
        
        case PNG_ColourType_Indexed :
        {
            switch( IHDR->BitDepth )
            {
                case PNG_BitDepth_1 :
                case PNG_BitDepth_2 :
                case PNG_BitDepth_4 :
                {
                    BytesPerPixel    = 1;
                    PixelsPerByte    = 8 / IHDR->BitDepth;
                    
                    break;
                }
                
                case PNG_BitDepth_8 :
                {
                    BytesPerPixel    = PNG_NumColourComponents_Indexed;
                    PixelsPerByte    = 1;
                    
                    break;
                }
                
                default :
                {
                    return( false );
                }
            }
            
            break;
        }
        
        case PNG_ColourType_GreyAlpha :
        {
            switch( IHDR->BitDepth )
            {
                case PNG_BitDepth_8 :
                case PNG_BitDepth_16 :
                {
                    BytesPerPixel    = ( IHDR->BitDepth / 8 ) * PNG_NumColourComponents_GreyAlpha;
                    PixelsPerByte    = 1;
                    
                    break;
                }
                
                default :
                {
                    return( false );
                }
            }
            
            break;
        }
        
        case PNG_ColourType_TrueAlpha :
        {
            switch( IHDR->BitDepth )
            {
                case PNG_BitDepth_8 :
                case PNG_BitDepth_16 :
                {
                    BytesPerPixel    = ( IHDR->BitDepth / 8 ) * PNG_NumColourComponents_TrueAlpha;
                    PixelsPerByte    = 1;
                    
                    break;
                }
                
                default :
                {
                    return( false );
                }
            }
            
            break;
        }
        
        default :
        {
            return( false );
        }
    }
    
    /*
     *  Calculate the size of the scanlines per pass
     */
    
    for( a = 0; a < PNG_Adam7_NumPasses; a++ )
    {
        BytesPerScanline[a] = ( PassWidth[a] * BytesPerPixel + ( PixelsPerByte - 1 ) ) / PixelsPerByte;
    }
    
    /*
     *  Calculate the size of all passes
     */
    
    TargetLength = 0;
    
    for( a = 0; a < PNG_Adam7_NumPasses; a++ )
    {
        TargetLength += ( ( BytesPerScanline[a] + ( BytesPerScanline[a] ? 1 : 0 ) ) * PassHeight[a] );
    }
    
    /*
     *  Check if we have enough data for the whole image.
     */
    
    if( !( DecompressedDataLength == TargetLength ) )
    {
        return( false );
    }
    
    /*
     *  Unfilter the image.
     */
    
    DecompPtr = DecompressedData;
    
    for( a = 0; a < PNG_Adam7_NumPasses; a++ )
    {
        if( !UnfilterImage( DecompPtr, PassHeight[a], BytesPerScanline[a], BytesPerPixel ) )
        {
            return( false );
        }
        
        DecompPtr += ( ( BytesPerScanline[a] + ( BytesPerScanline[a] ? 1 : 0 ) ) * PassHeight[a] );
    }
    
    /*
     *  Set the working pointers to the beginning of the buffers.
     */
    
    DecompPtr = DecompressedData;
    
    /*
     *  Create the output image.
     */
    
    for( a = 0; a < PNG_Adam7_NumPasses; a++ )
    {
        for( h = 0; h < PassHeight[a]; h++ )
        {
            /*
             *  Count the pixels on the scanline for those multipixel bytes
             */
            
            uint CurrPixel;
            
            /*
             *  skip FilterType
             *  but only when the pass has a width bigger than zero
             */
            
            if( BytesPerScanline[a] )
            {
                DecompPtr++;
            }
            
            /*
             *  Reset the pixel count.
             */
            
            CurrPixel = 0;
            
            for( w = 0; w < ( BytesPerScanline[a] / BytesPerPixel ); w++ )
            {
                if( PixelsPerByte > 1 )
                {
                    uchar8  Mask;
                    uint Shift;
                    uchar8  SinglePixel;
                    
                    for( p = 0; p < PixelsPerByte; p++ )
                    {
                        if( CurrPixel < PassWidth[a] )
                        {
                            Mask  = ( 1 << IHDR->BitDepth ) - 1;
                            Shift = ( PixelsPerByte - 1 - p ) * IHDR->BitDepth;
                            
                            SinglePixel = ( ( DecompPtr[0] & ( Mask << Shift ) ) >> Shift );
                            
                            OutPtr = OutBuffer + ( ( ( ( ( h * HSkip[a] ) + HOffset[a] ) * IHDR_Width ) + ( ( CurrPixel * WSkip[a] ) + WOffset[a] ) ) * Q3IMAGE_BYTESPERPIXEL );
                            
                            if( !ConvertPixel( IHDR, OutPtr, &SinglePixel, HasTransparentColour, TransparentColour, OutPal ) )
                            {
                                return( false );
                            }
                            
                            CurrPixel++;
                        }
                    }
                    
                }
                else
                {
                    OutPtr = OutBuffer + ( ( ( ( ( h * HSkip[a] ) + HOffset[a] ) * IHDR_Width ) + ( ( w * WSkip[a] ) + WOffset[a] ) ) * Q3IMAGE_BYTESPERPIXEL );
                    
                    if( !ConvertPixel( IHDR, OutPtr, DecompPtr, HasTransparentColour, TransparentColour, OutPal ) )
                    {
                        return( false );
                    }
                }
                
                DecompPtr += BytesPerPixel;
            }
        }
    }
    
    return( true );
}

/*
 *  The PNG loader
 */

void R_LoadPNG( pointer name, uchar8** pic, sint* width, sint* height )
{
    struct BufferedFile* ThePNG;
    uchar8* OutBuffer;
    uchar8* Signature;
    struct PNG_ChunkHeader* CH;
    uint ChunkHeaderLength;
    uint ChunkHeaderType;
    struct PNG_Chunk_IHDR* IHDR;
    uint IHDR_Width;
    uint IHDR_Height;
    PNG_ChunkCRC* CRC;
    uchar8* InPal;
    uchar8* DecompressedData;
    uint DecompressedDataLength;
    uint i;
    
    /*
     *  palette with 256 RGBA entries
     */
    
    uchar8 OutPal[1024];
    
    /*
     *  transparent colour from the tRNS chunk
     */
    
    bool HasTransparentColour = false;
    uchar8 TransparentColour[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    
    /*
     *  input verification
     */
    
    if( !( name && pic ) )
    {
        return;
    }
    
    /*
     *  Zero out return values.
     */
    
    *pic = nullptr;
    
    if( width )
    {
        *width = 0;
    }
    
    if( height )
    {
        *height = 0;
    }
    
    /*
     *  Read the file.
     */
    
    ThePNG = ReadBufferedFile( name );
    if( !ThePNG )
    {
        return;
    }
    
    /*
     *  Read the siganture of the file.
     */
    
    Signature = static_cast<uchar8*>( BufferedFileRead( ThePNG, PNG_Signature_Size ) );
    if( !Signature )
    {
        CloseBufferedFile( ThePNG );
        
        return;
    }
    
    /*
     *  Is it a PNG?
     */
    
    if( memcmp( Signature, PNG_Signature, PNG_Signature_Size ) )
    {
        CloseBufferedFile( ThePNG );
        
        return;
    }
    
    /*
     *  Read the first chunk-header.
     */
    
    CH = ( struct PNG_ChunkHeader* )BufferedFileRead( ThePNG, PNG_ChunkHeader_Size );
    if( !CH )
    {
        CloseBufferedFile( ThePNG );
        
        return;
    }
    
    /*
     *  PNG multi-byte types are in Big Endian
     */
    
    ChunkHeaderLength = BigLong( CH->Length );
    ChunkHeaderType   = BigLong( CH->Type );
    
    /*
     *  Check if the first chunk is an IHDR.
     */
    
    if( !( ( ChunkHeaderType == PNG_ChunkType_IHDR ) && ( ChunkHeaderLength == PNG_Chunk_IHDR_Size ) ) )
    {
        CloseBufferedFile( ThePNG );
        
        return;
    }
    
    /*
     *  Read the IHDR.
     */
    
    IHDR = ( struct PNG_Chunk_IHDR* )BufferedFileRead( ThePNG, PNG_Chunk_IHDR_Size );
    if( !IHDR )
    {
        CloseBufferedFile( ThePNG );
        
        return;
    }
    
    /*
     *  Read the CRC for IHDR
     */
    
    CRC = ( PNG_ChunkCRC* )BufferedFileRead( ThePNG, PNG_ChunkCRC_Size );
    if( !CRC )
    {
        CloseBufferedFile( ThePNG );
        
        return;
    }
    
    /*
     *  Here we could check the CRC if we wanted to.
     */
    
    /*
     *  multi-byte type swapping
     */
    
    IHDR_Width  = BigLong( IHDR->Width );
    IHDR_Height = BigLong( IHDR->Height );
    
    /*
     *  Check if Width and Height are valid.
     */
    
    if( !( ( IHDR_Width > 0 ) && ( IHDR_Height > 0 ) )
            || IHDR_Width > INT_MAX / Q3IMAGE_BYTESPERPIXEL / IHDR_Height )
    {
        CloseBufferedFile( ThePNG );
        
        CL_RefPrintf( PRINT_WARNING, "%s: invalid image size\n", name );
        
        return;
    }
    
    /*
     *  Do we need to check if the dimensions of the image are valid for Quake3?
     */
    
    /*
     *  Check if CompressionMethod and FilterMethod are valid.
     */
    
    if( !( ( IHDR->CompressionMethod == PNG_CompressionMethod_0 ) && ( IHDR->FilterMethod == PNG_FilterMethod_0 ) ) )
    {
        CloseBufferedFile( ThePNG );
        
        return;
    }
    
    /*
     *  Check if InterlaceMethod is valid.
     */
    
    if( !( ( IHDR->InterlaceMethod == PNG_InterlaceMethod_NonInterlaced )  || ( IHDR->InterlaceMethod == PNG_InterlaceMethod_Interlaced ) ) )
    {
        CloseBufferedFile( ThePNG );
        
        return;
    }
    
    /*
     *  Read palette for an indexed image.
     */
    
    if( IHDR->ColourType == PNG_ColourType_Indexed )
    {
        /*
         *  We need the palette first.
         */
        
        if( !FindChunk( ThePNG, PNG_ChunkType_PLTE ) )
        {
            CloseBufferedFile( ThePNG );
            
            return;
        }
        
        /*
         *  Read the chunk-header.
         */
        
        CH = ( struct PNG_ChunkHeader* )BufferedFileRead( ThePNG, PNG_ChunkHeader_Size );
        if( !CH )
        {
            CloseBufferedFile( ThePNG );
            
            return;
        }
        
        /*
         *  PNG multi-byte types are in Big Endian
         */
        
        ChunkHeaderLength = BigLong( CH->Length );
        ChunkHeaderType   = BigLong( CH->Type );
        
        /*
         *  Check if the chunk is a PLTE.
         */
        
        if( !( ChunkHeaderType == PNG_ChunkType_PLTE ) )
        {
            CloseBufferedFile( ThePNG );
            
            return;
        }
        
        /*
         *  Check if Length is divisible by 3
         */
        
        if( ChunkHeaderLength % 3 )
        {
            CloseBufferedFile( ThePNG );
            
            return;
        }
        
        /*
         *  Read the raw palette data
         */
        
        InPal = static_cast<uchar8*>( BufferedFileRead( ThePNG, ChunkHeaderLength ) );
        if( !InPal )
        {
            CloseBufferedFile( ThePNG );
            
            return;
        }
        
        /*
         *  Read the CRC for the palette
         */
        
        CRC = ( PNG_ChunkCRC* )BufferedFileRead( ThePNG, PNG_ChunkCRC_Size );
        if( !CRC )
        {
            CloseBufferedFile( ThePNG );
            
            return;
        }
        
        /*
         *  Set some default values.
         */
        
        for( i = 0; i < 256; i++ )
        {
            OutPal[i * Q3IMAGE_BYTESPERPIXEL + 0] = 0x00;
            OutPal[i * Q3IMAGE_BYTESPERPIXEL + 1] = 0x00;
            OutPal[i * Q3IMAGE_BYTESPERPIXEL + 2] = 0x00;
            OutPal[i * Q3IMAGE_BYTESPERPIXEL + 3] = 0xFF;
        }
        
        /*
         *  Convert to the Quake3 RGBA-format.
         */
        
        for( i = 0; i < ( ChunkHeaderLength / 3 ); i++ )
        {
            OutPal[i * Q3IMAGE_BYTESPERPIXEL + 0] = InPal[i * 3 + 0];
            OutPal[i * Q3IMAGE_BYTESPERPIXEL + 1] = InPal[i * 3 + 1];
            OutPal[i * Q3IMAGE_BYTESPERPIXEL + 2] = InPal[i * 3 + 2];
            OutPal[i * Q3IMAGE_BYTESPERPIXEL + 3] = 0xFF;
        }
    }
    
    /*
     *  transparency information is sometimes stored in a tRNS chunk
     */
    
    /*
     *  Let's see if there is a tRNS chunk
     */
    
    if( FindChunk( ThePNG, PNG_ChunkType_tRNS ) )
    {
        uchar8* Trans;
        
        /*
         *  Read the chunk-header.
         */
        
        CH = ( struct PNG_ChunkHeader* )BufferedFileRead( ThePNG, PNG_ChunkHeader_Size );
        if( !CH )
        {
            CloseBufferedFile( ThePNG );
            
            return;
        }
        
        /*
         *  PNG multi-byte types are in Big Endian
         */
        
        ChunkHeaderLength = BigLong( CH->Length );
        ChunkHeaderType   = BigLong( CH->Type );
        
        /*
         *  Check if the chunk is a tRNS.
         */
        
        if( !( ChunkHeaderType == PNG_ChunkType_tRNS ) )
        {
            CloseBufferedFile( ThePNG );
            
            return;
        }
        
        /*
         *  Read the transparency information.
         */
        
        Trans = static_cast<uchar8*>( BufferedFileRead( ThePNG, ChunkHeaderLength ) );
        if( !Trans )
        {
            CloseBufferedFile( ThePNG );
            
            return;
        }
        
        /*
         *  Read the CRC.
         */
        
        CRC = ( PNG_ChunkCRC* )BufferedFileRead( ThePNG, PNG_ChunkCRC_Size );
        if( !CRC )
        {
            CloseBufferedFile( ThePNG );
            
            return;
        }
        
        /*
         *  Only for Grey, True and Indexed ColourType should tRNS exist.
         */
        
        switch( IHDR->ColourType )
        {
            case PNG_ColourType_Grey :
            {
                if( ChunkHeaderLength != 2 )
                {
                    CloseBufferedFile( ThePNG );
                    
                    return;
                }
                
                HasTransparentColour = true;
                
                /*
                 *  Grey can have one colour which is completely transparent.
                 *  This colour is always stored in 16 bits.
                 */
                
                TransparentColour[0] = Trans[0];
                TransparentColour[1] = Trans[1];
                
                break;
            }
            
            case PNG_ColourType_True :
            {
                if( ChunkHeaderLength != 6 )
                {
                    CloseBufferedFile( ThePNG );
                    
                    return;
                }
                
                HasTransparentColour = true;
                
                /*
                 *  True can have one colour which is completely transparent.
                 *  This colour is always stored in 16 bits.
                 */
                
                TransparentColour[0] = Trans[0];
                TransparentColour[1] = Trans[1];
                TransparentColour[2] = Trans[2];
                TransparentColour[3] = Trans[3];
                TransparentColour[4] = Trans[4];
                TransparentColour[5] = Trans[5];
                
                break;
            }
            
            case PNG_ColourType_Indexed :
            {
                /*
                 *  Maximum of 256 one byte transparency entries.
                 */
                
                if( ChunkHeaderLength > 256 )
                {
                    CloseBufferedFile( ThePNG );
                    
                    return;
                }
                
                HasTransparentColour = true;
                
                /*
                 *  alpha values for palette entries
                 */
                
                for( i = 0; i < ChunkHeaderLength; i++ )
                {
                    OutPal[i * Q3IMAGE_BYTESPERPIXEL + 3] = Trans[i];
                }
                
                break;
            }
            
            /*
             *  All other ColourTypes should not have tRNS chunks
             */
            
            default :
            {
                CloseBufferedFile( ThePNG );
                
                return;
            }
        }
    }
    
    /*
     *  Rewind to the start of the file.
     */
    
    if( !BufferedFileRewind( ThePNG, -1 ) )
    {
        CloseBufferedFile( ThePNG );
        
        return;
    }
    
    /*
     *  Skip the signature
     */
    
    if( !BufferedFileSkip( ThePNG, PNG_Signature_Size ) )
    {
        CloseBufferedFile( ThePNG );
        
        return;
    }
    
    /*
     *  Decompress all IDAT chunks
     */
    
    DecompressedDataLength = DecompressIDATs( ThePNG, &DecompressedData );
    if( !( DecompressedDataLength && DecompressedData ) )
    {
        CloseBufferedFile( ThePNG );
        
        return;
    }
    
    /*
     *  Allocate output buffer.
     */
    
    OutBuffer = static_cast<uchar8*>( CL_RefMalloc( IHDR_Width * IHDR_Height * Q3IMAGE_BYTESPERPIXEL ) );
    if( !OutBuffer )
    {
        Z_Free( DecompressedData );
        CloseBufferedFile( ThePNG );
        
        return;
    }
    
    /*
     *  Interlaced and Non-interlaced images need to be handled differently.
     */
    
    switch( IHDR->InterlaceMethod )
    {
        case PNG_InterlaceMethod_NonInterlaced :
        {
            if( !DecodeImageNonInterlaced( IHDR, OutBuffer, DecompressedData, DecompressedDataLength, HasTransparentColour, TransparentColour, OutPal ) )
            {
                Z_Free( OutBuffer );
                Z_Free( DecompressedData );
                CloseBufferedFile( ThePNG );
                
                return;
            }
            
            break;
        }
        
        case PNG_InterlaceMethod_Interlaced :
        {
            if( !DecodeImageInterlaced( IHDR, OutBuffer, DecompressedData, DecompressedDataLength, HasTransparentColour, TransparentColour, OutPal ) )
            {
                Z_Free( OutBuffer );
                Z_Free( DecompressedData );
                CloseBufferedFile( ThePNG );
                
                return;
            }
            
            break;
        }
        
        default :
        {
            Z_Free( OutBuffer );
            Z_Free( DecompressedData );
            CloseBufferedFile( ThePNG );
            
            return;
        }
    }
    
    /*
     *  update the pointer to the image data
     */
    
    *pic = OutBuffer;
    
    /*
     *  Fill width and height.
     */
    
    if( width )
    {
        *width = IHDR_Width;
    }
    
    if( height )
    {
        *height = IHDR_Height;
    }
    
    /*
     *  DecompressedData is not needed anymore.
     */
    
    Z_Free( DecompressedData );
    
    /*
     *  We have all data, so close the file.
     */
    
    CloseBufferedFile( ThePNG );
}
