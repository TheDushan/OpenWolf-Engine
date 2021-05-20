////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2010 id Software LLC, a ZeniMax Media company.
// Copyright(C) 2011 - 2021 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   commandLineCompletion.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
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

idCmdCompletionLocal cmdCompletionLocal;
idCmdCompletionSystem *cmdCompletionSystem = &cmdCompletionLocal;

pointer completionString;

/*
===============
idCmdCompletionLocal::idCmdCompletionLocal
===============
*/
idCmdCompletionLocal::idCmdCompletionLocal(void) {
}

/*
===============
idCmdCompletionLocal::~idCmdCompletionLocal
===============
*/
idCmdCompletionLocal::~idCmdCompletionLocal(void) {
}

/*
==================
idCmdCompletionLocal::Clear
==================
*/
void idCmdCompletionLocal::Clear(field_t *edit) {
    ::memset(edit->buffer, 0, MAX_EDIT_LINE);

    edit->cursor = 0;
    edit->scroll = 0;
}

/*
==================
idCmdCompletionLocal::Set
==================
*/
void idCmdCompletionLocal::Set(field_t *edit, pointer content) {
    ::memset(edit->buffer, 0, MAX_EDIT_LINE);
    ::strncpy(edit->buffer, content, MAX_EDIT_LINE);

    edit->cursor = ::strlen(edit->buffer);

    if(edit->cursor > edit->widthInChars) {
        edit->scroll = edit->cursor - edit->widthInChars;
    } else {
        edit->scroll = 0;
    }
}

/*
==================
idCmdCompletionLocal::WordDelete
==================
*/
void idCmdCompletionLocal::WordDelete(field_t *edit) {
    while(edit->cursor) {
        if(edit->buffer[edit->cursor - 1] != ' ') {
            edit->buffer[edit->cursor - 1] = 0;
            edit->cursor--;
        } else {
            edit->cursor--;

            if(edit->buffer[edit->cursor - 1] != ' ') {
                return;
            }
        }
    }
}

/*
===============
idCmdCompletionLocal::FindMatches
===============
*/
void idCmdCompletionLocal::FindMatches(pointer s) {
    sint i;

    if(Q_stricmpn(s, completionString, ::strlen(completionString))) {
        return;
    }

    matchCount++;

    if(matchCount == 1) {
        Q_strncpyz(shortestMatch, s, sizeof(shortestMatch));

        return;
    }

    // cut shortestMatch to the amount common with s
    // was wrong when s had fewer chars than shortestMatch
    i = 0;

    do {
        if(::tolower(shortestMatch[i]) != ::tolower(s[i])) {
            shortestMatch[i] = 0;
        }
    } while(s[i++]);
}


/*
===============
idCmdCompletionLocal::PrintMatches
===============
*/
void idCmdCompletionLocal::PrintMatches(pointer s) {
    if(!Q_stricmpn(s, shortestMatch, ::strlen(shortestMatch))) {
        Com_Printf("    %s\n", s);
    }
}

/*
===============
idCmdCompletionLocal::PrintCvarMatches
===============
*/
void idCmdCompletionLocal::PrintCvarMatches(pointer s) {
    if(!Q_stricmpn(s, shortestMatch, ::strlen(shortestMatch))) {
        Com_Printf("    %-32s ^7\"%s^7\"\n", s, cvarSystem->VariableString(s));
    }
}

/*
===============
idCmdCompletionLocal::FindFirstSeparator
===============
*/
valueType *idCmdCompletionLocal::FindFirstSeparator(valueType *s) {
    sint i;

    for(i = 0; i < ::strlen(s); i++) {
        if(s[i] == ';') {
            return &s[i];
        }
    }

    return nullptr;
}

/*
===============
idCmdCompletionLocal::Complete
===============
*/
bool idCmdCompletionLocal::Complete(void) {
    sint64 completionOffset;

    if(matchCount == 0) {
        return true;
    }

    completionOffset = ::strlen(completionField->buffer) - ::strlen(
                           completionString);

    Q_strncpyz(&completionField->buffer[completionOffset], shortestMatch,
               sizeof(completionField->buffer) - completionOffset);

    completionField->cursor = ::strlen(completionField->buffer);

    if(matchCount == 1) {
        Q_strcat(completionField->buffer, sizeof(completionField->buffer), " ");
        completionField->cursor++;
        return true;
    }

    Com_Printf("%s^7%s\n", completionPrompt, completionField->buffer);

    return false;
}

