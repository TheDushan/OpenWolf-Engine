////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2008 Amanieu d'Antras
// Copyright(C) 2011 - 2021 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   consoleCurses.cpp
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

idConsoleCursesLocal consoleCursesLocal;
idConsoleCursesSystem *consoleCursesSystem = &consoleCursesLocal;

/*
===============
idConsoleCursesLocal::idConsoleCursesLocal
===============
*/
idConsoleCursesLocal::idConsoleCursesLocal(void) {
}

/*
===============
idConsoleCursesLocal::~idConsoleCursesLocal
===============
*/
idConsoleCursesLocal::~idConsoleCursesLocal(void) {
}

/*
==================
idConsoleCursesLocal::SetColor

Use grey instead of black
==================
*/
void idConsoleCursesLocal::SetColor(WINDOW *win, sint color) {
    if(com_ansiColor && !com_ansiColor->integer) {
        color = 7;
    }

    if(color == 0) {
        wattrset(win, COLOR_PAIR(color + 1) | A_BOLD);
    } else {
        wattrset(win, COLOR_PAIR(color + 1) | A_NORMAL);
    }
}

/*
==================
idConsoleCursesLocal::UpdateCursor

Update the cursor position
==================
*/
void idConsoleCursesLocal::UpdateCursor(void) {
    // pdcurses uses a different mechanism to move the cursor than ncurses
#ifdef _WIN32
    move(LINES - 1, Q_PrintStrlen(PROMPT) + 8 + input_field.cursor -
         input_field.scroll);
    wnoutrefresh(stdscr);
#else
    wmove(inputwin, 0, input_field.cursor - input_field.scroll);
    wnoutrefresh(inputwin);
#endif
}

/*
==================
idConsoleCursesLocal::DrawScrollBar
==================
*/
void idConsoleCursesLocal::DrawScrollBar(void) {
    sint scroll;

    if(lastline <= LOG_LINES) {
        scroll = 0;
    } else {
        scroll = scrollline * (LOG_LINES - 1) / (lastline - LOG_LINES);
    }

    if(com_ansiColor && !com_ansiColor->integer) {
        wbkgdset(scrollwin, SCRLBAR_LINE);
    } else {
        wbkgdset(scrollwin, SCRLBAR_LINE | COLOR_PAIR(6));
    }

    werase(scrollwin);
    wbkgdset(scrollwin, ' ');

    SetColor(scrollwin, 1);

    mvwaddch(scrollwin, scroll, 0, SCRLBAR_CURSOR);
    wnoutrefresh(scrollwin);
}

/*
==================
idConsoleCursesLocal::ColorPrint
==================
*/
void idConsoleCursesLocal::ColorPrint(WINDOW *win, pointer msg,
                                      bool stripcodes) {
    static valueType buffer[MAXPRINTMSG];
    sint length = 0;

    SetColor(win, 7);

    while(*msg) {
        if(Q_IsColorString(msg) || *msg == '\n') {
            // First empty the buffer
            if(length > 0) {
                buffer[length] = '\0';
                wprintw(win, "%s", buffer);
                length = 0;
            }

            if(*msg == '\n') {
                // Reset the color and then print a newline
                SetColor(win, 7);
                waddch(win, '\n');
                msg++;
            } else {
                // Set the color
                SetColor(win, ColorIndex(*(msg + 1)));

                if(stripcodes) {
                    msg += 2;
                } else {
                    if(length >= MAXPRINTMSG - 1) {
                        break;
                    }

                    buffer[length] = *msg;
                    length++;
                    msg++;

                    if(length >= MAXPRINTMSG - 1) {
                        break;
                    }

                    buffer[length] = *msg;
                    length++;
                    msg++;
                }
            }
        } else {
            if(length >= MAXPRINTMSG - 1) {
                break;
            }

            buffer[length] = *msg;
            length++;
            msg++;
        }
    }

    // Empty anything still left in the buffer
    if(length > 0) {
        buffer[length] = '\0';
        wprintw(win, "%s", buffer);
    }
}

