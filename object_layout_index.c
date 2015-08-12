#include <zend.h>
#include "object_layout_index.h"

object_layout_index_t *object_layout_index_ctor() {
  object_layout_index_t *ret = (object_layout_index_t *) emalloc(sizeof(object_layout_index_t));
  ret->index = (oli_entry **) ecalloc(10, sizeof(oli_entry *));
  ret->len = 0;
  ret->a = 10;
  return ret;
}

void object_layout_index_dtor(object_layout_index_t *p_oli) {
  for (size_t i = 0; i < p_oli->len; ++i) {
    oli_entry_dtor(p_oli->index[i]);
  }
  efree(p_oli->index);
  efree(p_oli);
}

void object_layout_index_push(object_layout_index_t *p_oli, oli_entry *entry) {
  if (p_oli->a < p_oli->len + 1) {
    p_oli->a *= 2;
    p_oli->index = (oli_entry **) realloc(p_oli->index, sizeof(oli_entry *) * p_oli->a);
  }
  p_oli->index[p_oli->len] = entry;
  ++p_oli->len;
}

oli_entry *oli_entry_ctor() {
  oli_entry *ret = (oli_entry *) emalloc(sizeof(oli_entry));
  ret->entries = (long *) ecalloc(10, sizeof(long));
  ret->len = 0;
  ret->a = 10;
  return ret;
}

void oli_entry_dtor(oli_entry *p_oli_entry) {
  efree(p_oli_entry->entries);
  efree(p_oli_entry);
}


void oli_entry_push(oli_entry *p_entry, long i) {
  if (p_entry->a < p_entry->len + 1) {
    p_entry->a *= 2;
    p_entry->entries = (long *) erealloc(p_entry->entries, sizeof(long)*p_entry->a);
  }
  p_entry->entries[p_entry->len] = i;
  ++p_entry->len;
}

static int compare(const void *a, const void *b) {
  if (*((long*) a) < *((long*) b)) return -1;
  else if (*((long *) a) == *((long *) b)) return 0;
  else return 1;
}

void oli_entry_sort(oli_entry *p_entry) {
  qsort(p_entry->entries, p_entry->len, sizeof(long), &compare);
}

int object_layout_index_find(object_layout_index_t *p_oli, oli_entry *entry) {
  int cont;
  for (size_t i = 0; i < p_oli->len; ++i) {
    cont = 0;
    if (p_oli->index[i]->len != entry->len) continue;
    for (size_t j = 0; j < p_oli->index[i]->len; ++j) {
      if (p_oli->index[i]->entries[j] != entry->entries[j]) {
        cont = 1;
        break;
      }
    } 
    if (cont) continue;
    return (int) i;
  }
  return -1;
}
