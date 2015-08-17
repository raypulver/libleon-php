#include <zend.h>
#include "string_index.h"

string_index_t *string_index_ctor() {
  string_index_t *ret = (string_index_t *) emalloc(sizeof(string_index_t));
  ret->len = 0;
#if PHP_API_VERSION <= 20131106
  ret->index = (zval **) ecalloc(10, sizeof(zval *));
#else
  ret->index = (zend_string **) ecalloc(10, sizeof(zend_string *));
#endif
  ret->a = 10;
  return ret;
}
void string_index_dtor(string_index_t *p) {
#if PHP_API_VERSION <= 20131106
  size_t i;
  for (i = 0; i < p->len; ++i) {
    zval_ptr_dtor(&p->index[i]);
  }
#endif
  efree(p->index);
  efree(p);
}
#if PHP_API_VERSION <= 20131106
static inline void string_index_push_no_check(string_index_t *p, zval *str) {
#else
static inline void string_index_push_no_check(string_index_t *p, zend_string *str) {
#endif
  if (p->a < (p->len + 1)) {
    p->a *= 2;
#if PHP_API_VERSION <= 20131106
    p->index = (zval **) erealloc(p->index, sizeof(zval *)*p->a);
#else
    p->index = (zend_string **) erealloc(p->index, sizeof(zend_string *)*p->a);
#endif
  }
#if PHP_API_VERSION <= 20131106
  zval *s = emalloc(sizeof(zval));
  memcpy(s, str, sizeof(zval));
#else
  zval *s = str;
#endif
  p->index[p->len] = s;
  p->len++;
}
#if PHP_API_VERSION <= 20131106
void string_index_set_push(string_index_t *p, zval *str) {
#else
void string_index_set_push(string_index_t *p, zend_string *str) {
#endif
  if (string_index_find(p, str) == -1) {
    string_index_push_no_check(p, str);
  }
}
#if PHP_API_VERSION <= 20131106
void string_index_push(string_index_t *p, zval *str) {
#else
void string_index_push(string_index_t *p, zend_string *str) {
#endif
  string_index_push_no_check(p, str);
}
#if PHP_API_VERSION <= 20131106
zval *string_index_get(string_index_t *p, int i) {
#else
zend_string *string_index_get(string_index_t *p, int i) {
#endif
  return p->index[i];
}
#if PHP_API_VERSION <= 20131106
long string_index_find(string_index_t *p, zval *str) {
#else
long string_index_find(string_index_t *p, zend_string *str) {
#endif
  long idx = -1, i;
  for (i = 0; i < p->len; ++i) {
#if PHP_API_VERSION <= 20131106
    if (!strcmp(p->index[i]->value.str.val, str->value.str.val)) {
#else
    if (!strcmp(p->index[i]->val, str->val)) {
#endif
      idx = i;
      break;
    }
  }
  return idx;
}
