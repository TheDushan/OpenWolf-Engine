////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2005 - 2006 Tim Angus
// Copyright(C) 2011 - 2023 Dusan Jocic <dusanjocic@msn.com>
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
// File name:   clientAVI.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0,
//              AppleClang 9.0.0.9000039
// Description:
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include <framework/precompiled.hpp>

idClientLANSystemLocal clientLANLocal;
idClientLANSystem *clientLANSystem = &clientLANLocal;

/*
===============
idClientLANSystemLocal::idClientLANSystemLocal
===============
*/
idClientLANSystemLocal::idClientLANSystemLocal(void) {
}

/*
===============
idClientLANSystemLocal::~idClientLANSystemLocal
===============
*/
idClientLANSystemLocal::~idClientLANSystemLocal(void) {
}

/*
====================
idClientLANSystemLocal::LoadCachedServers
====================
*/
void idClientLANSystemLocal::LoadCachedServers(void) {
    sint size;
    valueType filename[MAX_QPATH];
    fileHandle_t fileIn;

    cls.numglobalservers = cls.numfavoriteservers = 0;
    cls.numGlobalServerAddresses = 0;

    Q_strncpyz(filename, "servercache.dat", sizeof(filename));

    // Arnout: moved to mod/profiles dir
    if(fileSystem->SV_FOpenFileRead(filename, &fileIn)) {
        fileSystem->Read(&cls.numglobalservers, sizeof(sint), fileIn);
        fileSystem->Read(&cls.numfavoriteservers, sizeof(sint), fileIn);
        fileSystem->Read(&size, sizeof(sint), fileIn);

        if(size == sizeof(cls.globalServers) + sizeof(cls.favoriteServers)) {
            fileSystem->Read(&cls.globalServers, sizeof(cls.globalServers), fileIn);
            fileSystem->Read(&cls.favoriteServers, sizeof(cls.favoriteServers),
                             fileIn);
        } else {
            cls.numglobalservers = cls.numfavoriteservers = 0;
            cls.numGlobalServerAddresses = 0;
        }

        fileSystem->FCloseFile(fileIn);
    }
}

/*
====================
idClientLANSystemLocal::SaveServersToCache
====================
*/
void idClientLANSystemLocal::SaveServersToCache(void) {
    sint size;
    valueType filename[MAX_QPATH];
    fileHandle_t fileOut;

    Q_strncpyz(filename, "servercache.dat", sizeof(filename));

    // Arnout: moved to mod/profiles dir
    fileOut = fileSystem->SV_FOpenFileWrite(filename);
    //fileOut = fileSystem->FOpenFileWrite( filename );
    fileSystem->Write(&cls.numglobalservers, sizeof(sint), fileOut);
    fileSystem->Write(&cls.numfavoriteservers, sizeof(sint), fileOut);
    size = sizeof(cls.globalServers) + sizeof(cls.favoriteServers);
    fileSystem->Write(&size, sizeof(sint), fileOut);
    fileSystem->Write(&cls.globalServers, sizeof(cls.globalServers), fileOut);
    fileSystem->Write(&cls.favoriteServers, sizeof(cls.favoriteServers),
                      fileOut);
    fileSystem->FCloseFile(fileOut);
}


/*
====================
idClientLANSystemLocal::ResetPings
====================
*/
void idClientLANSystemLocal::ResetPings(sint source) {
    sint count, i;
    serverInfo_t *servers = nullptr;

    count = 0;

    switch(source) {
        case AS_LOCAL:
            servers = &cls.localServers[0];
            count = MAX_OTHER_SERVERS;
            break;

        case AS_GLOBAL:
            servers = &cls.globalServers[0];
            count = MAX_GLOBAL_SERVERS;
            break;

        case AS_FAVORITES:
            servers = &cls.favoriteServers[0];
            count = MAX_OTHER_SERVERS;
            break;
    }

    if(servers) {
        for(i = 0; i < count; i++) {
            servers[i].ping = -1;
        }
    }
}

/*
====================
idClientLANSystemLocal::AddServer
====================
*/
sint idClientLANSystemLocal::AddServer(sint source, pointer name,
                                       pointer address) {
    sint max, * count, i;
    netadr_t adr;
    serverInfo_t *servers = nullptr;
    max = MAX_OTHER_SERVERS;
    count = 0;

    switch(source) {
        case AS_LOCAL:
            count = &cls.numlocalservers;
            servers = &cls.localServers[0];
            break;

        case AS_GLOBAL:
            max = MAX_GLOBAL_SERVERS;
            count = &cls.numglobalservers;
            servers = &cls.globalServers[0];
            break;

        case AS_FAVORITES:
            count = &cls.numfavoriteservers;
            servers = &cls.favoriteServers[0];
            break;
    }

    if(servers && *count < max) {
        networkChainSystem->StringToAdr(address, &adr, NA_UNSPEC);

        for(i = 0; i < *count; i++) {
            if(networkSystem->CompareAdr(servers[i].adr, adr)) {
                break;
            }
        }

        if(i >= *count) {
            servers[*count].adr = adr;
            Q_strncpyz(servers[*count].hostName, name,
                       sizeof(servers[*count].hostName));
            servers[*count].visible = true;
            (*count)++;
            return 1;
        }

        return 0;
    }

    return -1;
}

