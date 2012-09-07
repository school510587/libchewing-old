/**
 * sort_dic.c
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
 * @file  sort_dic.c
 * @brief Sort and Index dictionary by key.
 *        Input:
 *        - data/dict.dat (content of dictionary)
 *        - data/phoneless/<id>/rev_index.dat (reverse index)
 *        Output:
 *        - data/phoneless/<id>/sq_index.dat (phrase dictionary index)
 *        - data/phoneless/<id>/index2phdict.dat
 *        - data/phoneless/<id>/key_tree.dic
 *        Read dictionary format :
 *        phrase frequency zuin1 zuin2 zuin3 ... \n
 *        Output format for sq_index.dat: (Sorted by key sequence)
 *        index frequency keyin1 keyin2 keyin3 ... \n
 *        Note: This program must run in data/phoneless/<id>/ directory.
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "internal/chewing-utf8-util.h"
#include "internal/global-private.h"
#include "phl.h"
#include "private.h"
#include "plat_mmap.h"
#define DICT_PATH (".." PLAT_SEPARATOR ".." PLAT_SEPARATOR DICT_FILE)
#define MAXLEN          149
#define MAXKEYIN        9
/**
 * struct RECORD
 *   This structure has 4 differences compared 
 * with original RECORD in src/tools/sort_dic.c:
 * 1. Field "phone" disappears. See the next 
 * structure for more information.
 * 2. An additional field "lstr" is used to 
 * keep the length of phrase in UTF-8 words. 
 * Also, it helps recognize whether the phrase 
 * contained in this RECORD is supported by 
 * the *.cin file from developper.
 * 3. This RECORD has an extra field "pos", which 
 * refers to position in DICT_FILE of each phrase.
 * 4. This RECORD has another extra field "next", 
 * which points to the next different phrase. It 
 * simulates a linked list of arrays, with next 
 * pointing to the next node (head of an array).
 */
typedef struct{
        char str[MAXLEN];
        int freq, lstr, next;
        long pos;
} RECORD;
/**
 * PHL_PHRASE
 *   The structure denotes a phoneless phrase. 
 * Instances sharing the same Chinese phrase 
 * have the same pointer value of phrase field. 
 * Field key[] serves the same as "phone" in 
 * src/tools/sort_dic.c, each phone becoming 
 * keyin sequence. Their length are the same, 
 * but they are given two different symbolic 
 * names.
 */
typedef struct{
        const RECORD* phrase;
        uint32 key[MAXKEYIN];
} PHL_PHRASE;
RECORD data[420000L];
PHL_PHRASE* phl_phr;
long nData, phl_phr_count;
size_t rev_data_len;
const char* rev_data; // Buffer for rev_map.
/**
 * ReadData()
 *   This function reads string, frequency 
 * and position for all phrases in DICT_FILE.
 */
int ReadData(const char* dict_path){
        plat_mmap dict_content;
        long curpos=0, file_size;
        size_t offset=0, csize;
        char* dict;
        plat_mmap_set_invalid(&dict_content);
        file_size=plat_mmap_create(&dict_content, dict_path, FLAG_ATTRIBUTE_READ);
        csize=file_size;
        dict=(char*)plat_mmap_set_view(&dict_content, &offset, &csize);
        if(dict==NULL) return EXIT_FAILURE;
        for(; curpos<file_size; nData++){
                data[nData].pos=curpos;
#ifdef USE_BINARY_DATA
                unsigned char size=dict[curpos++];
                if(curpos+size>file_size) return EXIT_FAILURE;
                strncpy(data[nData].str, dict+curpos, size);
                data[nData].str[size]='\0';
                curpos+=size;
                if(curpos+sizeof(data[nData].freq)>file_size) return EXIT_FAILURE;
                memcpy(&data[nData].freq, dict+curpos, sizeof(data[nData].freq));
                curpos+=sizeof(data[nData].freq);
#else
                if(sscanf(curpos, "%s%d", data[nData].str, &data[nData].freq)!=2) return EXIT_FAILURE;
                char* p=strchr(dict+curpos, '\t');
                curpos= p?(p-dict+1):file_size;
#endif
        }
        plat_mmap_close(&dict_content);
        return EXIT_SUCCESS;
}
/**
 * CountPHLPhrases()
 *   This function counts how many "phrases" 
 * will be generated (I.E. size of phl_phrase 
 * array). It also eliminates the same objects 
 * with the same str field, which is due to 
 * the same Chinese phrases having different 
 * phones.
 * Note: The policy to determine frequency 
 * of multiple same-word-different-zuin 
 * phrases is unbiased average so far.
 */
