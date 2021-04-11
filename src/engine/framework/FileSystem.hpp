////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2018 - 2021 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   FileSystem.hpp
// Created:     04/11/2018
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description: virtual file system
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __VFILES_H__
#define __VFILES_H__

/*
==============================================================

FILESYSTEM

No stdio calls should be used by any part of the game, because
we need to deal with all sorts of directory and seperator valueType
issues.
==============================================================
*/

#define MAX_FILE_HANDLES 64

#define MAX_ZPATH 256
#define MAX_SEARCH_PATHS 4096
#define MAX_FILEHASH_SIZE 1024

typedef struct
{
    valueType path[MAX_OSPATH]; // c:\quake3
    valueType fullpath[MAX_OSPATH];
    valueType gamedir[MAX_OSPATH]; // baseq3
} directory_t;

typedef struct searchpath_s
{
    struct searchpath_s* next;
    
    pack_t* pack; // only one of pack / dir will be non nullptr
    directory_t* dir;
} searchpath_t;

typedef union qfile_gus
{
    FILE* o;
    unzFile z;
} qfile_gut;

typedef struct qfile_us
{
    qfile_gut file;
    bool unique;
} qfile_ut;

typedef struct
{
    qfile_ut handleFiles;
    bool handleSync;
    sint baseOffset;
    sint fileSize;
    sint zipFilePos;
    bool zipFile;
    bool streamed;
    valueType name[MAX_ZPATH];
} fileHandleData_t;

static fileHandleData_t fsh[MAX_FILE_HANDLES];

// TTimo - show_bug.cgi?id=540
// wether we did a reorder on the current search path when joining the server
static bool fs_reordered;

// never load anything from pk3 files that are not present at the server when pure
// ex: when fs_numServerPaks != 0, FS_FOpenFileRead won't load anything outside of pk3 except .cfg .menu .game .dat
static sint fs_numServerPaks;
static sint fs_serverPaks[MAX_SEARCH_PATHS]; // checksums
static valueType* fs_serverPakNames[MAX_SEARCH_PATHS]; // pk3 names

// only used for autodownload, to make sure the client has at least
// all the pk3 files that are referenced at the server side
static sint fs_numServerReferencedPaks;
static sint fs_serverReferencedPaks[MAX_SEARCH_PATHS]; // checksums
static valueType* fs_serverReferencedPakNames[MAX_SEARCH_PATHS]; // pk3 names

// last valid game folder used
static valueType lastValidBase[MAX_OSPATH];
static valueType lastValidGame[MAX_OSPATH];

static bool legacy_bin = false;

// referenced flags
// these are in loop specific order so don't change the order
#define FS_GENERAL_REF  0x01
#define FS_UI_REF       0x02
#define FS_CGAME_REF    0x04
#define FS_QAGAME_REF   0x08
// number of id paks that will never be autodownloaded from baseq3
#define NUM_ID_PAKS     9

#ifdef WIN32
#define Q_rmdir _rmdir
#else
#define Q_rmdir rmdir
#endif

extern bool com_fullyInitialized;

// see idFileSystemLocal::FOpenFileRead_Filtered
static sint fs_filter_flag = 0;

#define PK3_SEEK_BUFFER_SIZE 65536

typedef struct
{
    valueType pakname[MAX_QPATH];
    bool ok;
} officialpak_t;

#define FS_EXCLUDE_DIR 0x1
#define FS_EXCLUDE_PK3 0x2

//
// idFileSystemLocal
//
class idFileSystemLocal : public idFileSystem
{
public:
    idFileSystemLocal( void );
    ~idFileSystemLocal( void );
    
