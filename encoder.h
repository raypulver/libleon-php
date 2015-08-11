#ifndef LEON_ENCODER_H
#define LEON_ENCODER_H

#include <zend.h>
#include "zend_smart_str.h"
#include "string_index.h"
#include "object_layout_index.h"

typedef struct {
  smart_str *buffer;
  string_index_t *string_index;
  unsigned char string_index_type;
  int endianness;
  object_layout_index_t *object_layout_index;
  unsigned char object_layout_type;
  int state;
} leon_encoder_t;

leon_encoder_t *encoder_ctor();
void encoder_dtor(leon_encoder_t *);
void write_string_index(leon_encoder_t *encoder, zval *payload, int depth);
void write_object_layout_index(leon_encoder_t *encoder, zval *payload, int depth);
void write_data(leon_encoder_t *encoder, zval *payload, int implicit, unsigned char type);
void write_data_with_spec(leon_encoder_t *encoder, zval *spec, zval *payload);
void write_long(leon_encoder_t *encoder, long value, unsigned char type);
void write_double(leon_encoder_t *encoder, double d, unsigned char type);
void write_zstr(leon_encoder_t *encoder, zend_string *str);
void write_zstr_index(leon_encoder_t *encoder, zend_string *str);

#endif