/*
==================
idConsoleCursesLocal::UpdateClock

Update the clock
==================
*/
void idConsoleCursesLocal::UpdateClock(void) {
    qtime_t realtime;

    Com_RealTime(&realtime);

    werase(clockwin);

    ColorPrint(clockwin, va("^0[^3%02d%c%02d^0] ", realtime.tm_hour,
                            (realtime.tm_sec & 1) ? ':' : ' ', realtime.tm_min), true);

    wnoutrefresh(clockwin);
}

/*
==================
idConsoleCursesLocal::Resize

The window has just been resized, move everything back into place
==================
*/
void idConsoleCursesLocal::Resize(void) {
#ifndef _WIN32
    struct winsize winsz = { 0, };

    ioctl(fileno(stdout), TIOCGWINSZ, &winsz);

    if(winsz.ws_col < 12 || winsz.ws_row < 5) {
        return;
    }

    resizeterm(winsz.ws_row + 1, winsz.ws_col + 1);
    resizeterm(winsz.ws_row, winsz.ws_col);

    delwin(logwin);
    delwin(borderwin);
    delwin(inputwin);
    delwin(scrollwin);

    erase();

    wnoutrefresh(stdscr);

    consoleCursesLocal.Init();
#endif
}

/*
==================
idConsoleCursesLocal::Clear_f
==================
*/
void idConsoleCursesLocal::Clear_f(void) {
    if(!curses_on) {
        Clear_f();
        return;
    }

    // Clear the log and the window
    ::memset(logbuf, 0, sizeof(logbuf));
    werase(logwin);
    pnoutrefresh(logwin, scrollline, 0, 2, 1, LOG_LINES + 1, LOG_COLS + 1);

    // Move the cursor back to the input field
    UpdateCursor();
    doupdate();
}

/*
==================
idConsoleCursesLocal::Shutdown

Never exit without calling this, or your terminal will be left in a pretty bad state
==================
*/
void idConsoleCursesLocal::Shutdown(void) {
    if(!curses_on) {
        Shutdown();
        return;
    }

    endwin();
}

/*
==================
idConsoleCursesLocal::Init

Initialize the console in curses mode, fall back to tty mode on failure
==================
*/
void idConsoleCursesLocal::Init(void) {
    sint col;

#ifndef _WIN32
    // If the process is backgrounded (running non interactively)
    // then SIGTTIN or SIGTOU is emitted, if not caught, turns into a SIGSTP
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
#endif

    // Initialize curses and set up the root window
    if(!curses_on) {
        SCREEN *test = newterm(nullptr, stdout, stdin);

        if(!test) {
            Init();
            Print("Couldn't initialize curses, falling back to tty\n");
            return;
        }

        endwin();
        delscreen(test);
        initscr();
        cbreak();
        noecho();
        nonl();

        intrflush(stdscr, FALSE);
        nodelay(stdscr, TRUE);
        keypad(stdscr, TRUE);
        wnoutrefresh(stdscr);

        // Set up colors
        if(has_colors()) {
            use_default_colors();
            start_color();

            init_pair(1, COLOR_BLACK, -1);
            init_pair(2, COLOR_RED, -1);
            init_pair(3, COLOR_GREEN, -1);
            init_pair(4, COLOR_YELLOW, -1);
            init_pair(5, COLOR_BLUE, -1);
            init_pair(6, COLOR_CYAN, -1);
            init_pair(7, COLOR_MAGENTA, -1);
            init_pair(8, -1, -1);
        }

        // Prevent bad libraries from messing up the console
        fclose(stderr);
    }

    // Create the border
    borderwin = newwin(LOG_LINES + 2, LOG_COLS + 2, 1, 0);
    SetColor(borderwin, 2);
    box(borderwin, 0, 0);
    wnoutrefresh(borderwin);

    // Create the log window
    logwin = newpad(MAX_LOG_LINES, LOG_COLS);
    scrollok(logwin, TRUE);
    idlok(logwin, TRUE);

    if(curses_on) {
        ColorPrint(logwin, logbuf, true);
    }

    getyx(logwin, lastline, col);

    if(col) {
        lastline++;
    }

    scrollline = lastline - LOG_LINES;

    if(scrollline < 0) {
        scrollline = 0;
    }

    pnoutrefresh(logwin, scrollline, 0, 2, 1, LOG_LINES + 1, LOG_COLS + 1);

    // Create the scroll bar
    scrollwin = newwin(LOG_LINES, 1, 2, COLS - 1);

    DrawScrollBar();
    SetColor(stdscr, 3);

    mvaddch(1, COLS - 1, SCRLBAR_UP);
    mvaddch(LINES - 2, COLS - 1, SCRLBAR_DOWN);

    // Create the input field
    inputwin = newwin(1, COLS - Q_PrintStrlen(PROMPT) - 8, LINES - 1,
                      Q_PrintStrlen(PROMPT) + 8);
    input_field.widthInChars = COLS - Q_PrintStrlen(PROMPT) - 9;

    if(curses_on) {
        if(input_field.cursor < input_field.scroll) {
            input_field.scroll = input_field.cursor;
        } else if(input_field.cursor >= input_field.scroll +
                  input_field.widthInChars) {
            input_field.scroll = input_field.cursor - input_field.widthInChars + 1;
        }

        ColorPrint(inputwin, input_field.buffer + input_field.scroll, false);
    }

    UpdateCursor();
    wnoutrefresh(inputwin);

    // Create the clock
    clockwin = newwin(1, 8, LINES - 1, 0);
    UpdateClock();

    // Display the title and input prompt
    move(0, (COLS - Q_PrintStrlen(TITLE)) / 2);

    ColorPrint(stdscr, TITLE, true);

    move(LINES - 1, 8);

    ColorPrint(stdscr, PROMPT, true);

    wnoutrefresh(stdscr);
    doupdate();

#ifndef _WIN32
    // Catch window resizes
    signal(SIGWINCH, static_cast<void *>(reinterpret_cast<void *&>(Resize)));
#endif

    curses_on = true;
}

