#include "map.h"

struct map_t * map_create() {
	return calloc(1, sizeof(struct map_t));
}

void map_free(struct map_t *map) {
	struct map_t *m, *tmp;

	m = map;
	while (m) {
		if (m->key)
			free(m->key);
		if (m->value)
			free(m->value);
		tmp = m;
		m = m->next;
		free(tmp);
	}
}

char * map_get(struct map_t *map, const char *key) {
	struct map_t *m;

	for (m = map; m != NULL; m = m->next) {
		if (m->key && !strcasecmp(key, m->key))
			return m->value;
	}

	return NULL;
}

void map_set(struct map_t *map, const char *key, const char *val) {
	struct map_t *m;

	m = map;

	// Map not empty?
	if (m->key) {
		for ( ; ; m = m->next) {
			// Key exists, freeing strings for reuse.
			if (!strcasecmp(key, m->key)) {
				free(m->key);
				free(m->value);
				break;
			}
			// Last element, adding a new one.
			else if (!m->next) {
				m->next = calloc(1, sizeof(struct map_t));
				m = m->next;
				break;
			}
		}
	}

	// Add key and value.
	m->key = calloc(strlen(key) + 1, sizeof(char));
	strcpy(m->key, key);
	m->value = calloc(strlen(val) + 1, sizeof(char));
	strcpy(m->value, val);
}
