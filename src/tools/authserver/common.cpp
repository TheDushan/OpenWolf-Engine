////////////////////////////////////////////////////////////////////////////////////////
// Copyright(C) 2008 - 2011  Mathieu Olivier
// Copyright(C) 2011 - 2021 Dusan Jocic <dusanjocic@msn.com>
//
// This file is part of OpenWolf.
//
// OpenWolf is free software; you can redistribute it
// and / or modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 3 of the License,
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
// File name:   common.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description: Utility functions for owauthserver
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#include "common.hpp"
#include "system.hpp"
#include "servers.hpp"

// ---------- Private variables ---------- //

// The log file
static FILE *log_file = NULL;

// Log file path
static char log_filepath [MAX_PATH] = DEFAULT_LOG_FILE;

// Should we (re)open the log file?
static volatile sig_atomic_t must_open_log = false;

// Should we close the log file?
static volatile sig_atomic_t must_close_log = false;


// ---------- Public variables ---------- //

// The current time (updated every time we receive a packet)
time_t crt_time;

// Maximum level for a message to be printed
msg_level_t max_msg_level = MSG_NORMAL;

// Peer address. We rebuild it every time we receive a new packet
char peer_address [128];

// Should we print the date before any new console message?
bool print_date = false;

// Are port numbers used when computing address hashes?
bool hash_ports = false;


// ---------- Private functions ---------- //

/*
====================
BuildDateString

Return a string containing the current date and time
====================
*/
static const char *BuildDateString(void) {
    static char datestring [80];

    size_t date_len = strftime(datestring, sizeof(datestring),
                               "%Y-%m-%d %H:%M:%S %Z", localtime(&crt_time));

    // If the datestring buffer was too small, its contents
    // is now "indeterminate", so we need to clear it
    if(date_len == 0) {
        datestring[0] = '\0';
    }

    return datestring;
}


/*
====================
CloseLogFile

Close the log file
====================
*/
static void CloseLogFile(const char *datestring) {
    if(log_file != NULL) {
        if(datestring == NULL) {
            datestring = BuildDateString();
        }

        fprintf(log_file, "\n> Closing log file (time: %s)\n", datestring);
        fclose(log_file);
        log_file = NULL;
    }
}


// ---------- Public functions (logging) ---------- //

/*
====================
Com_EnableLog

Enable the logging
====================
*/
void Com_EnableLog(void) {
    must_open_log = true;
}


/*
====================
Com_FlushLog

Enable the logging
====================
*/
void Com_FlushLog(void) {
    fflush(log_file);
}


/*
====================
Com_IsLogEnabled

Test if the logging is enabled
====================
*/
bool Com_IsLogEnabled(void) {
    return (log_file != NULL);
}


/*
====================
Com_SetLogFilePath

Change the log file path
====================
*/
bool Com_SetLogFilePath(const char *filepath) {
    if(filepath == NULL || filepath[0] == '\0') {
        return false;
    }

    strncpy(log_filepath, filepath, sizeof(log_filepath) - 1);
    log_filepath[sizeof(log_filepath) - 1] = '\0';

    return true;
}


/*
====================
Com_UpdateLogStatus

Update the logging status, opening or closing the log file when necessary
====================
*/
bool Com_UpdateLogStatus(bool init) {
    // If we need to (re)open the log file
    if(must_open_log) {
        const char *datestring;

        must_open_log = false;

        datestring = BuildDateString();
        CloseLogFile(datestring);

        log_file = fopen(log_filepath, "a");

        if(log_file == NULL) {
            Com_Printf(MSG_ERROR, "> ERROR: can't open log file \"%s\"\n",
                       log_filepath);
            return false;
        }

        // Make the log stream fully buffered (instead of line buffered)
        setvbuf(log_file, NULL, _IOFBF, SETVBUF_DEFAULT_SIZE);

        fprintf(log_file, "> Opening log file (time: %s)\n", datestring);

        // if we're opening the log after the initialization, print the list of servers
        if(! init) {
            Sv_PrintServerList(MSG_WARNING);
        }

    }

    // If we need to close the log file
    if(must_close_log) {
        must_close_log = false;
        CloseLogFile(NULL);
    }

    return true;
}


// ---------- Public functions (user hash table) ---------- //

/*
====================
Com_UserHashTable_Init

Initialize a user hash table
====================
*/
bool Com_UserHashTable_Init(user_hash_table_t *table,
                            size_t hash_size,
                            const char *table_name) {
    size_t array_size;
    user_t **result;

    assert(table_name[0] != '\0');

    array_size = (1 << hash_size) * sizeof(user_t *);
    result = (user_t **)malloc(array_size);

    if(result == NULL) {
        Com_Printf(MSG_ERROR,
                   "> ERROR: can't allocate the %s hash table (%s)\n",
                   table_name, strerror(errno));
        return false;
    }

    memset(result, 0, array_size);
    table->entries = result;

    Com_Printf(MSG_DEBUG,
               "> %c%s hash table allocated (%u entries)\n",
               toupper(table_name[0]), &table_name[1], 1 << hash_size);

    return true;
}


/*
====================
Com_UserHashTable_Add

Add a user to the hash table
====================
*/
void Com_UserHashTable_Add(user_hash_table_t *table, user_t *user,
                           unsigned int hash) {
    user_t **hash_entry_ptr;

    hash_entry_ptr = &table->entries[hash];
    user->next = *hash_entry_ptr;
    user->prev_ptr = hash_entry_ptr;
    *hash_entry_ptr = user;

    if(user->next != NULL) {
        user->next->prev_ptr = &user->next;
    }
}


