#pragma once

#include <stdint.h>

struct owntracks;

enum owntracks_status {
	OT_OK,
	OT_ERROR,
};

typedef unsigned char ot_accuracy;
typedef uint64_t ot_timestamp;

const ot_accuracy OT_ACCURACY_UNKNOWN = 0;
const ot_accuracy OT_ACCURACY_MIN = 1;
const ot_accuracy OT_ACCURACY_MAX = 12;

void owntracks_lib_init();
void owntracks_lib_cleanup();

struct owntracks *owntracks_new(
		const char *host, int port,
		const char *user, const char *password,
		const char *device, const char *tracker,
		const char *cafile, const char *certfile, const char *keyfile);

enum owntracks_status owntracks_submit(
		struct owntracks *o,
		double latitude, double longitude,
		ot_accuracy accuracy, double battery);

enum owntracks_status owntracks_submit_tst(
		struct owntracks *o,
		double latitude, double longitude,
		ot_accuracy accuracy, double battery,
		ot_timestamp timestamp);

void owntracks_free(struct owntracks *o);
