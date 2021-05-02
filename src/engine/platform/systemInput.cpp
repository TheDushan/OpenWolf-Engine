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
// File name:   systemInput.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <framework/precompiled.hpp>

static SDL_Joystick *stick = nullptr;

static bool mouseAvailable = false;
static bool mouseActive = false;

static sint vidRestartTime = 0;

static SDL_Window *SDL_window = nullptr;

/*
===============
idSystemLocal::PrintKey
===============
*/
void idSystemLocal::PrintKey(const SDL_Keysym *keysym, keyNum_t key,
                             bool down) {
    if(down) {
        Com_Printf("+ ");
    } else {
        Com_Printf("  ");
    }

    Com_Printf("Scancode: 0x%02x(%s) Sym: 0x%02x(%s)", keysym->scancode,
               SDL_GetScancodeName(keysym->scancode), keysym->sym,
               SDL_GetKeyName(keysym->sym));

    if(keysym->mod & KMOD_LSHIFT) {
        Com_Printf(" KMOD_LSHIFT");
    }

    if(keysym->mod & KMOD_RSHIFT) {
        Com_Printf(" KMOD_RSHIFT");
    }

    if(keysym->mod & KMOD_LCTRL) {
        Com_Printf(" KMOD_LCTRL");
    }

    if(keysym->mod & KMOD_RCTRL) {
        Com_Printf(" KMOD_RCTRL");
    }

    if(keysym->mod & KMOD_LALT) {
        Com_Printf(" KMOD_LALT");
    }

    if(keysym->mod & KMOD_RALT) {
        Com_Printf(" KMOD_RALT");
    }

    if(keysym->mod & KMOD_LGUI) {
        Com_Printf(" KMOD_LGUI");
    }

    if(keysym->mod & KMOD_RGUI) {
        Com_Printf(" KMOD_RGUI");
    }

    if(keysym->mod & KMOD_NUM) {
        Com_Printf(" KMOD_NUM");
    }

    if(keysym->mod & KMOD_CAPS) {
        Com_Printf(" KMOD_CAPS");
    }

    if(keysym->mod & KMOD_MODE) {
        Com_Printf(" KMOD_MODE");
    }

    if(keysym->mod & KMOD_RESERVED) {
        Com_Printf(" KMOD_RESERVED");
    }

    Com_Printf(" Q:0x%02x(%s)\n", key, Key_KeynumToString(key));
}

/*
===============
idSystemLocal::IsConsoleKey

TODO: If the SDL_Scancode situation improves, use it instead of
both of these methods
===============
*/
bool idSystemLocal::IsConsoleKey(keyNum_t key, sint character) {
    typedef struct consoleKey_s {
        keyType_t type;

        union {
            keyNum_t key;
            sint character;
        } u;
    } consoleKey_t;

    static consoleKey_t consoleKeys[MAX_CONSOLE_KEYS];
    static sint numConsoleKeys = 0;
    sint i;

    // Only parse the variable when it changes
    if(cl_consoleKeys->modified) {
        valueType *text_p, *token;

        cl_consoleKeys->modified = false;
        text_p = cl_consoleKeys->string;
        numConsoleKeys = 0;

        while(numConsoleKeys < MAX_CONSOLE_KEYS) {
            consoleKey_t *c = &consoleKeys[numConsoleKeys];
            sint charCode = 0;

            token = COM_Parse(&text_p);

            if(!token[0]) {
                break;
            }

            if(strlen(token) == 4) {
                charCode = Com_HexStrToInt(token);
            }

            if(charCode > 0) {
                c->type = CHARACTER;
                c->u.character = charCode;
            } else {
                c->type = QUAKE_KEY;
                c->u.key = (keyNum_t)Key_StringToKeynum(token);

                // 0 isn't a key
                if(c->u.key <= 0) {
                    continue;
                }
            }

            numConsoleKeys++;
        }
    }

    // If the character is the same as the key, prefer the character
    if(key == character) {
        key = static_cast<keyNum_t>(0);
    }

    for(i = 0; i < numConsoleKeys; i++) {
        consoleKey_t *c = &consoleKeys[i];

        switch(c->type) {
            case QUAKE_KEY:
                if(key && c->u.key == key) {
                    return true;
                }

                break;

            case CHARACTER:
                if(c->u.character == character) {
                    return true;
                }

                break;
        }
    }

    return false;
}

