/**
 * maketree.c
 *
 * Copyright (c) 2012
 *      Bo-cheng Jan.
 *      All rights reserved.
 *
 * Copyright (c) 2012
 *      libchewing Core Team. See ChangeLog for details.
 *
 * See the file "COPYING" for information on usage and redistribution
 * of this file.
 */
/**
 * @file maketree.c
 *
 * @brief Phoneless phrase tree generator.\n
 *
 *        This program reads in a dictionary with keyin phrases(in uint32 form).\n
 *        Output a database file which indicates a phoneless phrase tree.\n
 *        Each node represents a single keyin.\n
 *        The output file was a random access file, a record was defined:\n\code
 *        { 
 *               uint32 key; the keyin data
 *               int32 phraseno; 
 *               int32 begin, end; //the children of this node(-1, -1 indicate a leaf node)
 *        }\endcode
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "config.h"
#include "phl.h"
#define MAX_KEY_NODE     6400000
struct _tLISTNODE;
typedef struct _tNODE {
        struct _tLISTNODE *childList;
        uint32 key; // Change: From uint16 to uint32.
        int phraseno,nodeno;
} NODE;
typedef struct _tLISTNODE {
        struct _tNODE *pNode;
        struct _tLISTNODE *next;
} LISTNODE;
NODE *root, *queue[MAX_KEY_NODE];
int keyin_count, head, tail, node_count, tree_size;
void QueuePut( NODE *pN )
{
        queue[ head++ ] = pN;
        if(head==MAX_KEY_NODE){
                fprintf( stderr, "Queue size is not enough!\n" );
                exit( 1 );
        }
}

NODE* QueueGet()
{
        assert( head != tail );
        return queue[ tail++ ];
}

int QueueEmpty()
{
        return ( head == tail );
}

NODE *NewNode( uint32 key )
{
        NODE *pnew = (NODE *) malloc( sizeof( NODE ) );
        pnew->key = key;
        pnew->childList = NULL;
        pnew->phraseno = -1;
        pnew->nodeno = -1;
        return pnew;
}

void InitConstruct()
{
        /* root has special key value 0 */
        root = NewNode( 0 );
}

NODE* FindKey( NODE *pN, uint32 key )
{
        LISTNODE *p;

        for ( p = pN->childList; p; p = p->next ) {
                if ( p->pNode->key == key )
                        return p->pNode;
        }
        return NULL;
}

NODE* Insert( NODE *pN, uint32 key )
{
        LISTNODE *prev, *p;
        LISTNODE *pnew = (LISTNODE *) malloc( sizeof( LISTNODE ) );
        NODE *pnode = NewNode( key );
        
        pnew->pNode = pnode;
        pnew->next  = NULL;

        prev = pN->childList;
        if ( ! prev ) {
                pN->childList = pnew;
        }
        else {
                for ( 
                        p = prev->next;
                        (p) && (p->pNode->key < key);
                        prev = p, p = p->next )
                        ;
                prev->next = pnew;
                pnew->next = p;
        }
        return pnode;
}

void Construct(const char* bin_name)
{
        FILE *input=fopen(TMP_OUT, "r");
        NODE *pointer, *tp;
        uint32 key;

        if(!input){
                fprintf(stderr, "%s: Cannot open " TMP_OUT ".\n", bin_name);
                exit(1);
        }
        InitConstruct();
        
        while ( 1 ) {   
                fscanf( input, "%lu", &key );
                if ( feof( input ) ) break;
                pointer = root;
                /* for each key in a keyin sequence */
                for ( ; key != 0; fscanf( input, "%lu", &key ) ) {      
                        if ( ( tp = FindKey( pointer, key ) ) ) {
                                pointer = tp;
                        }
                        else {
                                tp = Insert( pointer, key );
                                pointer = tp;
                        }
                }
                pointer->phraseno = keyin_count++;
        }
}
/* Give the level-order travel number to each node */
void BFS1()
{
        NODE *pNode;
        LISTNODE *pList;

        QueuePut(root);
        while ( ! QueueEmpty() ) {
                pNode = QueueGet();
                pNode->nodeno = node_count++;

                for ( pList = pNode->childList; pList; pList = pList->next ) {
                        QueuePut( pList->pNode );
                }
        }
}
/**
 * Modification of I/O scheme:
 *   This tree is generated after libchewing 
 * construction, so the size of tree must be 
 * obtained from size of file returned by 
 * plat_mmap_create() and sizeof(PHLTreeType).
 */
void BFS2(const char* bin_name)
{
        NODE *pNode;
        LISTNODE *pList;
        PHLTreeType tree={0};
        FILE* output;
#ifdef USE_BINARY_DATA
        output=fopen(PHL_KEY_TREE_FILE, "wb");
#else
        output=fopen(PHL_KEY_TREE_FILE, "w");
#endif
        if(!output){
                fprintf(stderr, "%s: Cannot open " PHL_KEY_TREE_FILE " for output.\n", bin_name);
                exit(1);
        }
        QueuePut(root);
        tree_size=0;
        while(!QueueEmpty()){
                pNode = QueueGet();
                tree.keyin_id=pNode->key;
                tree.phrase_id=pNode->phraseno;
                /* compute the begin and end index */
                pList=pNode->childList;
                if(pList){
                        tree.child_begin = pList->pNode->nodeno;
                        for(; pList->next; pList=pList->next) QueuePut(pList->pNode);
                        QueuePut( pList->pNode );
                        tree.child_end = pList->pNode->nodeno;
                }
                else tree.child_begin=tree.child_end=-1;
#ifdef USE_BINARY_DATA
                fwrite(&tree, sizeof(PHLTreeType), 1, output);
#else
                fprintf(output, "%lu %d %d %d\n", tree.keyin_id, tree.phrase_id, tree.child_begin, tree.child_end);
#endif
                tree_size++;
        }
        fclose(output);
}
int main(int argc, char* argv[])
{
        Construct(argv[0]);
        BFS1();         
        BFS2(argv[0]);
        return 0;
}

