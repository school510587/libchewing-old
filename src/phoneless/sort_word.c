/**
 * sort_word.c
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "internal/chewing-utf8-util.h"
#include "phl.h"
#include "plat_mmap.h"
#define CHARDEF "%chardef"
#define BEGIN "begin"
#define END "end"
#define DO_WORD_ERROR (1)
#define MAX_WORD        (60000)
#define MAX_NUMBER      (12000)
#define MAX_BUF_LEN     (4096)
// Field num[2] of old WORD_DATA is replaced by keyseq.
typedef struct {
        uint32 keyseq;
        char word[8];
} WORD_DATA;

WORD_DATA word_data[ MAX_WORD ];
int nWord;
/*
 * clear_comment()
 *   For any line in *.cin format, clear its 
 * comment after tab. Here, this function 
 * works on the begin and end of CHARDEF block.
 */
void clear_comment(char* line){
        for(; *line!='\0' && *line!='\t' && *line!='\n' && *line!='\r'; line++);
        *line='\0'; // Comment (if exists) is cleared.
}
/**
 * SortKeyin()
 *   This function compares two WORD_DATA 
 * objects by keyseq field. If several words 
 * share the same keyin, the order of words 
 * is unpredictable, depending on qsort() 
 * function in <stdlib.h>.
 */
int SortKeyin(const void *a, const void *b){
        const WORD_DATA *p=(const WORD_DATA*)a, *q=(const WORD_DATA*)b;
        return p->keyseq-q->keyseq;
}
/**
 * SortWord()
 *   This function compares two WORD_DATA 
 * objects by its word field. If several 
 * keyins map to the same word, the order 
 * of keyins is unpredictable, depending on 
 * qsort() function in <stdlib.h>.
 */
int SortWord(const void* a, const void* b){
        WORD_DATA *p=(WORD_DATA*)a, *q=(WORD_DATA*)b;
        int c=strcmp(p->word, q->word);
        return c!=0 ? c : SortKeyin(a, b);
}

int DoWord( const char *line )
{
        char buffer[8];
        int i;
        if(sscanf(line, "%s%s", buffer, word_data[nWord].word)!=2){
                fputs("format error\n", stderr);
                return DO_WORD_ERROR;
        }
        if(strlen(buffer)>4){
                fprintf(stderr, "Too long keyin: %s\n", buffer);
                return DO_WORD_ERROR;
        }
        word_data[nWord++].keyseq=key2uint(buffer);
        return 0;
}
/**
 * OutputKeyIndex()
 *   This function is modified from Output() 
 * function in libchewing/src/tools/sort_word.c 
 * file. In this function, dict_file becomes an 
 * input from previously built dictionary. File 
 * index2dict sequencially stores indices for 
 * each word in word_data in type of long. If 
 * USE_BINARY_DATA is enabled, seq_file plays 
 * the same role as indexfile2 in the original 
 * source code. Namely, key_index is the same 
 * as indexfile.
 */
void OutputKeyIndex(){
        FILE *key_index, *index2dict, *dict_file;
        const char *dict, *p;
        int seq_num=0, i;
        uint32 previous=0;
        long file_size=0;
        plat_mmap dict_map;
        size_t offset=0, csize;
        int tmp;
        unsigned char size;
#ifdef USE_BINARY_DATA
        FILE *seq_file;
        key_index=fopen(PHL_INDEX_BEGIN_FILE, "wb");
        if(!key_index){
                perror(PHL_INDEX_BEGIN_FILE);
                exit(1);
        }
        seq_file=fopen(PHL_INDEX_KEY_FILE, "wb");
        if(!seq_file){
                perror(PHL_INDEX_KEY_FILE);
                exit(1);
        }
        index2dict=fopen(PHL_INDEX2CHDICT, "wb");
#else
        key_index=fopen(PHL_INDEX_FILE, "w");
        if(!key_index){
                perror(PHL_INDEX_FILE);
                exit(1);
        }
        index2dict=fopen(PHL_INDEX2CHDICT, "w");
#endif
        if(!index2dict){
                perror(PHL_INDEX2CHDICT);
                exit(1);
        }
        plat_mmap_set_invalid(&dict_map);
        file_size=plat_mmap_create(&dict_map, (".." PLAT_SEPARATOR ".." PLAT_SEPARATOR CHAR_FILE), FLAG_ATTRIBUTE_READ);
        csize=file_size;
        dict=(const char*)plat_mmap_set_view(&dict_map, &offset, &csize);
        if(!dict){
                fprintf(stderr, "Cannot read in dictionary.\n");
                exit(1);
        }
        for(i=0; i<nWord; i++){
                if(word_data[i].keyseq!=previous){
                        previous=word_data[i].keyseq;
#ifdef USE_BINARY_DATA
                        tmp=ftell(index2dict);
                        fwrite(&tmp, sizeof(tmp), 1, key_index);
                        fwrite(&previous, sizeof(previous), 1, seq_file);
#else
                        fprintf(key_index, "%x %ld\n", previous, ftell(index2dict));
#endif
                        seq_num++;
                }
                p=ueStrstr(dict, file_size, word_data[i].word, strlen(word_data[i].word));
                if(p==NULL){ // This word does not exist in dictionary.
                        fprintf(stderr, "Warning: Word `%s' is not supported now. It must be ignored.\n", word_data[i].word);
                        continue;
                }
#ifdef USE_BINARY_DATA
                tmp=p-dict-1;
                fwrite(&tmp, sizeof(tmp), 1, index2dict);
#else
                while(p>dict && *(p-1)!='\t') p--;
                fprintf(index2dict, "%x %d\t", word_data[i].keyseq, p-dict);
#endif
        }
#ifdef USE_BINARY_DATA // the ending entry
        tmp=file_size;
        fwrite( &tmp, sizeof(tmp), 1, key_index );
        previous = 0;
        fwrite( &previous, sizeof(previous), 1, seq_file );
        fclose(seq_file);
#else
        fprintf(key_index, "0 %ld\n", file_size);
        fclose(dict_file);
#endif
        plat_mmap_close(&dict_map);
        fclose(key_index);
        fclose(index2dict);
}
/**
 * OutputWordIndex()
 *   This function is modified from Output() 
 * function in libchewing/src/tools/sort_word.c 
 * file. In this function, only 1 output file 
 * is generated, which is largely different 
 * from old output policies in chewing. The 
 * output is used in src/phoneless/sort_dic.c 
 * so far. It is a single file for convenience. 
 * Each data contains word, number of keyins, 
 * and all keyin values. Except word, fields 
 * are 32-bit uint with USE_BINARY_DATA and 
 * hex strings separated by '\t's and '\n's 
 * without USE_BINARY_DATA. In addition, there 
 * is a 8-bit length field in front of each 
 * word in binary file. The first 4 bytes 
 * of binary file and first line of text 
 * file represent number of words, which may 
 * be different from pre-defined macro in 
 * chewing-definition.h.
 */