/*
===============
idSystemLocal::TranslateSDLToQ3Key
===============
*/
keyNum_t idSystemLocal::TranslateSDLToQ3Key(SDL_Keysym *keysym,
        bool down) {
    keyNum_t key = static_cast<keyNum_t>(0);

    if(keysym->sym >= SDLK_SPACE && keysym->sym < SDLK_DELETE) {
        // These happen to match the ASCII chars
        key = static_cast< keyNum_t >(keysym->sym);
    } else {
        switch(keysym->sym) {
            case SDLK_PAGEUP:
                key = K_PGUP;
                break;

            case SDLK_KP_9:
                key = K_KP_PGUP;
                break;

            case SDLK_PAGEDOWN:
                key = K_PGDN;
                break;

            case SDLK_KP_3:
                key = K_KP_PGDN;
                break;

            case SDLK_KP_7:
                key = K_KP_HOME;
                break;

            case SDLK_HOME:
                key = K_HOME;
                break;

            case SDLK_KP_1:
                key = K_KP_END;
                break;

            case SDLK_END:
                key = K_END;
                break;

            case SDLK_KP_4:
                key = K_KP_LEFTARROW;
                break;

            case SDLK_LEFT:
                key = K_LEFTARROW;
                break;

            case SDLK_KP_6:
                key = K_KP_RIGHTARROW;
                break;

            case SDLK_RIGHT:
                key = K_RIGHTARROW;
                break;

            case SDLK_KP_2:
                key = K_KP_DOWNARROW;
                break;

            case SDLK_DOWN:
                key = K_DOWNARROW;
                break;

            case SDLK_KP_8:
                key = K_KP_UPARROW;
                break;

            case SDLK_UP:
                key = K_UPARROW;
                break;

            case SDLK_ESCAPE:
                key = K_ESCAPE;
                break;

            case SDLK_KP_ENTER:
                key = K_KP_ENTER;
                break;

            case SDLK_RETURN:
                key = K_ENTER;
                break;

            case SDLK_TAB:
                key = K_TAB;
                break;

            case SDLK_F1:
                key = K_F1;
                break;

            case SDLK_F2:
                key = K_F2;
                break;

            case SDLK_F3:
                key = K_F3;
                break;

            case SDLK_F4:
                key = K_F4;
                break;

            case SDLK_F5:
                key = K_F5;
                break;

            case SDLK_F6:
                key = K_F6;
                break;

            case SDLK_F7:
                key = K_F7;
                break;

            case SDLK_F8:
                key = K_F8;
                break;

            case SDLK_F9:
                key = K_F9;
                break;

            case SDLK_F10:
                key = K_F10;
                break;

            case SDLK_F11:
                key = K_F11;
                break;

            case SDLK_F12:
                key = K_F12;
                break;

            case SDLK_F13:
                key = K_F13;
                break;

            case SDLK_F14:
                key = K_F14;
                break;

            case SDLK_F15:
                key = K_F15;
                break;

            case SDLK_BACKSPACE:
                key = K_BACKSPACE;
                break;

            case SDLK_KP_PERIOD:
                key = K_KP_DEL;
                break;

            case SDLK_DELETE:
                key = K_DEL;
                break;

            case SDLK_PAUSE:
                key = K_PAUSE;
                break;

            case SDLK_LSHIFT:
            case SDLK_RSHIFT:
                key = K_SHIFT;
                break;

            case SDLK_LCTRL:
            case SDLK_RCTRL:
                key = K_CTRL;
                break;

            case SDLK_RGUI:
            case SDLK_LGUI:
                key = K_COMMAND;
                break;

            case SDLK_RALT:
            case SDLK_LALT:
                key = K_ALT;
                break;

            case SDLK_KP_5:
                key = K_KP_5;
                break;

            case SDLK_INSERT:
                key = K_INS;
                break;

            case SDLK_KP_0:
                key = K_KP_INS;
                break;

            case SDLK_KP_MULTIPLY:
                key = K_KP_STAR;
                break;

            case SDLK_KP_PLUS:
                key = K_KP_PLUS;
                break;

            case SDLK_KP_MINUS:
                key = K_KP_MINUS;
                break;

            case SDLK_KP_DIVIDE:
                key = K_KP_SLASH;
                break;

            case SDLK_MODE:
                key = K_MODE;
                break;

            case SDLK_HELP:
                key = K_HELP;
                break;

            case SDLK_PRINTSCREEN:
                key = K_PRINT;
                break;

            case SDLK_SYSREQ:
                key = K_SYSREQ;
                break;

            case SDLK_MENU:
                key = K_MENU;
                break;

            case SDLK_POWER:
                key = K_POWER;
                break;

            case SDLK_UNDO:
                key = K_UNDO;
                break;

            case SDLK_SCROLLLOCK:
                key = K_SCROLLOCK;
                break;

            case SDLK_CAPSLOCK:
                key = K_CAPSLOCK;
                break;

            default:
                break;
        }
    }

    if(in_keyboardDebug->integer) {
        PrintKey(keysym, key, down);
    }

    if(IsConsoleKey(key, 0)) {
        // Console keys can't be bound or generate characters
        key = K_CONSOLE;
    }

    return key;
}

