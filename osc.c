#ifndef _BSD_SOURCE
    #define _BSD_SOURCE
#endif // _BSD_SOURCE
#include "osc.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <float.h>
#define htobe32(x) ((((x) & 0xff) << 24) | (((x) & 0xff00) << 8) | (((x) & 0xff0000) >> 8) | (((x) & 0xff000000) >> 24))
#define be32toh(x) htobe32(x)

int32_t osc_unpack_int32(int32_t data){
     return be32toh(data);
}
float osc_unpack_float(float data){
    // safe htobe32 for float
    float f = data, f2 = 0; // stored as 0x9a 0x99 0x99 0x3f
    *(uint32_t *)&f2 = htobe32(*(uint32_t *)&f); // result stored as 0x3f 0x99 0x99 0x9a
    return f2;
}

#ifndef OSC_TIMETAG_IMMEDIATE
void OSC_TIMETAG_IMMEDIATE(struct osc_timetag * tt){
    tt->sec = 0;
    tt->frac = 1;
}
#endif
#ifndef OSC_TIMETAG_NULL
void OSC_TIMETAG_NULL(struct osc_timetag * tt){
    tt->sec = 0;
    tt->frac = 0;
}
#endif
#ifndef OSC_MESSAGE_NULL
void OSC_MESSAGE_NULL(struct osc_message * msg){
    msg->address = NULL;
    msg->typetag = NULL;
    msg->raw_data = NULL;
}
#endif

size_t osc_message_serialized_length(const struct osc_message * msg){
    int32_t* delka = (int32_t*)msg->raw_data;
    int32_t real = osc_unpack_int32(*delka);
    return (size_t)real;
}
int osc_message_new(struct osc_message * msg){
    void *tmp = malloc(12);
    if(tmp == NULL){
        return 1;
    }
    int32_t *delka = (int32_t*)tmp;
    *delka = htobe32(8);
    char *message = (char*)tmp+4;
    memset(message, '\0',8);
    memset(message+4,',',1);
    msg->address = message;
    msg->typetag = message+4;
    msg->raw_data = tmp;
    return 0;
}
void osc_message_destroy(struct osc_message * msg){
    free(msg->raw_data);
    OSC_MESSAGE_NULL(msg);
}

static void size_set(char * msg, int32_t num){
    int32_t *delka = (int32_t*)msg;
    *delka = htobe32(num);
}

