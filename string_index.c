#include <zend.h>
#include "string_index.h"

string_index_t *string_index_ctor() {
  string_index_t *ret = (string_index_t *) emalloc(sizeof(string_index_t));
  ret->len = 0;
  ret->index = (zend_string **) ecalloc(10, sizeof(zend_string *));
  ret->a = 10;
  return ret;
}
void string_index_dtor(string_index_t *p) {
  efree(p->index);
  efree(p);
}
static inline void string_index_push_no_check(string_index_t *p, zend_string *str) {
  if (p->a < (p->len + 1)) {
    p->a *= 2;
    p->index = (zend_string **) erealloc(p->index, sizeof(zend_string *)*p->a);
  }
  p->index[p->len] = str;
  p->len++;
}
void string_index_set_push(string_index_t *p, zend_string *str) {
  if (string_index_find(p, str) == -1) {
    string_index_push_no_check(p, str);
  }
}
void string_index_push(string_index_t *p, zend_string *str) {
  string_index_push_no_check(p, str);
}
zend_string *string_index_get(string_index_t *p, int i) {
  return p->index[i];
}
long string_index_find(string_index_t *p, zend_string *str) {
  long idx = -1, i;
  for (i = 0; i < p->len; ++i) {
    if (!strcmp(p->index[i]->val, str->val)) {
      idx = i;
      break;
    }
  }
  return idx;
}