/*
================
idSystemLocal::TranslateKeyToCtrlChar

Translate key to ASCII Control character, return -1 if no conversation could be
made
================
*/
sint idSystemLocal::TranslateCtrlCharToKey(sint key) {
    switch(key) {
        case '-':

        // fallthrough
        case '/':

        // fallthrough
        case '7':
            return KEYBOARDCTRL('_');

        case '8':

        // fallthrough
        case K_BACKSPACE:
            return KEYBOARDCTRL('h');

        case K_TAB:
            return '\t';

        case K_ESCAPE:
            return '\x1B';
    }

    if((key >= '@' && key < '_') || (key >= 'a' && key <= 'z')) {
        return KEYBOARDCTRL(key);
    }

    return -1;
}


/*
===============
idSystemLocal::GobbleMotionEvents
===============
*/
void idSystemLocal::GobbleMotionEvents(void) {
    SDL_Event dummy[1];
    sint val = 0;

    // Gobble any mouse motion events
    SDL_PumpEvents();

    while((val = SDL_PeepEvents(dummy, 1, SDL_GETEVENT, SDL_MOUSEMOTION,
                                SDL_MOUSEMOTION)) > 0) {
    }

    if(val < 0) {
        Com_Printf("idSystemLocal::GobbleMotionEvents failed: %s\n",
                   SDL_GetError());
    }
}

/*
===============
idSystemLocal::ActivateMouse
===============
*/
void idSystemLocal::ActivateMouse(void) {
    static connstate_t lastState = CA_UNINITIALIZED;

    if(!mouseAvailable || !SDL_WasInit(SDL_INIT_VIDEO)) {
        return;
    }

    if(cls.state != lastState) {
        lastState = cls.state;
        return;
    }

    if(!mouseActive) {
        SDL_SetRelativeMouseMode(SDL_TRUE);
        SDL_SetWindowGrab(SDL_window, (SDL_bool)1);

        GobbleMotionEvents();
    }

    // in_nograb makes no sense in fullscreen mode
    if(!(SDL_GetWindowFlags(SDL_window) & SDL_WINDOW_FULLSCREEN)) {
        if(in_nograb->modified || !mouseActive) {
            if(in_nograb->integer) {
                SDL_SetRelativeMouseMode(SDL_FALSE);
                SDL_SetWindowGrab(SDL_window, (SDL_bool)0);
            } else {
                SDL_SetRelativeMouseMode(SDL_TRUE);
                SDL_SetWindowGrab(SDL_window, (SDL_bool)1);
            }

            in_nograb->modified = false;
        }
    }

    mouseActive = true;
}

/*
===============
idSystemLocal::DeactivateMouse
===============
*/
void idSystemLocal::DeactivateMouse(void) {
    if(!SDL_WasInit(SDL_INIT_VIDEO)) {
        return;
    }

    // Always show the cursor when the mouse is disabled,
    // but not when fullscreen
    if(!(SDL_GetWindowFlags(SDL_window) & SDL_WINDOW_FULLSCREEN)) {
        SDL_ShowCursor(1);
    }

    if(!mouseAvailable) {
        return;
    }

    if(mouseActive) {
        GobbleMotionEvents();

        SDL_SetWindowGrab(SDL_window, (SDL_bool)0);
        SDL_SetRelativeMouseMode(SDL_FALSE);

        // Don't warp the mouse unless the cursor is within the window
        if(SDL_GetWindowFlags(SDL_window) & SDL_WINDOW_MOUSE_FOCUS) {
            SDL_WarpMouseInWindow(SDL_window, cls.glconfig.vidWidth / 2,
                                  cls.glconfig.vidHeight / 2);
        }

        mouseActive = false;
    }
}

