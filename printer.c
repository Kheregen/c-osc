#include <stdio.h>
#include <string.h>

#include "osc.h"
#include "printer.h"

void print_timetag(const struct osc_timetag * t) {
    printf("[sec: %u, frac: %u]", osc_unpack_int32(t->sec), osc_unpack_int32(t->frac));
}

#ifdef OSC_TT_BLOB
void print_blob(const osc_blob b) {
    size_t len = osc_blob_data_size(b);
    printf("{delka: %lu, data: 0x", len);

    uint8_t * buff = (uint8_t *)osc_blob_data_ptr(b);

    for (size_t i = 0; i < len; ++i){
        printf("%hhx", buff[i]);
    }
    printf("}");
}
#endif

void print_message(struct osc_message * msg) {
    const char * prefix = "\t";

    uint32_t argc = osc_message_argc(msg);

    printf("Zprava: %s, typetag: %s, argc: %u\n", msg->address, msg->typetag, argc);

    for (uint32_t i = 0; i < argc; ++i) {
        if (msg->typetag[i] == 0) {
            fprintf(stderr, "Error, typetag ending with \\0 on position %u!", i);
            break;
        }

        const union osc_msg_argument * arg = osc_message_arg(msg, i);
        if (arg == NULL) {
            fprintf(stderr, "Error, argument on %u returned as NULL!", i);
            continue;
        }

        printf("%sarg%u: ", prefix, i);
        switch (msg->typetag[i+1]){
            case OSC_TT_INT:     printf("%d", osc_unpack_int32(arg->i)); break;
            case OSC_TT_FLOAT:   printf("%f", osc_unpack_float(arg->f)); break;
            case OSC_TT_STRING:  printf("%s", &arg->s); break;
            case OSC_TT_TIMETAG: print_timetag(&arg->t); break;
#ifdef OSC_TT_BLOB
            case OSC_TT_BLOB: print_blob((void *)&arg->b); break;
#endif
            default: printf("unknown"); break;
        }
        printf("\n");
    }
}
void print_bundle(struct osc_bundle * bn){
    printf("Bundle: timetag: ");
    print_timetag(bn->timetag);
    printf("\n");

    struct osc_message msg;
    OSC_MESSAGE_NULL(&msg);

    while ((msg = osc_bundle_next_message(bn, msg)).raw_data != NULL) {
        print_message(&msg);
    }
}

void print_payload(void * msg, size_t data_size __attribute__((unused))) {
    if (strncmp((char *)msg+sizeof(int32_t), "#bundle", 8) == 0){
        printf("Data zacinaji OSC retezcem '#bundle', interpretuji jako bundle...\n");
        struct osc_bundle bundle;
        bundle.raw_data = msg;
        bundle.timetag = (struct osc_timetag *)((char *)bundle.raw_data + 12);
        print_bundle(&bundle);
    } else {
        printf("Data nezacinaji platnym OSC retezcem '#bundle', interpretuji jako zpravu...\n");
        struct osc_message message;
        message.raw_data = msg;
        message.address = (char *)message.raw_data + 4;
        message.typetag = (char *)message.address + ((strlen(message.address)+4) & ~0x3);
        print_message(&message);
    }
}