int osc_message_set_address(struct osc_message * msg, const char * address){
    unsigned int address_length = strlen(msg->address);
    unsigned int new_address_len = strlen(address);
    size_t msg_len = osc_message_serialized_length(msg);
    if(address_length%4 != 0 || address_length == 0){
        address_length = ((address_length/4)*4)+4;
    }
    if(new_address_len%4 != 0 || new_address_len == 0){
        new_address_len = ((new_address_len/4)*4)+4;
    }
    else{
        new_address_len = new_address_len+4;
    }
    int diff = 0;
    diff = address_length - new_address_len;
    if(diff > 0 && diff < 4){
    //nova adresa se vleze do stare
        for(unsigned int i = 0; i <address_length;i++){
            if(i < new_address_len){
                msg->address[i] = address[i];
            }
            else{
                msg->address[i] = '\0';
            }
        }
    }
    if(diff <= 0){
    // nova dresa je vetsi nez predchozi, treba zvetsit misto
        char* temp = (char*)malloc(4+msg_len);
        if(temp == NULL){
            return 1;
        }
        int typetag_len = msg_len-address_length;
        size_t new_size = 4 + new_address_len + typetag_len;
        temp = realloc(temp, new_size);
        if(temp == NULL){
            return 1;
        }
        memcpy(temp,msg->raw_data,msg_len+4);
        char* ptr = (char*)temp+(4+address_length);
        char* ptr2 = (char*)temp +(4 + new_address_len);
        memmove(ptr2,ptr,typetag_len);
        for(unsigned int i = 0; i <new_address_len;i++){
            //char* ptr = temp+4+i;
            if(i < strlen(address)){
                temp[4+i] = address[i];
            }
            else{
                temp[4+i] = '\0';
            }
        }
        free(msg->raw_data);
        size_set(temp,new_address_len + typetag_len);
        msg->raw_data = temp;
        msg->address = temp+4;
        msg->typetag = msg->address + new_address_len;
    }
    if(diff >= 4){
    // nova adresa je mensi nez stavajici, treba zmensit misto
        char* temp =(char*) malloc(4+msg_len);
        int typetag_len = msg_len-address_length;
        if(temp == NULL){
            return 1;
        }
        memcpy(temp,msg->raw_data,msg_len+4);
        char* ptr = (char*)temp+(4+address_length);
        char* ptr2 = (char*)temp +(4 + new_address_len);
        memmove(ptr2,ptr,typetag_len);
        for(unsigned int i = 0;i < new_address_len;i++){
            if(i<strlen(address)){
                temp[4+i] = address[i];
            }
            else{
                temp[4+i] = '\0';
            }
        }
        msg_len = typetag_len + new_address_len;
        char* temp2 = (char*) malloc(msg_len+4);
        memcpy(temp2,temp,msg_len+4);
        if(temp2 == NULL){
            return 1;
        }
        free(temp);
        temp = temp2;
        size_set(temp,new_address_len+typetag_len);
        free(msg->raw_data);
        msg->raw_data = temp;
        msg->address = temp+4;
        msg->typetag = temp+4 + new_address_len;
    }
    return 0;
}
int osc_message_add_int32(struct osc_message * msg, int32_t data){
    int size = osc_message_serialized_length(msg)+4;
    char* temp = (char*) malloc(size);
    if(temp == NULL){
        return 1;
    }
    memcpy(temp,msg->raw_data,size);
    char* typetag;
    typetag = temp + 4 + (strlen(temp+4)/4)*4+4;
    for(int i = 0; i>=0;i++){
        if(typetag[i] == '\0'){
            typetag[i] = OSC_TT_INT;
            if((i-3)%4 == 0){
                temp = realloc(temp,size+4);
                typetag = temp + 4 + (strlen(temp+4)/4)*4+4;
                if(temp == NULL){
                    return 1;
                }
                memmove(temp+4+(strlen(temp+4)/4)*4+4 +i+5,temp+4+(strlen(temp+4)/4)*4+4 +i+1,size-4-((strlen(temp+4)/4)*4+4)-i-1);
                for(int j =0; j<4; j++){
                    typetag[i+1+j] = '\0';
                }
                size += 4;
            }
            i = -10;
        }
    }
    temp = realloc(temp, (int)size + sizeof(int32_t));
    if(temp == NULL){
        return 1;
    }
    int32_t* arg = (int32_t*)(temp + size);
    *arg = osc_unpack_int32(data);
    size_set(temp,size + sizeof(int32_t)-4);
    free(msg->raw_data);
    msg->raw_data = temp;
    msg->address = temp+4;
    msg->typetag = msg->address + ((strlen(msg->address)/4)*4+4);
    return 0;
}
int osc_message_add_float(struct osc_message * msg, float data){
    int size = osc_message_serialized_length(msg)+4;
    char* temp = (char*) malloc(size);
    if(temp == NULL){
        return 1;
    }
    memcpy(temp,msg->raw_data,size);
    char* typetag;
    typetag = temp + 4 + (strlen(temp+4)/4)*4+4;
    for(int i = 0; i>=0;i++){
        if(typetag[i] == '\0'){
            typetag[i] = OSC_TT_FLOAT;
            if((i-3)%4 == 0){
                temp = realloc(temp,size+4);
                typetag = temp + 4 + (strlen(temp+4)/4)*4+4;
                if(temp == NULL){
                    return 1;
                }
                memmove(temp+4+(strlen(temp+4)/4)*4+4 +i+5,temp+4+(strlen(temp+4)/4)*4+4 +i+1,size-4-((strlen(temp+4)/4)*4+4)-i-1);
                for(int j =0; j<4; j++){
                    typetag[i+1+j] = '\0';
                }
                size += 4;
            }
            i = -10;
        }
    }
    temp = realloc(temp, size + sizeof(float));
    if(temp == NULL){
        return 1;
    }
    float *arg = malloc(sizeof(float));
    *arg = osc_unpack_float(data);
    memcpy(temp + size, arg, sizeof(float));
    free(arg);
    size_set(temp,size + sizeof(float)-4);
    free(msg->raw_data);
    msg->raw_data = temp;
    msg->address = temp+4;
    msg->typetag = msg->address + ((strlen(msg->address)/4)*4+4);
    return 0;
}
int osc_message_add_timetag(struct osc_message * msg, struct osc_timetag tag){
    int size = osc_message_serialized_length(msg)+4;
    char* temp = (char*) malloc(size);
    if(temp == NULL){
        return 1;
    }
    memcpy(temp,msg->raw_data,size);
    char* typetag;
    typetag = temp + 4 + (strlen(temp+4)/4)*4+4;
    for(int i = 0; i>=0;i++){
        if(typetag[i] == '\0'){
            typetag[i] = OSC_TT_TIMETAG;
            if((i-3)%4 == 0){
                temp = realloc(temp,size+4);
                typetag = temp + 4 + (strlen(temp+4)/4)*4+4;
                if(temp == NULL){
                    return 1;
                }
                memmove(temp+4+(strlen(temp+4)/4)*4+4 +i+5,temp+4+(strlen(temp+4)/4)*4+4 +i+1,size-4-((strlen(temp+4)/4)*4+4)-i-1);
                for(int j =0; j<4; j++){
                    typetag[i+1+j] = '\0';
                }
                size += 4;
            }
            i = -10;
        }
    }
    temp = realloc(temp, size + sizeof(struct osc_timetag));
    if(temp == NULL){
        return 1;
    }
    int32_t* arg1 = malloc(sizeof(int32_t));
    int32_t* arg2 = malloc(sizeof(int32_t));

    *arg1 = osc_unpack_int32(tag.sec);
    *arg2 = osc_unpack_int32(tag.frac);

    memcpy(temp + size, arg1, sizeof(int32_t));
    memcpy(temp + size+4, arg2, sizeof(int32_t));
    size_set(temp,size + sizeof(struct osc_timetag)-4);
    free(msg->raw_data);
    msg->raw_data = temp;
    msg->address = temp+4;
    msg->typetag = msg->address + ((strlen(msg->address)/4)*4+4);
    free(arg1);
    free(arg2);
    return 0;
}
int osc_message_add_string(struct osc_message * msg, const char * data){
    int size = osc_message_serialized_length(msg)+4;
    char* temp = (char*) malloc(size);
    if(temp == NULL){
        return 1;
    }
    memcpy(temp,msg->raw_data,size);
    char* typetag;
    typetag = temp + 4 + (strlen(temp+4)/4)*4+4;
    for(int i = 0; i>=0;i++){
        if(typetag[i] == '\0'){
            typetag[i] = OSC_TT_STRING;
            if((i-3)%4 == 0){
                temp = realloc(temp,size+4);
                typetag = temp + 4 + (strlen(temp+4)/4)*4+4;
                if(temp == NULL){
                    return 1;
                }
                memmove(typetag+i+5,typetag + i+1,size-4-((strlen(temp+4)/4)*4+4)-(i+1));
                for(int j =0; j<4; j++){
                    typetag[i+1+j] = '\0';
                }
                size += 4;
            }
            i = -10;
        }
    }
    temp = realloc(temp, size + (strlen(data)/4)*4 + 4);
    if(temp == NULL){
        return 1;
    }
    for(unsigned int i = 0; i<(strlen(data)/4)*4 + 4;i++){
        if(i<strlen(data)){
            temp[size+i] = data[i];
        }
        else{
            temp[size+i] = '\0';
        }
    }
    size_set(temp,size + (strlen(data)/4)*4 + 4-4);
    free(msg->raw_data);
    msg->raw_data = temp;
    msg->address = temp+4;
    msg->typetag = msg->address + ((strlen(msg->address)/4)*4+4);
    return 0;
}