/*
===============
idSystemLocal::InitJoystick
===============
*/
void idSystemLocal::InitJoystick(void) {
    sint i = 0;
    sint total = 0;
    valueType buf[16384] = "";

    if(stick != nullptr) {
        SDL_JoystickClose(stick);
    }

    stick = nullptr;
    ::memset(&stick_state, '\0', sizeof(stick_state));

    if(!SDL_WasInit(SDL_INIT_JOYSTICK)) {
        Com_DPrintf("Calling SDL_Init(SDL_INIT_JOYSTICK)...\n");

        if(SDL_Init(SDL_INIT_JOYSTICK) == -1) {
            Com_DPrintf("SDL_Init(SDL_INIT_JOYSTICK) failed: %s\n", SDL_GetError());
            return;
        }

        Com_DPrintf("SDL_Init(SDL_INIT_JOYSTICK) passed.\n");
    }

    total = SDL_NumJoysticks();
    Com_DPrintf("%d possible joysticks\n", total);

    // Print list and build cvar to allow ui to select joystick.
    for(i = 0; i < total; i++) {
        Q_strcat(buf, sizeof(buf), SDL_JoystickNameForIndex(i));
        Q_strcat(buf, sizeof(buf), "\n");
    }

    if(!in_joystick->integer) {
        Com_DPrintf("Joystick is not active.\n");
        SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
        return;
    }

    if(in_joystickNo->integer < 0 || in_joystickNo->integer >= total) {
        cvarSystem->Set("in_joystickNo", "0");
    }

    stick = SDL_JoystickOpen(in_joystickNo->integer);

    if(stick == nullptr) {
        Com_DPrintf("No joystick opened.\n");
        return;
    }

    Com_DPrintf("Joystick %d opened\n", in_joystickNo->integer);
    Com_DPrintf("Name:       %s\n",
                SDL_JoystickNameForIndex(in_joystickNo->integer));
    Com_DPrintf("Axes:       %d\n", SDL_JoystickNumAxes(stick));
    Com_DPrintf("Hats:       %d\n", SDL_JoystickNumHats(stick));
    Com_DPrintf("Buttons:    %d\n", SDL_JoystickNumButtons(stick));
    Com_DPrintf("Balls:      %d\n", SDL_JoystickNumBalls(stick));
    Com_DPrintf("Use Analog: %s\n",
                in_joystickUseAnalog->integer ? "Yes" : "No");

    SDL_JoystickEventState(SDL_QUERY);
}

/*
===============
idSystemLocal::ShutdownJoystick
===============
*/
void idSystemLocal::ShutdownJoystick(void) {
    if(!SDL_WasInit(SDL_INIT_JOYSTICK)) {
        return;
    }

    if(stick) {
        SDL_JoystickClose(stick);
        stick = nullptr;
    }

    SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
}

