/*
===========================================================================

OpenWolf GPL Source Code
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 2009 Cyril Gantin
Copyright (C) 2011 Dusan Jocic <dusanjocic@msn.com>

OpenWolf is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

OpenWolf is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

===========================================================================
*/

/*
INFO: Integration of libcurl for requesting maps from an online repository.

Usage:
  \download <mapname>     - blocking download ( hold ESC to abort )
  \download <mapname> &   - background download
  \download -             - abort current background download
  \download               - show help or background download progress

Cvar dl_source defines the url from which to query maps, eg: http://someserver/somescript.php?q=%m
The %m token is replaced with the actual map name in the query.

The server MUST return an appropriate content-type. Accepted content-type values are either application/zip
or application/octet-stream. Other content-type values will be treated as errors or queries that didn't
yield results.

The server MAY return a content-disposition header with the actual name of the pk3 file. In the absence
of a content-disposition header, the client will write the pack to a default <mapname>.pk3 location. The
filename MUST have a pk3 extension. The filename MUST NOT have directory path information - slashes (/),
backslashes (\) and colons (:) are not accepted. Finally the filename MUST consist of us-ascii characters
only (chars 32 to 126, included). A filename that doesn't comply to these specifications will raise an
error and abort the transfer.

The server MAY redirect the query to another url. Multiple redirections are permitted - limit depends on
libcurl's default settings. The end query MUST return a "200 OK" http status code.

It is desirable that the server returns a content-length header with the size of the pk3 file.

The server MAY return a custom openwolf-motd header. Its value is a string that MUST NOT exceed 127
chars. The motd string will be displayed after the download is complete. This is the place
where you take credits for setting up a server. :)

Downloaded files are written to the current gamedir of the home directory - eg. C:\quake3\mymod\ in
windows; ~/.q3a/mymod/ in linux. Name collision with an existing pk3 file will result in a failure and
be left to the user to sort out.
*/
#ifndef __CLIENDOWNLOAD_LOCAL_HPP__
#define __CLIENDOWNLOAD_LOCAL_HPP__

//
// idClientDownloadSystemLocal
//
class idClientDownloadSystemLocal : public idClientDownloadSystem {
public:
    idClientDownloadSystemLocal();
    ~idClientDownloadSystemLocal();

    virtual void Init(void);
    virtual sint Active(void);
    virtual void Shutdown(void);
    virtual sint Begin(pointer map, bool nonblocking);
    virtual void End(CURLcode res, CURLMcode resm);
    virtual sint Continue(void);
    virtual void Interrupt(void);
    virtual void Info(bool console);

    static sint64 Curl_WriteCallback_f(void *ptr,
                                       sint64 size, sint64 nmemb, void *stream);
    static sint64 Curl_HeaderCallback_f(void *ptr,
                                        sint64 size, sint64 nmemb, void *stream);
    static sint64 Curl_VerboseCallback_f(CURL *curl, curl_infotype type,
                                         valueType *data, size_t size, void *userptr);
    static sint Curl_ProgressCallback_f(void *clientp, float32 dltotal,
                                        float32 dlnow, float32 ultotal, float32 ulnow);
    static void Curl_ShowVersion_f(void);
    static void Curl_Download_f(void);
};

extern idClientDownloadSystemLocal clientDownloadLocal;

#endif // !__CLIENDOWNLOAD_LOCAL_HPP__
