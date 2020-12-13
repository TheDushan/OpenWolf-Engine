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
// File name:   Huffman.cpp
// Created:
// Compilers:   Microsoft (R) C/C++ Optimizing Compiler Version 19.26.28806 for x64,
//              gcc (Ubuntu 9.3.0-10ubuntu2) 9.3.0
// Description: This is based on the Adaptive Huffman algorithm described in Sayood's Data
//              Compression book.  The ranks are not actually stored, but implicitly defined
//              by the location of a node within a doubly-linked list
// -------------------------------------------------------------------------------------
////////////////////////////////////////////////////////////////////////////////////////

#ifdef UPDATE_SERVER
#include <null/null_autoprecompiled.hpp>
#elif DEDICATED
#include <null/null_serverprecompiled.hpp>
#else
#include <framework/precompiled.hpp>
#endif

idHuffmanSystemLocal huffmanLocal;

/*
===============
idHuffmanSystemLocal::idHuffmanSystemLocal
===============
*/
idHuffmanSystemLocal::idHuffmanSystemLocal( void )
{
}

/*
===============
idHuffmanSystemLocal::~idHuffmanSystemLocal
===============
*/
idHuffmanSystemLocal::~idHuffmanSystemLocal( void )
{
}

//bani - optimized version
//clears data along the way so we dont have to memset() it ahead of time
void idHuffmanSystemLocal::putBit( sint bit, uchar8* fout, sint* offset )
{
    sint             x, y;
    
    bloc = *offset;
    x = bloc >> 3;
    y = bloc & 7;
    if( !y )
    {
        fout[x] = 0;
    }
    fout[x] |= bit << y;
    bloc++;
    *offset = bloc;
}

sint	idHuffmanSystemLocal::getBloc( void )
{
    return bloc;
}

void idHuffmanSystemLocal::setBloc( sint _bloc )
{
    bloc = _bloc;
}

//bani - optimized version
//optimization works on gcc 3.x, but not 2.95 ? most curious.
sint idHuffmanSystemLocal::getBit( uchar8* fin, sint* offset )
{
    sint             t;
    
    bloc = *offset;
    t = fin[bloc >> 3] >> ( bloc & 7 ) & 0x1;
    bloc++;
    *offset = bloc;
    return t;
}

//bani - optimized version
//clears data along the way so we dont have to memset() it ahead of time
void idHuffmanSystemLocal::add_bit( valueType bit, uchar8* fout )
{
    sint             x, y;
    
    y = bloc >> 3;
    x = bloc++ & 7;
    if( !x )
    {
        fout[y] = 0;
    }
    fout[y] |= bit << x;
}

//bani - optimized version
//optimization works on gcc 3.x, but not 2.95 ? most curious.
sint idHuffmanSystemLocal::get_bit( uchar8* fin )
{
    sint             t;
    
    t = fin[bloc >> 3] >> ( bloc & 7 ) & 0x1;
    bloc++;
    return t;
}

node_t** idHuffmanSystemLocal::get_ppnode( huff_t* huff )
{
    node_t**        tppnode;
    
    if( !huff->freelist )
    {
        return &( huff->nodePtrs[huff->blocPtrs++] );
    }
    else
    {
        tppnode = huff->freelist;
        huff->freelist = ( node_t** ) * tppnode;
        return tppnode;
    }
}

void idHuffmanSystemLocal::free_ppnode( huff_t* huff, node_t** ppnode )
{
    *ppnode = ( node_t* ) huff->freelist;
    huff->freelist = ppnode;
}

