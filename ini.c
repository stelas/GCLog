#include "ini.h"

struct map_t * load_ini(const char *file) {
	struct map_t *map;
	FILE *fh;
	char buf[INI_LINELEN];
	char *s, *val;

	map = map_create();

	if ((fh = fopen(file, "r"))) {
		while (fgets(buf, INI_LINELEN, fh)) {
			// Chomp newline.
			buf[strlen(buf) - 1] = '\0';

			// Suppress comment.
			if ((s = strchr(buf, '#')))
				*s = '\0';

			if ((val = strchr(buf, '='))) {
				// Split into trimmed key and value...
				*val++ = '\0';
				s = trim(buf);
				trim(val);
				// ...and store in map.
				map_set(map, s, val);
			}
		}
		fclose(fh);
	}

	return map;
}
