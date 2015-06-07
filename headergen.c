#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <ctype.h>

#include "osc.h"
#include "printer.h"

#define MENU_EOF -1

#define MENU_MAIN_QUIT 'q'
#define MENU_MAIN_HELP 'h'
#define MENU_MAIN_GENERATE 'g'
#define MENU_MAIN_BUNDLE 'b'
#define MENU_MAIN_MESSAGE 'm'

#define MENU_MESSAGE_ADDR 'a'
#define MENU_MESSAGE_DONE 'r'
#define MENU_MESSAGE_SHOW 'o'
#define MENU_MESSAGE_HELP 'h'
#define MENU_MESSAGE_ADD_INT 'i'
#define MENU_MESSAGE_ADD_FLOAT 'f'
#define MENU_MESSAGE_ADD_STRING 's'
#define MENU_MESSAGE_ADD_TIMETAG 't'
#define MENU_MESSAGE_ADD_BLOB 'b'

#define MENU_BUNDLE_DONE 'r'
#define MENU_BUNDLE_SHOW 'o'
#define MENU_BUNDLE_HELP 'h'
#define MENU_BUNDLE_TIMETAG 't'
#define MENU_BUNDLE_ADD_MESSAGE 'm'

#define TAB_SIZE 4
#define LINE_LENGTH 80
#define BIN_FMT "\\x%.2hhx"

/* srapnely funkci */
#define SETUP_INDENT(l) do {\
    size_t lx = (l); \
    assert(lx*TAB_SIZE < sizeof(indent)); \
    memset(indent, 0, sizeof(indent)); \
    memset(indent, ' ', lx*TAB_SIZE); \
} while (0)
#define MAKE_INDENT(l) char indent[20]; SETUP_INDENT(l);
#define SETUP_BREAK size_t ll_counter = 0;
#define PRINTF_IN do { \
    if (ll_counter == 0) {\
        printf("%s\"", indent); \
        ll_counter += indent_level; \
    } \
} while (0)
#define PRINTF_OUT if (ll_counter != 0) { printf("\"\n"); }
#define PRINTF(...) do { \
    PRINTF_IN;\
    ll_counter += fprintf(stdout, __VA_ARGS__); \
    if (ll_counter >= LINE_LENGTH) {\
         PRINTF_OUT;\
         ll_counter = 0; \
    } \
} while (0);
#define PRINTF_BYTE(c) do { \
    if (isprint(c)) { \
        PRINTF("%c", (c));\
    } else {\
        PRINTF(BIN_FMT, (unsigned char)(c));\
    }\
} while (0)

static char * comment_buffer;

static void clear_comment() {
    comment_buffer[0] = '\0';
}

static void comment_push(const char * commnt){
    size_t allocated = strlen(comment_buffer) + 1;
    size_t add = strlen(commnt);

    comment_buffer = realloc(comment_buffer, allocated + add);
    strcat(comment_buffer, commnt);
}

static void comment(size_t level, const char * com){

    MAKE_INDENT(level);

    size_t break_pos = LINE_LENGTH - (level*TAB_SIZE) - 3 - 4; // 3 for ' * ' and 4 as for word break

    size_t comm_len = strlen(com);
    printf("%s/*\n", indent);

    size_t break_used = 0;
    for (size_t curr_pos = 0; curr_pos < comm_len; curr_pos += break_used){
        const char * segment = com + curr_pos;
        const char * nl_pos = strchr(segment, '\n');
        const size_t max_segment_len = strlen(segment);
        const char * space_pos = strchr(segment + ((break_pos<max_segment_len)?break_pos:max_segment_len), ' ');

        if (((nl_pos != NULL) && (nl_pos < space_pos)) || space_pos == NULL) space_pos = nl_pos;
        if (space_pos == NULL) space_pos = com + max_segment_len;

        break_used = space_pos - segment;

        char format[20];
        sprintf(format, "%%s * %%.%lus\n", break_used);
        printf(format, indent, segment);
        ++break_used; // skip space
    }
    printf("%s */\n", indent);
}

