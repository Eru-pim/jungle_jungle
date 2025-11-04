#include "csapp.h"
#include "cache.h"
#include <string.h>
#include <time.h>
#include <pthread.h>

typedef struct cache_entry {
    char *url;
    char *content;
    size_t size;
    time_t last_used;
    struct cache_entry *next;
} cache_entry_t;

typedef struct cache {
    cache_entry_t *head;
    size_t total_size;
    pthread_rwlock_t lock;
} cache_t;

static cache_t cache;

void cache_init() {
    cache.head = NULL;
    cache.total_size = 0;
    pthread_rwlock_init(&cache.lock, NULL);
}

int cache_find(char *url, char *buf) {
    pthread_rwlock_rdlock(&cache.lock);

    cache_entry_t *iter = cache.head;
    while (iter != NULL) {
        if (strcasecmp(iter->url, url) == 0) {
            memcpy(buf, iter->content, iter->size);
            // iter->last_used = time(NULL);
            int size = iter->size;

            pthread_rwlock_unlock(&cache.lock);
            return size;
        }
        iter = iter->next;
    }
    pthread_rwlock_unlock(&cache.lock);
    return 0;
}

static void evict_lru() {
    if (cache.head == NULL) return;
    cache_entry_t *prev_vic = NULL;
    cache_entry_t *victim = cache.head;
    time_t last_time = victim->last_used;

    cache_entry_t *prev = NULL;
    cache_entry_t *iter = cache.head;
    while (iter != NULL) {
        if (iter->last_used < last_time) {
            prev_vic = prev;
            victim = iter;
            last_time = victim->last_used;
        }
        prev = iter;
        iter = iter->next;
    }

    if (prev_vic == NULL) {
        cache.head = victim->next;
    } else {
        prev_vic->next = victim->next;
    }

    cache.total_size -= victim->size;
    free(victim->url);
    free(victim->content);
    free(victim);
}

void cache_insert(char *url, char *content, size_t size) {
    if (size > MAX_OBJECT_SIZE) return;

    pthread_rwlock_wrlock(&cache.lock);

    cache_entry_t *iter = cache.head;
    while (iter != NULL) {
        if (strcasecmp(iter->url, url) == 0) {
            iter->last_used = time(NULL);
            pthread_rwlock_unlock(&cache.lock);
            return;
        }
        iter = iter->next;
    }

    while (cache.total_size + size > MAX_CACHE_SIZE) evict_lru();

    cache_entry_t *new_entry = malloc(sizeof(cache_entry_t));
    if (new_entry == NULL) {
        pthread_rwlock_unlock(&cache.lock);
        return;
    } 
    if ((new_entry->url = strdup(url)) == NULL) {
        free(new_entry);
        pthread_rwlock_unlock(&cache.lock);
        return;
    }
    if ((new_entry->content = malloc(size)) == NULL) {
        free(new_entry->url);
        free(new_entry);
        pthread_rwlock_unlock(&cache.lock);
        return;
    }
    memcpy(new_entry->content, content, size);
    new_entry->size = size;
    new_entry->last_used = time(NULL);
    
    new_entry->next = cache.head;
    cache.head = new_entry;
    cache.total_size += size;

    pthread_rwlock_unlock(&cache.lock);
}

static void cache_clear_nolock() {
    cache_entry_t *cur;
    while (cache.head != NULL) {
        cur = cache.head;
        free(cur->url);
        free(cur->content);
        cache.head = cur->next;
        free(cur);
    }
    cache.total_size = 0;
}

void cache_clear(void) {
    pthread_rwlock_wrlock(&cache.lock);
    cache_clear_nolock();
    pthread_rwlock_unlock(&cache.lock);
}

void cache_destroy() {
    pthread_rwlock_wrlock(&cache.lock);
    cache_clear_nolock();
    pthread_rwlock_unlock(&cache.lock);
    pthread_rwlock_destroy(&cache.lock);
}