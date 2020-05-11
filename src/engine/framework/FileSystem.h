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
// File name:   FileSystemv.h
// Version:     v1.00
// Created:     04/11/2018
// Compilers:   Visual Studio 2019, gcc 7.3.0
// Description: virtual file system
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __VFILES_H__
#define __VFILES_H__

/*
==============================================================

FILESYSTEM

No stdio calls should be used by any part of the game, because
we need to deal with all sorts of directory and seperator UTF8
issues.
==============================================================
*/

#define MAX_FILE_HANDLES 64

#define MAX_ZPATH 256
#define MAX_SEARCH_PATHS 4096
#define MAX_FILEHASH_SIZE 1024

typedef struct
{
    UTF8 path[MAX_OSPATH]; // c:\quake3
    UTF8 fullpath[MAX_OSPATH];
    UTF8 gamedir[MAX_OSPATH]; // baseq3
} directory_t;

typedef struct searchpath_s
{
    struct searchpath_s* next;
    
    pack_t* pack; // only one of pack / dir will be non NULL
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
    S32 baseOffset;
    S32 fileSize;
    S32 zipFilePos;
    bool zipFile;
    bool streamed;
    UTF8 name[MAX_ZPATH];
} fileHandleData_t;

static fileHandleData_t fsh[MAX_FILE_HANDLES];

// TTimo - show_bug.cgi?id=540
// wether we did a reorder on the current search path when joining the server
static bool fs_reordered;

// never load anything from pk3 files that are not present at the server when pure
// ex: when fs_numServerPaks != 0, FS_FOpenFileRead won't load anything outside of pk3 except .cfg .menu .game .dat
static S32 fs_numServerPaks;
static S32 fs_serverPaks[MAX_SEARCH_PATHS]; // checksums
static UTF8* fs_serverPakNames[MAX_SEARCH_PATHS]; // pk3 names

// only used for autodownload, to make sure the client has at least
// all the pk3 files that are referenced at the server side
static S32 fs_numServerReferencedPaks;
static S32 fs_serverReferencedPaks[MAX_SEARCH_PATHS]; // checksums
static UTF8* fs_serverReferencedPakNames[MAX_SEARCH_PATHS]; // pk3 names

// last valid game folder used
static UTF8 lastValidBase[MAX_OSPATH];
static UTF8 lastValidGame[MAX_OSPATH];

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
static S32 fs_filter_flag = 0;

#define PK3_SEEK_BUFFER_SIZE 65536