static void snapshot_message(size_t indent_level, const char * prefix, const struct osc_message * msg){

    MAKE_INDENT(indent_level);
    // first - 4 bytes are message length
    const char * data = msg->raw_data;
    size_t pos_limit = osc_message_serialized_length(msg) + sizeof(uint32_t);

    printf("%sstatic const struct {\n", indent);
    SETUP_INDENT(++indent_level);
    printf("%sunsigned char raw_data[%lu];\n", indent, pos_limit + 1);
    SETUP_INDENT(--indent_level);
    printf("%s} msg_%s_data = {\n", indent, prefix);
    SETUP_INDENT(++indent_level);

    SETUP_BREAK;

    // content aware print
    for (size_t pos = 0; pos < pos_limit; ++pos) {
        PRINTF_BYTE(data[pos]);
    }

    PRINTF_OUT;
    SETUP_INDENT(--indent_level);
    printf("%s};\n\n", indent);

}

static void snapshot_bundle(size_t indent_level, const char * prefix, const struct osc_bundle * bnd){

    MAKE_INDENT(indent_level);
    // first - 4 bytes are message length
    const char * data = bnd->raw_data;
    size_t pos_limit = osc_bundle_serialized_length(bnd) + sizeof(uint32_t);

    printf("%sstatic const struct {\n", indent);
    SETUP_INDENT(++indent_level);
    printf("%sunsigned char raw_data[%lu];\n", indent, pos_limit + 1);
    SETUP_INDENT(--indent_level);
    printf("%s} bnd_%s_data = {\n", indent, prefix);
    SETUP_INDENT(++indent_level);

    SETUP_BREAK;

    for (size_t pos = 0; pos < pos_limit; ++pos) {
        PRINTF_BYTE(data[pos]);
    }

    PRINTF_OUT;
    SETUP_INDENT(--indent_level);
    printf("%s};\n\n", indent);

}

int message_examples() {
    char buffer[255];
    char buffer2[255];

    struct osc_message msg_ref;
    OSC_MESSAGE_NULL(&msg_ref);

    comment(0, "prazdna zprava, vysledek volani osc_message_new");
    osc_message_new(&msg_ref);
    snapshot_message(0, "msg_empty", &msg_ref);


    uint32_t CISLO = 0x12345678u;
    osc_message_add_int32(&msg_ref, CISLO);
    comment(0, "msg_empty + osc_message_add_int32");
    sprintf(buffer, "msg_num_0x%x", CISLO);
    snapshot_message(0, buffer, &msg_ref);

    sprintf(buffer2, "%s + osc_message_add_string", buffer);
    const char * string_selze = "tady uz pamet nestaci";
    osc_message_add_string(&msg_ref, string_selze);
    comment(0, buffer2);
    sprintf(buffer, "msg_%s", msg_ref.typetag+1);
    snapshot_message(0, buffer, &msg_ref);

    sprintf(buffer2, "%s + osc_message_add_float %f", buffer, (float)CISLO);
    osc_message_add_float(&msg_ref, CISLO);
    comment(0, buffer2);
    sprintf(buffer, "msg_%s", msg_ref.typetag+1);
    snapshot_message(0, buffer, &msg_ref);

    struct osc_timetag ref_t;
    ref_t.sec = 0x12345678u;
    ref_t.frac = 0x9ab0cdefu;

    sprintf(buffer2, "%s + osc_message_add_timetag [sec=%u, frac=%u]", buffer, ref_t.sec, ref_t.frac);
    osc_message_add_timetag(&msg_ref, ref_t);
    comment(0, buffer2);
    sprintf(buffer, "msg_%s", msg_ref.typetag+1);
    snapshot_message(0, buffer, &msg_ref);

    // konec
    osc_message_destroy(&msg_ref);

    return 0;
}

static int get_menu_choice(){
    int c = 0;
    char buffer[32];
    do {
	    memset(buffer, 0, 32);
	    fgets(buffer, 32, stdin);

	    if (feof(stdin)) {
	        c = -1;
    	} else if (c == 0){
	        c = buffer[0];
        }
	/* that is until the whole input line either has been read or newline reached*/
    } while ((buffer[31] != 0) && (buffer[31] != '\n') && !feof(stdin));

    return c;
}

static int prompt_run(){
    printf("menu >>> ");
    fflush(stdout);
    return get_menu_choice();
}
static int prompt_message(){
    printf("zprava >>> ");
    fflush(stdout);
    return get_menu_choice();
}
static int prompt_bundle(){
    printf("bundle >>> ");
    fflush(stdout);
    return get_menu_choice();
}