/*
==================
idConsoleCursesLocal::Input
==================
*/
valueType *idConsoleCursesLocal::Input(void) {
    sint chr, num_chars = 0;
    static valueType text[MAX_EDIT_LINE];
    static sint lasttime = -1;

    if(!curses_on) {
        return Input();
    }

    if(com_ansiColor->modified) {
        Resize();
        com_ansiColor->modified = false;
    }

    if(Com_RealTime(nullptr) != lasttime) {
        lasttime = Com_RealTime(nullptr);
        UpdateClock();
        num_chars++;
    }

    while(1) {
        chr = getch();
        num_chars++;

        // Special characters
        switch(chr) {
            case ERR:
                if(num_chars > 1) {
                    werase(inputwin);

                    if(input_field.cursor < input_field.scroll) {
                        input_field.scroll = input_field.cursor - INPUT_SCROLL;

                        if(input_field.scroll < 0) {
                            input_field.scroll = 0;
                        }
                    } else if(input_field.cursor >= input_field.scroll +
                              input_field.widthInChars) {
                        input_field.scroll = input_field.cursor - input_field.widthInChars +
                                             INPUT_SCROLL;
                    }

                    ColorPrint(inputwin, input_field.buffer + input_field.scroll, false);
#ifdef _WIN32
                    wrefresh(inputwin);   // If this is not done the cursor moves strangely
#else
                    wnoutrefresh(inputwin);
#endif
                    UpdateCursor();
                    doupdate();
                }

                return nullptr;

            case '\n':
            case '\r':
            case KEY_ENTER:
                if(!input_field.buffer[0]) {
                    continue;
                }

                consoleHistorySystem->Add(input_field.buffer);
                Q_strcpy_s(text, input_field.buffer);
                cmdCompletionSystem->Clear(&input_field);
                werase(inputwin);
                wnoutrefresh(inputwin);
                UpdateCursor();
                //doupdate();
                Com_Printf(PROMPT "^7%s\n", text);
                return text;

            case 21: // Ctrl-U
                cmdCompletionSystem->Clear(&input_field);
                werase(inputwin);
                wnoutrefresh(inputwin);
                UpdateCursor();
                continue;

            case '\t':
            case KEY_STAB:
                cmdCompletionSystem->AutoComplete(&input_field, PROMPT);
                input_field.cursor = strlen(input_field.buffer);
                continue;

            case '\f':
                Resize();
                continue;

            case KEY_LEFT:
                if(input_field.cursor > 0) {
                    input_field.cursor--;
                }

                continue;

            case KEY_RIGHT:
                if(input_field.cursor < strlen(input_field.buffer)) {
                    input_field.cursor++;
                }

                continue;

            case KEY_UP:
                Q_strncpyz(input_field.buffer, consoleHistorySystem->Prev(),
                           sizeof(input_field.buffer));
                input_field.cursor = strlen(input_field.buffer);
                continue;

            case KEY_DOWN:

                Q_strncpyz(input_field.buffer,
                           consoleHistorySystem->Next(input_field.buffer),
                           sizeof(input_field.buffer));
                input_field.cursor = strlen(input_field.buffer);
                continue;

            case KEY_HOME:
                input_field.cursor = 0;
                continue;

            case KEY_END:
                input_field.cursor = strlen(input_field.buffer);
                continue;

            case KEY_NPAGE:
                if(lastline > scrollline + LOG_LINES) {
                    scrollline += LOG_SCROLL;

                    if(scrollline + LOG_LINES > lastline) {
                        scrollline = lastline - LOG_LINES;
                    }

                    pnoutrefresh(logwin, scrollline, 0, 2, 1, LOG_LINES + 1, LOG_COLS + 1);
                    DrawScrollBar();
                }

                continue;

            case KEY_PPAGE:
                if(scrollline > 0) {
                    scrollline -= LOG_SCROLL;

                    if(scrollline < 0) {
                        scrollline = 0;
                    }

                    pnoutrefresh(logwin, scrollline, 0, 2, 1, LOG_LINES + 1, LOG_COLS + 1);
                    DrawScrollBar();
                }

                continue;

            case '\b':
            case 127:
            case KEY_BACKSPACE:
                if(input_field.cursor <= 0) {
                    continue;
                }

                input_field.cursor--;

            // Fall through
            case KEY_DC:
                if(input_field.cursor < strlen(input_field.buffer)) {
                    ::memmove(input_field.buffer + input_field.cursor,
                              input_field.buffer + input_field.cursor + 1,
                              strlen(input_field.buffer) - input_field.cursor);
                }

                continue;
        }

        // Normal characters
        if(chr >= ' ' && chr < 256 &&
                strlen(input_field.buffer) + 1 < sizeof(input_field.buffer)) {
            ::memmove(input_field.buffer + input_field.cursor + 1,
                      input_field.buffer + input_field.cursor,
                      strlen(input_field.buffer) - input_field.cursor);
            input_field.buffer[input_field.cursor] = chr;
            input_field.cursor++;
        }
    }
}

