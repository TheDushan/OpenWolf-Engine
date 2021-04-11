////////////////////////////////////////////////////////////////////////////////////////
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
// File name:   FileSystem_api.hpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifndef __FILESYSTEM_API_H__
#define __FILESYSTEM_API_H__

// Dushan
// INFO: Little crapy, but its working
// Basically PAK3 files are ZIP files with BZ2 compression

typedef struct fileInPack_s
{
    valueType* name; // name of the file
    uint32 pos; // file info position in zip
    uint32	len;// uncompress file size
    struct fileInPack_s* next; // next file in the hash
} fileInPack_t;

typedef struct pack_s
{
    valueType pakFilename[MAX_OSPATH]; // c:\quake3\baseq3\pak0.pk3
    valueType pakPathname[MAX_OSPATH];
    valueType pakBasename[MAX_OSPATH]; // pak0
    valueType pakGamename[MAX_OSPATH]; // baseq3
    void* handle; // handle to zip file
    sint checksum; // regular checksum
    sint pure_checksum; // checksum for pure
    sint numfiles; // number of files in pk3
    sint referenced; // referenced file flags
    uint64 hashSize; // hash table size (power of 2)
    fileInPack_t** hashTable; // hash table
    fileInPack_t* buildBuffer; // buffer with the filenames etc.
} pack_t;

#define MAX_FOUND_FILES 0x1000