static void help_run(){
    printf("Vitejte v hlavnim menu. Zde mate malou napovedu:\n");
    printf("%c zobrazi napovedu\n", MENU_MAIN_HELP);
    printf("%c ukonci program\n", MENU_MAIN_QUIT);
    printf("%c odesle pripravena data\n", MENU_MAIN_GENERATE);
    printf("%c zalozi novy bundle (rozpracovana data jsou znicena)\n", MENU_MAIN_BUNDLE);
    printf("%c zalozi novou samostatnou zpravu (rozpracovana data jsou znicena)\n", MENU_MAIN_MESSAGE);
}
static void help_message(){
    printf("%c zobrazi napovedu\n", MENU_MESSAGE_HELP);
    printf("%c uloz zpravu a vrat se o uroven vyse\n", MENU_MESSAGE_DONE);
    printf("%c vypise prave rozpracovanou zpravu\n", MENU_MESSAGE_SHOW);
    printf("%c nastavi adresu zpravy\n", MENU_MESSAGE_ADDR);
    printf("%c prida argument typu int\n", MENU_MESSAGE_ADD_INT);
    printf("%c prida argument typu float\n", MENU_MESSAGE_ADD_FLOAT);
    printf("%c prida argument typu string\n", MENU_MESSAGE_ADD_STRING);
    printf("%c prida argument typu osc_timetag\n", MENU_MESSAGE_ADD_TIMETAG);
#ifdef OSC_TT_BLOB
    printf("%c prida argument typu osc_blob\n", MENU_MESSAGE_ADD_BLOB);
#endif
}
static void help_bundle(){
    printf("%c zobrazi napovedu\n", MENU_BUNDLE_HELP);
    printf("%c uloz bundle a vrat se o uroven vyse\n", MENU_BUNDLE_DONE);
    printf("%c vypise prave rozpracovanou zpravu\n", MENU_BUNDLE_SHOW);
    printf("%c nastavi timetag\n", MENU_MESSAGE_ADD_TIMETAG);
    printf("%c prida do bundle zpravu\n", MENU_BUNDLE_ADD_MESSAGE);
}

static char * read_string() {
    const unsigned int BUFFER_SIZE = 1024;
    char * buffer = calloc(1, BUFFER_SIZE+1);
    assert(buffer != NULL);

    printf("Zadejte radek textu (maximalni delka %u): ", BUFFER_SIZE);

    fgets(buffer, BUFFER_SIZE, stdin);

    size_t content_length = strlen(buffer);
    if ((content_length > 1) && (buffer[content_length - 1] == '\n')) { buffer[content_length-1] = 0; }
    content_length = strlen(buffer);
    if ((content_length > 1) && (buffer[content_length - 1] == '\r')) { buffer[content_length-1] = 0; }

    comment_push(buffer);
    comment_push("\n");

    return buffer;
}

static struct osc_timetag read_timetag() {
    struct osc_timetag retval;

    char buffer[32];

    int ok = 0;
    while (!ok){
        printf("sec: ");

	    memset(buffer, 0, 32);
	    fgets(buffer, 32, stdin);

	    char * endptr = NULL;
	    retval.sec = strtol(buffer, &endptr, 10);

	    if ((buffer[31] == '\n') || (buffer[31] == 0)){
	        ok = 1;
	    } else {
    	    fprintf(stderr, "chybny vstup! %s\n", buffer);
	    }
    }

    comment_push(buffer);
    if (buffer[strlen(buffer)] != '\n') comment_push("\n");

    ok = 0;
    while (!ok){
        printf("frac: ");

	    memset(buffer, 0, 32);
	    fgets(buffer, 32, stdin);

	    char * endptr = NULL;
	    retval.frac = strtol(buffer, &endptr, 10);

	    if ((buffer[31] == '\n') || (buffer[31] == 0)){
    	    ok = 1;
	    } else {
	        fprintf(stderr, "chybny vstup! %s\n", buffer);
	    }
    }

    comment_push(buffer);
    if (buffer[strlen(buffer)] != '\n') comment_push("\n");

    return retval;
}

