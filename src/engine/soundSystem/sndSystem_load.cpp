////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 1999 - 2010 id Software LLC, a ZeniMax Media company.
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
// File name:   sndSystem_load.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description: Loads a sound module, and redirects calls to it
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <framework/precompiled.hpp>

static bool useBuiltin = false;
static void *openALModule;

idAudioOpenALSystem *soundOpenALSystem;
idAudioOpenALSystem *(*openALEntry)(openALImports_t *openalimports);
static openALImports_t exports;

/*
===============
SndPrintf
Glue
===============
*/
void QDECL idSoundSystemLocal::SndPrintf(sint print_level,
        pointer fmt, ...) {
    va_list     argptr;
    valueType       msg[MAXPRINTMSG];

    va_start(argptr, fmt);
    Q_vsprintf_s(msg, sizeof(msg), fmt, argptr);
    va_end(argptr);

    if(print_level == PRINT_ALL) {
        common->Printf("%s", msg);
    } else if(print_level == PRINT_WARNING) {
        // yellow
        common->Printf(S_COLOR_YELLOW "%s", msg);
    }

    if(developer->integer) {
        if(print_level == PRINT_DEVELOPER) {
            // red
            common->Printf(S_COLOR_RED "%s", msg);
        }
    }
}

/*
 * Routing, runtime loading
 */

/*
===============
S_StartCapture
===============
*/
void *Snd_Malloc(uint64 size) {
    return memorySystem->Malloc(size);
}

/*
===============
Snd_Free
===============
*/
void Snd_Free(void *ptr) {
    return memorySystem->Free(ptr);
}

/*
====================
idClientGameSystemLocal::CreateExportTable
====================
*/
static void CreateExportTable(void) {
    exports.soundOpenALSystem = soundOpenALSystem;
    exports.soundSystem = soundSystem;
    exports.fileSystem = fileSystem;
    exports.cvarSystem = cvarSystem;
    exports.cmdSystem = cmdSystem;
    exports.idsystem = idsystem;
    exports.parseSystem = ParseSystem;
    exports.memorySystem = memorySystem;
    exports.common = common;
}

/*
===============
S_InitModule
===============
*/
static bool S_InitModule(void) {
    valueType fn[1024];

    common->Printf("using sound module %s\n", s_module->string);

    ::snprintf(fn, sizeof(fn), "%s/soundSystem%s." ARCH_STRING DLL_EXT,
               idsystem->Cwd(),
               s_module->string);

    if((openALModule = SDL_LoadObject(fn)) == 0) {
        common->Printf("can't load sound module - bailing\n");
        common->Printf("------------------------------------\n");
        return false;
    }

    // Load in the entry point.
    openALEntry = (idAudioOpenALSystem * (QDECL *)(openALImports_t *))
                  idsystem->GetProcAddress(openALModule, "GetSndAPI");

    if(!openALEntry) {
        common->Error(ERR_DROP, "error loading entry point on openAL.\n");
    }

    if(!openALModule) {
        SDL_UnloadObject(openALModule);
        openALModule = nullptr;
        common->Printf("can't find GetSndAPI - bailing\n");
        common->Printf("------------------------------------\n");
        return false;
    }

    CreateExportTable();

    // Call the dll entry point.
    if(openALEntry) {
        soundOpenALSystem = openALEntry(&exports);
    }

    if(!soundOpenALSystem->Init()) {
        SDL_UnloadObject(openALModule);
        openALModule = nullptr;
        soundOpenALSystem = nullptr;
        common->Printf("call to Init failed - bailing\n");
        common->Printf("------------------------------------\n");
        return false;
    }

    return true;
}

/*
===============
idSoundSystemLocal::Init
===============
*/
void idSoundSystemLocal::Init(void) {
    convar_t *cv;

    common->Printf("------ Initializing Sound -----\n");

    cv = cvarSystem->Get("s_initsound", "1", 0,
                         "Toggle weather sound is initialized or not (on next game)");

    if(!cv->integer) {
        common->Printf("idSoundSystemLocal::Init - Sound disabled.\n");
        common->Printf("------------------------------------\n");
        useBuiltin = true;
    }
    else
    {
        codec_init();

        cv = cvarSystem->Get("s_usemodule", "1", CVAR_ARCHIVE | CVAR_LATCH,
            "Toggle using a sound system module.");

        useBuiltin = false;

        if (!S_InitModule()) {
            useBuiltin = true;
        }

        if (useBuiltin) {
            common->Printf("using builtin sound system\n");
            SOrig_Init();
        }
    }

    common->Printf("------------------------------------\n");
}

