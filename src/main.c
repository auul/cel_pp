#include "data.h"
#include "parse.h"

#include <stdio.h>

int main(int argc, char **args)
{
	if (argc < 2) {
		return 0;
	}

	const char *src = args[1];
	cel_pp_data data;

	if (parse_data(&data, &src)) {
		data_print(&data);
		printf("\n");
	} else {
		printf("No Match: %s\n", src);
	}

	return 0;
}
