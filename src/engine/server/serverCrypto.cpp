////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2010 id Software LLC, a ZeniMax Media company.
// Copyright(C) 2011 - 2019 Dusan Jocic <dusanjocic@msn.com>
//
// This file is part of the OpenWolf GPL Source Code.
// OpenWolf Source Code is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OpenWolf Source Code is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OpenWolf Source Code.  If not, see <http://www.gnu.org/licenses/>.
//
// In addition, the OpenWolf Source Code is also subject to certain additional terms.
// You should have received a copy of these additional terms immediately following the
// terms and conditions of the GNU General Public License which accompanied the
// OpenWolf Source Code. If not, please request a copy in writing from id Software
// at the address below.
//
// If you have questions concerning this license or the applicable additional terms,
// you may contact in writing id Software LLC, c/o ZeniMax Media Inc.,
// Suite 120, Rockville, Maryland 20850 USA.
//
// -------------------------------------------------------------------------------------
// File name:   serverCrypto.cpp
// Version:     v1.02
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifdef UPDATE_SERVER
#include <null/null_autoprecompiled.h>
#elif DEDICATED
#include <null/null_serverprecompiled.h>
#else
#include <framework/precompiled.h>
#endif

// sanity checks
#if CRYPTOCIPHERHEXSIZE > 256 // compat limit: MAXCVARVALUESTRING
#error CRYPTOCIPHERHEXLEN does not fit inside a cvar (max 255 chars)
#endif

#if CRYPTOHASHBINSIZE < crypto_generichash_BYTES_MIN
#error CRYPTOHASHBINLEN is too small
#endif

#if CRYPTOHASHBINSIZE > crypto_generichash_BYTES_MAX
#error CRYPTOHASHBINLEN is too large
#endif

idServerCryptoSystemLocal serverCryptoSystemLocal;
idServerCryptoSystem* serverCryptoSystem = &serverCryptoSystemLocal;

/*
===============
idServerCryptoSystemLocal::idServerCryptoSystemLocal
===============
*/
idServerCryptoSystemLocal::idServerCryptoSystemLocal( void )
{
}

/*
===============
idServerCryptoSystemLocal::~idServerCryptoSystemLocal
===============
*/
idServerCryptoSystemLocal::~idServerCryptoSystemLocal( void )
{
}

/*
===============
idServerCryptoSystemLocal::InitCrypto
===============
*/
bool idServerCryptoSystemLocal::InitCrypto( void )
{
    if( sodium_init() == -1 )
    {
        Com_Printf( "Failed to initialize libsodium crypto module!\n" );
        return false;
    }
    
    Com_Printf( "Initialized libsodium crypto module successfully\n" );
    
    return true;
}

/*
===============
idServerCryptoSystemLocal::BinaryToHex
===============
*/
bool idServerCryptoSystemLocal::BinaryToHex( valueType* outHex, size_t hexMaxLen, const uchar8* inBin, size_t binSize )
{
    if( sodium_bin2hex( outHex, hexMaxLen, inBin, binSize ) != outHex )
    {
        Com_Printf( "Binary -> Hexadecimal conversion failed!\n" );
        return false;
    }
    
    return true;
}

/*
===============
idServerCryptoSystemLocal::HexToBinary
===============
*/
bool idServerCryptoSystemLocal::HexToBinary( uchar8* outBin, size_t binMaxLen, pointer inHex )
{
    if( sodium_hex2bin( outBin, binMaxLen, inHex, strlen( inHex ), "", nullptr, nullptr ) != 0 )
    {
        Com_Printf( "Hexadecimal -> Binary conversion failed!\n" );
        return false;
    }
    
    return true;
}

/*
===============
idServerCryptoSystemLocal::GenerateCryptoKeys
===============
*/
bool idServerCryptoSystemLocal::GenerateCryptoKeys( publicKey_t* pk, secretKey_t* sk )
{
    if( crypto_box_keypair( pk->keyBin, sk->keyBin ) )
    {
        Com_Printf( "Failed to generate public/secret key pair\n" );
        return false;
    }
    
    if( !BinaryToHex( pk->keyHex, sizeof( pk->keyHex ), pk->keyBin, sizeof( pk->keyBin ) )
            || !BinaryToHex( sk->keyHex, sizeof( sk->keyHex ), sk->keyBin, sizeof( sk->keyBin ) ) )
    {
        return false;
    }
    
    return true;
}

/*
===============
idServerCryptoSystemLocal::LoadKeyFromFile
===============
*/
bool idServerCryptoSystemLocal::LoadKeyFromFile( pointer filename, uchar8* out, size_t outSize )
{
    fileHandle_t file;
    long size = fileSystem->FOpenFileRead( filename, &file, true );
    
    if( !file )
    {
        Com_Printf( "Failed to open key file for reading: %s\n", filename );
        return false;
    }
    
    if( size != ( long )outSize )
    {
        // we need the key to have been saved with the exact same size
        Com_Printf( "Key file has incorrect size: %s. Expected %ld bytes, got %ld\n", filename, ( long )outSize, size );
        fileSystem->FCloseFile( file );
        return false;
    }
    
    fileSystem->Read( out, outSize, file );
    fileSystem->FCloseFile( file );
    
    return true;
}