/*
===============
idSoundSystemLocal::Shutdown
===============
*/
void idSoundSystemLocal::Shutdown(void) {
    if(useBuiltin) {
        SOrig_Shutdown();
    } else {
        if(openALModule == nullptr) {
            return;
        }

        soundOpenALSystem->Shutdown();
        SDL_UnloadObject(openALModule);
        openALModule = nullptr;
    }

    codec_shutdown();
}

/*
===============
idSoundSystemLocal::StartSound
===============
*/
void idSoundSystemLocal::StartSound(vec3_t origin, sint entnum,
                                    sint entchannel, sfxHandle_t sfx) {
    if(com_minimized->integer || com_unfocused->integer) {
        return;
    }

    if(useBuiltin) {
        SOrig_StartSound(origin, entnum, entchannel, sfx);
    } else {
        soundOpenALSystem->StartSound(origin, entnum, entchannel, sfx);
    }
}

/*
===============
idSoundSystemLocal::StartLocalSound
===============
*/
void idSoundSystemLocal::StartLocalSound(sfxHandle_t sfx,
        sint channelNum) {
    if(useBuiltin) {
        SOrig_StartLocalSound(sfx, channelNum);
    } else {
        soundOpenALSystem->StartLocalSound(sfx, channelNum);
    }
}

/*
===============
idSoundSystemLocal::StartBackgroundTrack
===============
*/
void idSoundSystemLocal::StartBackgroundTrack(pointer intro,
        pointer loop) {
    if(useBuiltin) {
        SOrig_StartBackgroundTrack(intro, loop);
    } else {
        soundOpenALSystem->StartBackgroundTrack(intro, loop);
    }
}

/*
===============
idSoundSystemLocal::StopBackgroundTrack
===============
*/
void idSoundSystemLocal::StopBackgroundTrack(void) {
    if(useBuiltin) {
        SOrig_StopBackgroundTrack();
    } else {
        soundOpenALSystem->StopBackgroundTrack();
    }
}

/*
===============
idSoundSystemLocal::RawSamples
===============
*/
void idSoundSystemLocal::RawSamples(sint stream, sint samples, sint rate,
                                    sint width, sint channels, const uchar8 *data, float32 volume,
                                    sint entityNum) {
    if(useBuiltin) {
        SOrig_RawSamples(stream, samples, rate, width, channels, data, volume,
                         entityNum);
    } else {
        soundOpenALSystem->RawSamples(stream, samples, rate, width, channels, data,
                                      volume, entityNum);
    }
}

/*
===============
idSoundSystemLocal::StartCapture
===============
*/
void idSoundSystemLocal::StopAllSounds(void) {
    if(useBuiltin) {
        SOrig_StopAllSounds();
    } else {
        soundOpenALSystem->StopAllSounds();
    }
}

/*
===============
idSoundSystemLocal::ClearLoopingSounds
===============
*/
void idSoundSystemLocal::ClearLoopingSounds(bool killall) {
    if(useBuiltin) {
        SOrig_ClearLoopingSounds(killall);
    } else {
        soundOpenALSystem->ClearLoopingSounds(killall);
    }
}

/*
===============
idSoundSystemLocal::AddLoopingSound
===============
*/
void idSoundSystemLocal::AddLoopingSound(sint entityNum,
        const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx) {
    if(useBuiltin) {
        SOrig_AddLoopingSound(entityNum, origin, velocity, sfx);
    } else {
        soundOpenALSystem->AddLoopingSound(entityNum, origin, velocity, sfx);
    }
}

