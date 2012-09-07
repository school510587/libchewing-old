#include "phl.h"
uint32 key2uint(const void* seq){
        uint32 r=0;
        const unsigned char* p=(unsigned char*)seq;
        for(; *p!='\0'; p++)         r=(r<<8)|*p;
        return r;
}