/*
====================
idClientLANSystemLocal::RemoveServer
====================
*/
void idClientLANSystemLocal::RemoveServer(sint source, pointer addr) {
    sint *count, i;
    serverInfo_t *servers = nullptr;

    count = 0;

    switch(source) {
        case AS_LOCAL:
            count = &cls.numlocalservers;
            servers = &cls.localServers[0];
            break;

        case AS_GLOBAL:
            count = &cls.numglobalservers;
            servers = &cls.globalServers[0];
            break;

        case AS_FAVORITES:
            count = &cls.numfavoriteservers;
            servers = &cls.favoriteServers[0];
            break;
    }

    if(servers) {
        netadr_t comp;

        networkChainSystem->StringToAdr(addr, &comp, NA_UNSPEC);

        for(i = 0; i < *count; i++) {
            if(networkSystem->CompareAdr(comp, servers[i].adr)) {
                sint j = i;

                while(j < *count - 1) {
                    ::memcpy(&servers[j], &servers[j + 1], sizeof(servers[j]));
                    j++;
                }

                (*count)--;

                break;
            }
        }
    }
}

/*
====================
idClientLANSystemLocal::GetServerCount
====================
*/
sint idClientLANSystemLocal::GetServerCount(sint source) {
    switch(source) {
        case AS_LOCAL:
            return cls.numlocalservers;
            break;

        case AS_GLOBAL:
            return cls.numglobalservers;
            break;

        case AS_FAVORITES:
            return cls.numglobalservers;
            break;
    }

    return 0;
}

/*
====================
idClientLANSystemLocal::GetLocalServerAddressString
====================
*/
void idClientLANSystemLocal::GetServerAddressString(sint source, sint n,
        valueType *buf, uint64 buflen) {
    switch(source) {
        case AS_LOCAL:
            if(n >= 0 && n < MAX_OTHER_SERVERS) {
                Q_strncpyz(buf, networkSystem->AdrToStringwPort(cls.localServers[n].adr),
                           buflen);
                return;
            }

            break;

        case AS_GLOBAL:
            if(n >= 0 && n < MAX_GLOBAL_SERVERS) {
                Q_strncpyz(buf, networkSystem->AdrToStringwPort(cls.globalServers[n].adr),
                           buflen);
                return;
            }

            break;

        case AS_FAVORITES:
            if(n >= 0 && n < MAX_OTHER_SERVERS) {
                Q_strncpyz(buf, networkSystem->AdrToStringwPort(
                               cls.favoriteServers[n].adr), buflen);
                return;
            }

            break;
    }

    buf[0] = '\0';
}

/*
====================
idClientLANSystemLocal::GetServerInfo
====================
*/
void idClientLANSystemLocal::GetServerInfo(sint source, sint n,
        valueType *buf, uint64 buflen) {
    valueType info[MAX_STRING_CHARS];
    serverInfo_t *server = nullptr;

    info[0] = '\0';

    switch(source) {
        case AS_LOCAL:
            if(n >= 0 && n < MAX_OTHER_SERVERS) {
                server = &cls.localServers[n];
            }

            break;

        case AS_GLOBAL:
            if(n >= 0 && n < MAX_GLOBAL_SERVERS) {
                server = &cls.globalServers[n];
            }

            break;

        case AS_FAVORITES:
            if(n >= 0 && n < MAX_OTHER_SERVERS) {
                server = &cls.favoriteServers[n];
            }

            break;
    }

    if(server && buf) {
        buf[0] = '\0';

        Info_SetValueForKey(info, "hostname", server->hostName);
        Info_SetValueForKey(info, "serverload", va(nullptr, "%i", server->load));
        Info_SetValueForKey(info, "mapname", server->mapName);
        Info_SetValueForKey(info, "clients", va(nullptr, "%i", server->clients));
        Info_SetValueForKey(info, "sv_maxclients", va(nullptr, "%i",
                            server->maxClients));
        Info_SetValueForKey(info, "ping", va(nullptr, "%i", server->ping));
        Info_SetValueForKey(info, "minping", va(nullptr, "%i", server->minPing));
        Info_SetValueForKey(info, "maxping", va(nullptr, "%i", server->maxPing));
        Info_SetValueForKey(info, "game", server->game);
        Info_SetValueForKey(info, "gametype", va(nullptr, "%i", server->gameType));
        Info_SetValueForKey(info, "nettype", va(nullptr, "%i", server->netType));
        Info_SetValueForKey(info, "addr",
                            networkSystem->AdrToStringwPort(server->adr));
        Info_SetValueForKey(info, "sv_allowAnonymous", va(nullptr, "%i",
                            server->allowAnonymous));
        Info_SetValueForKey(info, "friendlyFire", va(nullptr, "%i",
                            server->friendlyFire));
        Info_SetValueForKey(info, "maxlives", va(nullptr, "%i", server->maxlives));
        Info_SetValueForKey(info, "needpass", va(nullptr, "%i", server->needpass));
        Info_SetValueForKey(info, "gamename", server->gameName);
        Info_SetValueForKey(info, "g_antilag", va(nullptr, "%i", server->antilag));
        Info_SetValueForKey(info, "weaprestrict", va(nullptr, "%i",
                            server->weaprestrict));
        Info_SetValueForKey(info, "balancedteams", va(nullptr, "%i",
                            server->balancedteams));

        Q_strncpyz(buf, info, buflen);
    } else {
        if(buf) {
            buf[0] = '\0';
        }
    }
}

