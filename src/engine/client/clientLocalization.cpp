////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2021 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   clientLocalization.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <framework/precompiled.hpp>

idClientLocalizationSystemLocal clientLocalizationLocal;
idClientLocalizationSystem *clientLocalizationSystem =
    &clientLocalizationLocal;

/*
===============
idClientLocalizationSystemLocal::idClientLocalizationSystemLocal
===============
*/
idClientLocalizationSystemLocal::idClientLocalizationSystemLocal(void) {
}

/*
===============
idClientLocalizationSystemLocal::~idClientLocalizationSystemLocal
===============
*/
idClientLocalizationSystemLocal::~idClientLocalizationSystemLocal(void) {
}

/*
=======================
idClientLocalizationSystemLocal::AllocTrans
=======================
*/
trans_t *idClientLocalizationSystemLocal::AllocTrans(valueType *original,
        valueType *translated[MAX_LANGUAGES]) {
    trans_t        *t;
    sint             i;

    t = (trans_t *)malloc(sizeof(trans_t));
    memset(t, 0, sizeof(trans_t));

    if(original) {
        strncpy(t->original, original, MAX_TRANS_STRING);
    }

    if(translated) {
        for(i = 0; i < MAX_LANGUAGES; i++) {
            strncpy(t->translated[i], translated[i], MAX_TRANS_STRING);
        }
    }

    return t;
}

/*
=======================
idClientLocalizationSystemLocal::generateHashValue
=======================
*/
sint32 idClientLocalizationSystemLocal::generateHashValue(pointer fname) {
    sint             i;
    sint32            hash;
    valueType            letter;

    hash = 0;
    i = 0;

    while(fname[i] != '\0') {
        letter = tolower(fname[i]);
        hash += static_cast<sint32>(letter) * (i + 119);
        i++;
    }

    hash &= (FILE_HASH_SIZE - 1);
    return hash;
}

/*
=======================
idClientLocalizationSystemLocal::LookupTrans
=======================
*/
trans_t *idClientLocalizationSystemLocal::LookupTrans(valueType *original,
        valueType *translated[MAX_LANGUAGES], bool isLoading) {
    trans_t        *t, *newt, *prev = nullptr;
    sint32            hash;

    hash = generateHashValue(original);

    for(t = transTable[hash]; t; prev = t, t = t->next) {
        if(!Q_stricmp(original, t->original)) {
            if(isLoading) {
                if(developer->integer) {
                    Com_Printf(S_COLOR_YELLOW "WARNING: Duplicate string found: \"%s\"\n",
                               original);
                }
            }

            return t;
        }
    }

    newt = AllocTrans(original, translated);

    if(prev) {
        prev->next = newt;
    } else {
        transTable[hash] = newt;
    }

    if(cl_debugTranslation->integer >= 1 && !isLoading) {
        if(developer->integer) {
            Com_Printf("Missing translation: \'%s\'\n", original);
        }
    }

    // see if we want to save out the translation table everytime a string is added
    if(cl_debugTranslation->integer == 2 && !isLoading) {
        SaveTransTable("new", true);
    }

    return newt;
}

