////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2010 id Software LLC, a ZeniMax Media company.
// Copyright(C) 2011 - 2023 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   consoleHistory.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0,
//              AppleClang 9.0.0.9000039
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifdef UPDATE_SERVER
#include <server/serverAutoPrecompiled.hpp>
#elif DEDICATED
#include <server/serverDedPrecompiled.hpp>
#else
#include <framework/precompiled.hpp>
#endif

idConsoleHistoryLocal consoleHistoryLocal;
idConsoleHistorySystem *consoleHistorySystem = &consoleHistoryLocal;

/*
===============
idConsoleHistoryLocal::idConsoleHistoryLocal
===============
*/
idConsoleHistoryLocal::idConsoleHistoryLocal(void) {
}

/*
===============
idConsoleHistoryLocal::~idConsoleHistoryLocal
===============
*/
idConsoleHistoryLocal::~idConsoleHistoryLocal(void) {
}

/*
==================
idConsoleHistoryLocal::Load
==================
*/
void idConsoleHistoryLocal::Load(void) {
    sint i;
    fileHandle_t f;
    valueType *buf, * end;
    valueType buffer[sizeof(history)];

    fileSystem->SV_FOpenFileRead(CON_HISTORY_FILE, &f);

    if(!f) {
        common->Printf("Couldn't read %s.\n", CON_HISTORY_FILE);
        return;
    }

    ::memset(buffer, '\0', sizeof(buffer));
    ::memset(history, '\0', sizeof(history));

    fileSystem->Read(buffer, sizeof(buffer), f);
    fileSystem->FCloseFile(f);

    buf = buffer;

    for(i = 0; i < CON_HISTORY; i++) {
        end = strchr(buf, '\n');

        if(!end) {
            Q_strncpyz(history[i], buf, sizeof(history[0]));
            break;
        }

        *end = '\0';

        Q_strncpyz(history[i], buf, sizeof(history[0]));
        buf = end + 1;

        if(!*buf) {
            break;
        }
    }

    if(i > CON_HISTORY) {
        i = CON_HISTORY;
    }

    hist_current = hist_next = i + 1;
}

/*
==================
idConsoleHistoryLocal::Save
==================
*/
void idConsoleHistoryLocal::Save(void) {
    sint i;
    fileHandle_t f;

    f = fileSystem->SV_FOpenFileWrite(CON_HISTORY_FILE);

    if(!f) {
        common->Printf("Couldn't write %s.\n", CON_HISTORY_FILE);
        return;
    }

    i = (hist_next + 1) % CON_HISTORY;

    do {
        valueType *buf;

        if(!history[i][0]) {
            i = (i + 1) % CON_HISTORY;
            continue;
        }

        buf = va("%s\n", history[i]);
        fileSystem->Write(buf, strlen(buf), f);
        i = (i + 1) % CON_HISTORY;
    } while(i != (hist_next - 1) % CON_HISTORY);

    fileSystem->FCloseFile(f);
}

/*
==================
idConsoleHistoryLocal::Add
==================
*/
void idConsoleHistoryLocal::Add(pointer field) {
    pointer prev = history[(hist_next - 1) % CON_HISTORY];

    // don't add "", "\" or "/"
    if(!field[0] || ((field[0] == '/' || field[0] == '\\') && !field[1])) {
        hist_current = hist_next;
        return;
    }

    // don't add if same as previous (treat leading \ and / as equivalent)
    if((*field == *prev || (*field == '/' && *prev == '\\') ||
            (*field == '\\' && *prev == '/')) && !::strcmp(&field[1], &prev[1])) {
        hist_current = hist_next;
        return;
    }

    Q_strncpyz(history[hist_next % CON_HISTORY], field, sizeof(history[0]));

    hist_next++;
    hist_current = hist_next;
    Save();
}

/*
==================
idConsoleHistoryLocal::Prev
==================
*/
pointer idConsoleHistoryLocal::Prev(void) {
    if((hist_current - 1) % CON_HISTORY != hist_next % CON_HISTORY &&
            history[(hist_current - 1) % CON_HISTORY][0]) {
        hist_current--;
    }

    return history[hist_current % CON_HISTORY];
}

/*
==================
idConsoleHistoryLocal::Next
==================
*/
pointer idConsoleHistoryLocal::Next(pointer field) {
    if(hist_current % CON_HISTORY != hist_next % CON_HISTORY) {
        hist_current++;
    }

    if(hist_current % CON_HISTORY == hist_next % CON_HISTORY) {
        if(*field && strcmp(field, history[(hist_current - 1) % CON_HISTORY])) {
            Add(field);
        }

        return "";
    }

    return history[hist_current % CON_HISTORY];
}
