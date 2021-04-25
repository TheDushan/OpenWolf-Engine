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
// File name:   CVarSystem.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description: dynamic variable tracking
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifdef UPDATE_SERVER
#include <server/serverAutoPrecompiled.hpp>
#elif DEDICATED
#include <server/serverDedPrecompiled.hpp>
#else
#include <framework/precompiled.hpp>
#endif

convar_t *cvar_vars;
convar_t *cvar_cheats;
sint cvar_modifiedFlags;

#define MAX_CVARS 4096
convar_t cvar_indexes[MAX_CVARS];
sint cvar_numIndexes;

#define FILE_HASH_SIZE 512
static convar_t *hashTable[FILE_HASH_SIZE];

idCVarSystemLocal cvarSystemLocal;
idCVarSystem *cvarSystem = &cvarSystemLocal;

/*
===============
idCVarSystemLocal::idCVarSystemLocal
===============
*/
idCVarSystemLocal::idCVarSystemLocal(void) {
}

/*
===============
idCVarSystemLocal::~idCVarSystemLocal
===============
*/
idCVarSystemLocal::~idCVarSystemLocal(void) {
}

/*
================
idCVarSystemLocal::generateHashValue

return a hash value for the filename
================
*/
sint32 idCVarSystemLocal::generateHashValue(pointer fname) {
    sint i;
    sint32 hash;
    valueType letter;

    if(!fname) {
        Com_Error(ERR_DROP, "null name in generateHashValue");   //gjd
    }

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
============
idCVarSystemLocal::ValidateString
============
*/
bool idCVarSystemLocal::ValidateString(pointer s) {
    if(!s) {
        return false;
    }

    if(strchr(s, '\\')) {
        return false;
    }

    if(strchr(s, '\"')) {
        return false;
    }

    if(strchr(s, ';')) {
        return false;
    }

    return true;
}

/*
============
idCVarSystemLocal::FindVar
============
*/
convar_t *idCVarSystemLocal::FindVar(pointer var_name) {
    convar_t *var;
    sint32 hash;

    if(!var_name) {
        return nullptr;
    }

    hash = generateHashValue(var_name);

    for(var = hashTable[hash]; var; var = var->hashNext) {
        if(!Q_stricmp(var_name, var->name)) {
            return var;
        }
    }

    return nullptr;
}

/*
============
idCVarSystemLocal::VariableValue
============
*/
float32 idCVarSystemLocal::VariableValue(pointer var_name) {
    convar_t *var;

    var = FindVar(var_name);

    if(!var) {
        return 0;
    }

    return var->value;
}


/*
============
idCVarSystemLocal::VariableIntegerValue
============
*/
sint idCVarSystemLocal::VariableIntegerValue(pointer var_name) {
    convar_t *var;

    var = idCVarSystemLocal::FindVar(var_name);

    if(!var) {
        return 0;
    }

    return var->integer;
}


/*
============
idCVarSystemLocal::VariableString
============
*/
valueType *idCVarSystemLocal::VariableString(pointer var_name) {
    convar_t         *var;

    var = FindVar(var_name);

    if(!var) {
        return "";
    }

    return var->string;
}


/*
============
idCVarSystemLocal::VariableStringBuffer
============
*/
void idCVarSystemLocal::VariableStringBuffer(pointer var_name,
        valueType *buffer, uint64 bufsize) {
    convar_t *var;

    var = FindVar(var_name);

    if(!var) {
        *buffer = 0;
    } else {
        Q_strncpyz(buffer, var->string, bufsize);
    }
}

/*
============
idCVarSystemLocal::VariableStringBuffer
============
*/
void idCVarSystemLocal::LatchedVariableStringBuffer(pointer var_name,
        valueType *buffer, uint64 bufsize) {
    convar_t *var;

    var = FindVar(var_name);

    if(!var) {
        *buffer = 0;
    } else {
        if(var->latchedString) {
            Q_strncpyz(buffer, var->latchedString, bufsize);
        } else {
            Q_strncpyz(buffer, var->string, bufsize);
        }
    }
}

/*
============
idCVarSystemLocal::Flags
============
*/
sint idCVarSystemLocal::Flags(pointer var_name) {
    convar_t *var;

    if(!(var = FindVar(var_name))) {
        return CVAR_NONEXISTENT;
    } else {
        return var->flags;
    }
}

/*
============
idCVarSystemLocal::CommandCompletion
============
*/
void idCVarSystemLocal::CommandCompletion(void (*callback)(pointer s)) {
    convar_t *cvar;

    for(cvar = cvar_vars; cvar; cvar = cvar->next) {
        if(cvar->name) {
            callback(cvar->name);
        }
    }
}

/*
============
idCVarSystemLocal::ClearForeignCharacters
some cvar values need to be safe from foreign characters
============
*/
valueType *idCVarSystemLocal::ClearForeignCharacters(pointer value) {
    static valueType clean[MAX_CVAR_VALUE_STRING];
    sint i, j;

    j = 0;

    for(i = 0; value[i] != '\0'; i++) {
        if((value)[i] != 0xFF && ((value)[i] <= 127 || (value)[i] >= 161)) {
            clean[j] = value[i];
            j++;
        }
    }

    clean[j] = '\0';

    return clean;
}

/*
============
idCVarSystemLocal::Validate
============
*/
const valueType *idCVarSystemLocal::Validate(convar_t *var, pointer value,
        bool warn) {
    static valueType s[MAX_CVAR_VALUE_STRING];
    float32 valuef;
    bool changed = false;

    if(!var->validate) {
        return value;
    }

    if(!value) {
        return value;
    }

    if(Q_isanumber(value)) {
        valuef = atof(value);

        if(var->integral) {
            if(!Q_isintegral(valuef)) {
                if(warn) {
                    Com_Printf(S_COLOR_YELLOW "'%s' must be integral", var->name);
                }

                valuef = static_cast<sint>(valuef);
                changed = true;
            }
        }
    } else {
        if(warn) {
            Com_Printf(S_COLOR_YELLOW "'%s' must be numeric", var->name);
        }

        valuef = atof(var->resetString);
        changed = true;
    }

    if(valuef < var->min) {
        if(warn) {
            if(changed) {
                Com_Printf(" and is");
            } else {
                Com_Printf(S_COLOR_YELLOW "Attempted to set '%s'", var->name);
            }

            if(Q_isintegral(var->min)) {
                Com_Printf(" out of range (min %d)\n", static_cast<sint>(var->min));
            } else {
                Com_Printf(" out of range (min %f)\n", var->min);
            }
        }

        valuef = var->min;
        changed = true;
    } else if(valuef > var->max) {
        if(warn) {
            if(changed) {
                Com_Printf(" and is");
            } else {
                Com_Printf(S_COLOR_YELLOW "Attempted to set '%s'", var->name);
            }

            if(Q_isintegral(var->max)) {
                Com_Printf(" out of range (max %d)\n", static_cast<sint>(var->max));
            } else {
                Com_Printf(" out of range (max %f)\n", var->max);
            }
        }

        valuef = var->max;
        changed = true;
    }

    if(changed) {
        if(Q_isintegral(valuef)) {
            Q_vsprintf_s(s, sizeof(s), sizeof(s), "%d", static_cast<sint>(valuef));
        } else {
            Q_vsprintf_s(s, sizeof(s), sizeof(s), "%f", valuef);
        }

        return s;
    } else {
        return value;
    }
}

/*
============
idCVarSystemLocal::Get

If the variable already exists, the value will not be set unless CVAR_ROM
The flags will be or'ed in if the variable exists.
You can pass nullptr for the description.
============
*/
convar_t *idCVarSystemLocal::Get(pointer var_name, pointer var_value,
                                 sint flags, pointer description) {
    convar_t *var;
    sint32 hash;

    if(!var_name || !var_value) {
        Com_Error(ERR_FATAL, "idCVarSystemLocal::Get: nullptr parameter");
    }

    if(!ValidateString(var_name)) {
        Com_Printf("idCVarSystemLocal::Get: invalid cvar name string: %s\n",
                   var_name);
        var_name = "BADNAME";
    }

#if 0 // FIXME: values with backslash happen

    if(!Cvar_ValidateString(var_value)) {
        Com_Printf("idCVarSystemLocal::Get: invalid cvar value string: %s\n",
                   var_value);
        var_value = "BADVALUE";
    }

#endif

    var = FindVar(var_name);

    if(var) {
        //var_value = Validate( var, var_value, false );

        // if the C code is now specifying a variable that the user already
        // set a value for, take the new value as the reset value
        if((var->flags & CVAR_USER_CREATED) && !(flags & CVAR_USER_CREATED) &&
                var_value[0]) {
            var->flags &= ~CVAR_USER_CREATED;
            Z_Free(var->resetString);
            var->resetString = CopyString(var_value);
        }

        var->flags |= flags;

        // ZOID--needs to be set so that cvars the game sets as
        // SERVERINFO get sent to clients
        cvar_modifiedFlags |= flags;

        // only allow one non-empty reset string without a warning
        if(!var->resetString[0]) {
            // we don't have a reset string yet
            Z_Free(var->resetString);
            var->resetString = CopyString(var_value);
        } else if(var_value[0] && strcmp(var->resetString, var_value)) {
            Com_DPrintf("idCVarSystemLocal::Get: Warning: convar \"%s\" given initial values: \"%s\" and \"%s\"\n",
                        var_name, var->resetString, var_value);
        }

        // if we have a latched string, take that value now
        if(var->latchedString) {
            valueType           *s;

            s = var->latchedString;
            var->latchedString =
                nullptr;   // otherwise idCVarSystemLocal::GetSet2 would free it
            GetSet2(var_name, s, true);
            Z_Free(s);
        }

        // TTimo
        // if CVAR_USERINFO was toggled on for an existing cvar, check wether the value needs to be cleaned from foreigh characters
        // (for instance, seta name "name-with-foreign-chars" in the config file, and toggle to CVAR_USERINFO happens later in CL_Init)
        if(flags & CVAR_USERINFO) {
            valueType *cleaned = ClearForeignCharacters(
                                     var->string);  // NOTE: it is probably harmless to call idCVarSystemLocal::Set2 in all cases, but I don't want to risk it

            if(strcmp(var->string, cleaned)) {
                GetSet2(var->name, var->string,
                        false);      // call idCVarSystemLocal::Set2 with the value to be cleaned up for verbosity
            }
        }

        if(description) {
            if(var->description) {
                Z_Free(var->description);
            }

            var->description = CopyString(description);
        }

        // use a CVAR_SET for rom sets, get won't override
#if 0

        // CVAR_ROM always overrides
        if(flags & CVAR_ROM) {
            Set2(var_name, var_value, true);
        }

#endif
        return var;
    }

    //
    // allocate a new cvar
    //
    if(cvar_numIndexes >= MAX_CVARS) {
        Com_Error(ERR_FATAL,
                  "idCVarSystemLocal::Get: MAX_CVARS (%d) hit -- too many cvars!",
                  MAX_CVARS);
    }

    var = &cvar_indexes[cvar_numIndexes];
    cvar_numIndexes++;
    var->name = CopyString(var_name);
    var->string = CopyString(var_value);

    if(description) {
        var->description = CopyString(description);
    }

    var->modified = true;
    var->modificationCount = 1;
    float64 strValue = strtod(var->string, nullptr);
    var->value = strValue;
    var->integer = strValue;
    var->resetString = CopyString(var_value);

    // link the variable in
    var->next = cvar_vars;
    cvar_vars = var;

    var->flags = flags;

    hash = generateHashValue(var_name);
    var->hashNext = hashTable[hash];
    hashTable[hash] = var;

    return var;
}

/*
============
idCVarSystemLocal::GetSet2
============
*/
#define FOREIGN_MSG "Foreign characters are not allowed in userinfo variables.\n"
convar_t *idCVarSystemLocal::GetSet2(pointer var_name, pointer value,
                                     bool force) {
    convar_t *var;

    if(strcmp("com_hunkused", var_name) != 0) {
        //Com_DPrintf( "idCVarSystemLocal::Set2: %s %s\n", var_name, value );
    }

    if(!ValidateString(var_name)) {
        Com_Printf("invalid cvar name string: %s\n", var_name);
        var_name = "BADNAME";
    }

    var = FindVar(var_name);

    if(!var) {
        if(!value) {
            return nullptr;
        }

        // create it
        if(!force) {
            return Get(var_name, value, CVAR_USER_CREATED, nullptr);
        } else {
            return Get(var_name, value, 0, nullptr);
        }
    }

    if(!value) {
        value = var->resetString;
    }

    //value = Validate( var, value, true );

    if(var->flags & CVAR_USERINFO) {
        valueType *cleaned = ClearForeignCharacters(value);

        if(strcmp(value, cleaned)) {
#ifdef DEDICATED
            Com_Printf(FOREIGN_MSG);
#else
            Com_Printf("%s", clientLocalizationSystem->TranslateStringBuf(FOREIGN_MSG));
#endif
            Com_Printf("Using %s instead of %s\n", cleaned, value);
            return GetSet2(var_name, cleaned, force);
        }
    }

    if(!strcmp(value, var->string)) {
        if((var->flags & CVAR_LATCH) && var->latchedString) {
            if(!strcmp(value, var->latchedString)) {
                return var;
            }
        } else {
            return var;
        }
    }

    if(!strcmp(value, var->string)) {

        if((var->flags & CVAR_LATCH) && var->latchedString) {
            Com_Printf("Cvar %s is no longer latched to \"%s\".\n", var->name,
                       var->latchedString);
            Z_Free(var->latchedString);
            var->latchedString = nullptr;
            var->modified = true;
            var->modificationCount++;
        }

        return var;
    }

    // note what types of cvars have been modified (userinfo, archive, serverinfo, systeminfo)
    cvar_modifiedFlags |= var->flags;

    if(!force) {
        // ydnar: don't set unsafe variables when com_crashed is set
        if((var->flags & CVAR_UNSAFE) && com_crashed != nullptr &&
                com_crashed->integer) {
            Com_Printf("%s is unsafe. Check com_crashed.\n", var_name);
            return var;
        }

        if(var->flags & CVAR_ROM) {
            Com_Printf("%s is read only.\n", var_name);
            return var;
        }

        if(var->flags & CVAR_INIT) {
            Com_Printf("%s can only be set at startup time by using a \"+set\" command line parameter.\n",
                       var_name);
            return var;
        }

        if((var->flags & CVAR_CHEAT) && !cvar_cheats->integer) {
            Com_Printf("%s is cheat protected.\n", var_name);
            return var;
        }

        if(var->flags & CVAR_SHADER) {
            Com_Printf("%s will be changed upon recompiling shaders.\n", var_name);
            Set("r_recompileShaders", "1");
        }

        if(var->flags & CVAR_LATCH) {
            if(var->latchedString) {
                if(strcmp(value, var->latchedString) == 0) {
                    return var;
                }

                Z_Free(var->latchedString);
            } else {
                if(strcmp(value, var->string) == 0) {
                    return var;
                }
            }

            Com_Printf("%s will be changed to \"%s\" upon restarting.\n", var_name,
                       value);

            var->latchedString = CopyString(value);
            var->modified = true;
            var->modificationCount++;
            return var;
        }
    } else {
        if(var->latchedString) {
            Z_Free(var->latchedString);
            var->latchedString = nullptr;
        }
    }

    if(!strcmp(value, var->string)) {
        return var;             // not changed

    }

    var->modified = true;
    var->modificationCount++;

    Z_Free(var->string);         // free the old value string

    var->string = CopyString(value);
    float64 strValue = strtod(var->string, nullptr);
    var->value = strValue;
    var->integer = strValue;

    return var;
}

/*
============
idCVarSystemLocal::Set
============
*/
void idCVarSystemLocal::Set(pointer var_name, pointer value) {
    GetSet2(var_name, value, true);
}

/*
============
idCVarSystemLocal::SetLatched
============
*/
void idCVarSystemLocal::SetLatched(pointer var_name, pointer value) {
    GetSet2(var_name, value, false);
}

/*
============
idCVarSystemLocal::SetValue
============
*/
void idCVarSystemLocal::SetValue(pointer var_name, float32 value) {
    valueType val[32];

    if(value == static_cast<sint>(value)) {
        Q_vsprintf_s(val, sizeof(val), sizeof(val), "%i",
                     static_cast<sint>(value));
    } else {
        Q_vsprintf_s(val, sizeof(val), sizeof(val), "%f", value);
    }

    Set(var_name, val);
}

/*
============
idCVarSystemLocal::SetValueSafe
============
*/
void idCVarSystemLocal::SetValueSafe(pointer var_name, float32 value) {
    valueType    val[32];

    if(value == static_cast<sint>(value)) {
        Q_vsprintf_s(val, sizeof(val), sizeof(val), "%i",
                     static_cast<sint>(value));
    } else {
        Q_vsprintf_s(val, sizeof(val), sizeof(val), "%f", value);
    }

    GetSet2(var_name, val, false);
}

/*
============
idCVarSystemLocal::SetValueLatched
============
*/
void idCVarSystemLocal::SetValueLatched(pointer var_name, float32 value) {
    valueType val[32];

    if(value == static_cast<sint>(value)) {
        Q_vsprintf_s(val, sizeof(val), sizeof(val), "%i",
                     static_cast<sint>(value));
    } else {
        Q_vsprintf_s(val, sizeof(val), sizeof(val), "%f", value);
    }

    GetSet2(var_name, val, false);
}

/*
============
idCVarSystemLocal::Reset
============
*/
void idCVarSystemLocal::Reset(pointer var_name) {
    GetSet2(var_name, nullptr, false);
}

/*
============
idCVarSystemLocal::SetCheatState

Any testing variables will be reset to the safe values
============
*/
void idCVarSystemLocal::SetCheatState(void) {
    convar_t *var;

    // set all default vars to the safe value
    for(var = cvar_vars; var; var = var->next) {
        if(var->flags & CVAR_CHEAT) {
            if(strcmp(var->resetString, var->string)) {
                Set(var->name, var->resetString);
            }
        }
    }
}

/*
============
idCVarSystemLocal::Command

Handles variable inspection and changing from the console
============
*/
bool idCVarSystemLocal::Command(void) {
    convar_t *v;
    valueType *args = cmdSystem->Args();

    // check variables
    v = FindVar(cmdSystem->Argv(0));

    if(!v) {
        return false;
    }

    // perform a variable print or set
    if(cmdSystem->Argc() == 1) {
        Com_Printf("\"%s\" is:\"%s" S_COLOR_WHITE "\" default:\"%s" S_COLOR_WHITE
                   "\"\n", v->name, v->string, v->resetString);

        if(v->latchedString) {
            Com_Printf("latched: \"%s\"\n", v->latchedString);
        }

        return true;
    }

    // set the value if forcing isn't required
    if(args[0] == '!') {
        GetSet2(v->name, va("%i", !(v->integer)), false);
    } else {
        GetSet2(v->name, args, false);
    }

    return true;
}


/*
============
idCVarSystemLocal::Toggle_f

Toggles a cvar for easy single key binding,
optionally through a list of given values
============
*/
void idCVarSystemLocal::Toggle_f(void) {
    sint i, c;
    pointer varname, curval;

    c = cmdSystem->Argc();

    if(c < 2) {
        Com_Printf("usage: toggle <variable> [<value> ...]\n");
        return;
    }

    varname = cmdSystem->Argv(1);

    if(c == 2) {
        cvarSystemLocal.GetSet2(varname, va("%d",
                                            !cvarSystemLocal.VariableValue(varname)), false);
        return;
    }

    curval = cvarSystemLocal.VariableString(cmdSystem->Argv(1));

    // don't bother checking the last value for a match, since the desired
    //  behaviour is the same as if the last value didn't match:
    //  set the variable to the first value
    for(i = 2; i < c - 1; ++i) {
        if(!strcmp(curval, cmdSystem->Argv(i))) {
            cvarSystemLocal.GetSet2(varname, cmdSystem->Argv(i + 1), false);
            return;
        }
    }

    // fallback
    cvarSystemLocal.GetSet2(varname, cmdSystem->Argv(2), false);
}


/*
============
idCVarSystemLocal::Cycle_f

Cycles a cvar for easy single key binding
============
*/
void idCVarSystemLocal::Cycle_f(void) {
    sint start, end, step, oldvalue, value;

    if(cmdSystem->Argc() < 4 || cmdSystem->Argc() > 5) {
        Com_Printf("usage: cycle <variable> <start> <end> [step]\n");
        return;
    }

    oldvalue = value = cvarSystemLocal.VariableValue(cmdSystem->Argv(1));
    start = atoi(cmdSystem->Argv(2));
    end = atoi(cmdSystem->Argv(3));

    if(cmdSystem->Argc() == 5) {
        step = abs(atoi(cmdSystem->Argv(4)));
    } else {
        step = 1;
    }

    if(abs(end - start) < step) {
        step = 1;
    }

    if(end < start) {
        value -= step;

        if(value < end) {
            value = start - (step - (oldvalue - end + 1));
        }
    } else {
        value += step;

        if(value > end) {
            value = start + (step - (end - oldvalue + 1));
        }
    }

    cvarSystemLocal.GetSet2(cmdSystem->Argv(1), va("%i", value), false);
}

/*
============
idCVarSystemLocal::Set_f

Allows setting and defining of arbitrary cvars from console, even if they
weren't declared in C code.
============
*/
void idCVarSystemLocal::Set_f(void) {
    sint c, unsafe = 0;
    valueType *value;

    c = cmdSystem->Argc();

    if(c < 3) {
        Com_Printf("usage: set <variable> <value> [unsafe]\n");
        return;
    }

    // ydnar: handle unsafe vars
    if(c >= 4 && !strcmp(cmdSystem->Argv(c - 1), "unsafe")) {
        c--;
        unsafe = 1;

        if(com_crashed != nullptr && com_crashed->integer) {
            Com_Printf("%s is unsafe. Check com_crashed.\n", cmdSystem->Argv(1));
            return;
        }
    }

    // 3rd arg onwards, raw
    value = ::strdup(cmdSystem->FromNth(2));

    if(unsafe) {
        valueType *end = value + strlen(value);

        // skip spaces
        while(--end > value) {
            if(*end != ' ') {
                break;
            }
        }

        ++end;

        // skip "unsafe" (may be quoted, so just scan it)
        while(--end > value) {
            if(*end == ' ') {
                break;
            }
        }

        ++end;

        // skip spaces
        while(--end > value) {
            if(*end != ' ') {
                break;
            }
        }

        // end of string :-)
        end[1] = 0;
    }

    cvarSystemLocal.GetSet2(cmdSystem->Argv(1), Com_UnquoteStr(value), false);
    free(value);
}

/*
============
idCVarSystemLocal::SetU_f

As Cvar_Set, but also flags it as serverinfo
============
*/
void idCVarSystemLocal::SetU_f(void) {
    convar_t *v;

    if(cmdSystem->Argc() != 3 && cmdSystem->Argc() != 4) {
        Com_Printf("usage: setu <variable> <value> [unsafe]\n");
        return;
    }

    Set_f();

    v = cvarSystemLocal.FindVar(cmdSystem->Argv(1));

    if(!v) {
        return;
    }

    v->flags |= CVAR_USERINFO;
}

/*
============
idCVarSystemLocal::SetS_f

As Cvar_Set, but also flags it as serverinfo
============
*/
void idCVarSystemLocal::SetS_f(void) {
    convar_t *v;

    if(cmdSystem->Argc() != 3 && cmdSystem->Argc() != 4) {
        Com_Printf("usage: sets <variable> <value> [unsafe]\n");
        return;
    }

    Set_f();

    v = cvarSystemLocal.FindVar(cmdSystem->Argv(1));

    if(!v) {
        return;
    }

    v->flags |= CVAR_SERVERINFO;
}

/*
============
idCVarSystemLocal::SetA_f

As Cvar_Set, but also flags it as archived
============
*/
void idCVarSystemLocal::SetA_f(void) {
    convar_t *v;

    if(cmdSystem->Argc() != 3 && cmdSystem->Argc() != 4) {
        Com_Printf("usage: seta <variable> <value> [unsafe]\n");
        return;
    }

    Set_f();

    v = cvarSystemLocal.FindVar(cmdSystem->Argv(1));

    if(!v) {
        return;
    }

    v->flags |= CVAR_ARCHIVE;
}

/*
============
idCVarSystemLocal::Reset_f
============
*/
void idCVarSystemLocal::Reset_f(void) {
    if(cmdSystem->Argc() != 2) {
        Com_Printf("usage: reset <variable>\n");
        return;
    }

    cvarSystemLocal.Reset(cmdSystem->Argv(1));
}

/*
============
idCVarSystemLocal::WriteVariables

Appends lines containing "set variable value" for all variables
with the archive flag set to true.
============
*/
void idCVarSystemLocal::WriteVariables(fileHandle_t f) {
    convar_t *var;
    valueType buffer[1024];

    for(var = cvar_vars; var; var = var->next) {
        if(var->flags & CVAR_ARCHIVE) {
            // write the latched value, even if it hasn't taken effect yet
            if(var->latchedString) {
                if(strlen(var->name) + strlen(var->latchedString) + 10 > sizeof(buffer)) {
                    Com_Printf(S_COLOR_YELLOW "WARNING: value of variable "
                               "\"%s\" too long to write to file\n", var->name);
                    continue;
                }

                Q_vsprintf_s(buffer, sizeof(buffer), sizeof(buffer), "seta %s \"%s\"\n",
                             var->name, var->latchedString);
            } else {
                if(strlen(var->name) + strlen(var->string) + 10 > sizeof(buffer)) {
                    Com_Printf(S_COLOR_YELLOW "WARNING: value of variable "
                               "\"%s\" too long to write to file\n", var->name);
                    continue;
                }

                Q_vsprintf_s(buffer, sizeof(buffer), sizeof(buffer), "seta %s \"%s\"\n",
                             var->name, var->string);
            }

            fileSystem->Write(buffer, strlen(buffer), f);
        }
    }
}

/*
============
idCVarSystemLocal::List_f
============
*/
void idCVarSystemLocal::List_f(void) {
    sint i;
    convar_t *var;
    valueType *match;

    if(cmdSystem->Argc() > 1) {
        match = cmdSystem->Argv(1);
    } else {
        match = nullptr;
    }

    i = 0;

    for(var = cvar_vars; var; var = var->next, i++) {
        if(match && !Com_Filter(match, var->name, false)) {
            continue;
        }

        if(var->flags & CVAR_SERVERINFO) {
            Com_Printf("S");
        } else {
            Com_Printf(" ");
        }

        if(var->flags & CVAR_USERINFO) {
            Com_Printf("U");
        } else {
            Com_Printf(" ");
        }

        if(var->flags & CVAR_ROM) {
            Com_Printf("R");
        } else {
            Com_Printf(" ");
        }

        if(var->flags & CVAR_INIT) {
            Com_Printf("I");
        } else {
            Com_Printf(" ");
        }

        if(var->flags & CVAR_ARCHIVE) {
            Com_Printf("A");
        } else {
            Com_Printf(" ");
        }

        if(var->flags & CVAR_LATCH) {
            Com_Printf("L");
        } else {
            Com_Printf(" ");
        }

        if(var->flags & CVAR_CHEAT) {
            Com_Printf("C");
        } else {
            Com_Printf(" ");
        }

        Com_Printf(" %s \"%s\"   ^2%s\n", var->name, var->string,
                   var->description);

    }

    Com_Printf("\n%i total convars\n", i);
    Com_Printf("%i convars indexes\n", cvar_numIndexes);
}

/*
============
idCVarSystemLocal::Restart_f

Resets all cvars to their hardcoded values
============
*/
void idCVarSystemLocal::Restart_f(void) {
    convar_t *var;
    convar_t **prev;

    prev = &cvar_vars;

    while(1) {
        var = *prev;

        if(!var) {
            break;
        }

        // don't mess with rom values, or some inter-module
        // communication will get broken (com_cl_running, etc)
        if(var->flags & (CVAR_ROM | CVAR_INIT | CVAR_NORESTART)) {
            prev = &var->next;
            continue;
        }

        // throw out any variables the user created
        if(var->flags & CVAR_USER_CREATED) {
            *prev = var->next;

            if(var->name) {
                Z_Free(var->name);
            }

            if(var->string) {
                Z_Free(var->string);
            }

            if(var->latchedString) {
                Z_Free(var->latchedString);
            }

            if(var->resetString) {
                Z_Free(var->resetString);
            }

            if(var->description) {
                Z_Free(var->description);
            }

            // clear the var completely, since we
            // can't remove the index from the list
            ::memset(var, 0, sizeof(*var));
            continue;
        }

        cvarSystemLocal.Set(var->name, var->resetString);

        prev = &var->next;
    }
}

/*
=====================
idCVarSystemLocal::InfoString
=====================
*/
valueType *idCVarSystemLocal::InfoString(sint bit) {
    static valueType info[MAX_INFO_STRING];
    convar_t *var;

    info[0] = 0;

    for(var = cvar_vars; var; var = var->next) {
        if(var->flags & bit) {
            Info_SetValueForKey(info, var->name, var->string);
        }
    }

    return info;
}

/*
=====================
idCVarSystemLocal::InfoString_Big

  handles large info strings ( CS_SYSTEMINFO )
=====================
*/
valueType *idCVarSystemLocal::InfoString_Big(sint bit) {
    static valueType info[BIG_INFO_STRING];
    convar_t *var;

    info[0] = 0;

    for(var = cvar_vars; var; var = var->next) {
        if(var->flags & bit) {
            Info_SetValueForKey_Big(info, var->name, var->string);
        }
    }

    return info;
}

/*
=====================
idCVarSystemLocal::InfoStringBuffer
=====================
*/
void idCVarSystemLocal::InfoStringBuffer(sint bit, valueType *buff,
        uint64 buffsize) {
    Q_strncpyz(buff, InfoString(bit), buffsize);
}

/*
=====================
idCVarSystemLocal::CheckRange
=====================
*/
void idCVarSystemLocal::CheckRange(convar_t *var, float32 min, float32 max,
                                   bool integral) {
    var->validate = true;
    var->min = min;
    var->max = max;
    var->integral = integral;

    // Force an initial range check
    Set(var->name, var->string);
}

/*
=====================
Cvar_Register

basically a slightly modified Cvar_Get for the interpreted modules
=====================
*/
void idCVarSystemLocal::Register(vmConvar_t *vmCvar, pointer varName,
                                 pointer defaultValue, sint flags, pointer description) {
    convar_t *cv;

    if((flags & (CVAR_ARCHIVE | CVAR_ROM)) == (CVAR_ARCHIVE | CVAR_ROM)) {
        Com_DPrintf(S_COLOR_YELLOW
                    "WARNING: Unsetting CVAR_ROM cvar '%s', since it is also CVAR_ARCHIVE\n",
                    varName);
        flags &= ~CVAR_ROM;
    }

    cv = Get(varName, defaultValue, flags, description);

    if(!vmCvar) {
        return;
    }

    vmCvar->handle = cv - cvar_indexes;
    vmCvar->modificationCount = -1;

    Update(vmCvar);
}

/*
=====================
idCVarSystemLocal::Update

updates an interpreted modules' version of a cvar
=====================
*/
void idCVarSystemLocal::Update(vmConvar_t *vmCvar) {
    convar_t *cv = nullptr; // bk001129

    // bk
    assert(vmCvar);

    if(static_cast< uint >(vmCvar->handle) >= cvar_numIndexes) {
        Com_Error(ERR_DROP, "idCVarSystemLocal::Update: handle %d out of range",
                  static_cast<uint>(vmCvar->handle));
    }

    cv = cvar_indexes + vmCvar->handle;

    if(cv->modificationCount == vmCvar->modificationCount) {
        return;
    }

    if(!cv->string) {
        // variable might have been cleared by a cvar_restart
        return;
    }

    vmCvar->modificationCount = cv->modificationCount;

    // bk001129 - mismatches.
    if(strlen(cv->string) + 1 > MAX_CVAR_VALUE_STRING) {
        Com_Error(ERR_DROP,
                  "idCVarSystemLocal::Update: src %s length %lu exceeds MAX_CVAR_VALUE_STRING(%lu)",
                  cv->string, static_cast<uint32>(::strlen(cv->string)),
                  static_cast<uint32>(sizeof(vmCvar->string)));
    }

    // bk001212 - Q_strncpyz guarantees zero padding and dest[MAX_CVAR_VALUE_STRING-1]==0
    // bk001129 - paranoia. Never trust the destination string.
    // bk001129 - beware, sizeof(valueType*) is always 4 (for cv->string).
    //            sizeof(vmCvar->string) always MAX_CVAR_VALUE_STRING
    //Q_strncpyz( vmCvar->string, cv->string, sizeof( vmCvar->string ) ); // id
    Q_strncpyz(vmCvar->string, cv->string, MAX_CVAR_VALUE_STRING);

    vmCvar->value = cv->value;
    vmCvar->integer = cv->integer;
}

/*
==================
idCVarSystemLocal::CompleteCvarName
==================
*/
void idCVarSystemLocal::CompleteCvarName(valueType *args, sint argNum) {
    if(argNum == 2) {
        // Skip "<cmd> "
        valueType *p = Com_SkipTokens(args, 1, " ");

        if(p > args) {
            cmdCompletionSystem->CompleteCommand(p, false, true);
        }
    }
}

/*
============
idCVarSystemLocal::Init

Reads in all archived cvars
============
*/
void idCVarSystemLocal::Init(void) {
    ::memset(cvar_indexes, '\0', sizeof(cvar_indexes));
    ::memset(hashTable, '\0', sizeof(hashTable));

    cvar_cheats = Get("sv_cheats", "1", CVAR_ROM | CVAR_SYSTEMINFO,
                      "Indicates whether cheats are enabled or not");

    cmdSystem->AddCommand("toggle", &idCVarSystemLocal::Toggle_f,
                          "Toggles a cvar for easy single key binding, optionally through a list of given values");
    cmdSystem->SetCommandCompletionFunc("toggle",
                                        &idCVarSystemLocal::CompleteCvarName);

    cmdSystem->AddCommand("cycle", &idCVarSystemLocal::Cycle_f,
                          "For cycling a specified cvar through the specified values.Bind x “cycle s_mute 0 1");  // ydnar
    cmdSystem->SetCommandCompletionFunc("cycle",
                                        &idCVarSystemLocal::CompleteCvarName);

    cmdSystem->AddCommand("set", &idCVarSystemLocal::Set_f,
                          "To set some cvar to the specified value, e.g. / set r_gamma 1");
    cmdSystem->SetCommandCompletionFunc("set",
                                        &idCVarSystemLocal::CompleteCvarName);

    cmdSystem->AddCommand("sets", &idCVarSystemLocal::SetS_f,
                          "Specifies it is to be shown in serverinfo. Often doesnt have any effect on game");
    cmdSystem->SetCommandCompletionFunc("sets",
                                        &idCVarSystemLocal::CompleteCvarName);

    cmdSystem->AddCommand("setu", &idCVarSystemLocal::SetU_f,
                          "To set some cvar to the specified value but also flags it as serverinfo");
    cmdSystem->SetCommandCompletionFunc("setu",
                                        &idCVarSystemLocal::CompleteCvarName);

    cmdSystem->AddCommand("seta", &idCVarSystemLocal::SetA_f,
                          "To set some cvar to the specified value but also flags it as archived");
    cmdSystem->SetCommandCompletionFunc("seta",
                                        &idCVarSystemLocal::CompleteCvarName);

    cmdSystem->AddCommand("reset", &idCVarSystemLocal::Reset_f,
                          "Reseting a variable");
    cmdSystem->SetCommandCompletionFunc("reset",
                                        &cvarSystemLocal.CompleteCvarName);

    cmdSystem->AddCommand("cvarlist", &cvarSystemLocal.List_f,
                          "Lists all cvars in the console");
    cmdSystem->AddCommand("cvar_restart", &cvarSystemLocal.Restart_f,
                          "Returns all console variables to their default settings. Does not affect key bindings. ");
}

/*
============
idCVarSystemLocal::Init

Reads in all archived cvars
============
*/
void idCVarSystemLocal::Shutdown(void) {
    cmdSystem->RemoveCommand("toggle");
    cmdSystem->RemoveCommand("cycle");
    cmdSystem->RemoveCommand("set");
    cmdSystem->RemoveCommand("sets");
    cmdSystem->RemoveCommand("setu");
    cmdSystem->RemoveCommand("seta");
    cmdSystem->RemoveCommand("reset");
    cmdSystem->RemoveCommand("cvarlist");

}