/*
====================
idClientLANSystemLocal::GetServerPing
====================
*/
sint idClientLANSystemLocal::GetServerPing(sint source, sint n) {
    serverInfo_t *server = nullptr;

    switch(source) {
        case AS_LOCAL:
            if(n >= 0 && n < MAX_OTHER_SERVERS) {
                server = &cls.localServers[n];
            }

            break;

        case AS_GLOBAL:
            if(n >= 0 && n < MAX_GLOBAL_SERVERS) {
                server = &cls.globalServers[n];
            }

            break;

        case AS_FAVORITES:
            if(n >= 0 && n < MAX_OTHER_SERVERS) {
                server = &cls.favoriteServers[n];
            }

            break;
    }

    if(server) {
        return server->ping;
    }

    return -1;
}

/*
====================
idClientLANSystemLocal::GetServerPtr
====================
*/
serverInfo_t *idClientLANSystemLocal::GetServerPtr(sint source, sint n) {
    switch(source) {
        case AS_LOCAL:
            if(n >= 0 && n < MAX_OTHER_SERVERS) {
                return &cls.localServers[n];
            }

            break;

        case AS_GLOBAL:
            if(n >= 0 && n < MAX_GLOBAL_SERVERS) {
                return &cls.globalServers[n];
            }

            break;

        case AS_FAVORITES:
            if(n >= 0 && n < MAX_OTHER_SERVERS) {
                return &cls.favoriteServers[n];
            }

            break;
    }

    return nullptr;
}

/*
====================
idClientLANSystemLocal::CompareServers
====================
*/
sint idClientLANSystemLocal::CompareServers(sint source, sint sortKey,
        sint sortDir, sint s1, sint s2) {
    sint res, clients1, clients2;
    valueType name1[MAX_NAME_LENGTH], name2[MAX_NAME_LENGTH];
    serverInfo_t *server1, * server2;

    server1 = GetServerPtr(source, s1);
    server2 = GetServerPtr(source, s2);

    if(!server1 || !server2) {
        return 0;
    }

    res = 0;

    switch(sortKey) {
        case SORT_HOST:
            //% res = Q_stricmp( server1->hostName, server2->hostName );
            Q_strncpyz(name1, server1->hostName, sizeof(name1));
            Q_CleanStr(name1);
            Q_strncpyz(name2, server2->hostName, sizeof(name2));
            Q_CleanStr(name2);

            res = Q_stricmp(name1, name2);
            break;

        case SORT_MAP:
            res = Q_stricmp(server1->mapName, server2->mapName);
            break;

        case SORT_CLIENTS:

            // sub sort by max clients
            if(server1->clients == server2->clients) {
                clients1 = server1->maxClients;
                clients2 = server2->maxClients;
            } else {
                clients1 = server1->clients;
                clients2 = server2->clients;
            }

            if(clients1 < clients2) {
                res = -1;
            } else if(clients1 > clients2) {
                res = 1;
            } else {
                res = 0;
            }

            break;

        case SORT_GAME:
            if(server1->gameType < server2->gameType) {
                res = -1;
            } else if(server1->gameType > server2->gameType) {
                res = 1;
            } else {
                res = 0;
            }

            break;

        case SORT_PING:
            if(server1->ping < server2->ping) {
                res = -1;
            } else if(server1->ping > server2->ping) {
                res = 1;
            } else {
                res = 0;
            }

            break;
    }

    if(sortDir) {
        if(res < 0) {
            return 1;
        }

        if(res > 0) {
            return -1;
        }

        return 0;
    }

    return res;
}

