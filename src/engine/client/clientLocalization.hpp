////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2021 Dusan Jocic <dusanjocic@msn.com>
//
// This file is part of OpenWolf.
//
// OpenWolf is free software; you can redistribute it
// and / or modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the License,
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
// File name:   clientLocalization.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __CLIENTLOCALIZATION_HPP__
#define __CLIENTLOCALIZATION_HPP__

#define FILE_HASH_SIZE_      1024
#define MAX_VA_STRING       32000
#define MAX_TRANS_STRING    4096

// NERVE - SMF - Localization code
typedef struct trans_s {
    valueType            original[MAX_TRANS_STRING];
    valueType            translated[MAX_LANGUAGES][MAX_TRANS_STRING];
    struct trans_s *next;
    float32           x_offset;
    float32           y_offset;
    bool        fromFile;
} trans_t;

static trans_t *transTable[FILE_HASH_SIZE_];

//
// idClientLocalizationSystemLocal
//
class idClientLocalizationSystemLocal : public idClientLocalizationSystem {
public:
    idClientLocalizationSystemLocal();
    ~idClientLocalizationSystemLocal();

    virtual void InitTranslation(void);
    virtual void TranslateString(pointer string, valueType *dest_buffer);
    virtual pointer TranslateStringBuf(pointer string);

    static trans_t *AllocTrans(valueType *original,
                               valueType *translated[MAX_LANGUAGES]);
    static sint32 generateHashValue(pointer fname);
    static trans_t *LookupTrans(valueType *original,
                                valueType *translated[MAX_LANGUAGES], bool isLoading);
    static void SaveTransTable(pointer fileName, bool newOnly);
    static bool CheckTranslationString(valueType *original,
                                       valueType *translated);
    static void LoadTransTable(pointer fileName);
    static void ReloadTranslation(void);
    static void SaveNewTranslations_f(void);
    static void SaveTranslations_f(void);
    static void LoadTranslations_f(void);
};

extern idClientLocalizationSystemLocal clientLocalizationLocal;

#endif // !__CLIENTLOCALIZATION_HPP__
