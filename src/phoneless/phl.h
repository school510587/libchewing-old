#ifndef PHL_H
#define PHL_H
#include "internal/global-private.h"
/**
 *  Keyin-to-word index files.
 * 
 *   The following 4 definitions works the 
 * same as CHAR_FILE, CHAR_INDEX_FILE, 
 * CHAR_INDEX_BEGIN_FILE, CHAR_INDEX_PHONE_FILE 
 * respectively.
 */
#define PHL_INDEX2CHDICT "index2chdict.dat"
#define PHL_INDEX_FILE "phl_index.dat"
#define PHL_INDEX_BEGIN_FILE "phl_index_begin.dat"
#define PHL_INDEX_KEY_FILE "phl_index_key.dat"
/**
 *   Keyin-to-phrase index files.
 * 
 *   The following 3 definitions works the
 * same as PHONE_TREE_FILE, DICT_FILE and 
 * PH_INDEX_FILE respectively.
 */
#define PHL_KEY_TREE_FILE "key_tree.dic"
#define PHL_INDEX2PHDICT "index2phdict.dat"
#define PHL_SQ_INDEX_FILE "sq_index.dat"
/**
 * Word-to-keyin index file.
 * 
 *   The following file is initially designed 
 * for sort_dic.c to retrieve keyin sequences 
 * by words. Detailed comment is in sort_word.c 
 * file.
 */
#define REV_INDEX_FILE "rev_index.dat"
/**
 *   TMP_OUT is a temporary file generated 
 * by sort_dic, which serves the same as 
 * phoneid.dic in phonetic system.
 */
#define TMP_OUT "keyid.dic"
/**
 * uint32
 *   All phoneless data is expressed in 
 * unsigned long type. Here uint32 is a 
 * short name for it.
 */
typedef unsigned long uint32;
/**
 * key2uint()
 *   This function converts byte sequence 
 * into uint32 type. The argument can be 
 * char / unsigned char buffer, or any 
 * reasonable pointer. The input buffer 
 * is interpreted as unsigned char buffer 
 * and is read until '\0'. Only the last 
 * 4 characters before '\0' is written 
 * into the result from high to low.
 * (I.E. "abcd" generates 0x61626364.)
 */
uint32 key2uint(const void*);
/**
 * PHLTreeType
 * 
 *   This structure is modified from TreeType 
 * in include/internal/chewing-private.h file.
 * The only difference is that phone_id field 
 * becomes keyin_id.
 */
typedef struct{
        uint32 keyin_id;
        int phrase_id, child_begin, child_end;
} PHLTreeType;
#endif