/*
====================
idClientLANSystemLocal::GetPingQueueCount
====================
*/
sint idClientLANSystemLocal::GetPingQueueCount(void) {
    return (idClientBrowserSystemLocal::GetPingQueueCount());
}

/*
====================
idClientLANSystemLocal::ClearPing
====================
*/
void idClientLANSystemLocal::ClearPing(sint n) {
    idClientBrowserSystemLocal::ClearPing(n);
}

/*
====================
idClientLANSystemLocal::GetPing
====================
*/
void idClientLANSystemLocal::GetPing(sint n, valueType *buf, uint64 buflen,
                                     sint *pingtime) {
    idClientBrowserSystemLocal::GetPing(n, buf, buflen, pingtime);
}

/*
====================
idClientLANSystemLocal::GetPingInfo
====================
*/
void idClientLANSystemLocal::GetPingInfo(sint n, valueType *buf,
        uint64 buflen) {
    idClientBrowserSystemLocal::GetPingInfo(n, buf, buflen);
}

/*
====================
idClientLANSystemLocal::MarkServerVisible
====================
*/
void idClientLANSystemLocal::MarkServerVisible(sint source, sint n,
        bool visible) {
    if(n == -1) {
        sint count = MAX_OTHER_SERVERS;
        serverInfo_t *server = nullptr;

        switch(source) {
            case AS_LOCAL:
                server = &cls.localServers[0];
                break;

            case AS_GLOBAL:
                server = &cls.globalServers[0];
                count = MAX_GLOBAL_SERVERS;
                break;

            case AS_FAVORITES:
                server = &cls.favoriteServers[0];
                break;
        }

        if(server) {
            for(n = 0; n < count; n++) {
                server[n].visible = visible;
            }
        }

    } else {
        switch(source) {
            case AS_LOCAL:
                if(n >= 0 && n < MAX_OTHER_SERVERS) {
                    cls.localServers[n].visible = visible;
                }

                break;

            case AS_GLOBAL:
                if(n >= 0 && n < MAX_GLOBAL_SERVERS) {
                    cls.globalServers[n].visible = visible;
                }

                break;

            case AS_FAVORITES:
                if(n >= 0 && n < MAX_OTHER_SERVERS) {
                    cls.favoriteServers[n].visible = visible;
                }

                break;
        }
    }
}

/*
=======================
idClientLANSystemLocal::ServerIsVisible
=======================
*/
sint idClientLANSystemLocal::ServerIsVisible(sint source, sint n) {
    switch(source) {
        case AS_LOCAL:
            if(n >= 0 && n < MAX_OTHER_SERVERS) {
                return cls.localServers[n].visible;
            }

            break;

        case AS_GLOBAL:
            if(n >= 0 && n < MAX_GLOBAL_SERVERS) {
                return cls.globalServers[n].visible;
            }

            break;

        case AS_FAVORITES:
            if(n >= 0 && n < MAX_OTHER_SERVERS) {
                return cls.favoriteServers[n].visible;
            }

            break;
    }

    return false;
}

/*
=======================
idClientLANSystemLocal::UpdateVisiblePings
=======================
*/
bool idClientLANSystemLocal::UpdateVisiblePings(sint source) {
    return idClientBrowserSystemLocal::UpdateVisiblePings(source);
}

/*
====================
idClientLANSystemLocal::GetServerStatus
====================
*/
sint idClientLANSystemLocal::GetServerStatus(valueType *serverAddress,
        valueType *serverStatus, uint64 maxLen) {
    return idClientBrowserSystemLocal::ServerStatus(serverAddress,
            serverStatus, maxLen);
}

/*
=======================
idClientLANSystemLocal::ServerIsInFavoriteList
=======================
*/
bool idClientLANSystemLocal::ServerIsInFavoriteList(sint source, sint n) {
    sint i;
    serverInfo_t *server = nullptr;

    switch(source) {
        case AS_LOCAL:
            if(n >= 0 && n < MAX_OTHER_SERVERS) {
                server = &cls.localServers[n];
            }

            break;

        case AS_GLOBAL:
            if(n >= 0 && n < MAX_GLOBAL_SERVERS) {
                server = &cls.globalServers[n];
            }

            break;

        case AS_FAVORITES:
            if(n >= 0 && n < MAX_OTHER_SERVERS) {
                return true;
            }

            break;
    }

    if(!server) {
        return false;
    }

    for(i = 0; i < cls.numfavoriteservers; i++) {
        if(networkSystem->CompareAdr(cls.favoriteServers[i].adr, server->adr)) {
            return true;
        }
    }

    return false;
}