/*
===============
idSystemLocal::JoyMove
===============
*/
void idSystemLocal::JoyMove(sint eventTime) {
    bool joy_pressed[ARRAY_LEN(joy_keys)];
    uint axes = 0;
    uint hats = 0;
    sint total = 0;
    sint i = 0;

    if(!stick) {
        return;
    }

    SDL_JoystickUpdate();

    ::memset(joy_pressed, '\0', sizeof(joy_pressed));

    // update the ball state.
    total = SDL_JoystickNumBalls(stick);

    if(total > 0) {
        sint balldx = 0;
        sint balldy = 0;

        for(i = 0; i < total; i++) {
            sint dx = 0;
            sint dy = 0;
            SDL_JoystickGetBall(stick, i, &dx, &dy);
            balldx += dx;
            balldy += dy;
        }

        if(balldx || balldy) {
            // !!! FIXME: is this good for stick balls, or just mice?
            // Scale like the mouse input...
            if(abs(balldx) > 1) {
                balldx *= 2;
            }

            if(abs(balldy) > 1) {
                balldy *= 2;
            }

            Com_QueueEvent(eventTime, SYSE_MOUSE, balldx, balldy, 0, nullptr);
        }
    }

    // now query the stick buttons...
    total = SDL_JoystickNumButtons(stick);

    if(total > 0) {
        if(total > ARRAY_LEN(stick_state.buttons)) {
            total = ARRAY_LEN(stick_state.buttons);
        }

        for(i = 0; i < total; i++) {
            bool pressed = (SDL_JoystickGetButton(stick, i) != 0);

            if(pressed != stick_state.buttons[i]) {
                Com_QueueEvent(eventTime, SYSE_KEY, K_JOY1 + i, pressed, 0, nullptr);
                stick_state.buttons[i] = pressed;
            }
        }
    }

    // look at the hats...
    total = SDL_JoystickNumHats(stick);

    if(total > 0) {
        if(total > 4) {
            total = 4;
        }

        for(i = 0; i < total; i++) {
            (reinterpret_cast<uchar8 *>(&hats))[i] = SDL_JoystickGetHat(stick, i);
        }
    }

    // update hat state
    if(hats != stick_state.oldhats) {
        for(i = 0; i < 4; i++) {
            if((reinterpret_cast<uchar8 *>(&hats))[i] != (reinterpret_cast<uchar8 *>
                    (&stick_state.oldhats))[i]) {
                // release event
                switch((reinterpret_cast<uchar8 *>(&stick_state.oldhats))[i]) {
                    case SDL_HAT_UP:
                        Com_QueueEvent(eventTime, SYSE_KEY, hat_keys[4 * i + 0], false, 0,
                                       nullptr);
                        break;

                    case SDL_HAT_RIGHT:
                        Com_QueueEvent(eventTime, SYSE_KEY, hat_keys[4 * i + 1], false, 0,
                                       nullptr);
                        break;

                    case SDL_HAT_DOWN:
                        Com_QueueEvent(eventTime, SYSE_KEY, hat_keys[4 * i + 2], false, 0,
                                       nullptr);
                        break;

                    case SDL_HAT_LEFT:
                        Com_QueueEvent(eventTime, SYSE_KEY, hat_keys[4 * i + 3], false, 0,
                                       nullptr);
                        break;

                    case SDL_HAT_RIGHTUP:
                        Com_QueueEvent(eventTime, SYSE_KEY, hat_keys[4 * i + 0], false, 0,
                                       nullptr);
                        Com_QueueEvent(eventTime, SYSE_KEY, hat_keys[4 * i + 1], false, 0,
                                       nullptr);
                        break;

                    case SDL_HAT_RIGHTDOWN:
                        Com_QueueEvent(eventTime, SYSE_KEY, hat_keys[4 * i + 2], false, 0,
                                       nullptr);
                        Com_QueueEvent(eventTime, SYSE_KEY, hat_keys[4 * i + 1], false, 0,
                                       nullptr);
                        break;

                    case SDL_HAT_LEFTUP:
                        Com_QueueEvent(eventTime, SYSE_KEY, hat_keys[4 * i + 0], false, 0,
                                       nullptr);
                        Com_QueueEvent(eventTime, SYSE_KEY, hat_keys[4 * i + 3], false, 0,
                                       nullptr);
                        break;

                    case SDL_HAT_LEFTDOWN:
                        Com_QueueEvent(eventTime, SYSE_KEY, hat_keys[4 * i + 2], false, 0,
                                       nullptr);
                        Com_QueueEvent(eventTime, SYSE_KEY, hat_keys[4 * i + 3], false, 0,
                                       nullptr);
                        break;

                    default:
                        break;
                }

                // press event
                switch(((Uint8 *)&hats)[i]) {
                    case SDL_HAT_UP:
                        Com_QueueEvent(eventTime, SYSE_KEY, hat_keys[4 * i + 0], true, 0, nullptr);
                        break;

                    case SDL_HAT_RIGHT:
                        Com_QueueEvent(eventTime, SYSE_KEY, hat_keys[4 * i + 1], true, 0, nullptr);
                        break;

                    case SDL_HAT_DOWN:
                        Com_QueueEvent(eventTime, SYSE_KEY, hat_keys[4 * i + 2], true, 0, nullptr);
                        break;

                    case SDL_HAT_LEFT:
                        Com_QueueEvent(eventTime, SYSE_KEY, hat_keys[4 * i + 3], true, 0, nullptr);
                        break;

                    case SDL_HAT_RIGHTUP:
                        Com_QueueEvent(eventTime, SYSE_KEY, hat_keys[4 * i + 0], true, 0, nullptr);
                        Com_QueueEvent(eventTime, SYSE_KEY, hat_keys[4 * i + 1], true, 0, nullptr);
                        break;

                    case SDL_HAT_RIGHTDOWN:
                        Com_QueueEvent(eventTime, SYSE_KEY, hat_keys[4 * i + 2], true, 0, nullptr);
                        Com_QueueEvent(eventTime, SYSE_KEY, hat_keys[4 * i + 1], true, 0, nullptr);
                        break;

                    case SDL_HAT_LEFTUP:
                        Com_QueueEvent(eventTime, SYSE_KEY, hat_keys[4 * i + 0], true, 0, nullptr);
                        Com_QueueEvent(eventTime, SYSE_KEY, hat_keys[4 * i + 3], true, 0, nullptr);
                        break;

                    case SDL_HAT_LEFTDOWN:
                        Com_QueueEvent(eventTime, SYSE_KEY, hat_keys[4 * i + 2], true, 0, nullptr);
                        Com_QueueEvent(eventTime, SYSE_KEY, hat_keys[4 * i + 3], true, 0, nullptr);
                        break;

                    default:
                        break;
                }
            }
        }
    }

    // save hat state
    stick_state.oldhats = hats;

    // finally, look at the axes...
    total = SDL_JoystickNumAxes(stick);

    if(total > 0) {
        if(in_joystickUseAnalog->integer) {
            if(total > MAX_JOYSTICK_AXIS) {
                total = MAX_JOYSTICK_AXIS;
            }

            for(i = 0; i < total; i++) {
                schar16 axis = SDL_JoystickGetAxis(stick, i);
                float32 f = (static_cast<float32>(abs(axis))) / 32767.0f;

                if(f < in_joystickThreshold->value) {
                    axis = 0;
                }

                if(axis != stick_state.oldaaxes[i]) {
                    Com_QueueEvent(eventTime, SYSE_JOYSTICK_AXIS, i, axis, 0, nullptr);
                    stick_state.oldaaxes[i] = axis;
                }
            }
        } else {
            if(total > 16) {
                total = 16;
            }

            for(i = 0; i < total; i++) {
                schar16 axis = SDL_JoystickGetAxis(stick, i);

                float32 f = (static_cast<float32>(axis)) / 32767.0f;

                if(f < -in_joystickThreshold->value) {
                    axes |= (1 << (i * 2));
                } else if(f > in_joystickThreshold->value) {
                    axes |= (1 << ((i * 2) + 1));
                }
            }
        }
    }

    /* Time to update axes state based on old vs. new. */
    if(axes != stick_state.oldaxes) {
        for(i = 0; i < 16; i++) {
            if((axes & (1 << i)) && !(stick_state.oldaxes & (1 << i))) {
                Com_QueueEvent(eventTime, SYSE_KEY, joy_keys[i], true, 0, nullptr);
            }

            if(!(axes & (1 << i)) && (stick_state.oldaxes & (1 << i))) {
                Com_QueueEvent(eventTime, SYSE_KEY, joy_keys[i], false, 0, nullptr);
            }
        }
    }

    /* Save for future generations. */
    stick_state.oldaxes = axes;
}

