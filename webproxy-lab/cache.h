#ifndef CACHE_H
#define CACHE_H

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

#include <stddef.h>

void cache_init();
int cache_find(char *url, char *buf);
void cache_insert(char *url, char *content, size_t size);
void cache_clear();
void cache_destroy();

#endif