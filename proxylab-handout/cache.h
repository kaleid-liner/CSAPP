#ifndef _CACHE_H
#define _CACHE_H

#include "hashmap.h"
#include "proxy.h"

typedef struct cache {
    map_t cache_block;
    char *timeline[MAX_OBJECT_SIZE];
    size_t cur;
} cache_t;

typedef struct cache_content {
    char *data;
    size_t content_length;
} cache_content_t;

/*
 * return 0 if success
*/
int cache_new(cache_t *cache);

/*
 * return 0 if success
*/
int insert_cache(cache_t *cache, const char *url,
                 const char *header, size_t header_length,
                 const char *body, size_t body_length);

/*
 * do nothing if cache is empty
*/
void remove_cache(cache_t *cache);

/*
 *  data will be set to value if cache is found
 *  return 0 if find cache 
*/
int find_cache(cache_t *cache, char *url, cache_content_t **data);

#endif