/*
===============
idSystemLocal::ProcessEvents
===============
*/
void idSystemLocal::ProcessEvents(sint eventTime) {
    SDL_Event e;
    keyNum_t key = (keyNum_t)0;
    sint mx = 0, my = 0;
    static keyNum_t lastKeyDown = (keyNum_t)0;

    if(!SDL_WasInit(SDL_INIT_VIDEO)) {
        return;
    }

    while(SDL_PollEvent(&e)) {
        switch(e.type) {
            case SDL_KEYDOWN:
                if(e.key.repeat && clientGUISystem->GetCatcher() == 0) {
                    break;
                }

                if((key = TranslateSDLToQ3Key(&e.key.keysym, true))) {
                    Com_QueueEvent(eventTime, SYSE_KEY, key, true, 0, nullptr);
                }

                if(key == K_BACKSPACE || key == K_TAB || key == K_ESCAPE ||
                        keys[K_CTRL].down) {
                    sint k = TranslateCtrlCharToKey(key);

                    if(k != -1) {
                        Com_QueueEvent(eventTime, SYSE_CHAR, k, 0, 0, nullptr);
                    }
                } else if(keys[K_CTRL].down && key >= 'a' && key <= 'z') {
                    Com_QueueEvent(eventTime, SYSE_CHAR, KEYBOARDCTRL(key), 0, 0, nullptr);
                }

                lastKeyDown = key;
                break;

            case SDL_KEYUP:
                if((key = TranslateSDLToQ3Key(&e.key.keysym, false))) {
                    Com_QueueEvent(eventTime, SYSE_KEY, key, false, 0, nullptr);
                }

                lastKeyDown = static_cast< keyNum_t >(0);
                break;

            case SDL_TEXTINPUT:
                if(lastKeyDown != K_CONSOLE) {
                    valueType *c = e.text.text;

                    // Quick and dirty UTF-8 to UTF-32 conversion
                    while(*c) {
                        sint utf32 = 0;

                        if((*c & 0x80) == 0) {
                            utf32 = *c++;
                        } else if((*c & 0xE0) == 0xC0) { // 110x xxxx
                            utf32 |= (*c++ & 0x1F) << 6;
                            utf32 |= (*c++ & 0x3F);
                        } else if((*c & 0xF0) == 0xE0) { // 1110 xxxx
                            utf32 |= (*c++ & 0x0F) << 12;
                            utf32 |= (*c++ & 0x3F) << 6;
                            utf32 |= (*c++ & 0x3F);
                        } else if((*c & 0xF8) == 0xF0) { // 1111 0xxx
                            utf32 |= (*c++ & 0x07) << 18;
                            utf32 |= (*c++ & 0x3F) << 6;
                            utf32 |= (*c++ & 0x3F) << 6;
                            utf32 |= (*c++ & 0x3F);
                        } else {
                            Com_DPrintf("Unrecognised UTF-8 lead byte: 0x%x\n", static_cast<uint>(*c));
                            c++;
                        }

                        if(utf32 != 0) {
                            if(IsConsoleKey((keyNum_t)0, utf32)) {
                                Com_QueueEvent(eventTime, SYSE_KEY, K_CONSOLE, true, 0, nullptr);
                                Com_QueueEvent(eventTime, SYSE_KEY, K_CONSOLE, false, 0, nullptr);
                            } else {
                                Com_QueueEvent(eventTime, SYSE_CHAR, utf32, 0, 0, nullptr);
                            }
                        }
                    }
                }

                break;

            case SDL_MOUSEMOTION:
                if(mouseActive) {
                    mx += e.motion.xrel;
                    my += e.motion.yrel;
                }

                break;

            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP: {
                sint b;

                switch(e.button.button) {
                    case SDL_BUTTON_LEFT:
                        b = K_MOUSE1;
                        break;

                    case SDL_BUTTON_MIDDLE:
                        b = K_MOUSE3;
                        break;

                    case SDL_BUTTON_RIGHT:
                        b = K_MOUSE2;
                        break;

                    case SDL_BUTTON_X1:
                        b = K_MOUSE4;
                        break;

                    case SDL_BUTTON_X2:
                        b = K_MOUSE5;
                        break;

                    default:
                        b = K_AUX1 + (e.button.button - SDL_BUTTON_X2 + 1) % 16;
                        break;
                }

                Com_QueueEvent(eventTime, SYSE_KEY, b,
                               (e.type == SDL_MOUSEBUTTONDOWN ? true : false), 0, nullptr);
            }
            break;

            case SDL_MOUSEWHEEL:
                if(e.wheel.y > 0) {
                    Com_QueueEvent(eventTime, SYSE_KEY, K_MWHEELUP, true, 0, nullptr);
                    Com_QueueEvent(eventTime, SYSE_KEY, K_MWHEELUP, false, 0, nullptr);
                } else {
                    Com_QueueEvent(eventTime, SYSE_KEY, K_MWHEELDOWN, true, 0, nullptr);
                    Com_QueueEvent(eventTime, SYSE_KEY, K_MWHEELDOWN, false, 0, nullptr);
                }

                break;

            case SDL_QUIT:
                cmdBufferSystem->ExecuteText(EXEC_NOW, "quit Closed window\n");
                break;

            case SDL_WINDOWEVENT:
                switch(e.window.event) {
                    case SDL_WINDOWEVENT_RESIZED: {
                        sint width, height;

                        width = e.window.data1;
                        height = e.window.data2;

                        // ignore this event on fullscreen
                        if(cls.glconfig.isFullscreen) {
                            break;
                        }

                        // check if size actually changed
                        if(cls.glconfig.vidWidth == width && cls.glconfig.vidHeight == height) {
                            break;
                        }

                        cvarSystem->SetValue("r_customwidth", width);
                        cvarSystem->SetValue("r_customheight", height);
                        cvarSystem->Set("r_mode", "-1");

                        // wait until user stops dragging for 1 second, so we aren't constantly recreating the GL context while he tries to drag...
                        vidRestartTime = idsystem->Milliseconds() + 1000;
                    }
                    break;

                    case SDL_WINDOWEVENT_HIDDEN:
                    case SDL_WINDOWEVENT_MINIMIZED:
                        cvarSystem->SetValue("com_minimized", 1);
                        break;

                    case SDL_WINDOWEVENT_SHOWN:
                    case SDL_WINDOWEVENT_RESTORED:
                    case SDL_WINDOWEVENT_MAXIMIZED:
                        cvarSystem->SetValue("com_minimized", 0);
                        break;

                    case SDL_WINDOWEVENT_FOCUS_LOST:
                        cvarSystem->SetValue("com_unfocused", 1);
                        break;

                    case SDL_WINDOWEVENT_FOCUS_GAINED:
                        cvarSystem->SetValue("com_unfocused", 0);
                        break;
                }

                break;

            default:
                break;
        }
    }

    if(mx || my) {
        Com_QueueEvent(eventTime, SYSE_MOUSE, mx, my, 0, nullptr);
    }
}