/*
===============
idCmdCompletionLocal::CompleteKeyname
===============
*/
void idCmdCompletionLocal::CompleteKeyname(void) {
#ifndef DEDICATED
    matchCount = 0;
    shortestMatch[0] = 0;

    clientKeysSystem->KeynameCompletion(FindMatches);

    if(!Complete()) {
        clientKeysSystem->KeynameCompletion(PrintMatches);
    }

#endif
}

/*
===============
idCmdCompletionLocal::CompleteCgame
===============
*/
void idCmdCompletionLocal::CompleteCgame(sint argNum) {
#ifndef DEDICATED
    matchCount = 0;
    shortestMatch[0] = 0;

    clientGameSystem->CgameCompletion(FindMatches, argNum);

    if(!Complete()) {
        clientGameSystem->CgameCompletion(PrintMatches, argNum);
    }

#endif
}

/*
===============
idCmdCompletionLocal::CompleteFilename
===============
*/
void idCmdCompletionLocal::CompleteFilename(pointer dir, pointer ext,
        bool stripExt) {
    matchCount = 0;
    shortestMatch[0] = 0;

    fileSystem->FilenameCompletion(dir, ext, stripExt, FindMatches);

    if(!Complete()) {
        fileSystem->FilenameCompletion(dir, ext, stripExt, PrintMatches);
    }
}

/*
===============
idCmdCompletionLocal::CompleteAlias
===============
*/
void idCmdCompletionLocal::CompleteAlias(void) {
    matchCount = 0;
    shortestMatch[0] = 0;

    cmdSystem->AliasCompletion(FindMatches);

    if(!Complete()) {
        cmdSystem->AliasCompletion(PrintMatches);
    }
}

/*
===============
idCmdCompletionLocal::CompleteDelay
===============
*/
void idCmdCompletionLocal::CompleteDelay(void) {
    matchCount = 0;
    shortestMatch[0] = 0;

    cmdSystem->DelayCompletion(FindMatches);

    if(!Complete()) {
        cmdSystem->DelayCompletion(PrintMatches);
    }
}

/*
===============
idCmdCompletionLocal::CompleteCommand
===============
*/
void idCmdCompletionLocal::CompleteCommand(valueType *cmd, bool doCommands,
        bool doCvars) {
    sint completionArgument = 0;

    // Skip leading whitespace and quotes
    cmd = Com_SkipCharset(cmd, " \"");

    cmdSystem->TokenizeStringIgnoreQuotes(cmd);
    completionArgument = cmdSystem->Argc();

    // If there is trailing whitespace on the cmd
    if(*(cmd + ::strlen(cmd) - 1) == ' ') {
        completionString = "";
        completionArgument++;
    } else {
        completionString = cmdSystem->Argv(completionArgument - 1);
    }

    if(completionString == nullptr) {
        return;
    }

#ifndef DEDICATED

    // Unconditionally add a '\' to the start of the buffer
    if(completionField->buffer[0] && completionField->buffer[0] != '\\') {
        if(completionField->buffer[0] != '/') {
            // Buffer is full, refuse to complete
            if(::strlen(completionField->buffer) + 1 >= sizeof(
                        completionField->buffer)) {
                return;
            }

            ::memmove(&completionField->buffer[1], &completionField->buffer[0],
                      ::strlen(completionField->buffer) + 1);

            completionField->cursor++;
        }

        completionField->buffer[0] = '\\';
    }

#endif

    if(completionArgument > 1) {
        pointer baseCmd = cmdSystem->Argv(0);
        valueType *p;

#ifndef DEDICATED

        // This should always be true
        if(baseCmd[0] == '\\' || baseCmd[0] == '/') {
            baseCmd++;
        }

#endif

        if((p = FindFirstSeparator(cmd))) {
            // Compound command
            CompleteCommand(p + 1, true, true);
        } else {
            cmdSystem->CompleteArgument(baseCmd, cmd, completionArgument);
        }
    } else {
        if(completionString[0] == '\\' || completionString[0] == '/') {
            completionString++;
        }

        matchCount = 0;
        shortestMatch[0] = 0;

        if(::strlen(completionString) == 0) {
            return;
        }

        if(doCommands) {
            cmdSystem->CommandCompletion(FindMatches);
        }

        if(doCvars) {
            cvarSystem->CommandCompletion(FindMatches);
        }

        if(!Complete()) {
            // run through again, printing matches
            if(doCommands) {
                cmdSystem->CommandCompletion(PrintMatches);
            }

            if(doCvars) {
                cvarSystem->CommandCompletion(PrintCvarMatches);
            }
        }
    }
}

