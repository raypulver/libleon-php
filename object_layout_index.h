#ifndef LEON_OBJECT_LAYOUT_INDEX_H
#define LEON_OBJECT_LAYOUT_INDEX_H

typedef struct {
  long *entries;
  size_t len;
  size_t a;
} oli_entry;

typedef struct {
  oli_entry **index;
  size_t len;
  size_t a;
} object_layout_index_t;

object_layout_index_t *object_layout_index_ctor();
void object_layout_index_dtor(object_layout_index_t *p_oli);
void object_layout_index_push(object_layout_index_t *p_oli, oli_entry *entry);
int object_layout_index_find(object_layout_index_t *p_oli, oli_entry *entry);

oli_entry *oli_entry_ctor();
void oli_entry_dtor(oli_entry *p_oli_entry);
void oli_entry_push(oli_entry *p_oli_entry, long i);
int compare(const void *a, const void *b);
void zend_always_inline oli_entry_sort(oli_entry *p_entry) {
  qsort(p_entry->entries, p_entry->len, sizeof(long), &compare);
}

#endif