/*
===============
idSystemLocal::Frame
===============
*/
void idSystemLocal::Frame(void) {
    static sint eventTime;
    bool loading;

    JoyMove(eventTime);

    // If not DISCONNECTED (main menu) or ACTIVE (in game), we're loading
    loading = (bool)(cls.state != CA_DISCONNECTED && cls.state != CA_ACTIVE &&
                     !(clientGUISystem->GetCatcher() & KEYCATCH_UI));

    if(!cls.glconfig.isFullscreen &&
            (clientGUISystem->GetCatcher() & KEYCATCH_CONSOLE)) {
        // Console is down in windowed mode
        DeactivateMouse();
    } else if(!cls.glconfig.isFullscreen && loading) {
        // Loading in windowed mode
        DeactivateMouse();
    } else if(!(SDL_GetWindowFlags(SDL_window) & SDL_WINDOW_INPUT_FOCUS)) {
        // Window not got focus
        DeactivateMouse();
    } else {
        ActivateMouse();
    }

    ProcessEvents(eventTime);

    // In case we had to delay actual restart of video system
    if((vidRestartTime != 0) && (vidRestartTime < idsystem->Milliseconds())) {
        vidRestartTime = 0;
        cmdBufferSystem->AddText("vid_restart\n");
    }

    // Set event time for next frame to earliest possible time an event could happen
    eventTime = idsystem->Milliseconds();
}