/*
===============
idCmdCompletionLocal::AutoComplete

Perform Tab expansion
===============
*/
void idCmdCompletionLocal::AutoComplete(field_t *field, pointer prompt) {
    completionField = field;
    completionPrompt = prompt;

    CompleteCommand(completionField->buffer, true, true);
}

/*
===================
idCmdCompletionLocal::Draw

Handles horizontal scrolling and cursor blinking
x, y, and width are in pixels
===================
*/
void idCmdCompletionLocal::VariableSizeDraw(field_t *edit, sint x, sint y,
        sint size, bool showCursor, bool noColorEscape, float32 alpha) {
#ifndef DEDICATED
    uint64 i, len, drawLen, prestep;
    valueType cursorChar, str[MAX_STRING_CHARS];

    // - 1 so there is always a space for the cursor
    drawLen = edit->widthInChars - 1;

    len = ::strlen(edit->buffer);

    // guarantee that cursor will be visible
    if(len <= drawLen) {
        prestep = 0;
    } else {
        if(edit->scroll + drawLen > len) {
            edit->scroll = len - drawLen;

            if(edit->scroll < 0) {
                edit->scroll = 0;
            }
        }

        prestep = edit->scroll;
    }

    if(prestep + drawLen > len) {
        drawLen = len - prestep;
    }

    // extract <drawLen> characters from the field at <prestep>
    if(drawLen >= MAX_STRING_CHARS) {
        Com_Error(ERR_DROP, "drawLen >= MAX_STRING_CHARS");
    }

    ::memcpy(str, edit->buffer + prestep, drawLen);
    str[drawLen] = 0;

    // draw it
    if(size == SMALLCHAR_WIDTH) {
        float32 color[4];

        color[0] = color[1] = color[2] = 1.0;
        color[3] = alpha;

        clientScreenSystem->DrawSmallStringExt(x, y, str, color, false,
                                               noColorEscape);
    } else {
        // draw big string with drop shadow
        clientScreenSystem->DrawBigString(x, y, str, 1.0, noColorEscape);
    }

    // draw the cursor
    if(showCursor) {
        if(static_cast<sint>(cls.realtime >> 8) & 1) {
            // off blink
            return;
        }

        if(key_overstrikeMode) {
            cursorChar = 11;
        } else {
            cursorChar = 10;
        }

        i = drawLen - strlen(str);

        if(size == SMALLCHAR_WIDTH) {
            float32 xlocation = x + clientScreenSystem->ConsoleFontStringWidth(
                                    str + prestep, edit->cursor - prestep);

            clientScreenSystem->DrawConsoleFontChar(xlocation, y, cursorChar);
        } else {
            str[0] = cursorChar;
            str[1] = 0;
            clientScreenSystem->DrawBigString(x + static_cast<sint>
                                              (edit->cursor - prestep - i) * size, y, str, 1.0f, false);

        }
    }

#endif
}

/*
==================
idCmdCompletionLocal::Draw
==================
*/
void idCmdCompletionLocal::Draw(field_t *edit, sint x, sint y,
                                bool showCursor, bool noColorEscape, float32 alpha) {
    VariableSizeDraw(edit, x, y, SMALLCHAR_WIDTH, showCursor, noColorEscape,
                     alpha);
}

/*
==================
idCmdCompletionLocal::BigDraw
==================
*/
void idCmdCompletionLocal::BigDraw(field_t *edit, sint x, sint y,
                                   bool showCursor, bool noColorEscape) {
    VariableSizeDraw(edit, x, y, BIGCHAR_WIDTH, showCursor, noColorEscape,
                     1.0f);
}

/*
================
idCmdCompletionLocal::Paste
================
*/
void idCmdCompletionLocal::Paste(field_t *edit) {
    valueType *pasteBuffer;
    uint64 pasteLen, i;

    pasteBuffer = idsystem->SysGetClipboardData();

    if(!pasteBuffer) {
        return;
    }

    // send as if typed, so insert / overstrike works properly
    pasteLen = ::strlen(pasteBuffer);

    for(i = 0; i < pasteLen; i++) {
        cmdCompletionSystem->CharEvent(edit, pasteBuffer[i]);
    }
}

