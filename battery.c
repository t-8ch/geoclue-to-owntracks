#include "battery.h"

#define _GNU_SOURCE

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#ifndef __linux__
#error Your platform is not supported
#endif

const double battery_error = 0;

static const char const *sys_fs_dir = "/sys/class/power_supply/BAT0/";

static uint64_t get_battery_value(const char *s) {
	uint64_t r;
	int status;
	char *path;
	FILE *file;
	char buf[50];

	status = asprintf(&path, "%s/%s", sys_fs_dir, s);
	if (-1 == status)
		goto error;

	file = fopen(path, "r");
	if (!file)
		goto error;

	fread(buf, sizeof(buf) - 1, 1, file);
	if (ferror(file))
		goto error;

	buf[sizeof(buf)] = 0;

	r = strtoll(buf, NULL, 10);

exit:
	free(path);
	fclose(file);
	return r;
error:
	r = 0;
	goto exit;


}
double battery_get_percent(void) {
	uint64_t now, full;

	now = get_battery_value("energy_now");
	full = get_battery_value("energy_full");

	if (!full)
		return full;

	return 100 * ((double) now / (double) full);
}
