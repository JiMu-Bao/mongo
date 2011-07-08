/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2008-2011 WiredTiger, Inc.
 *	All rights reserved.
 */

#include "wiredtiger.h"
#include "util.h"

static int usage(void);

int
util_stat(int argc, char *argv[])
{
	WT_CONNECTION *conn;
	WT_CURSOR *cursor;
	WT_ITEM key, value;
	WT_SESSION *session;
	size_t len;
	int ch, debug, ret, tret;
	char cursor_config[100];
	char *tablename;

	conn = NULL;
	tablename = NULL;
	debug = 0;

	while ((ch = getopt(argc, argv, "d")) != EOF)
		switch (ch) {
		case 'd':
			debug = 1;
			break;
		case '?':
		default:
			return (usage());
		}
	argc -= optind;
	argv += optind;

	/* The remaining argument is the table name. */
	if (argc != 1)
		return (usage());

	len = sizeof("stat:") + strlen(*argv);
	if ((tablename = calloc(len, 1)) == NULL) {
		fprintf(stderr, "%s: %s\n", progname, strerror(errno));
		return (EXIT_FAILURE);
	}
	snprintf(tablename, len, "stat:%s", *argv);

	if ((ret = wiredtiger_open(home,
	    verbose ? verbose_handler : NULL, NULL, &conn)) != 0 ||
	    (ret = conn->open_session(conn, NULL, NULL, &session)) != 0)
		goto err;

	snprintf(cursor_config, sizeof(cursor_config), "dump=print%s",
	    debug ? ",debug" : "");


	if ((ret = session->open_cursor(session, tablename, NULL,
	    cursor_config, &cursor)) != 0) {
		fprintf(stderr, "%s: cursor open(%s) failed: %s\n",
		    progname, tablename, wiredtiger_strerror(ret));
		goto err;
	}

	while ((ret = cursor->next(cursor)) == 0) {
		if ((ret = cursor->get_key(cursor, &key)) != 0)
			break;
		if ((ret = cursor->get_value(cursor, &value)) != 0)
			break;
		if (fwrite(key.data, 1, key.size, stdout) != key.size ||
		    fwrite("\n", 1, 1, stdout) != 1 ||
		    fwrite(value.data, 1, value.size, stdout) != value.size ||
		    fwrite("\n", 1, 1, stdout) != 1) {
			ret = errno;
			break;
		}
	}
	if (ret == WT_NOTFOUND)
		ret = 0;
	else {
		fprintf(stderr, "%s: cursor get(%s) failed: %s\n",
		    progname, tablename, wiredtiger_strerror(ret));
		goto err;
	}

	if (0) {
err:		ret = 1;
	}
	if (conn != NULL && (tret = conn->close(conn, NULL)) != 0 && ret == 0)
		ret = tret;

	if (tablename != NULL)
		free(tablename);

	return ((ret == 0) ? EXIT_SUCCESS : EXIT_FAILURE);
}

static int
usage(void)
{
	(void)fprintf(stderr,
	    "usage: %s%s "
	    "stat [-d] file\n",
	    progname, usage_prefix);
	return (EXIT_FAILURE);
}
