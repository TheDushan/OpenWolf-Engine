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
// File name:   serverClient.h
// Version:     v1.01
// Created:
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __SERVERCLIENT_H__
#define __SERVERCLIENT_H__

//
// idServerClientSystemLocal
//
class idServerClientSystemLocal : public idServerClientSystem
{
public:
    idServerClientSystemLocal();
    ~idServerClientSystemLocal();
    
    virtual void DropClient( client_t* drop, StringEntry reason );
    virtual void ExecuteClientCommand( client_t* cl, StringEntry s, bool clientOK, bool premaprestart );
    virtual void ClientEnterWorld( client_t* client, usercmd_t* cmd );
    virtual void CloseDownload( client_t* cl );
    virtual void UserinfoChanged( client_t* cl );
    virtual void FreeClient( client_t* client );
    virtual void ClientThink( S32 client, usercmd_t* cmd );
    virtual void WriteDownloadToClient( client_t* cl, msg_t* msg );
    virtual void GetChallenge( netadr_t from );
    virtual void DirectConnect( netadr_t from );
    virtual void ExecuteClientMessage( client_t* cl, msg_t* msg );
    
public:
    static bool CheckFunstuffExploit( UTF8* userinfo, UTF8* key );
    static void UpdateUserinfo_f( client_t* cl );
    static void SendClientGameState( client_t* client );
    static void StopDownload_f( client_t* cl );
    static void DoneDownload_f( client_t* cl );
    static void NextDownload_f( client_t* cl );
    static void BeginDownload_f( client_t* cl );
    static void WWWDownload_f( client_t* cl );
    static void BadDownload( client_t* cl, msg_t* msg );
    static bool CheckFallbackURL( client_t* cl, msg_t* msg );
    static void Disconnect_f( client_t* cl );
    static void VerifyPaks_f( client_t* cl );
    static void ResetPureClient_f( client_t* cl );
    static bool ClientCommand( client_t* cl, msg_t* msg, bool premaprestart );
    static void UserMove( client_t* cl, msg_t* msg, bool delta );
    static void SendUserinfoToAuthServer( StringEntry userinfo );
    
private:
    // The value below is how many extra characters we reserve for every instance of '$' in a
    // ut_radio, say, or similar client command.  Some jump maps have very long $location's.
    // On these maps, it may be possible to crash the server if a carefully-crafted
    // client command is sent.  The constant below may require further tweaking.  For example,
    // a text of "$location" would have a total computed length of 25, because "$location" has
    // 9 characters, and we increment that by 16 for the '$'.
    static const S32 STRLEN_INCREMENT_PER_DOLLAR_VAR = 16;
    
    // Don't allow more than this many dollared-strings (e.g. $location) in a client command
    // such as ut_radio and say.  Keep this value low for safety, in case some things like
    // $location expand to very large strings in some maps.  There is really no reason to have
    // more than 6 dollar vars (such as $weapon or $location) in things you tell other people.
    static const S32 MAX_DOLLAR_VARS = 6;
    
    // When a radio text (as in "ut_radio 1 1 text") is sent, weird things start to happen
    // when the text gets to be greater than 118 in length.  When the text is really large the
    // server will crash.  There is an in-between gray zone above 118, but I don't really want
    // to go there.  This is the maximum length of radio text that can be sent, taking into
    // account increments due to presence of '$'.
    static const S32 MAX_RADIO_STRLEN = 118;
    
    // Don't allow more than this text length in a command such as say.  I pulled this
    // value out of my ass because I don't really know exactly when problems start to happen.
    // This value takes into account increments due to the presence of '$'.
    static const S32 MAX_SAY_STRLEN = 256;
};

extern idServerClientSystemLocal serverClientLocal;

#endif //!__SERVERCLIENT_H__
