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
// File name:   dl_local.h
// Version:     v1.02
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __DOWNLOADLOCAL_H__
#define __DOWNLOADLOCAL_H__

#define APP_NAME "ID_DOWNLOAD"
#define APP_VERSION "2.0"

// initialize once
static S32 dl_initialized = 0;

static CURLM* dl_multi = nullptr;
static CURL* dl_request = nullptr;
static FILE* dl_file = nullptr;

//
// idServerCcmdsSystemLocal
//
class idDownloadSystemLocal : public idDownloadSystem
{
public:
    idDownloadSystemLocal();
    ~idDownloadSystemLocal();
    
    virtual void InitDownload( void );
    virtual void Shutdown( void );
    virtual S32 BeginDownload( StringEntry localName, StringEntry remoteName, S32 debug );
    virtual dlStatus_t DownloadLoop( void );
    
    static U64 FWriteFile( void* ptr, U64 size, U64 nmemb, void* stream );
    static S32 Progress( void* clientp, F64 dltotal, F64 dlnow, F64 ultotal, F64 ulnow );
};

extern idDownloadSystemLocal downloadLocal;

#endif //!__DOWNLOADLOCAL_H__
