#ifndef _MAP_H_
#define _MAP_H_

#include <stdlib.h>
#include <string.h>

struct map_t {
	char *key;
	char *value;
	struct map_t *next;
};

struct map_t * map_create();
void map_free(struct map_t *map);
char * map_get(struct map_t *map, const char *key);
void map_set(struct map_t *map, const char *key, const char *val);

#endif