/*
====================
Com_UserHashTable_Remove

Remove a user from its hash table
====================
*/
void Com_UserHashTable_Remove(user_t *user) {
    *user->prev_ptr = user->next;

    if(user->next != NULL) {
        user->next->prev_ptr = user->prev_ptr;
    }
}


// ---------- Public functions (misc) ---------- //

/*
====================
Com_Printf

Print a message to screen, depending on its verbose level
====================
*/
void Com_Printf(msg_level_t msg_level, const char *format, ...) {
    // If the message level is above the maximum level, or if we output
    // neither to the console nor to a log file, there nothing to do
    if(msg_level > max_msg_level ||
            (log_file == NULL && daemon_state == DAEMON_STATE_EFFECTIVE)) {
        return;
    }

    // Print a time stamp if necessary
    if(print_date) {
        const char *datestring = BuildDateString();

        if(daemon_state < DAEMON_STATE_EFFECTIVE) {
            printf("\n* %s\n", datestring);
        }

        if(log_file != NULL) {
            fprintf(log_file, "\n* %s\n", datestring);
        }

        print_date = false;
    }

    if(daemon_state < DAEMON_STATE_EFFECTIVE) {
        va_list args;

        va_start(args, format);
        vprintf(format, args);
        va_end(args);
    }

    if(log_file != NULL) {
        va_list args;

        va_start(args, format);
        vfprintf(log_file, format, args);
        va_end(args);
    }
}


/*
====================
Com_SignalHandler

Handling of the signals sent to this process
====================
*/
void Com_SignalHandler(int Signal) {
    switch(Signal) {
#ifdef SIGUSR1

        case SIGUSR1:
            must_open_log = true;
            break;
#endif
#ifdef SIGUSR2

        case SIGUSR2:
            must_close_log = true;
            break;
#endif

        default:
            // We aren't suppose to be here...
            assert(false);
            break;
    }
}


/*
====================
Com_AddressHash

Compute the hash of a server address
====================
*/
unsigned int Com_AddressHash(const struct sockaddr_storage *address,
                             size_t hash_size) {
    unsigned int hash;

    if(address->ss_family == AF_INET6) {
        const struct sockaddr_in6 *addr6;
        const unsigned int *ipv6_ptr;

        addr6 = (const struct sockaddr_in6 *)address;
        ipv6_ptr = (const unsigned int *)&addr6->sin6_addr.s6_addr;

        // Since an IPv6 device can have multiple addresses, we only hash
        // the non-configurable part of its public address (meaning the first
        // 64 bits, or subnet part)
        hash = ipv6_ptr[0] ^ ipv6_ptr[1];

        if(hash_ports) {
            hash ^= addr6->sin6_port;
        }
    } else {
        const struct sockaddr_in *addr4;

        assert(address->ss_family == AF_INET);

        addr4 = (const struct sockaddr_in *)address;
        hash = addr4->sin_addr.s_addr;

        if(hash_ports) {
            hash ^= addr4->sin_port;
        }
    }

    // Merge all the bits in the first 16 bits
    hash = (hash & 0xFFFF) ^ (hash >> 16);

    // Merge the bits we won't use in the upper part into the lower part.
    // If hash_size < 8, some bits will be lost, but it's not a real problem
    hash = (hash ^ (hash >> hash_size)) & ((1 << hash_size) - 1);

    return hash;
}


/*
====================
Com_SameIPv4Addr

Compare 2 IPv4 addresses and return "true" if they're equal
====================
*/
bool Com_SameIPv4Addr(const struct sockaddr_storage *addr1,
                      const struct sockaddr_storage *addr2,
                      bool *same_public_address) {
    const struct sockaddr_in *addr1_in, *addr2_in;

    assert(addr1->ss_family == AF_INET);
    assert(addr2->ss_family == AF_INET);

    addr1_in = (const struct sockaddr_in *)addr1;
    addr2_in = (const struct sockaddr_in *)addr2;

    // Same address?
    if(addr1_in->sin_addr.s_addr == addr2_in->sin_addr.s_addr) {
        *same_public_address = true;

        // Same port?
        if(addr1_in->sin_port == addr2_in->sin_port) {
            return true;
        }
    } else {
        *same_public_address = false;
    }

    return false;
}


/*
====================
Com_SameIPv6Addr

Compare 2 IPv6 addresses and return "true" if they're equal
====================
*/
bool Com_SameIPv6Addr(const struct sockaddr_storage *addr1,
                      const struct sockaddr_storage *addr2,
                      bool *same_public_address) {
    const struct sockaddr_in6 *addr1_in6, *addr2_in6;
    const unsigned char *addr1_buff, *addr2_buff;

    assert(addr1->ss_family == AF_INET6);
    assert(addr2->ss_family == AF_INET6);

    addr1_in6 = (const struct sockaddr_in6 *)addr1;
    addr1_buff = (const unsigned char *)&addr1_in6->sin6_addr.s6_addr;

    addr2_in6 = (const struct sockaddr_in6 *)addr2;
    addr2_buff = (const unsigned char *)&addr2_in6->sin6_addr.s6_addr;

    // Same subnet address (first 64 bits)?
    if(memcmp(addr1_buff, addr2_buff, 8) == 0) {
        *same_public_address = true;

        // Same scope ID, port, and host address (last 64 bits)?
        if(addr1_in6->sin6_scope_id == addr2_in6->sin6_scope_id &&
                addr1_in6->sin6_port == addr2_in6->sin6_port &&
                memcmp(addr1_buff + 8, addr2_buff + 8, 8) == 0) {
            return true;
        }
    } else {
        *same_public_address = false;
    }

    return false;
}