size_t osc_message_argc(const struct osc_message * msg){
    size_t count = 0;
    while(msg->typetag[1+count] != '\0'){
        count++;
    }
    return count;
}

const union osc_msg_argument * osc_message_arg(const struct osc_message * msg, size_t arg_index){
    if( osc_message_argc(msg) < arg_index){
        return NULL;
    }
    else{
        char* temp;
        int help =1;
        do{
            temp = msg->typetag+help;
            help++;
        }while((msg->typetag[help-2] != '\0')||((help-1)%4 != 0));
        size_t counter = 1;
        while(counter-1 != arg_index){
            switch(msg->typetag[counter]){
            case 'i':
                temp = temp+4;
                break;
            case 'f':
                temp = temp+4;
                break;
            case 't':
                temp = temp+8;
                break;
            case 's':
                temp = temp + (strlen(temp)/4)*4+4;
                break;
            };
            counter++;
        }
        return (union osc_msg_argument* )temp;
    }
    return NULL;
}

#ifndef OSC_BUNDLE_NULL
void OSC_BUNDLE_NULL(struct osc_bundle * bnd){
    bnd->timetag = NULL;
    bnd->raw_data = NULL;
}
#endif

size_t osc_bundle_serialized_length(const struct osc_bundle * bundle){
    int32_t* delka = bundle->raw_data;
    int32_t real = osc_unpack_int32(*delka);
    return (size_t)real;
}
int osc_bundle_new(struct osc_bundle * bnd){
    char* temp= calloc(20,1);
    if(temp == NULL){
        return 1;
    }
    int32_t* size = (int32_t*)temp;
    *size = osc_unpack_int32(16);
    memcpy((char*)temp+4,"#bundle\0",8);
    struct osc_timetag ptr;
    ptr.sec = osc_unpack_int32(0);
    ptr.frac = osc_unpack_int32(1);
    memcpy(temp+12,&ptr.sec,4);
    memcpy(temp+16,&ptr.frac,4);
    if(bnd->raw_data != NULL){
        free(bnd->raw_data);
    }
    bnd->raw_data = temp;
    bnd->timetag = (struct osc_timetag*)(temp+12);
    return 0;
}

