#ifndef LEON_PARSER_H
#define LEON_PARSER_H

#include <zend.h>
#include <zend_string.h>
#include "string_index.h"
#include "object_layout_index.h"

typedef struct {
  char *payload;
  size_t payload_len;
  string_index_t *string_index;
  unsigned char string_index_type;
  int endianness;
  object_layout_index_t *object_layout_index;
  unsigned char object_layout_type;
  int state;
  int i;
} leon_parser_t;

leon_parser_t *parser_ctor(char *payload, size_t );
void parser_dtor(leon_parser_t *p);
void parse_string_index(leon_parser_t *p);
void parse_object_layout_index(leon_parser_t *p);
unsigned char read_uint8(leon_parser_t *p);
void parse_value(leon_parser_t *p, unsigned char type, zval *output);
void parse_value_with_spec(leon_parser_t *p, zval *spec, zval *output);
void read_string_as_zval(leon_parser_t *, zval *output);
#if PHP_API_VERSION > 20131106
zend_string *read_string(leon_parser_t *p);
#else
void *read_string(leon_parser_t *p, char **str, size_t *length);
#endif

#endif
