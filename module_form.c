/*
 * module_form.c
 *
 * template file to create the other module_* files from
 *
 * Copyright (C) 2001,2005 Greg Kroah-Hartman <greg@kroah.com>
 *
 *	This program is free software; you can redistribute it and/or modify it
 *	under the terms of the GNU General Public License as published by the
 *	Free Software Foundation version 2 of the License.
 * 
 *	This program is distributed in the hope that it will be useful, but
 *	WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *	General Public License for more details.
 * 
 *	You should have received a copy of the GNU General Public License along
 *	with this program; if not, write to the Free Software Foundation, Inc.,
 *	675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <stddef.h>	/* for NULL */
#include <stdlib.h>	/* for getenv() */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "logging.h"
#include "hotplug_version.h"
#include "hotplug_util.h"

#ifdef USE_LOG
unsigned char logname[LOGNAME_SIZE];
void log_message(int level, const char *format, ...)
{
	va_list args;

	va_start(args, format);
	vsyslog(level, format, args);
	va_end(args);
}
#endif

#define main(type)					\
int main(int argc, char *argv[], char *envp[])		\
{							\
	char *action;					\
	char *subsystem;				\
	int retval = 1;					\
							\
	logging_init("module_"#type);			\
							\
	if (argc != 2) {				\
		dbg("this handler expects a parameter, aborting."); \
		goto exit;				\
	}						\
							\
	subsystem = argv[1];				\
	if (strcmp(#type, subsystem) != 0) {		\
		dbg("subsystem '%s' is not supported by this handler, aborting.", subsystem);	\
		goto exit;				\
	}						\
							\
	action = getenv("ACTION");			\
	dbg("action = '%s'", action);			\
	if (action == NULL) {				\
		dbg("missing ACTION environment variable, aborting.");	\
		goto exit;						\
	}								\
									\
	if (strcmp(ADD_STRING, action) == 0) {				\
		retval = hotplug_add();					\
	} else if (strcmp(REMOVE_STRING, action) == 0) {		\
		retval = hotplug_remove();				\
	} else {							\
		dbg("we do not handle %s", action);			\
		retval = 0;						\
	}								\
									\
exit:									\
	logging_close();						\
	return retval;							\
}

