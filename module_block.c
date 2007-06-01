/*
 * module_block.c
 *
 * Creates block device nodes using hotplug environment variables.
 *
 * Copyright (C) 2005 Greg Kroah-Hartman <greg@kroah.com>
 * Copyright (C) 2007 Andreas Oberritter
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
#include <ctype.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "module_form.c"

struct timeout {
	unsigned long val;
};

static unsigned long timeout_ms(void)
{
	struct timeval tv;

	gettimeofday(&tv, NULL);

	return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}

static void timeout_init(struct timeout *t, unsigned long ms)
{
	t->val = timeout_ms() + ms;
}

static int timeout_exceeded(struct timeout *t)
{
	return !((long)timeout_ms() - (long)t->val < 0);
}

static const char *devpath_to_pathname(const char *devpath)
{
	static char pathname[FILENAME_MAX];
	const char *str;

	str = basename(devpath);
	if (!str)
		return NULL;

	snprintf(pathname, sizeof(pathname), "/dev/%s", str);

	return pathname;
}

static int pidfile_read(const char filename[], pid_t *pid)
{
	FILE *f;
	int ret;

	f = fopen(filename, "r");
	if (f == NULL) {
		perror(filename);
		return -1;
	}

	ret = fscanf(f, "%d\n", pid);
	fclose(f);

	return (ret == 1) ? 0 : -1;
}

static int pidfile_write(const char filename[], pid_t pid)
{
	FILE *f;

	f = fopen(filename, "w");
	if (f == NULL) {
		perror(filename);
		return -1;
	}

	fprintf(f, "%d\n", pid);
	fclose(f);

	return 0;
}

static const char *pidfile_name(const char dirname[], const char filename[])
{
	static char pathname[FILENAME_MAX];

	snprintf(pathname, FILENAME_MAX, "/var/run/%s/%s.pid", dirname, filename);

	return pathname;
}

static int bdpoll_exec(const char pathname[])
{
	const char *filename;
	pid_t pid;
	char *argv[3];

	pid = fork();
	if (pid == -1) {
		perror("fork");
		return -1;
	} else if (pid == 0) {
		argv[0] = "bdpoll";
		argv[1] = (char *)pathname;
		argv[2] = NULL;
		if (execvp(argv[0], argv) == -1)
			perror(argv[0]);
		return -1;
	} else {
		filename = pidfile_name("bdpoll", basename(pathname));
		return pidfile_write(filename, pid);
	}
}

static int bdpoll_kill(const char pathname[])
{
	const char *filename;
	struct timeout t;
	pid_t pid, wpid;

	filename = pidfile_name("bdpoll", basename(pathname));
	if (pidfile_read(filename, &pid) == -1)
		return -1;

	if (kill(pid, SIGTERM) == -1) {
		perror("kill");
		return -1;
	}

	timeout_init(&t, 1000);
	do {
		wpid = waitpid(pid, NULL, WNOHANG);
		if (wpid == -1) {
			perror("waitpid");
			return -1;
		}
		if (wpid > 0)
			goto exit;
	} while (!timeout_exceeded(&t));

	if (kill(pid, SIGKILL) == -1) {
		perror("kill");
		return -1;
	}

exit:
	unlink(filename);
	return 0;
}

static int hotplug_add(void)
{
	const char *devpath, *minor, *major, *pathname;
	int retval = 1;
	dev_t dev;

	/*
	 * DEVPATH=/block/sda
	 * DEVPATH=/block/sda/sda1
	 */
	devpath = getenv("DEVPATH");
	if (!devpath) {
		dbg("missing DEVPATH environment variable, aborting.");
		goto exit;
	}

	minor = getenv("MINOR");
	if (!minor) {
		dbg("missing MINOR environment variable, aborting.");
		goto exit;
	}

	major = getenv("MAJOR");
	if (!major) {
		dbg("missing MAJOR environment variable, aborting.");
		goto exit;
	}

	pathname = devpath_to_pathname(devpath);
	if (!pathname) {
		dbg("could not get pathname.");
		goto exit;
	}

	unlink(pathname);

	dev = (atoi(major) << 8) | atoi(minor);
	if (mknod(pathname, S_IFBLK | S_IRUSR | S_IWUSR, dev) == -1) {
		dbg("mknod: %s", strerror(errno));
		goto exit;
	}

	if (!isdigit(pathname[strlen(pathname) - 1])) {
		if (bdpoll_exec(pathname) == -1) {
			dbg("could not exec bdpoll");
			goto exit;
		}
	}

	retval = 0;
exit:
	return retval;
}

static int hotplug_remove(void)
{
	const char *devpath, *pathname;
	int retval = 1;

	devpath = getenv("DEVPATH");
	if (!devpath) {
		dbg("missing DEVPATH environment variable, aborting.");
		goto exit;
	}

	pathname = devpath_to_pathname(devpath);
	if (!pathname) {
		dbg("could not get pathname.");
		goto exit;
	}

	unlink(pathname);

	if (!isdigit(pathname[strlen(pathname) - 1])) {
		if (bdpoll_kill(pathname) == -1) {
			dbg("could not kill bdpoll");
			goto exit;
		}
	}

	retval = 0;
exit:
	return retval;
}

main(block);