void osc_bundle_destroy(struct osc_bundle * bn){
    bn->timetag = NULL;
    free(bn->raw_data);
    bn->raw_data = NULL;
}

void osc_bundle_set_timetag(struct osc_bundle * bundle, struct osc_timetag timetag){
    bundle->timetag->sec = osc_unpack_int32(timetag.sec);
    bundle->timetag->frac = osc_unpack_int32(timetag.frac);
}
int osc_bundle_add_message(struct osc_bundle * bundle, const struct osc_message * msg){
    char* temp = malloc(osc_bundle_serialized_length(bundle)+4);
    if(temp == NULL){
        return 1;
    }
    memcpy(temp,bundle->raw_data,osc_bundle_serialized_length(bundle)+4);
    size_t msg_size = osc_message_serialized_length(msg);
    temp = realloc(temp,osc_bundle_serialized_length(bundle)+4+msg_size+4);
    if(temp == NULL){
        return 0;
    }
    memcpy(temp+4+osc_bundle_serialized_length(bundle),msg->raw_data,msg_size+4);
    size_set(temp,osc_bundle_serialized_length(bundle)+msg_size+4);
    free(bundle->raw_data);
    bundle->raw_data = temp;
    bundle->timetag = (struct osc_timetag*)(temp+12);
    return 0;
}

struct osc_message osc_bundle_next_message(const struct osc_bundle * bundle, struct osc_message prev){
    size_t bnd_size = osc_bundle_serialized_length(bundle);
    struct osc_message next;
    char* ptr = bundle->raw_data;
    if(prev.raw_data == NULL){
        if(bnd_size == 16){
            OSC_MESSAGE_NULL(&prev);
            return prev;
        }
        next.raw_data = ptr + 4 + 8 + 8;
        next.address = (char*)next.raw_data + 4;
        next.typetag = next.address + (strlen(next.address)/4)*4 +4;

        return next;
    }
    else{
        ptr = (char*)prev.raw_data + 4 + osc_message_serialized_length(&prev)-1;

        if(ptr == ((char*)bundle->raw_data + bnd_size + 4 -1)){
            OSC_MESSAGE_NULL(&prev);
            return prev;
        }
        next.raw_data = ptr+1;
        next.address = (char*)next.raw_data + 4;
        next.typetag = next.address + (strlen(next.address)/4)*4 +4;
        return next;
    }
    return next;
}