void CountPHLPhrases(){
        long i, j;
        int fq;
        const char *p, *q;
        for(i=0; i<nData; i=data[i].next){
                int b;
                size_t c=1, t; // 1 is multiplicative zero.
                for(j=i+1, fq=0; j<nData && !strcmp(data[j-1].str, data[j].str); j++) fq+=data[j-1].freq;
                data[i].lstr=ueStrLen(data[i].str);
                data[i].next=j;
                for(j=0; j<data[i].lstr; j++){ // For each UTF-8 word.
                        p=ueStrSeek(data[i].str, j); // The j-th word.
                        b=ueBytesFromChar(*p);
                        if(q=ueStrstr(rev_data, rev_data_len, p, b)){
                                memcpy(&t, q+b, sizeof(t));
                                c*=t;
                        }
                        else{
                                c=0;
                                break;
                        } // The phrase contains unsupported word(s).
                }
                if(c>0){ // It is a supported phrase.
                        data[i].freq+=fq;
                        if(data[i].next>i+1) data[i].freq=(int)round(data[i].freq/(double)(data[i].next-i));
                        phl_phr_count+=c;
                } // Unsupported phrases are recognized by their lstr fields.
                else data[i].lstr=EOF;
        }
}
/**
 * PhraseSetKey()
 *   This function finds all possibilities 
 * of key sequences for each phrase. p is 
 * the record providing a Chinese phrase. 
 * w is the position where the function is 
 * currently. This function has a recursive 
 * call of itself in its main for-loop, which 
 * causes a stack depth of length equal to 
 * the length of target Chinese phrase (see 
 * RECORD for RECORD::lstr). Note that this 
 * function must be called with w=0 elsewhere, 
 * which enables initialization of x. Also, 
 * it uses phl_phr_count to position in 
 * phl_phr, so phl_phr_count is set to 0 
 * before calling this function.
 */
void PhraseSetKey(const RECORD* r, int w){
        static uint32 x[MAXKEYIN]={0};
        if(w<r->lstr){
                const char *p, *q;
                int b, c, i, t;
                if(w==0) memset(x, 0, sizeof(x)); // Initialization.
                else if(w<0) return;
                p=ueStrSeek((char*)r->str, w);
                b=ueBytesFromChar(*p);
                q=ueStrstr(rev_data, rev_data_len, p, b);
#ifdef USE_BINARY_DATA
                memcpy(&c, q+=b, sizeof(c));
                q+=sizeof(c); // q points to the 1st keyin.
#else
                q=strchr(q, '\t');
                sscanf(q, "%x", &c);
#endif
                for(i=0; i<c; i++){ // For each possible input.
#ifdef USE_BINARY_DATA
                        memcpy(&t, q, sizeof(t));
                        q+=sizeof(t);
#else
                        q=strchr(q, '\t');
                        sscanf(q, "%x", &t);
#endif
                        x[w]=t; // w-th key is determined.
                        PhraseSetKey(r, w+1);
                }
        }
        else{ // A keyin sequence is determined in x.key field.
                phl_phr[phl_phr_count].phrase=r;
                memcpy(phl_phr[phl_phr_count].key, x, sizeof(x));
                phl_phr_count++; // New keyin is inserted.
        }
}
/**
 * CompPHLPhrase()
 *   This function compares two PHL_PHRASE 
 * objects using their key fields. If the 
 * same key is given, they are sorted using 
 * freq field.
 */
int CompPHLPhrase(const void* a, const void* b){
        PHL_PHRASE *p=(PHL_PHRASE*)a, *q=(PHL_PHRASE*)b;
        long cmp, i;
        for(i=0; i<MAXKEYIN; i++){
                cmp=p->key[i]-q->key[i];
                if(cmp) return cmp;
        }
        return p->phrase->freq-q->phrase->freq;
}
/**
 * CompRecord()
 *   This CompRecord function compares two 
 * RECORD objects using their str fields. 
 * If the same str is given, they are sorted 
 * using freq field.
 */
