#ifndef _INI_H_
#define _INI_H_

#include <stdio.h>
#include "trim.h"
#include "map.h"

#define INI_LINELEN	100

struct map_t * load_ini(const char *file);

#endif