/* Swap the location of these two nodes in the tree */
void idHuffmanSystemLocal::swap( huff_t* huff, node_t* node1, node_t* node2 )
{
    node_t*         par1, *par2;
    
    par1 = node1->parent;
    par2 = node2->parent;
    
    if( par1 )
    {
        if( par1->left == node1 )
        {
            par1->left = node2;
        }
        else
        {
            par1->right = node2;
        }
    }
    else
    {
        huff->tree = node2;
    }
    
    if( par2 )
    {
        if( par2->left == node2 )
        {
            par2->left = node1;
        }
        else
        {
            par2->right = node1;
        }
    }
    else
    {
        huff->tree = node1;
    }
    
    node1->parent = par2;
    node2->parent = par1;
}

/* Swap these two nodes in the linked list (update ranks) */
void idHuffmanSystemLocal::swaplist( node_t* node1, node_t* node2 )
{
    node_t*         par1;
    
    par1 = node1->next;
    node1->next = node2->next;
    node2->next = par1;
    
    par1 = node1->prev;
    node1->prev = node2->prev;
    node2->prev = par1;
    
    if( node1->next == node1 )
    {
        node1->next = node2;
    }
    if( node2->next == node2 )
    {
        node2->next = node1;
    }
    if( node1->next )
    {
        node1->next->prev = node1;
    }
    if( node2->next )
    {
        node2->next->prev = node2;
    }
    if( node1->prev )
    {
        node1->prev->next = node1;
    }
    if( node2->prev )
    {
        node2->prev->next = node2;
    }
}

/* Do the increments */
void idHuffmanSystemLocal::increment( huff_t* huff, node_t* node )
{
    node_t*         lnode;
    
    if( !node )
    {
        return;
    }
    
    if( node->next != nullptr && node->next->weight == node->weight )
    {
        lnode = *node->head;
        if( lnode != node->parent )
        {
            swap( huff, lnode, node );
        }
        swaplist( lnode, node );
    }
    if( node->prev && node->prev->weight == node->weight )
    {
        *node->head = node->prev;
    }
    else
    {
        *node->head = nullptr;
        free_ppnode( huff, node->head );
    }
    node->weight++;
    if( node->next && node->next->weight == node->weight )
    {
        node->head = node->next->head;
    }
    else
    {
        node->head = get_ppnode( huff );
        *node->head = node;
    }
    if( node->parent )
    {
        increment( huff, node->parent );
        if( node->prev == node->parent )
        {
            swaplist( node, node->parent );
            if( *node->head == node )
            {
                *node->head = node->parent;
            }
        }
    }
}

void idHuffmanSystemLocal::addRef( huff_t* huff, uchar8 ch )
{
    node_t*         tnode, *tnode2;
    
    if( huff->loc[ch] == nullptr )
    {
        /* if this is the first transmission of this node */
        tnode = &( huff->nodeList[huff->blocNode++] );
        tnode2 = &( huff->nodeList[huff->blocNode++] );
        
        tnode2->symbol = INTERNAL_NODE;
        tnode2->weight = 1;
        tnode2->next = huff->lhead->next;
        if( huff->lhead->next )
        {
            huff->lhead->next->prev = tnode2;
            if( huff->lhead->next->weight == 1 )
            {
                tnode2->head = huff->lhead->next->head;
            }
            else
            {
                tnode2->head = get_ppnode( huff );
                *tnode2->head = tnode2;
            }
        }
        else
        {
            tnode2->head = get_ppnode( huff );
            *tnode2->head = tnode2;
        }
        huff->lhead->next = tnode2;
        tnode2->prev = huff->lhead;
        
        tnode->symbol = ch;
        tnode->weight = 1;
        tnode->next = huff->lhead->next;
        if( huff->lhead->next )
        {
            huff->lhead->next->prev = tnode;
            if( huff->lhead->next->weight == 1 )
            {
                tnode->head = huff->lhead->next->head;
            }
            else
            {
                /* this should never happen */
                tnode->head = get_ppnode( huff );
                *tnode->head = tnode2;
            }
        }
        else
        {
            /* this should never happen */
            tnode->head = get_ppnode( huff );
            *tnode->head = tnode;
        }
        huff->lhead->next = tnode;
        tnode->prev = huff->lhead;
        tnode->left = tnode->right = nullptr;
        
        if( huff->lhead->parent )
        {
            if( huff->lhead->parent->left == huff->lhead )
            {
                /* lhead is guaranteed to by the NYT */
                huff->lhead->parent->left = tnode2;
            }
            else
            {
                huff->lhead->parent->right = tnode2;
            }
        }
        else
        {
            huff->tree = tnode2;
        }
        
        tnode2->right = tnode;
        tnode2->left = huff->lhead;
        
        tnode2->parent = huff->lhead->parent;
        huff->lhead->parent = tnode->parent = tnode2;
        
        huff->loc[ch] = tnode;
        
        increment( huff, tnode2->parent );
    }
    else
    {
        increment( huff, huff->loc[ch] );
    }
}

