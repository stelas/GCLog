#include "trim.h"

char * ltrim(char *s) {
	char *start = s;

	// Find first non-whitespace character.
	while (isspace(*start))
		start++;

	// Move string part to beginning, including null terminator.
	memmove(s, start, strlen(start) + 1);

	return s;
}

char * rtrim(char *s) {
	char *end = s + strlen(s);

	// Find last non-whitespace character.
	while ((end != s) && isspace(*(end - 1)))
		end--;

	// Mark new string endpoint.
	*end = '\0';

	return s;
}

char * trim(char *s) {
	return ltrim(rtrim(s));
}
