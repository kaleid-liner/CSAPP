#include "cache.h"

int cache_new(cache_t *cache)
{
    cache->cache_block = hashmap_new();
    cache->cur = 0;
    return (cache->cache_block != NULL);
}

int insert_cache(cache_t *cache, const char *url,
                 const char *header, size_t header_length,
                 const char *body, size_t body_length)
{
    size_t content_length = header_length + body_length;
    if (content_length > MAX_OBJECT_SIZE)
        return -1;
    if (hashmap_length(cache->cache_block) >= MAX_CACHE_SIZE) {
        remove_cache(cache);
    }
    char *key = malloc(strlen(url) + 1);
    strcpy(key, url);
    cache_content_t *value = malloc(sizeof(cache_content_t));
    value->content_length = content_length;
    value->data = malloc(content_length);
    memcpy(value->data, header, header_length);
    memcpy(value->data + header_length, body, body_length);

    if (hashmap_put(cache->cache_block, key, value) == MAP_OK) {
        cache->timeline[cache->cur++] = key;
        cache->cur = cache->cur % MAX_CACHE_SIZE;
    }
    else {
        free(key);
        free(value->data);
        free(value);
        return -1;
    }
    return 0;
}

void remove_cache(cache_t *cache)
{
    if (cache == NULL) {
        return;
    }
    char *to_remove = cache->timeline[cache->cur];
    if (to_remove != NULL) {
        void *tmp = NULL;
        if (hashmap_get(cache->cache_block, to_remove, &tmp) == MAP_OK) {
            cache_content_t *value = tmp;
            hashmap_remove(cache->cache_block, to_remove);
            free(value->data);
            free(value);
        }
        free(to_remove);
    }
}

int find_cache(cache_t *cache, char *url, cache_content_t **data)
{
    void *tmp;
    int ret =  hashmap_get(cache->cache_block, url, &tmp);
    *data = tmp;
    return ret;
}