/* Get a symbol */
sint idHuffmanSystemLocal::Receive( node_t* node, sint* ch, uchar8* fin )
{
    while( node && node->symbol == INTERNAL_NODE )
    {
        if( get_bit( fin ) )
        {
            node = node->right;
        }
        else
        {
            node = node->left;
        }
    }
    if( !node )
    {
        return 0;
//      Com_Error(ERR_DROP, "Illegal tree!\n");
    }
    return ( *ch = node->symbol );
}

/* Get a symbol */
void idHuffmanSystemLocal::offsetReceive( node_t* node, sint* ch, uchar8* fin, sint* offset )
{
    bloc = *offset;
    while( node && node->symbol == INTERNAL_NODE )
    {
        if( get_bit( fin ) )
        {
            node = node->right;
        }
        else
        {
            node = node->left;
        }
    }
    if( !node )
    {
        *ch = 0;
        return;
//      Com_Error(ERR_DROP, "Illegal tree!\n");
    }
    *ch = node->symbol;
    *offset = bloc;
}

/* Send the prefix code for this node */
void idHuffmanSystemLocal::send( node_t* node, node_t* child, uchar8* fout )
{
    if( node->parent )
    {
        send( node->parent, node, fout );
    }
    if( child )
    {
        if( node->right == child )
        {
            add_bit( 1, fout );
        }
        else
        {
            add_bit( 0, fout );
        }
    }
}

/* Send a symbol */
void idHuffmanSystemLocal::transmit( huff_t* huff, sint ch, uchar8* fout )
{
    sint             i;
    
    if( huff->loc[ch] == nullptr )
    {
        /* node_t hasn't been transmitted, send a NYT, then the symbol */
        transmit( huff, NYT, fout );
        for( i = 7; i >= 0; i-- )
        {
            add_bit( ( valueType )( ( ch >> i ) & 0x1 ), fout );
        }
    }
    else
    {
        send( huff->loc[ch], nullptr, fout );
    }
}

void idHuffmanSystemLocal::DynDecompress( msg_t* mbuf, sint offset )
{
    sint             ch, cch, i, j, size;
    uchar8            seq[65536];
    uchar8*           buffer;
    huff_t          huff;
    
    size = mbuf->cursize - offset;
    buffer = mbuf->data + offset;
    
    if( size <= 0 )
    {
        return;
    }
    
    ::memset( &huff, 0, sizeof( huff_t ) );
    // Initialize the tree & list with the NYT node
    huff.tree = huff.lhead = huff.ltail = huff.loc[NYT] = &( huff.nodeList[huff.blocNode++] );
    huff.tree->symbol = NYT;
    huff.tree->weight = 0;
    huff.lhead->next = huff.lhead->prev = nullptr;
    huff.tree->parent = huff.tree->left = huff.tree->right = nullptr;
    
    cch = buffer[0] * 256 + buffer[1];
    // don't overflow with bad messages
    if( cch > mbuf->maxsize - offset )
    {
        cch = mbuf->maxsize - offset;
    }
    bloc = 16;
    
    for( j = 0; j < cch; j++ )
    {
        ch = 0;
        // don't overflow reading from the messages
        // FIXME: would it be better to have a overflow check in get_bit ?
        if( ( bloc >> 3 ) > size )
        {
            seq[j] = 0;
            break;
        }
        Receive( huff.tree, &ch, buffer );	/* Get a character */
        if( ch == NYT )
        {
            /* We got a NYT, get the symbol associated with it */
            ch = 0;
            for( i = 0; i < 8; i++ )
            {
                ch = ( ch << 1 ) + get_bit( buffer );
            }
        }
        
        seq[j] = ch;			/* Write symbol */
        
        addRef( &huff, ( uchar8 ) ch );	/* Increment node */
    }
    mbuf->cursize = cch + offset;
    ::memcpy( mbuf->data + offset, seq, cch );
}