typedef struct
{
    UTF8 pakname[MAX_QPATH];
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
    virtual S32 LoadStack( void );
    virtual S64 HashFileName( StringEntry fname, S32 hashSize );
    virtual fileHandle_t HandleForFile( void );
    virtual FILE* FileForHandle( fileHandle_t f );
    virtual void ForceFlush( fileHandle_t f );
    virtual S32 filelength( fileHandle_t f );
    virtual void ReplaceSeparators( UTF8* path );
    virtual UTF8* BuildOSPath( StringEntry base, StringEntry game, StringEntry qpath );
    virtual void BuildOSHomePath( UTF8* ospath, S32 size, S32 qpath );
    virtual S32 CreatePath( StringEntry OSPath_ );
    virtual void FSCopyFile( UTF8* fromOSPath, UTF8* toOSPath );
    virtual bool Remove( StringEntry osPath );
    virtual void HomeRemove( StringEntry homePath );
    virtual bool FileExists( StringEntry file );
    virtual bool SV_FileExists( StringEntry file );
    virtual bool OS_FileExists( StringEntry file );
    virtual fileHandle_t SV_FOpenFileWrite( StringEntry filename );
    virtual S32 SV_FOpenFileRead( StringEntry filename, fileHandle_t* fp );
    virtual void SV_Rename( StringEntry from, StringEntry to );
    virtual void Rename( StringEntry from, StringEntry to );
    virtual void FCloseFile( fileHandle_t f );
    virtual fileHandle_t FOpenFileWrite( StringEntry filename );
    virtual fileHandle_t FOpenFileAppend( StringEntry filename );
    virtual S32 FOpenFileDirect( StringEntry filename, fileHandle_t* f );
    virtual fileHandle_t FOpenFileUpdate( StringEntry filename, S32* length );
    virtual bool FilenameCompare( StringEntry s1, StringEntry s2 );
    virtual UTF8* ShiftedStrStr( StringEntry string, StringEntry substring, S32 shift );
    virtual UTF8* ShiftStr( StringEntry string, S32 shift );
    virtual S32 FOpenFileRead( StringEntry filename, fileHandle_t* file, bool uniqueFILE );
    virtual S32 FOpenFileRead_Filtered( StringEntry qpath, fileHandle_t* file, bool uniqueFILE, S32 filter_flag );
    virtual bool CL_ExtractFromPakFile( StringEntry base, StringEntry gamedir, StringEntry filename );
    virtual bool AllowDeletion( UTF8* filename );
    virtual S32 DeleteDir( UTF8* dirname, bool nonEmpty, bool recursive );
    virtual S32 OSStatFile( UTF8* ospath );
    virtual S32 Delete( UTF8* filename );
    virtual S32 FPrintf( fileHandle_t f, StringEntry fmt, ... );
    virtual S32 Read2( void* buffer, S32 len, fileHandle_t f );
    virtual S32 Read( void* buffer, S32 len, fileHandle_t f );
    virtual S32 Write( const void* buffer, S32 len, fileHandle_t h );
    virtual void Printf( fileHandle_t h, StringEntry fmt, ... );
    virtual S32 Seek( fileHandle_t f, S64 offset, S32 origin );
    virtual S32 FileIsInPAK( StringEntry filename, S32* pChecksum );
    virtual S32 ReadFile( StringEntry qpath, void** buffer );
    virtual void FreeFile( void* buffer );
    virtual void WriteFile( StringEntry qpath, const void* buffer, S32 size );
    virtual pack_t* LoadZipFile( StringEntry zipfile, StringEntry basename );
    virtual S32 ReturnPath( StringEntry zname, UTF8* zpath, S32* depth );
    virtual S32 AddFileToList( UTF8* name, UTF8* list[MAX_FOUND_FILES], S32 nfiles );
    virtual UTF8** ListFilteredFiles( StringEntry path, StringEntry extension, UTF8* filter, S32* numfiles );
    virtual UTF8** ListFiles( StringEntry path, StringEntry extension, S32* numfiles );
    virtual void FreeFileList( UTF8** list );
    virtual S32 GetFileList( StringEntry path, StringEntry extension, UTF8* listbuf, S32 bufsize );
    virtual U32 CountFileList( UTF8** list );
    virtual UTF8** ConcatenateFileLists( UTF8** list0, UTF8** list1, UTF8** list2 );
    virtual S32 GetModList( UTF8* listbuf, S32 bufsize );
    static void Dir_f( void );
    virtual void ConvertPath( UTF8* s );
    virtual S32 PathCmp( StringEntry s1, StringEntry s2 );
    virtual void SortFileList( UTF8** filelist, S32 numfiles );
    static void NewDir_f( void );
    static void Path_f( void );
    static void TouchFile_f( void );
    static void Which_f( void );
    static S32 paksort( const void* a, const void* b );
    virtual bool IsExt( StringEntry filename, StringEntry ext, S32 namelen );
    static void AddGameDirectory( StringEntry path, StringEntry dir );
    virtual bool idPak( UTF8* pak, UTF8* base );
    virtual bool VerifyOfficialPaks( void );
    virtual bool ComparePaks( UTF8* neededpaks, S32 len, bool dlstring );
    virtual void Shutdown( bool closemfp );
    virtual void ReorderPurePaks( void );
    virtual void Startup( StringEntry gameName );
    virtual StringEntry GamePureChecksum( void );
    virtual StringEntry LoadedPakChecksums( void );
    virtual StringEntry LoadedPakNames( void );
    virtual StringEntry LoadedPakPureChecksums( void );
    virtual StringEntry ReferencedPakChecksums( void );
    virtual StringEntry ReferencedPakNames( void );
    virtual StringEntry ReferencedPakPureChecksums( void );
    virtual void ClearPakReferences( S32 flags );
    virtual void PureServerSetLoadedPaks( StringEntry pakSums, StringEntry pakNames );
    virtual void PureServerSetReferencedPaks( StringEntry pakSums, StringEntry pakNames );
    virtual void InitFilesystem( void );
    virtual void Restart( S32 checksumFeed );
    virtual bool ConditionalRestart( S32 checksumFeed );
    virtual S32 FOpenFileByMode( StringEntry qpath, fileHandle_t* f, fsMode_t mode );
    virtual S32 FTell( fileHandle_t f );
    virtual void Flush( fileHandle_t f );
    virtual bool VerifyPak( StringEntry pak );
    virtual bool IsPure( void );
    virtual U32 ChecksumOSPath( UTF8* OSPath );
    virtual void FilenameCompletion( StringEntry dir, StringEntry ext, bool stripExt, void( *callback )( StringEntry s ) );
    virtual StringEntry GetGameDir( void );
    virtual bool IsFileEmpty( UTF8* filename );
};

extern idFileSystemLocal fileSystemLocal;

#endif // !__VFILES_H__
