#include "php.h"
#include "hash_array.h"

hash_array_t *hash_array_ctor() {
  hash_array_t *ret = (hash_array_t *) emalloc(sizeof(hash_array_t));
  ret->index = (hash_entry **) ecalloc(10, sizeof(hash_entry *));
  ret->len = 0;
  return ret;
}

void hash_array_dtor(hash_array_t *p) {
  for (size_t i = 0; i < p->len; ++i) {
    efree(p->index[i]);
  }
  efree(p->index);
  efree(p);
}

void hash_array_push(hash_array_t *p, hash_entry *e) {
  if (sizeof(p->index)/sizeof(hash_entry *) < p->len + 1) {
    p->index = (hash_entry **) erealloc(p->index, sizeof(p->index)*2);
  }
  p->index[p->len] = e;
  p->len++;
}

static int hash_entry_cmp(const void *a, const void *b) {
  if ((*((hash_entry **) a))->key->len < (*((hash_entry **) b))->key->len) { return -1; }
  if ((*((hash_entry **) a))->key->len > (*((hash_entry **) b))->key->len) { return 1; }
  else return memcmp((void *) (*((hash_entry **) a))->key->val, (void *) (*((hash_entry **) b))->key->val, (*((hash_entry **) a))->key->len);
}
void hash_array_sort(hash_array_t *p) {
  qsort(p->index, p->len, sizeof(hash_entry *), &hash_entry_cmp);
}