/*
===============
idSystemLocal::InitKeyLockStates
===============
*/
void idSystemLocal::InitKeyLockStates(void) {
    const uchar8 *keystate = SDL_GetKeyboardState(nullptr);

    keys[K_SCROLLOCK].down = keystate[SDL_SCANCODE_SCROLLLOCK];
    keys[K_KP_NUMLOCK].down = keystate[SDL_SCANCODE_NUMLOCKCLEAR];
    keys[K_CAPSLOCK].down = keystate[SDL_SCANCODE_CAPSLOCK];
}

/*
===============
idSystemLocal::Init
===============
*/
void idSystemLocal::Init(void *windowData) {
    sint appState;

    if(!SDL_WasInit(SDL_INIT_VIDEO)) {
        Com_Error(ERR_FATAL,
                  "idSystemLocal::Init called before SDL_Init( SDL_INIT_VIDEO )");
        return;
    }

    SDL_window = static_cast< SDL_Window * >(windowData);

    Com_DPrintf("\n------- Input Initialization -------\n");

    SDL_StartTextInput();

    mouseAvailable = (in_mouse->value != 0);

    if(in_mouse->integer == 2) {
        Com_DPrintf("IN_Init: Not using raw input\n");
        SDL_SetHint(SDL_HINT_MOUSE_RELATIVE_MODE_WARP, "1");
    } else {
        Com_DPrintf("IN_Init: Using raw mouse input\n");
        SDL_SetHint(SDL_HINT_MOUSE_RELATIVE_MODE_WARP, "0");
    }

    DeactivateMouse();

    appState = SDL_GetWindowFlags(SDL_window);
    cvarSystem->SetValue("com_unfocused",
                         !(appState & SDL_WINDOW_INPUT_FOCUS));
    cvarSystem->SetValue("com_minimized", appState & SDL_WINDOW_MINIMIZED);

    InitKeyLockStates();

    InitJoystick();
    Com_DPrintf("------------------------------------\n");
}

/*
===============
idSystemLocal::Shutdown
===============
*/
void idSystemLocal::Shutdown(void) {
    SDL_StopTextInput();

    DeactivateMouse();
    mouseAvailable = false;

    ShutdownJoystick();

    SDL_window = nullptr;
}

/*
===============
idSystemLocal::Restart
===============
*/
void idSystemLocal::Restart(void) {
    ShutdownJoystick();
    systemLocal.Init(SDL_window);
}
