////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2018 - 2019 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   serverWorld.h
// Version:     v1.01
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __SERVERCRYPTO_H__
#define __SERVERCRYPTO_H__

//
// idServerWorldSystemLocal
//
class idServerCryptoSystemLocal : public idServerCryptoSystem
{
public:
    idServerCryptoSystemLocal();
    ~idServerCryptoSystemLocal();
    
    virtual bool InitCrypto( void );
    virtual bool GenerateCryptoKeys( publicKey_t* pk, secretKey_t* sk );
    virtual bool LoadCryptoKeysFromFS( publicKey_t* pk, pointer pkFilename, secretKey_t* sk, pointer skFilename );
    virtual bool SaveCryptoKeysToFS( publicKey_t* pk, pointer pkFilename, secretKey_t* sk, pointer skFilename );
    virtual bool EncryptString( publicKey_t* pk, pointer inRaw, valueType* outHex, size_t outHexSize );
    virtual bool DecryptString( publicKey_t* pk, secretKey_t* sk, pointer inHex, valueType* outRaw, size_t outRawSize );
    virtual bool CryptoHash( pointer inRaw, valueType* outHex, size_t outHexSize );
    static bool LoadKeyFromFile( pointer filename, uchar8* out, size_t outSize );
    static bool HexToBinary( uchar8* outBin, size_t binMaxLen, pointer inHex );
    static bool BinaryToHex( valueType* outHex, size_t hexMaxLen, const uchar8* inBin, size_t binSize );
    static bool SaveKeyToFile( pointer filename, const uchar8* in, size_t inSize );
};

extern idServerCryptoSystemLocal serverCryptoSystemLocal;

#endif //!__SERVERCRYPTO_H__