int CompRecord(const void* a, const void* b){
        RECORD *p=(RECORD*)a, *q=(RECORD*)b;
        int cmp=strcmp(p->str, q->str);
        return cmp ? cmp : (p->freq-q->freq);
}
/**
 * CompKey()
 *   This function is rewritten from CompUint() 
 * function. It returns 1 when sequences of 
 * data[a] and data[b] are not the same.
 */
int CompKey(long a, long b){
        long i;
        for(i=0; i<MAXKEYIN; i++) if(phl_phr[a].key[i]!=phl_phr[b].key[i]) return 1;
        return 0;
}
int main(int argc, char* argv[]){
        FILE *sq_index, *tree_data, *index2dict;
        plat_mmap rev_map;
        long i, k;
        size_t offset=0, csize;
        int tmp;
#ifdef USE_BINARY_DATA
      //  unsigned char size;
#endif
#ifdef USE_BINARY_DATA
        sq_index=fopen(PHL_SQ_INDEX_FILE, "wb");
#else
        sq_index=fopen(PHL_SQ_INDEX_FILE, "w");
#endif
        if(!sq_index){
                perror(PHL_SQ_INDEX_FILE);
                exit(1);
        }
#ifdef USE_BINARY_DATA
        index2dict=fopen(PHL_INDEX2PHDICT, "wb");
#else
        index2dict=fopen(PHL_INDEX2PHDICT, "w");
#endif
        if(!index2dict){
                perror(PHL_INDEX2PHDICT);
                exit(1);
        }
        tree_data=fopen(TMP_OUT, "w");
        if(!tree_data){
                perror(TMP_OUT);
                return 1;
        }
        plat_mmap_set_invalid(&rev_map);
        rev_data_len=plat_mmap_create(&rev_map, REV_INDEX_FILE, FLAG_ATTRIBUTE_READ);
        csize=rev_data_len;
        rev_data=(const char*)plat_mmap_set_view(&rev_map, &offset, &csize);
        if(ReadData(DICT_PATH)!=EXIT_SUCCESS){
                fprintf(stderr, "%s: Phrase dictionary file is corrupted.\n", argv[0]);
                return EXIT_FAILURE;
        }
        qsort(data, nData, sizeof(RECORD), CompRecord);
        CountPHLPhrases();
        phl_phr=ALC(PHL_PHRASE, phl_phr_count);
        if(phl_phr==NULL){
                fprintf(stderr, "%s: Cannot allocate %ld instances for phoneless phrases.\n", argv[0], phl_phr_count);
                return 1;
        }
        phl_phr_count=0; // Then re-count in PhraseSetKey().
        for(i=0; i<nData; i=data[i].next) if(data[i].lstr!=EOF) PhraseSetKey(&data[i], 0);
        qsort(phl_phr, phl_phr_count, sizeof(PHL_PHRASE), CompPHLPhrase);
        for(i=0; i<phl_phr_count; i++){
                if(i==0 || CompKey(i, i-1)!=0){
#ifdef USE_BINARY_DATA
                        tmp=ftell(index2dict);
                        fwrite(&tmp, sizeof(tmp), 1, sq_index);
#else
                        fprintf(sq_index, "%ld\n", ftell(index2dict));
#endif
                        for(k=0; phl_phr[i].key[k]; k++) fprintf(tree_data, "%lu ", phl_phr[i].key[k]);
                        fputs("0\n", tree_data);
                }
#ifdef USE_BINARY_DATA
                fwrite(&phl_phr[i].phrase->pos, sizeof(phl_phr[i].phrase->pos), 1, index2dict);
                fwrite(&phl_phr[i].phrase->freq, sizeof(phl_phr[i].phrase->freq), 1, index2dict);
#else
                fprintf(index2dict, "%ld %d\t", phl_phr[i].phrase->pos, phl_phr[i].phrase->freq);
#endif
        }
#ifdef USE_BINARY_DATA
        tmp=ftell(index2dict);
        fwrite(&tmp, sizeof(tmp), 1, sq_index);
#else
        fprintf(sq_index, "%ld\n", ftell(index2dict));
#endif
        free(phl_phr);
        fclose(sq_index);
        fclose(tree_data);
        fclose(index2dict);
        return 0;
}