static int32_t read_int() {
    int retval = 0;
    int ok = 0;

    char buffer[32];
    while (!ok){
	    printf("hodnota: ");

	    memset(buffer, 0, 32);
	    fgets(buffer, 32, stdin);

	    char * endptr = NULL;
	    retval = strtol(buffer, &endptr, 10);

	    if ((buffer[31] == '\n') || (buffer[31] == 0)){
	        ok = 1;
	    } else {
	        fprintf(stderr, "chybny vstup! %s\n", buffer);
	    }
    }

    comment_push(buffer);
    if (buffer[strlen(buffer)] != '\n') comment_push("\n");

    return retval;
}

static float read_float() {
    float retval = 0;
    int ok = 0;
    char buffer[32];

    while (!ok){
	    printf("hodnota: ");

	    memset(buffer, 0, 32);
	    fgets(buffer, 32, stdin);

	    char * endptr = NULL;
	    retval = strtof(buffer, &endptr);

	    if ((buffer[31] == '\n') || (buffer[31] == 0)){
	        ok = 1;
	    } else {
	        fprintf(stderr, "chybny vstup! %s\n", buffer);
	    }
    }

    comment_push(buffer);
    if (buffer[strlen(buffer)] != '\n') comment_push("\n");

    return retval;
}

static int menu_message(struct osc_message ** msg){
    int retval = 0;

    // init bundle
    *msg = calloc(1, sizeof(**msg));
    osc_message_new(*msg);

    int keep_running = 1;
    int option = MENU_MESSAGE_HELP;

    do {
        if (option != MENU_MESSAGE_HELP) {
            char tmp_comm[] = {option, '\n', '\0'};
            comment_push(tmp_comm);
        }

	    switch (option) {
	        case MENU_MESSAGE_HELP: {
        		help_message();
		        break;
    	    }
	        case MENU_EOF: {
	            retval = MENU_EOF;
	        }
	        case MENU_MESSAGE_DONE: {
	            keep_running = 0;
	        	continue;
            }
	        case MENU_MESSAGE_ADD_INT: {
		        int32_t i = read_int();
		        osc_message_add_int32(*msg, i);
		        break;
            }
	        case MENU_MESSAGE_ADD_FLOAT: {
		        float f = read_float();
		        osc_message_add_float(*msg, f);
		        break;
            }
	        case MENU_MESSAGE_ADD_STRING: {
		        char * str = read_string();
		        osc_message_add_string(*msg, str);
		        free(str);
		        break;
            }
	        case MENU_MESSAGE_ADD_TIMETAG: {
		        struct osc_timetag t = read_timetag();
		        osc_message_add_timetag(*msg, t);
		        break;
            }
#ifdef OSC_TT_BLOB
	        case MENU_MESSAGE_ADD_BLOB: {
		        printf("Zadany retezec bude interpretovan jako sekvence byte blobu.\n");
		        char * str = read_string();

                size_t len = strlen(str);
                osc_blob b = osc_blob_new(len);

                if (b == NULL) {
                     fprintf(stderr, "Nepodarilo se alokovat pamet pro blob!!!");
                     free(str);
                     break;
                }

                memcpy(osc_blob_data_ptr(b), str, len);

		        osc_message_add_blob(*msg, b);

                osc_blob_destroy(b);
		        free(str);
		        break;
            }
#endif
	        case MENU_MESSAGE_ADDR: {
		        char * str = read_string();
		        osc_message_set_address(*msg, str);
		        free(str);
		        break;
            }
	        case MENU_MESSAGE_SHOW: {
		        print_message(*msg);
		        break;
	        }
            case 0xa:
                break;
            default: {
		        fprintf(stderr, "Nerozpoznany prikaz 0x%x (%c)!\n", option, option);
		        break;
	        }
	    }

	    option = prompt_message();
    } while (keep_running);

    return retval;
}

