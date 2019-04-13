/**
 * Personal Hash Map ADT
 */
#ifndef ADTHASHMAP_H
#define ADTHASHMAP_H

struct node {
    void *key;
    void *data;
    struct node *next;
};

struct hashmap {
    int num_buckets;
    int keysize;
    int datasize;
    struct node **buckets;
};

typedef struct hashmap Hashmap;
typedef struct node Node;

Hashmap *hmap_new(int num_buckets, int keysize, int datasize);
Node *hmap_add(Hashmap *hmap, const void *key, const void *value);
void *hmap_get(const Hashmap *hmap, const void *key);
void hmap_free(Hashmap *hmap);

#endif