/*
=======================
idClientLocalizationSystemLocal::SaveTransTable
=======================
*/
void idClientLocalizationSystemLocal::SaveTransTable(pointer fileName,
        bool newOnly) {
    sint             bucketlen, bucketnum, maxbucketlen, avebucketlen;
    sint             untransnum, transnum;
    pointer     buf;
    fileHandle_t    f;
    trans_t        *t;
    sint             i, j, len;

    if(cl.corruptedTranslationFile) {
        Com_Printf(S_COLOR_YELLOW
                   "WARNING: Cannot save corrupted translation file. Please reload first.");
        return;
    }

    fileSystem->FOpenFileByMode(fileName, &f, FS_WRITE);

    bucketnum = 0;
    maxbucketlen = 0;
    avebucketlen = 0;
    transnum = 0;
    untransnum = 0;

    // write out version, if one
    if(strlen(cl.translationVersion)) {
        buf = va("#version\t\t\"%s\"\n", cl.translationVersion);
    } else {
        buf = va("#version\t\t\"1.0 01/01/01\"\n");
    }

    len = strlen(buf);
    fileSystem->Write(buf, len, f);

    // write out translated strings
    for(j = 0; j < 2; j++) {

        for(i = 0; i < FILE_HASH_SIZE; i++) {
            t = transTable[i];

            if(!t || (newOnly && t->fromFile)) {
                continue;
            }

            bucketlen = 0;

            for(; t; t = t->next) {
                bucketlen++;

                if(strlen(t->translated[0])) {
                    if(j) {
                        continue;
                    }

                    transnum++;
                } else {
                    if(!j) {
                        continue;
                    }

                    untransnum++;
                }

                buf = va("{\n\tenglish\t\t\"%s\"\n", t->original);
                len = strlen(buf);
                fileSystem->Write(buf, len, f);

                buf = va("\tfrench\t\t\"%s\"\n", t->translated[LANGUAGE_FRENCH]);
                len = strlen(buf);
                fileSystem->Write(buf, len, f);

                buf = va("\tgerman\t\t\"%s\"\n", t->translated[LANGUAGE_GERMAN]);
                len = strlen(buf);
                fileSystem->Write(buf, len, f);

                buf = va("\titalian\t\t\"%s\"\n", t->translated[LANGUAGE_ITALIAN]);
                len = strlen(buf);
                fileSystem->Write(buf, len, f);

                buf = va("\tspanish\t\t\"%s\"\n", t->translated[LANGUAGE_SPANISH]);
                len = strlen(buf);
                fileSystem->Write(buf, len, f);

                buf = "}\n";
                len = strlen(buf);
                fileSystem->Write(buf, len, f);
            }

            if(bucketlen > maxbucketlen) {
                maxbucketlen = bucketlen;
            }

            if(bucketlen) {
                bucketnum++;
                avebucketlen += bucketlen;
            }
        }
    }

    Com_Printf("Saved translation table.\nTotal = %i, Translated = %i, Untranslated = %i, aveblen = %2.2f, maxblen = %i\n",
               transnum + untransnum, transnum, untransnum,
               static_cast<float32>(avebucketlen) / bucketnum, maxbucketlen);

    fileSystem->FCloseFile(f);
}

/*
=======================
idClientLocalizationSystemLocal::CheckTranslationString

NERVE - SMF - compare formatting UTF8acters
=======================
*/
bool idClientLocalizationSystemLocal::CheckTranslationString(
    valueType *original,
    valueType *translated) {
    valueType format_org[128], format_trans[128];
    sint len, i;

    memset(format_org, 0, 128);
    memset(format_trans, 0, 128);

    // generate formatting string for original
    len = strlen(original);

    for(i = 0; i < len; i++) {
        if(original[i] != '%') {
            continue;
        }

        strcat(format_org, va("%c%c ", '%', original[i + 1]));
    }

    // generate formatting string for translated
    len = strlen(translated);

    if(!len) {
        return true;
    }

    for(i = 0; i < len; i++) {
        if(translated[i] != '%') {
            continue;
        }

        strcat(format_trans, va("%c%c ", '%', translated[i + 1]));
    }

    // compare
    len = strlen(format_org);

    if(len != strlen(format_trans)) {
        return false;
    }

    for(i = 0; i < len; i++) {
        if(format_org[i] != format_trans[i]) {
            return false;
        }
    }

    return true;
}

