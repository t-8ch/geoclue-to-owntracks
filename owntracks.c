#define _GNU_SOURCE

#include "owntracks.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#include <mosquitto.h>
#include <json.h>

#define GOTO_ERROR(r) { reason = #r; goto error; };

/*
 * FIXME:
 *  * keep connection open
 *  * integrate with glib mainloop?
 */

static int keepalive_seconds = 120;

struct owntracks_mosquitto_userdata {
	bool publish_done;
};

struct owntracks {
	struct mosquitto* mosquitto;
	char *user;
	char *host;
	int port;
	char *device;
	char *tracker;
	struct owntracks_mosquitto_userdata *mosquitto_userdata;
};

void owntracks_lib_init() {
	mosquitto_lib_init();
}

void owntracks_lib_cleanup() {
	mosquitto_lib_cleanup();
}

const static char *format_payload(double latitude, double longitude, ot_accuracy accuracy, double battery, const char *tracker, ot_timestamp timestamp) {
	struct json_object
		*j_root = NULL, *j_lat = NULL, *j_lon = NULL,
		*j_bat = NULL, *j_tid = NULL, *j_acc = NULL,
		*j_type = NULL, *j_tst;

	j_root = json_object_new_object();
	if (!j_root)
		goto error;

	j_tid = json_object_new_string(tracker);
	if (!j_tid)
		goto error;
	json_object_object_add(j_root, "tid", j_tid);

	j_lat = json_object_new_double(latitude);
	if (!j_lat)
		goto error;
	json_object_object_add(j_root, "lat", j_lat);

	j_lon = json_object_new_double(longitude);
	if (!j_lon)
		goto error;
	json_object_object_add(j_root, "lon", j_lon);

	if (battery >= 0) {
		j_bat = json_object_new_double(battery);
		if (!j_bat)
			goto error;
		json_object_object_add(j_root, "batt", j_bat);
	}

	if (OT_ACCURACY_UNKNOWN != accuracy) {
		j_acc = json_object_new_int(accuracy);
		if (!j_acc)
			goto error;
		json_object_object_add(j_root, "acc", j_acc);
	}

	j_type = json_object_new_string("location");
	json_object_object_add(j_root, "_type", j_type);

	j_tst = json_object_new_int64(timestamp);
	if (!j_tst)
		goto error;
	json_object_object_add(j_root, "tst", j_tst);

	return json_object_to_json_string(j_root);

error:
	if (j_root)
		json_object_put(j_root);
	if (j_lat)
		json_object_put(j_lat);
	if (j_lon)
		json_object_put(j_lon);
	if (j_bat)
		json_object_put(j_bat);
	if (j_tid)
		json_object_put(j_tid);
	if (j_acc)
		json_object_put(j_acc);
	if (j_type)
		json_object_put(j_type);
	if (j_tst)
		json_object_put(j_tst);

	return NULL;
}

static void on_publish(struct mosquitto *o, void *userdata, int mid) {
	struct owntracks_mosquitto_userdata *u = (struct owntracks_mosquitto_userdata *) userdata;

	u->publish_done = true;
}

enum owntracks_status owntracks_submit(
		struct owntracks *o,
		double latitude, double longitude,
		ot_accuracy accuracy, double battery) {
	ot_timestamp tst = time(NULL);
	return owntracks_submit_tst(o, latitude, longitude, accuracy, battery, tst);
}

enum owntracks_status owntracks_submit_tst(
		struct owntracks *o,
		double latitude, double longitude,
		ot_accuracy accuracy, double battery, ot_timestamp timestamp) {
	int status;
	char *topic;
	const char *payload;
	enum owntracks_status r = OT_ERROR;

	status = asprintf(&topic, "owntracks/%s/%s",
			o->user, o->device);

	if (-1 == status) {
		topic = NULL;
		goto error;
	}

	payload = format_payload(latitude, longitude, accuracy, battery, o->tracker, timestamp);

	if (!payload)
		goto error;

	if (mosquitto_connect(o->mosquitto, o->host, o->port, keepalive_seconds) \
			!= MOSQ_ERR_SUCCESS)
		goto error;

	o->mosquitto_userdata->publish_done = false;
	mosquitto_publish_callback_set(o->mosquitto, on_publish);
	status = mosquitto_publish(o->mosquitto, NULL, topic, strlen(payload), payload, 2, true);

	do {
		mosquitto_loop(o->mosquitto, -1, 1);
	} while (!o->mosquitto_userdata->publish_done);

	mosquitto_disconnect(o->mosquitto);

	r = OT_OK;

exit:
	free(topic);
	/* FIXME: discarding const... */
	free((void *) payload);
	return r;

error:
	r = OT_ERROR;
	goto exit;
}

struct owntracks *owntracks_new(
		const char *host, int port,
		const char *user, const char *password,
		const char *device, const char *tracker,
		const char *cafile, const char *certfile, const char *keyfile) {

	struct owntracks *o;
	struct owntracks_mosquitto_userdata *omu;
	struct mosquitto *m;
	char *reason = NULL;
	
	o = calloc(1, sizeof(*o));
	if (!o)
		GOTO_ERROR(alloc owntracks);

	omu = calloc(1, sizeof(*omu));
	if (!o)
		GOTO_ERROR(alloc owntracks mosquitto user data);

	o->mosquitto_userdata = omu;

	m = mosquitto_new("owntracks-foo", false, omu);
	if (!m)
		GOTO_ERROR(mosquitto_new)

	if (mosquitto_username_pw_set(m, user, password) \
			!= MOSQ_ERR_SUCCESS)
		GOTO_ERROR(username_pw);

	if (mosquitto_tls_insecure_set(m, false) \
			!= MOSQ_ERR_SUCCESS)
		GOTO_ERROR(tls_insecure)

	if (mosquitto_tls_opts_set(m, 1 /* SSL_VERIFY_PEER */, "tlsv1.2", NULL) \
			!= MOSQ_ERR_SUCCESS)
		GOTO_ERROR(tls_opts);

	if (cafile || certfile || keyfile)
		if (mosquitto_tls_set(m, cafile, NULL, certfile, keyfile, NULL) \
			!= MOSQ_ERR_SUCCESS)
			GOTO_ERROR(tls);

	if (!host)
		goto error;
	o->host = strdup(host);
	if (!o->host)
		goto error;

	o->port = port;

	if (!user)
		goto error;
	o->user = strdup(user);
	if (!o->user)
		goto error;

	if (!device)
		goto error;
	o->device = strdup(device);
	if (!o->device)
		goto error;

	if (!tracker)
		goto error;
	o->tracker = strdup(tracker);
	if (!o->tracker)
		goto error;

	o->mosquitto = m;

	return o;

error:
	owntracks_free(o);
	if (reason) {
		fprintf(stderr, "Could not initialize Owntracks: %s\n", reason);
	}
	return NULL;
}

void owntracks_free(struct owntracks *o) {
	if (!o)
		return;

	mosquitto_destroy(o->mosquitto);

	if (o->mosquitto_userdata)
		free(o->mosquitto_userdata);

	free(o->user);
	free(o->tracker);
	free(o->device);
	free(o);
}