void OutputWordIndex(){
        FILE* result;
        const char *p, *current;
        int i, j=0, k;
#ifdef USE_BINARY_DATA
        int count;
        unsigned char size;
        result=fopen(REV_INDEX_FILE, "wb");
#else
        result=fopen(REV_INDEX_FILE, "w");
#endif
        if(!result){
                perror(REV_INDEX_FILE);
                exit(1);
        }
#ifdef USE_BINARY_DATA
        fwrite(&nWord, sizeof(nWord), 1, result);
#else
        fprintf(result, "%x\n", nWord);
#endif
        do{ // Fetch groups of keyins sharing the same word.
                i=j; // The group is in interval [i, j).
                current=word_data[i].word;
                while(j<nWord && !strcmp(word_data[j].word, current)) j++;
#ifdef USE_BINARY_DATA
                size=strlen(current);
                count=j-i;
                fwrite(&size, sizeof(size), 1, result);
                fwrite(current, sizeof(char), size, result);
                fwrite(&count, sizeof(count), 1, result);
                for(k=i; k<j; k++) fwrite(&word_data[k].keyseq, sizeof(uint32), 1, result);
#else
                fprintf(result, "%s\t%x", current, j-i);
                for(k=i; k<j; k++) fprintf(result, "\t%x", word_data[k].keyseq);
                fputc('\n', result);
#endif
        }while(j<nWord);
        fclose(result);
}
int main(int argc, char* argv[]){
        FILE *cinfile;
        char buf[ MAX_BUF_LEN ], **cin_name;
        if(argc<2){
                fprintf(stderr, "%s: Too few argument(s).\n", argv[0]);
                fputs("  Make sure that you've given *.cin input.\n", stderr);
        }
        for(cin_name=argv+1; cin_name<argv+argc; cin_name++){
                size_t l=strlen(*cin_name), sl=strlen(PLAT_SEPARATOR);
                if(l>4 && !strcmp((*cin_name)+l-4, ".cin")) break;
        } // pick out an argument fitting in the form *.cin
        if(*cin_name==NULL){
                fputs("No *.cin file name.\n", stderr);
                return 1;
        }
        cinfile=fopen(*cin_name, "r");
        if ( ! cinfile ) {
                perror(*cin_name);
                return 1;
        }

        for(;;){
                if(fgets(buf, MAX_BUF_LEN, cinfile)==NULL){
                        fprintf(stderr, "%s: No expected `%s %s' line.\n", *cin_name, CHARDEF, BEGIN);
                        exit(1);
                }
                clear_comment(buf);
                if(!strcmp(strtok(buf, " "), CHARDEF)){
                        if(!strcmp(strtok(NULL, " "), BEGIN)) break;
                }
        }
        for(;;){
                fgets( buf, MAX_BUF_LEN, cinfile );
                if(buf[0]=='%' || buf[0]=='\0') break; // End of block or file.
                else if(buf[0]=='\n') continue; // Empty lines are ignored.
                if ( DoWord(buf)==DO_WORD_ERROR){
                        fprintf(stderr, "The file %s is corrupted!\n", *cin_name);
                        return 1;
                }
        }
        fclose( cinfile );
        clear_comment(buf);
        if(strcmp(strtok(buf, " "), CHARDEF)){
                fprintf(stderr, "%s: %s block does not end with the same keyword %s!\n", *cin_name, CHARDEF, CHARDEF);
                return 1;
        }
        if(strcmp(strtok(NULL, " "), END)){
                fprintf(stderr, "%s: It is not `%s' follows %s at the end of %s block!\n", *cin_name, END, CHARDEF, CHARDEF);
                exit(1);
        }
        if(nWord>0){
                qsort(word_data, nWord, sizeof(WORD_DATA), SortKeyin);
                OutputKeyIndex();
                /**
                 * Note: The following qsort()
                 * call is disabled because 
                 * sort_dic.c applies sequential 
                 * search rather than binary 
                 * search.
                 */
                //qsort(word_data, nWord, sizeof(WORD_DATA), SortWord);
                OutputWordIndex();
        }
        else{
                fprintf(stderr, "Warning: %s has an empty %s block.\n", *cin_name, CHARDEF);
                return 1;
        }
        return 0;
}