/*
=================
idCmdCompletionLocal::KeyDownEvent

Performs the basic line editing functions for the console,
in-game talk, and menu fields

Key events are used for non-printable characters, others are gotten from char events.
=================
*/
void idCmdCompletionLocal::KeyDownEvent(field_t *edit, sint key) {
#ifndef DEDICATED
    uint64 len;

    // shift-insert is paste
    if(((key == K_INS) || (key == K_KP_INS)) && keys[K_SHIFT].down) {
        Paste(edit);
        return;
    }

    len = ::strlen(edit->buffer);

    switch(key) {
        case K_DEL:
        case K_KP_DEL:
            if(edit->cursor < len) {
                ::memmove(edit->buffer + edit->cursor, edit->buffer + edit->cursor + 1,
                          len - edit->cursor);
            }

            break;

        case K_RIGHTARROW:
        case K_KP_RIGHTARROW:
            if(edit->cursor < len) {
                edit->cursor++;
            }

            break;

        case K_LEFTARROW:
        case K_KP_LEFTARROW:
            if(edit->cursor > 0) {
                edit->cursor--;
            }

            break;

        case K_HOME:
        case K_KP_HOME:
            edit->cursor = 0;
            break;

        case 'a':
            if(keys[K_CTRL].down) {
                edit->cursor = 0;
            }

            break;

        case K_END:
        case K_KP_END:
            edit->cursor = len;
            break;

        case 'e':
            if(keys[K_CTRL].down) {
                edit->cursor = len;
            }

            break;

        case K_INS:
        case K_KP_INS:
            key_overstrikeMode = (bool)!key_overstrikeMode;
            break;
    }

    // Change scroll if cursor is no longer visible
    if(edit->cursor < edit->scroll) {
        edit->scroll = edit->cursor;
    } else if(edit->cursor >= edit->scroll + edit->widthInChars &&
              edit->cursor <= len) {
        edit->scroll = edit->cursor - edit->widthInChars + 1;
    }

#endif
}

/*
==================
idCmdCompletionLocal::CharEvent
==================
*/
void idCmdCompletionLocal::CharEvent(field_t *edit, valueType ch) {
#ifndef DEDICATED
    sint len;

    // ctrl-v is paste
    if(ch == 'v' - 'a' + 1) {
        Paste(edit);
        return;
    }

    // ctrl-c or ctrl-u clear the field
    if(ch == 'c' - 'a' + 1 || ch == 'u' - 'a' + 1) {
        Clear(edit);
        return;
    }

    len = ::strlen(edit->buffer);

    // ctrl-h is backspace
    if(ch == 'h' - 'a' + 1) {
        if(edit->cursor > 0) {
            ::memmove(edit->buffer + edit->cursor - 1, edit->buffer + edit->cursor,
                      len + 1 - edit->cursor);

            edit->cursor--;

            if(edit->cursor < edit->scroll) {
                edit->scroll--;
            }
        }

        return;
    }

    // ctrl-a is home
    if(ch == 'a' - 'a' + 1) {
        edit->cursor = 0;
        edit->scroll = 0;
        return;
    }

    // ctrl-e is end
    if(ch == 'e' - 'a' + 1) {
        edit->cursor = len;
        edit->scroll = edit->cursor - edit->widthInChars;

        return;
    }

    // ignore any other non printable chars
    if(ch < 32 || ch == 0x7f) {
        return;
    }

    if(key_overstrikeMode) {
        // - 2 to leave room for the leading slash and trailing \0
        if(edit->cursor == MAX_EDIT_LINE - 2) {
            return;
        }

        edit->buffer[edit->cursor] = ch;
        edit->cursor++;
    }
    // insert mode
    else {
        // - 2 to leave room for the leading slash and trailing \0
        if(len == MAX_EDIT_LINE - 2) {
            // all full
            return;
        }

        ::memmove(edit->buffer + edit->cursor + 1, edit->buffer + edit->cursor,
                  len + 1 - edit->cursor);

        edit->buffer[edit->cursor] = ch;
        edit->cursor++;
    }


    if(edit->cursor >= edit->widthInChars) {
        edit->scroll++;
    }

    if(edit->cursor == len + 1) {
        edit->buffer[edit->cursor] = 0;
    }

#endif
}