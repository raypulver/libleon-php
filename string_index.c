#include <zend.h>
#include "string_index.h"

string_index_t *string_index_ctor() {
  string_index_t *ret = (string_index_t *) emalloc(sizeof(string_index_t));
  ret->len = 0;
  ret->index = (zend_string **) ecalloc(10, sizeof(zend_string *));
  return ret;
}
void string_index_dtor(string_index_t *p) {
  /*int num_strings = sizeof(p->index)/sizeof(zend_string*);
  for (int i = 0; i < num_strings; ++i) {
    efree(p->index[i]);
  }*/
  efree(p->index);
  efree(p);
}
static inline void string_index_push_no_check(string_index_t *p, zend_string *str) {
  if (sizeof(p->index)/sizeof(zend_string *) < (p->len + 1)) {
    p->index = (zend_string **) erealloc(p->index, 10*sizeof(p->index));
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
  long idx = -1;
  for (long i = 0; i < p->len; ++i) {
    if (!strcmp(p->index[i]->val, str->val)) {
      idx = i;
      break;
    }
  }
  return idx;
}
