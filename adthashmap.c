#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "adthashmap.h"
#define HASHMAGIC 31

static Node *hmap_findnode(const Hashmap *hmap, const void *key);
static int hashfn(const void *, int, int);

Hashmap *hmap_new(int num_buckets, int keysize, int datasize)
{
    Hashmap *hmap;

    if (num_buckets <= 0 || keysize <= 0 || datasize <= 0)
        return NULL;
    if ((hmap = malloc(sizeof(Hashmap))) == NULL)
        return NULL;
    hmap->num_buckets = num_buckets;
    hmap->keysize = keysize;
    hmap->datasize = datasize;
    if ((hmap->buckets = calloc(hmap->num_buckets, sizeof(Node *))) == NULL)
        return NULL;
    return hmap;
}

Node *hmap_add(Hashmap *hmap, const void *key, const void *value)
{
    Node *node = hmap_findnode(hmap, key);
    int bucketi;

    if (node != NULL) {
        memcpy(node->data, value, hmap->datasize);
        return node;
    }
    bucketi = hashfn(key, hmap->keysize, hmap->num_buckets);
    if ((node = malloc(sizeof(Node))) == NULL)
        return NULL;
    if ((node->key = malloc(sizeof(hmap->keysize))) == NULL)
        return NULL;
    if ((node->data = malloc(sizeof(hmap->datasize))) == NULL)
        return NULL;
    memcpy(node->key, key, hmap->keysize);
    memcpy(node->data, value, hmap->datasize);
    node->next = hmap->buckets[bucketi];
    hmap->buckets[bucketi] = node;
    return node;
}

void *hmap_get(const Hashmap *hmap, const void *key)
{
    Node *node = hmap_findnode(hmap, key);

    if (node == NULL)
        return NULL;
    return node->data;
}

void hmap_free(Hashmap *hmap)
{
    for (int i = 0; i < hmap->num_buckets; ++i)
        for (Node *node = hmap->buckets[i], *next; node != NULL; node = next) {
            next = node->next;
            free(node->key);
            free(node->data);
            free(node);
        }
    free(hmap);
}

static Node *hmap_findnode(const Hashmap *hmap, const void *key)
{
    int bucketi = hashfn(key, hmap->keysize, hmap->num_buckets);
    Node *node;

    for (node = hmap->buckets[bucketi]; node != NULL; node = node->next)
        if (memcmp(node->key, key, hmap->keysize) == 0)
            break;
    return node;
}

/* Java's hash function */
static int hashfn(const void *key, int keysize, int num_buckets)
{
    unsigned int prehash = 0;

    for (int i = 0; i < keysize; ++i)
        prehash += pow(HASHMAGIC, i) * *((char *) key + i);
    return (int) (prehash % num_buckets);
}