/*
===============
idServerCryptoSystemLocal::LoadCryptoKeysFromFS
===============
*/
bool idServerCryptoSystemLocal::LoadCryptoKeysFromFS( publicKey_t* pk, pointer pkFilename, secretKey_t* sk, pointer skFilename )
{
    bool success = true;
    
    if( pk )
    {
        success = success
                  && LoadKeyFromFile( pkFilename, pk->keyBin, sizeof( pk->keyBin ) )
                  && BinaryToHex( pk->keyHex, sizeof( pk->keyHex ), pk->keyBin, sizeof( pk->keyBin ) )
                  ? true : false;
    }
    
    if( sk )
    {
        success = success
                  && LoadKeyFromFile( skFilename, sk->keyBin, sizeof( sk->keyBin ) )
                  && BinaryToHex( sk->keyHex, sizeof( sk->keyHex ), sk->keyBin, sizeof( sk->keyBin ) )
                  ? true : false;
    }
    
    return success;
}

/*
===============
idServerCryptoSystemLocal::SaveKeyToFile
===============
*/
bool idServerCryptoSystemLocal::SaveKeyToFile( pointer filename, const uchar8* in, size_t inSize )
{
    fileHandle_t file = fileSystem->FOpenFileWrite( filename );
    
    if( !file )
    {
        Com_Printf( "Failed to open key file for writing: %s\n", filename );
        return false;
    }
    
    fileSystem->Write( in, inSize, file );
    fileSystem->FCloseFile( file );
    
    return true;
}


/*
===============
idServerCryptoSystemLocal::SaveCryptoKeysToFS
===============
*/
bool idServerCryptoSystemLocal::SaveCryptoKeysToFS( publicKey_t* pk, pointer pkFilename, secretKey_t* sk, pointer skFilename )
{
    bool success = true;
    
    if( pk )
    {
        success = success
                  && SaveKeyToFile( pkFilename, pk->keyBin, sizeof( pk->keyBin ) )
                  ? true : false;
    }
    
    if( sk )
    {
        success = success
                  && SaveKeyToFile( skFilename, sk->keyBin, sizeof( sk->keyBin ) )
                  ? true : false;
    }
    
    return success;
}

/*
===============
idServerCryptoSystemLocal::EncryptString
===============
*/
bool idServerCryptoSystemLocal::EncryptString( publicKey_t* pk, pointer inRaw, valueType* outHex, size_t outHexSize )
{
    if( strlen( inRaw ) > CRYPTOCIPHERRAWSIZE - 1 )
    {
        Com_Printf( "String is too large to be encrypted: %s\n", inRaw );
        return false;
    }
    
    if( outHexSize < CRYPTOCIPHERHEXSIZE )
    {
        Com_Printf( "Encryption output buffer is too small\n" );
        return false;
    }
    
    uchar8 cipherBin[CRYPTOCIPHERBINSIZE];
    
    if( crypto_box_seal( cipherBin, ( const uchar8* )inRaw, CRYPTOCIPHERRAWSIZE - 1, pk->keyBin ) != 0 )
    {
        Com_Printf( "Failed to encrypt string: %s\n", inRaw );
        return false;
    }
    
    if( !BinaryToHex( outHex, outHexSize, cipherBin, sizeof( cipherBin ) ) )
    {
        return false;
    }
    
    return true;
}

/*
===============
idServerCryptoSystemLocal::DecryptString
===============
*/
bool idServerCryptoSystemLocal::DecryptString( publicKey_t* pk, secretKey_t* sk, pointer inHex, valueType* outRaw, size_t outRawSize )
{
    if( strlen( inHex ) > CRYPTOCIPHERHEXSIZE - 1 )
    {
        Com_Printf( "String is too large to be decrypted: %s\n", inHex );
        return false;
    }
    
    if( outRawSize < CRYPTOCIPHERRAWSIZE )
    {
        Com_Printf( "Decryption output buffer is too small\n" );
        return false;
    }
    
    uchar8 cipherBin[CRYPTOCIPHERBINSIZE];
    
    if( !HexToBinary( cipherBin, sizeof( cipherBin ), inHex ) )
    {
        return false;
    }
    
    if( crypto_box_seal_open( ( uchar8* )outRaw, cipherBin, sizeof( cipherBin ), pk->keyBin, sk->keyBin ) != 0 )
    {
        Com_Printf( "Failed to decrypt string: %s\n", inHex );
        return false;
    }
    
    return true;
}

/*
===============
idServerCryptoSystemLocal::CryptoHash
===============
*/
bool idServerCryptoSystemLocal::CryptoHash( pointer inRaw, valueType* outHex, size_t outHexSize )
{
    if( outHexSize < CRYPTOHASHHEXSIZE )
    {
        Com_Printf( "Hash output buffer is too small\n" );
        return false;
    }
    
    uchar8 hashBin[CRYPTOHASHBINSIZE];
    
    if( crypto_generichash( hashBin, sizeof( hashBin ), ( const uchar8* )inRaw, strlen( inRaw ), nullptr, 0 ) != 0 )
    {
        Com_Printf( "Failed to hash string: %s\n", inRaw );
        return false;
    }
    
    if( !BinaryToHex( outHex, outHexSize, hashBin, sizeof( hashBin ) ) )
    {
        return false;
    }
    
    return true;
}