/*
=======================
idClientLocalizationSystemLocal::LoadTransTable
=======================
*/
void idClientLocalizationSystemLocal::LoadTransTable(pointer fileName) {
    valueType            translated[MAX_LANGUAGES][MAX_VA_STRING];
    valueType            original[MAX_VA_STRING];
    bool        aborted;
    valueType           *text;
    fileHandle_t    f;
    valueType           *text_p;
    valueType           *token;
    sint             len, i;
    trans_t        *t;
    sint             count;

    count = 0;
    aborted = false;
    cl.corruptedTranslationFile = false;

    len = fileSystem->FOpenFileByMode(fileName, &f, FS_READ);

    if(len <= 0) {
        return;
    }

    // Gordon: shouldn't this be a z_malloc or something?
    text = static_cast<valueType *>(malloc(len + 1));

    if(!text) {
        return;
    }

    fileSystem->Read(text, len, f);
    text[len] = 0;
    fileSystem->FCloseFile(f);

    // parse the text
    text_p = text;

    do {
        token = COM_Parse(&text_p);

        if(Q_stricmp("{", token)) {
            // parse version number
            if(!Q_stricmp("#version", token)) {
                token = COM_Parse(&text_p);
                Q_strcpy_s(cl.translationVersion, token);
                continue;
            }

            break;
        }

        // english
        token = COM_Parse(&text_p);

        if(Q_stricmp("english", token)) {
            aborted = true;
            break;
        }

        token = COM_Parse(&text_p);
        Q_strcpy_s(original, token);

        if(cl_debugTranslation->integer == 3) {
            Com_Printf("%i Loading: \"%s\"\n", count, original);
        }

        // french
        token = COM_Parse(&text_p);

        if(Q_stricmp("french", token)) {
            aborted = true;
            break;
        }

        token = COM_Parse(&text_p);
        Q_strcpy_s(translated[LANGUAGE_FRENCH], token);

        if(!CheckTranslationString(original, translated[LANGUAGE_FRENCH])) {
            Com_Printf(S_COLOR_YELLOW
                       "WARNING: Translation formatting doesn't match up with English version!\n");
            aborted = true;
            break;
        }

        // german
        token = COM_Parse(&text_p);

        if(Q_stricmp("german", token)) {
            aborted = true;
            break;
        }

        token = COM_Parse(&text_p);
        Q_strcpy_s(translated[LANGUAGE_GERMAN], token);

        if(!CheckTranslationString(original, translated[LANGUAGE_GERMAN])) {
            Com_Printf(S_COLOR_YELLOW
                       "WARNING: Translation formatting doesn't match up with English version!\n");
            aborted = true;
            break;
        }

        // italian
        token = COM_Parse(&text_p);

        if(Q_stricmp("italian", token)) {
            aborted = true;
            break;
        }

        token = COM_Parse(&text_p);
        Q_strcpy_s(translated[LANGUAGE_ITALIAN], token);

        if(!CheckTranslationString(original, translated[LANGUAGE_ITALIAN])) {
            Com_Printf(S_COLOR_YELLOW
                       "WARNING: Translation formatting doesn't match up with English version!\n");
            aborted = true;
            break;
        }

        // spanish
        token = COM_Parse(&text_p);

        if(Q_stricmp("spanish", token)) {
            aborted = true;
            break;
        }

        token = COM_Parse(&text_p);
        Q_strcpy_s(translated[LANGUAGE_SPANISH], token);

        if(!CheckTranslationString(original, translated[LANGUAGE_SPANISH])) {
            Com_Printf(S_COLOR_YELLOW
                       "WARNING: Translation formatting doesn't match up with English version!\n");
            aborted = true;
            break;
        }

        // do lookup
        t = LookupTrans(original, nullptr, true);

        if(t) {
            t->fromFile = true;

            for(i = 0; i < MAX_LANGUAGES; i++) {
                strncpy(t->translated[i], translated[i], MAX_TRANS_STRING);
            }
        }

        token = COM_Parse(&text_p);

        // set offset if we have one
        if(!Q_stricmp("offset", token)) {
            token = COM_Parse(&text_p);
            t->x_offset = atof(token);

            token = COM_Parse(&text_p);
            t->y_offset = atof(token);

            token = COM_Parse(&text_p);
        }

        if(Q_stricmp("}", token)) {
            aborted = true;
            break;
        }

        count++;
    } while(token);

    if(aborted) {
        sint             i, line = 1;

        for(i = 0; i < len && (text + i) < text_p; i++) {
            if(text[i] == '\n') {
                line++;
            }
        }

        Com_Printf(S_COLOR_YELLOW "WARNING: Problem loading %s on line %i\n",
                   fileName, line);
        cl.corruptedTranslationFile = true;
    } else {
        Com_Printf("Loaded %i translation strings from %s\n", count, fileName);
    }

    // cleanup
    free(text);
}

/*
=======================
idClientLocalizationSystemLocal::ReloadTranslation
=======================
*/
void idClientLocalizationSystemLocal::ReloadTranslation(void) {
    sint i;
    valueType **fileList;
    uint64 numFiles;

    for(i = 0; i < FILE_HASH_SIZE; i++) {
        if(transTable[i]) {
            free(transTable[i]);
        }
    }

    memset(transTable, 0, sizeof(trans_t *) * FILE_HASH_SIZE);
    LoadTransTable("scripts/translation.lang");

    fileList = fileSystem->ListFiles("translations", ".lang", &numFiles);

    for(i = 0; i < numFiles; i++) {
        LoadTransTable(va("translations/%s", fileList[i]));
    }
}

