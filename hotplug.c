/*
 * hotplug.c - /etc/hotplug.d/ multiplexer
 * 
 * Copyright (C) 2004,2005 Greg Kroah-Hartman <greg@kroah.com>
 * Copyright (C) 2004 Kay Sievers <kay@vrfy.org>
 *
 *	This program is free software; you can redistribute it and/or modify it
 *	under the terms of the GNU General Public License as published by the
 *	Free Software Foundation version 2 of the License.
 */

/*
 * This essentially emulates the following shell script logic in C:
 *
 *	DIR="/etc/hotplug.d"
 *
 *	for I in "${DIR}/$1/"*.hotplug "${DIR}/"default/ *.hotplug ; do
 *		if [ -f $I ]; then
 *			test -x $I && $I $1 ;
 *		fi
 *	done
 *	exit 1
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include "logging.h"
#include "list.h"
#include "hotplug_version.h"

#define HOT_SUFFIX	".hotplug"

#define strfieldcpy(to, from) \
do { \
	to[sizeof(to)-1] = '\0'; \
	strncpy(to, from, sizeof(to)-1); \
} while (0)

#ifdef USE_LOG
void log_message(int level, const char *format, ...)
{
	va_list args;

	va_start(args, format);
	vsyslog(level, format, args);
	va_end(args);
}
#endif

static char *subsystem;

static int run_program(const char *filename)
{
	pid_t pid;

	dbg("running %s", filename);
	pid = fork();
	switch (pid) {
	case 0:
		/* child */
		execl(filename, filename, subsystem, NULL);
		dbg("exec of child failed");
		_exit(1);
	case -1:
		dbg("fork of child failed");
		break;
		return -1;
	default:
		waitpid(pid, NULL, 0);
	}

	return 0;
}

struct files {
	struct list_head list;
	char name[PATH_MAX];
};

/* sort files in lexical order */
static int file_list_insert(char *filename, struct list_head *file_list)
{
	struct files *loop_file;
	struct files *new_file;

	list_for_each_entry(loop_file, file_list, list) {
		if (strcmp(loop_file->name, filename) > 0) {
			break;
		}
	}

	new_file = malloc(sizeof(struct files));
	if (new_file == NULL) {
		dbg("error malloc");
		return -ENOMEM;
	}

	strfieldcpy(new_file->name, filename);
	list_add_tail(&new_file->list, &loop_file->list);
	return 0;
}


/* calls function for every file found in specified directory */
static int call_foreach_file(const char *dirname)
{
	struct dirent *ent;
	DIR *dir;
	char *ext;
	struct files *loop_file;
	struct files *tmp_file;
	LIST_HEAD(file_list);

	dbg("open directory '%s'", dirname);
	dir = opendir(dirname);
	if (dir == NULL) {
		dbg("unable to open '%s'", dirname);
		return -1;
	}

	while (1) {
		ent = readdir(dir);
		if (ent == NULL || ent->d_name[0] == '\0')
			break;

		if (ent->d_name[0] == '.')
			continue;

		/* look for file with specified suffix */
		ext = strrchr(ent->d_name, '.');
		if (ext == NULL)
			continue;

		if (strcmp(ext, HOT_SUFFIX) != 0)
			continue;

		dbg("put file '%s/%s' in list", dirname, ent->d_name);
		file_list_insert(ent->d_name, &file_list);
	}

	/* call function for every file in the list */
	list_for_each_entry_safe(loop_file, tmp_file, &file_list, list) {
		char filename[PATH_MAX];

		snprintf(filename, PATH_MAX, "%s/%s", dirname, loop_file->name);
		filename[PATH_MAX-1] = '\0';

		run_program(filename);

		list_del(&loop_file->list);
		free(loop_file);
	}

	closedir(dir);
	return 0;
}
/* 
 * runs files in these directories in order:
 * 	argv[1]/
 * 	default/
 */
int main(int argc, char *argv[])
{
	char dirname[PATH_MAX];
#ifndef DEBUG
	int fd;
	fd = open("/dev/null", O_RDWR);
	if (fd >= 0) {
		dup2(fd, STDOUT_FILENO);
		dup2(fd, STDIN_FILENO);
		dup2(fd, STDERR_FILENO);
	}
	close(fd);
#endif
	if (argc < 2)
		return 1;

	subsystem = argv[1];
	logging_init("hotplug");

	if (subsystem) {
		snprintf(dirname, PATH_MAX, "%s/%s", HOTPLUG_DIR, subsystem);
		dirname[PATH_MAX-1] = '\0';
		call_foreach_file(dirname);
	}

	snprintf(dirname, PATH_MAX, "%s/default", HOTPLUG_DIR);
	dirname[PATH_MAX-1] = '\0';
	call_foreach_file(dirname);

	logging_close();
	return 0;
}
