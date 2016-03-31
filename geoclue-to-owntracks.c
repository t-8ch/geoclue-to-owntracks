/*
 * Copyright (C) 2013 Red Hat, Inc.
 * Copyright (C) 2016 Thomas Weißschuh.
 *
 * Geoclue is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free
 * Software Foundation; version 3.
 *
 * Geoclue is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along
 * with Geoclue; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Original Author: Zeeshan Ali (Khattak) <zeeshanak@gnome.org>
 * Author: Thomas Weißschuh <geoclue-to-owntracks@t-8ch.de>
 */

#include <stdlib.h>
#include <locale.h>
#include <string.h>

#include <geoclue.h>

#include "owntracks.h"
#include "battery.h"

/* Commandline options */
static GClueAccuracyLevel accuracy_level = GCLUE_ACCURACY_LEVEL_EXACT;
static gint owntracks_port = 1883;
static gint64 timeout_interval = 900;
/* free me? */
static gchar *owntracks_host = "localhost";
static gchar *owntracks_user = NULL;
static gchar *owntracks_password = NULL;
static gchar *owntracks_device = NULL;
static gchar *owntracks_tracker = NULL;
static gchar *owntracks_cafile = NULL;
static gchar *owntracks_certfile = NULL;
static gchar *owntracks_client_cert_password = NULL;
static gchar *owntracks_keyfile = NULL;
static gchar *owntracks_client_key_password = NULL;

static struct owntracks *owntracks;

static GOptionEntry geoclue_entries[] = {
	{ "accuracy-level",
		'a',
		G_OPTION_FLAG_NONE,
		G_OPTION_ARG_INT,
		&accuracy_level,
		"Request accuracy level A. "
			"Country = 1, "
			"City = 4, "
			"Neighborhood = 5, "
			"Street = 6, "
			"Exact = 8.",
		"A" },
	{ "interval",
		'i',
		G_OPTION_FLAG_NONE,
		G_OPTION_ARG_STRING,
		&timeout_interval,
		"Interval",
		NULL },
	{ NULL },
};

static GOptionEntry owntracks_entries[] =
{
	{ "host",
		'h',
		G_OPTION_FLAG_NONE,
		G_OPTION_ARG_STRING,
		&owntracks_host,
		"Host to connect to",
		NULL },
	{ "port",
		'p',
		G_OPTION_FLAG_NONE,
		G_OPTION_ARG_INT,
		&owntracks_port,
		"Port to connect to",
		NULL },
	{ "user",
		'u',
		G_OPTION_FLAG_NONE,
		G_OPTION_ARG_STRING,
		&owntracks_user,
		"User",
		NULL },
	{ "device",
		'd',
		G_OPTION_FLAG_NONE,
		G_OPTION_ARG_STRING,
		&owntracks_device,
		"Device",
		NULL },
	{ "tracker",
		't',
		G_OPTION_FLAG_NONE,
		G_OPTION_ARG_STRING,
		&owntracks_tracker,
		"Tracker",
		NULL },
	{ "cafile",
		0,
		G_OPTION_FLAG_NONE,
		G_OPTION_ARG_STRING,
		&owntracks_cafile,
		"CA file",
		NULL },
	{ "certfile",
		0,
		G_OPTION_FLAG_NONE,
		G_OPTION_ARG_STRING,
		&owntracks_certfile,
		"CA file",
		NULL },
	{ "keyfile",
		0,
		G_OPTION_FLAG_NONE,
		G_OPTION_ARG_STRING,
		&owntracks_keyfile,
		"Key file",
		NULL },
	{ NULL },
};

GClueSimple *simple = NULL;
GClueClient *client = NULL;
GMainLoop *main_loop;

static void update_owntracks(GClueSimple *simple) {
	GClueLocation *location;
	enum owntracks_status status;

	if (!owntracks)
		return;

	location = gclue_simple_get_location(simple);
	g_info("Submitting data to owntracks");
	status = owntracks_submit(
			owntracks,
			gclue_location_get_latitude (location),
			gclue_location_get_longitude (location),
			0 /* gclue_location_get_accuracy (location) */,
			battery_get_percent());

	if (OT_OK != status)
		g_warning("Could not submit data");
}

gboolean update_owntracks_interval(gpointer user_data) {
	update_owntracks((GClueSimple *) user_data);
	return TRUE;
}

	static void
on_client_active_notify (GClueClient *client,
		GParamSpec *pspec,
		gpointer    user_data)
{
	if (gclue_client_get_active (client))
		return;

	g_print ("Geolocation disabled. Quiting..\n");
}

	static void
on_simple_ready (GObject      *source_object,
		GAsyncResult *res,
		gpointer      user_data)
{
	GError *error = NULL;

	simple = gclue_simple_new_finish (res, &error);
	if (error != NULL) {
		g_critical ("Failed to connect to GeoClue2 service: %s", error->message);

		exit (-1);
	}
	client = gclue_simple_get_client (simple);
	g_object_ref (client);

	update_owntracks(simple);

	g_signal_connect (simple,
			"notify::location",
			G_CALLBACK(update_owntracks),
			NULL);
	g_signal_connect (client,
			"notify::active",
			G_CALLBACK (on_client_active_notify),
			NULL);

	g_timeout_add_seconds(timeout_interval, update_owntracks_interval, simple);
}

gint main (gint argc, gchar *argv[]) {
	GOptionContext *context;
	GError *error = NULL;
	GOptionGroup *geoclue_opts, *owntracks_opts;

	context = g_option_context_new ("- Submit GeoClue to Owntracks");

	geoclue_opts = g_option_group_new(
			"geoclue",
			"Options for GeoClue", "Options for GeoClue",
			NULL, NULL);
	g_option_group_add_entries(geoclue_opts, geoclue_entries);

	owntracks_opts = g_option_group_new(
			"owntracks",
			"Options for Owntracks", "Options for Owntracks",
			NULL, NULL);
	g_option_group_add_entries(owntracks_opts, owntracks_entries);

	g_option_context_add_group(context, geoclue_opts);
	g_option_context_add_group(context, owntracks_opts);

	if (!g_option_context_parse (context, &argc, &argv, &error)) {
		g_critical ("option parsing failed: %s\n", error->message);
		exit (-1);
	}
	g_option_context_free (context);

	gclue_simple_new ("owntracks",
			accuracy_level,
			NULL,
			on_simple_ready,
			NULL);

	/* we need to be a glib loop source to be finalized.... */
	owntracks_lib_init();
	owntracks = owntracks_new(
			owntracks_host, owntracks_port,
			owntracks_user, owntracks_password,
			owntracks_device, owntracks_tracker,
			owntracks_cafile, owntracks_certfile, owntracks_keyfile);

	if (!owntracks) {
		g_critical("Could not create owntracks client\n");
		exit(-1);
	}

	main_loop = g_main_loop_new (NULL, FALSE);
	g_main_loop_run (main_loop);

	return EXIT_SUCCESS;
}