/*
=======================
idClientLocalizationSystemLocal::InitTranslation
=======================
*/
void idClientLocalizationSystemLocal::InitTranslation(void) {
    sint i;
    uint64 numFiles;
    valueType **fileList;

    memset(transTable, 0, sizeof(trans_t *) * FILE_HASH_SIZE);
    LoadTransTable("scripts/translation.lang");

    fileList = fileSystem->ListFiles("translations", ".lang", &numFiles);

    for(i = 0; i < numFiles; i++) {
        LoadTransTable(va("translations/%s", fileList[i]));
    }
}

/*
=======================
idClientLocalizationSystemLocal::TranslateString
=======================
*/
void idClientLocalizationSystemLocal::TranslateString(pointer string,
        valueType *dest_buffer) {
    sint             i, count, currentLanguage;
    trans_t        *t;
    bool        newline = false;
    valueType           *buf;

    buf = dest_buffer;
    currentLanguage = cl_language->integer - 1;

    // early bail if we only want english or bad language type
    if(!string) {
        ::strcpy(buf, "(null)");
        return;
    } else if(currentLanguage < 0 || currentLanguage >= MAX_LANGUAGES ||
              !strlen(string)) {
        ::strcpy(buf, string);
        return;
    }

    // ignore newlines
    if(string[strlen(string) - 1] == '\n') {
        newline = true;
    }

    for(i = 0, count = 0; string[i] != '\0'; i++) {
        if(string[i] != '\n') {
            buf[count++] = string[i];
        }
    }

    buf[count] = '\0';

    t = LookupTrans(buf, nullptr, false);

    if(t && strlen(t->translated[currentLanguage])) {
        sint             offset = 0;

        if(cl_debugTranslation->integer >= 1) {
            buf[0] = '^';
            buf[1] = '1';
            buf[2] = '[';
            offset = 3;
        }

        ::strcpy(buf + offset, t->translated[currentLanguage]);

        if(cl_debugTranslation->integer >= 1) {
            sint             len2 = strlen(buf);

            buf[len2] = ']';
            buf[len2 + 1] = '^';
            buf[len2 + 2] = '7';
            buf[len2 + 3] = '\0';
        }

        if(newline) {
            sint             len2 = strlen(buf);

            buf[len2] = '\n';
            buf[len2 + 1] = '\0';
        }
    } else {
        sint             offset = 0;

        if(cl_debugTranslation->integer >= 1) {
            buf[0] = '^';
            buf[1] = '1';
            buf[2] = '[';
            offset = 3;
        }

        ::strcpy(buf + offset, string);

        if(cl_debugTranslation->integer >= 1) {
            sint             len2 = strlen(buf);
            bool        addnewline = false;

            if(buf[len2 - 1] == '\n') {
                len2--;
                addnewline = true;
            }

            buf[len2] = ']';
            buf[len2 + 1] = '^';
            buf[len2 + 2] = '7';
            buf[len2 + 3] = '\0';

            if(addnewline) {
                buf[len2 + 3] = '\n';
                buf[len2 + 4] = '\0';
            }
        }
    }
}

/*
=======================
idClientLocalizationSystemLocal::TranslateStringBuf

TTimo - handy, stores in a static buf, converts \n to chr(13)
=======================
*/
pointer idClientLocalizationSystemLocal::TranslateStringBuf(
    pointer string) {
    valueType           *p;
    sint             i, l;
    static valueType     buf[MAX_VA_STRING];

    TranslateString(string, buf);

    while((p = strstr(buf, "\\n")) != nullptr) {
        *p = '\n';
        p++;
        // ::memcpy(p, p+1, strlen(p) ); b0rks on win32
        l = strlen(p);

        for(i = 0; i < l; i++) {
            *p = *(p + 1);
            p++;
        }
    }

    return buf;
}

/*
=======================
idClientLocalizationSystemLocal::SaveTranslations_f
=======================
*/
void idClientLocalizationSystemLocal::SaveTranslations_f(void) {
    SaveTransTable("scripts/translation.lang", false);
}

/*
=======================
idClientLocalizationSystemLocal::SaveNewTranslations_f
=======================
*/
void idClientLocalizationSystemLocal::SaveNewTranslations_f(void) {
    valueType fileName[512];

    if(cmdSystem->Argc() != 2) {
        Com_Printf("usage: SaveNewTranslations <filename>\n");
        return;
    }

    Q_strcpy_s(fileName, va("translations/%s.lang", cmdSystem->Argv(1)));

    SaveTransTable(fileName, true);
}

/*
=======================
idClientLocalizationSystemLocal::LoadTranslations_f
=======================
*/
void idClientLocalizationSystemLocal::LoadTranslations_f(void) {
    ReloadTranslation();
}