void idHuffmanSystemLocal::DynCompress( msg_t* mbuf, sint offset )
{
    sint             i, ch, size;
    uchar8            seq[65536];
    uchar8*           buffer;
    huff_t          huff;
    
    size = mbuf->cursize - offset;
    buffer = mbuf->data + +offset;
    
    if( size <= 0 )
    {
        return;
    }
    
    ::memset( &huff, 0, sizeof( huff_t ) );
    // Add the NYT (not yet transmitted) node into the tree/list */
    huff.tree = huff.lhead = huff.loc[NYT] = &( huff.nodeList[huff.blocNode++] );
    huff.tree->symbol = NYT;
    huff.tree->weight = 0;
    huff.lhead->next = huff.lhead->prev = nullptr;
    huff.tree->parent = huff.tree->left = huff.tree->right = nullptr;
    huff.loc[NYT] = huff.tree;
    
    seq[0] = ( size >> 8 );
    seq[1] = size & 0xff;
    
    bloc = 16;
    
    for( i = 0; i < size; i++ )
    {
        ch = buffer[i];
        transmit( &huff, ch, seq );	/* Transmit symbol */
        addRef( &huff, ( uchar8 ) ch );	/* Do update */
    }
    
    bloc += 8;					// next byte
    
    mbuf->cursize = ( bloc >> 3 ) + offset;
    ::memcpy( mbuf->data + offset, seq, ( bloc >> 3 ) );
}

sint	idHuffmanSystemLocal::ReadBit( uchar8* buffer, sint bitIndex )
{
    return ( buffer[( bitIndex >> 3 )] >> ( bitIndex & 7 ) ) & 0x1;
}


void idHuffmanSystemLocal::WriteBit( sint bit, uchar8* buffer, sint bitIndex )
{
    // is this the first bit of a new byte?
    if( ( bitIndex & 7 ) == 0 )
    {
        buffer[bitIndex >> 3] = 0;
    }
    buffer[bitIndex >> 3] |= bit << ( bitIndex & 7 );
}


sint idHuffmanSystemLocal::ReadSymbol( sint* symbol, uchar8* buffer, sint bitIndex )
{
    const uchar16 code = ( ( *( const uint* )( buffer + ( bitIndex >> 3 ) ) ) >> ( ( uint )bitIndex & 7 ) ) & 0x7FF;
    const uchar16 entry = huff_decodeTable[code];
    
    *symbol = ( sint )( entry & 0xFF );
    
    return ( sint )( entry >> 8 );
}


sint idHuffmanSystemLocal::WriteSymbol( sint symbol, uchar8* buffer, sint bitIndex )
{
    const uchar16 entry = huff_encodeTable[symbol];
    const sint bitCount = ( sint )( entry & 15 );
    const sint code = ( sint )( ( entry >> 4 ) & 0x7FF );
    sint bits = ( sint )code;
    
    for( sint i = 0; i < bitCount; i++ )
    {
        WriteBit( bits & 1, buffer, bitIndex );
        bits >>= 1;
        bitIndex++;
    }
    
    return bitCount;
}

