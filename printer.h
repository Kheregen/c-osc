#ifndef PRINTER_H
#define PRINTER_H

#include "osc.h"

void print_timetag(const struct osc_timetag * t);
void print_message(struct osc_message * msg);
void print_bundle(struct osc_bundle * bn);

#ifdef OSC_TT_BLOB
void print_blob(osc_blob b);
#endif

void print_payload(void * buffer, size_t data_size);

#endif // PRINTER_H