    virtual bool Initialized( void );
    virtual bool PakIsPure( pack_t* pack );
    virtual sint LoadStack( void );
    virtual sint32 HashFileName( pointer fname, uint64 hashSize );
    virtual fileHandle_t HandleForFile( void );
    virtual FILE* FileForHandle( fileHandle_t f );
    virtual void ForceFlush( fileHandle_t f );
    virtual sint filelength( fileHandle_t f );
    virtual void ReplaceSeparators( valueType* path );
    virtual valueType* BuildOSPath( pointer base, pointer game, pointer qpath );
    virtual void BuildOSHomePath( valueType* ospath, sint size, sint qpath );
    virtual sint CreatePath( pointer OSPath_ );
    virtual void FSCopyFile( valueType* fromOSPath, valueType* toOSPath );
    virtual bool Remove( pointer osPath );
    virtual void HomeRemove( pointer homePath );
    virtual bool FileExists( pointer file );
    virtual bool SV_FileExists( pointer file );
    virtual bool OS_FileExists( pointer file );
    virtual fileHandle_t SV_FOpenFileWrite( pointer filename );
    virtual sint SV_FOpenFileRead( pointer filename, fileHandle_t* fp );
    virtual void SV_Rename( pointer from, pointer to );
    virtual void Rename( pointer from, pointer to );
    virtual void FCloseFile( fileHandle_t f );
    virtual fileHandle_t FOpenFileWrite( pointer filename );
    virtual fileHandle_t FOpenFileAppend( pointer filename );
    virtual sint FOpenFileDirect( pointer filename, fileHandle_t* f );
    virtual fileHandle_t FOpenFileUpdate( pointer filename, sint* length );
    virtual bool FilenameCompare( pointer s1, pointer s2 );
    virtual valueType* ShiftedStrStr( pointer string, pointer substring, sint shift );
    virtual valueType* ShiftStr( pointer string, sint shift );
    virtual sint FOpenFileRead( pointer filename, fileHandle_t* file, bool uniqueFILE );
    virtual sint FOpenFileRead_Filtered( pointer qpath, fileHandle_t* file, bool uniqueFILE, sint filter_flag );
    virtual bool CL_ExtractFromPakFile( pointer base, pointer gamedir, pointer filename );
    virtual bool AllowDeletion( valueType* filename );
    virtual sint DeleteDir( valueType* dirname, bool nonEmpty, bool recursive );
    virtual sint OSStatFile( valueType* ospath );
    virtual sint Delete( valueType* filename );
    virtual sint FPrintf( fileHandle_t f, pointer fmt, ... );
    virtual sint Read2( void* buffer, sint len, fileHandle_t f );
    virtual sint Read( void* buffer, sint len, fileHandle_t f );
    virtual sint Write( const void* buffer, sint len, fileHandle_t h );
    virtual void Printf( fileHandle_t h, pointer fmt, ... );
    virtual sint Seek( fileHandle_t f, sint32 offset, sint origin );
    virtual sint FileIsInPAK( pointer filename, sint* pChecksum );
    virtual sint ReadFile( pointer qpath, void** buffer );
    virtual void FreeFile( void* buffer );
    virtual void WriteFile( pointer qpath, const void* buffer, sint size );
    virtual pack_t* LoadZipFile( pointer zipfile, pointer basename );
    virtual sint ReturnPath( pointer zname, valueType* zpath, sint* depth );
    virtual sint AddFileToList( valueType* name, valueType* list[MAX_FOUND_FILES], sint nfiles );
    virtual valueType** ListFilteredFiles( pointer path, pointer extension, valueType* filter, uint64* numfiles );
    virtual valueType** ListFiles( pointer path, pointer extension, uint64* numfiles );
    virtual void FreeFileList( valueType** list );
    virtual uint64 GetFileList( pointer path, pointer extension, valueType* listbuf, uint64 bufsize );
    virtual uint CountFileList( valueType** list );
    virtual valueType** ConcatenateFileLists( valueType** list0, valueType** list1, valueType** list2 );
    virtual uint64 GetModList( valueType* listbuf, uint64 bufsize );
    static void Dir_f( void );
    virtual void ConvertPath( valueType* s );
    virtual sint PathCmp( pointer s1, pointer s2 );
    virtual void SortFileList( valueType** filelist, uint64 numfiles );
    static void NewDir_f( void );
    static void Path_f( void );
    static void TouchFile_f( void );
    static void Which_f( void );
    static sint paksort( const void* a, const void* b );
    virtual bool IsExt( pointer filename, pointer ext, uint64 namelen );
    static void AddGameDirectory( pointer path, pointer dir );
    virtual bool idPak( valueType* pak, valueType* base );
    virtual bool VerifyOfficialPaks( void );
    virtual bool ComparePaks( valueType* neededpaks, sint len, bool dlstring );
    virtual void Shutdown( bool closemfp );
    virtual void ReorderPurePaks( void );
    virtual void Startup( pointer gameName );
    virtual pointer GamePureChecksum( void );
    virtual pointer LoadedPakChecksums( void );
    virtual pointer LoadedPakNames( void );
    virtual pointer LoadedPakPureChecksums( void );
    virtual pointer ReferencedPakChecksums( void );
    virtual pointer ReferencedPakNames( void );
    virtual pointer ReferencedPakPureChecksums( void );
    virtual void ClearPakReferences( sint flags );
    virtual void PureServerSetLoadedPaks( pointer pakSums, pointer pakNames );
    virtual void PureServerSetReferencedPaks( pointer pakSums, pointer pakNames );
    virtual void InitFilesystem( void );
    virtual void Restart( sint checksumFeed );
    virtual bool ConditionalRestart( sint checksumFeed );
    virtual sint FOpenFileByMode( pointer qpath, fileHandle_t* f, fsMode_t mode );
    virtual sint FTell( fileHandle_t f );
    virtual void Flush( fileHandle_t f );
    virtual bool VerifyPak( pointer pak );
    virtual bool IsPure( void );
    virtual uint ChecksumOSPath( valueType* OSPath );
    virtual void FilenameCompletion( pointer dir, pointer ext, bool stripExt, void( *callback )( pointer s ) );
    virtual pointer GetGameDir( void );
    virtual bool IsFileEmpty( valueType* filename );
    virtual valueType* GetFullGamePath( valueType* filename );
    virtual void Rmdir( pointer osPath, bool recursive );
    virtual void HomeRmdir( pointer homePath, bool recursive );
    
    static void Restart_f( void );
};

extern idFileSystemLocal fileSystemLocal;

#endif // !__VFILES_H__