static int menu_bundle(struct osc_bundle ** bundle){
    int retval = 0;

    // init bundle
    *bundle = calloc(1, sizeof(**bundle));
    osc_bundle_new(*bundle);

    int keep_running = 1;
    int option = MENU_BUNDLE_HELP;

    do {
        if (option != MENU_BUNDLE_HELP) {
            char tmp_comm[] = {option, '\n', '\0'};
            comment_push(tmp_comm);
        }

        switch (option) {
	        case MENU_BUNDLE_HELP: {
        		help_bundle();
		        break;
    	    }
	        case MENU_EOF: {
	        	retval = MENU_EOF;
    	    }
	        case MENU_BUNDLE_DONE: {
	            keep_running = 0;
        		continue;
            }
    	    case MENU_BUNDLE_TIMETAG: {
	            struct osc_timetag tag = read_timetag();
	        	osc_bundle_set_timetag(*bundle, tag);
    	    }
	        case MENU_BUNDLE_ADD_MESSAGE: {
		        struct osc_message * message = NULL;
		        option = menu_message(&message);

		        if (message != NULL){
		            osc_bundle_add_message(*bundle, message);
		            osc_message_destroy(message);
                    free(message);
		        }
		        if (option != 0){ continue; }

		        break;
            }
	        case MENU_BUNDLE_SHOW: {
		        print_bundle(*bundle);
		        break;
	        }
            case 0xa:
                break;
            default: {
		        fprintf(stderr, "Nerozpoznany prikaz 0x%x (%c)!\n", option, option);
		        break;
	        }
	    }

    	option = prompt_bundle();
    } while (keep_running);

    return retval;
}

static void generate(void * data) {
    if (strncmp((char *)data+sizeof(int32_t), "#bundle", 8) == 0){
        struct osc_bundle bundle;
        bundle.raw_data = data;
        bundle.timetag = (struct osc_timetag *)((char *)bundle.raw_data + 12);

        comment(0, comment_buffer);

        char hash[9];
        memset(hash, 0, sizeof(hash));
        unsigned char * th = (unsigned char *)bundle.timetag;
        for (size_t i = 0; i < sizeof(struct osc_timetag); ++i){
            char tmpbuff[5];
            sprintf(tmpbuff, "%hhx", th[i]);
            strcat(hash, tmpbuff);
        }

        snapshot_bundle(0, hash, &bundle);
    } else {
        struct osc_message message;
        message.raw_data = data;
        message.address = (char *)message.raw_data + 4;
        message.typetag = (char *)message.address + ((strlen(message.address)+4) & ~0x3);
        comment(0, comment_buffer);
        snapshot_message(0, message.typetag+1, &message);
    }
}

void run(){

    int keep_running = 1;

    void * data = NULL;
    // initialize comment buffer
    comment_buffer = calloc(1, 1);

    int option = MENU_MAIN_HELP;

    do {
        switch (option) {
	        case MENU_MAIN_HELP: {
    		    help_run();
	    	    break;
	        }
	        case MENU_EOF:
    	    case MENU_MAIN_QUIT: {
	    	    if (data != NULL){ free(data); data = NULL; }
	            keep_running = 0;
	    	    continue;
            }
	        case MENU_MAIN_MESSAGE: {
                clear_comment();
                comment_push("Prikazy pro rekonstrukci vygenerovane zpravy:\n");

	    	    struct osc_message * message = NULL;
	    	    option = menu_message(&message);

	    	    if (message != NULL){
	    	        if (data != NULL) { free(data); }
	    	        data = message->raw_data;
                    free(message);
	    	    }

	    	    if (option != 0){ continue; }

	    	    break;
            }
       	    case MENU_MAIN_BUNDLE: {
                clear_comment();
                comment_push("Prikazy pro rekonstrukci vygenerovaneho bundle:\n");

                struct osc_bundle * bundle = NULL;
	    	    option = menu_bundle(&bundle);

	    	    if (bundle != NULL){
	    	        if (data != NULL) { free(data); }
	    	        data = bundle->raw_data;
                    free(bundle);
	    	    }

	    	    if (option != 0){ continue; }

	    	    break;
	        }
	        case MENU_MAIN_GENERATE: {
                if (data != NULL) {
	    	        generate(data);
                } else {
                    fprintf(stderr, "Nejprve je potreba sestavit bundle ci zpravu, teprve pote lze generovat kod.\n");
                }
	    	    break;
	        }
            case 0xa:
                break;
            default: {
	    	    fprintf(stderr, "Nerozpoznany prikaz 0x%x (%c)!\n", option, option);
        		break;
	        }
    	}

	    option = prompt_run();
    } while (keep_running);

    if (data != NULL) { free(data); }
    free(comment_buffer);
}

int main(){
    // okay, let's do this
    run();
    //message_examples();

    return 0;
}