/*
==================
idConsoleCursesLocal::Print
==================
*/
void idConsoleCursesLocal::Print(pointer msg) {
    sint col;
    bool scroll = (lastline > scrollline &&
                   lastline <= scrollline + LOG_LINES);

    if(!curses_on) {
        Print(msg);
        return;
    }

    // Print the message in the log window
    ColorPrint(logwin, msg, true);
    getyx(logwin, lastline, col);

    if(col) {
        lastline++;
    }

    if(scroll) {
        scrollline = lastline - LOG_LINES;

        if(scrollline < 0) {
            scrollline = 0;
        }

        pnoutrefresh(logwin, scrollline, 0, 2, 1, LOG_LINES + 1, LOG_COLS + 1);
    }

    // Add the message to the log buffer
    if(insert + strlen(msg) >= logbuf + sizeof(logbuf)) {
        ::memmove(logbuf, logbuf + sizeof(logbuf) / 2, sizeof(logbuf) / 2);
        ::memset(logbuf + sizeof(logbuf) / 2, 0, sizeof(logbuf) / 2);

        insert -= sizeof(logbuf) / 2;
    }

    ::strcpy(insert, msg);
    insert += strlen(msg);

    // Update the scrollbar
    DrawScrollBar();

    // Move the cursor back to the input field
    UpdateCursor();
    doupdate();
}