/*
===============
idSoundSystemLocal::AddRealLoopingSound
===============
*/
void idSoundSystemLocal::AddRealLoopingSound(sint entityNum,
        const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx) {
    if(useBuiltin) {
        SOrig_AddRealLoopingSound(entityNum, origin, velocity, sfx);
    } else {
        soundOpenALSystem->AddRealLoopingSound(entityNum, origin, velocity, sfx);
    }
}

/*
===============
idSoundSystemLocal::StopLoopingSound
===============
*/
void idSoundSystemLocal::StopLoopingSound(sint entityNum) {
    if(useBuiltin) {
        SOrig_StopLoopingSound(entityNum);
    } else {
        soundOpenALSystem->StopLoopingSound(entityNum);
    }
}

/*
===============
idSoundSystemLocal::Respatialize
===============
*/
void idSoundSystemLocal::Respatialize(sint entityNum, const vec3_t origin,
                                      vec3_t axis[3], sint inwater) {
    if(useBuiltin) {
        SOrig_Respatialize(entityNum, origin, axis, inwater);
    } else {
        soundOpenALSystem->Respatialize(entityNum, origin, axis, inwater);
    }
}

/*
===============
idSoundSystemLocal::UpdateEntityPosition
===============
*/
void idSoundSystemLocal::UpdateEntityPosition(sint entityNum,
        const vec3_t origin) {
    if(useBuiltin) {
        SOrig_UpdateEntityPosition(entityNum, origin);
    } else {
        soundOpenALSystem->UpdateEntityPosition(entityNum, origin);
    }
}

/*
===============
idSoundSystemLocal::Update
===============
*/
void idSoundSystemLocal::Update(void) {
    if(useBuiltin) {
        SOrig_Update();
    } else {
        soundOpenALSystem->Update();
    }
}

/*
===============
idSoundSystemLocal::DisableSounds
===============
*/
void idSoundSystemLocal::DisableSounds(void) {
    if(useBuiltin) {
        SOrig_DisableSounds();
    } else {
        soundOpenALSystem->DisableSounds();
    }
}

/*
===============
idSoundSystemLocal::BeginRegistration
===============
*/
void idSoundSystemLocal::BeginRegistration(void) {
    if(useBuiltin) {
        SOrig_BeginRegistration();
    } else {
        soundOpenALSystem->BeginRegistration();
    }
}

/*
===============
idSoundSystemLocal::RegisterSound
===============
*/
sfxHandle_t idSoundSystemLocal::RegisterSound(pointer sample,
        bool compressed) {
    if(useBuiltin) {
        return SOrig_RegisterSound(sample, compressed);
    } else {
        return soundOpenALSystem->RegisterSound(sample, compressed);
    }
}

/*
===============
idSoundSystemLocal::ClearSoundBuffer
===============
*/
void idSoundSystemLocal::ClearSoundBuffer(void) {
    if(useBuiltin) {
        SOrig_ClearSoundBuffer();
    } else {
        soundOpenALSystem->ClearSoundBuffer();
    }
}

/*
=================
idSoundSystemLocal::SoundDuration
=================
*/
sint idSoundSystemLocal::SoundDuration(sfxHandle_t handle) {
    if(useBuiltin) {
        return SOrig_SoundDuration(handle);
    } else {
        return soundOpenALSystem->SoundDuration(handle);
    }
}

/*
=================
idSoundSystemLocal::GetVoiceAmplitude
=================
*/
sint idSoundSystemLocal::GetVoiceAmplitude(sint entnum) {
    if(useBuiltin) {
        return SOrig_GetVoiceAmplitude(entnum);
    } else {
        return soundOpenALSystem->GetVoiceAmplitude(entnum);
    }
}

/*
=================
S_GetSoundLength
=================
*/
sint idSoundSystemLocal::GetSoundLength(sfxHandle_t sfxHandle) {
    if(useBuiltin) {
        return SOrig_GetSoundLength(sfxHandle);
    } else {
        return soundOpenALSystem->GetSoundLength(sfxHandle);
    }
}

/*
=================
S_GetCurrentSoundTime
=================
*/
sint idSoundSystemLocal::GetCurrentSoundTime(void) {
    if(useBuiltin) {
        return SOrig_GetCurrentSoundTime();
    } else {
        return soundOpenALSystem->GetCurrentSoundTime();
    }
}