// idFileSystem
class idFileSystem
{
public:
    virtual bool Initialized( void ) = 0;
    virtual bool PakIsPure( pack_t* pack ) = 0;
    virtual sint LoadStack( void ) = 0;
    virtual sint32 HashFileName( pointer fname, uint64 hashSize ) = 0;
    virtual fileHandle_t HandleForFile( void ) = 0;
    virtual FILE* FileForHandle( fileHandle_t f ) = 0;
    virtual void ForceFlush( fileHandle_t f ) = 0;
    virtual sint filelength( fileHandle_t f ) = 0;
    virtual void ReplaceSeparators( valueType* path ) = 0;
    virtual valueType* BuildOSPath( pointer base, pointer game, pointer qpath ) = 0;
    virtual void BuildOSHomePath( valueType* ospath, sint size, sint qpath ) = 0;
    virtual sint CreatePath( pointer OSPath_ ) = 0;
    virtual void FSCopyFile( valueType* fromOSPath, valueType* toOSPath ) = 0;
    virtual bool Remove( pointer osPath ) = 0;
    virtual void HomeRemove( pointer homePath ) = 0;
    virtual bool FileExists( pointer file ) = 0;
    virtual bool SV_FileExists( pointer file ) = 0;
    virtual bool OS_FileExists( pointer file ) = 0;
    virtual fileHandle_t SV_FOpenFileWrite( pointer filename ) = 0;
    virtual sint SV_FOpenFileRead( pointer filename, fileHandle_t* fp ) = 0;
    virtual void SV_Rename( pointer from, pointer to ) = 0;
    virtual void Rename( pointer from, pointer to ) = 0;
    virtual void FCloseFile( fileHandle_t f ) = 0;
    virtual fileHandle_t FOpenFileWrite( pointer filename ) = 0;
    virtual fileHandle_t FOpenFileAppend( pointer filename ) = 0;
    virtual sint FOpenFileDirect( pointer filename, fileHandle_t* f ) = 0;
    virtual fileHandle_t FOpenFileUpdate( pointer filename, sint* length ) = 0;
    virtual bool FilenameCompare( pointer s1, pointer s2 ) = 0;
    virtual valueType* ShiftedStrStr( pointer string, pointer substring, sint shift ) = 0;
    virtual valueType* ShiftStr( pointer string, sint shift ) = 0;
    virtual sint FOpenFileRead( pointer filename, fileHandle_t* file, bool uniqueFILE ) = 0;
    virtual sint FOpenFileRead_Filtered( pointer qpath, fileHandle_t* file, bool uniqueFILE, sint filter_flag ) = 0;
    virtual bool CL_ExtractFromPakFile( pointer base, pointer gamedir, pointer filename ) = 0;
    virtual bool AllowDeletion( valueType* filename ) = 0;
    virtual sint DeleteDir( valueType* dirname, bool nonEmpty, bool recursive ) = 0;
    virtual sint OSStatFile( valueType* ospath ) = 0;
    virtual sint Delete( valueType* filename ) = 0;
    virtual sint FPrintf( fileHandle_t f, pointer fmt, ... ) = 0;
    virtual sint Read2( void* buffer, sint len, fileHandle_t f ) = 0;
    virtual sint Read( void* buffer, sint len, fileHandle_t f ) = 0;
    virtual sint Write( const void* buffer, sint len, fileHandle_t h ) = 0;
    virtual void Printf( fileHandle_t h, pointer fmt, ... ) = 0;
    virtual sint Seek( fileHandle_t f, sint32 offset, sint origin ) = 0;
    virtual sint FileIsInPAK( pointer filename, sint* pChecksum ) = 0;
    virtual sint ReadFile( pointer qpath, void** buffer ) = 0;
    virtual void FreeFile( void* buffer ) = 0;
    virtual void WriteFile( pointer qpath, const void* buffer, sint size ) = 0;
    virtual pack_t* LoadZipFile( pointer zipfile, pointer basename ) = 0;
    virtual sint ReturnPath( pointer zname, valueType* zpath, sint* depth ) = 0;
    virtual sint AddFileToList( valueType* name, valueType* list[MAX_FOUND_FILES], sint nfiles ) = 0;
    virtual valueType** ListFilteredFiles( pointer path, pointer extension, valueType* filter, uint64* numfiles ) = 0;
    virtual valueType** ListFiles( pointer path, pointer extension, uint64* numfiles ) = 0;
    virtual void FreeFileList( valueType** list ) = 0;
    virtual uint64 GetFileList( pointer path, pointer extension, valueType* listbuf, uint64 bufsize ) = 0;
    virtual uint CountFileList( valueType** list ) = 0;
    virtual valueType** ConcatenateFileLists( valueType** list0, valueType** list1, valueType** list2 ) = 0;
    virtual uint64 GetModList( valueType* listbuf, uint64 bufsize ) = 0;
    virtual void ConvertPath( valueType* s ) = 0;
    virtual sint PathCmp( pointer s1, pointer s2 ) = 0;
    virtual void SortFileList( valueType** filelist, uint64 numfiles ) = 0;
    virtual bool IsExt( pointer filename, pointer ext, uint64 namelen ) = 0;
    virtual bool idPak( valueType* pak, valueType* base ) = 0;
    virtual bool VerifyOfficialPaks( void ) = 0;
    virtual bool ComparePaks( valueType* neededpaks, sint len, bool dlstring ) = 0;
    virtual void Shutdown( bool closemfp ) = 0;
    virtual void ReorderPurePaks( void ) = 0;
    virtual void Startup( pointer gameName ) = 0;
    virtual pointer GamePureChecksum( void ) = 0;
    virtual pointer LoadedPakChecksums( void ) = 0;
    virtual pointer LoadedPakNames( void ) = 0;
    virtual pointer LoadedPakPureChecksums( void ) = 0;
    virtual pointer ReferencedPakChecksums( void ) = 0;
    virtual pointer ReferencedPakNames( void ) = 0;
    virtual pointer ReferencedPakPureChecksums( void ) = 0;
    virtual void ClearPakReferences( sint flags ) = 0;
    virtual void PureServerSetLoadedPaks( pointer pakSums, pointer pakNames ) = 0;
    virtual void PureServerSetReferencedPaks( pointer pakSums, pointer pakNames ) = 0;
    virtual void InitFilesystem( void ) = 0;
    virtual void Restart( sint checksumFeed ) = 0;
    virtual bool ConditionalRestart( sint checksumFeed ) = 0;
    virtual sint FOpenFileByMode( pointer qpath, fileHandle_t* f, fsMode_t mode ) = 0;
    virtual sint FTell( fileHandle_t f ) = 0;
    virtual void Flush( fileHandle_t f ) = 0;
    virtual bool VerifyPak( pointer pak ) = 0;
    virtual bool IsPure( void ) = 0;
    virtual uint ChecksumOSPath( valueType* OSPath ) = 0;
    virtual void FilenameCompletion( pointer dir, pointer ext, bool stripExt, void( *callback )( pointer s ) ) = 0;
    virtual pointer GetGameDir( void ) = 0;
    virtual bool IsFileEmpty( valueType* filename ) = 0;
    virtual valueType* GetFullGamePath( valueType* filename ) = 0;
    virtual void Rmdir( pointer osPath, bool recursive ) = 0;
    virtual void HomeRmdir( pointer homePath, bool recursive ) = 0;
};

extern idFileSystem* fileSystem;

#endif // !__FILESYSTEM_API_H__
