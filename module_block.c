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
#include <libgen.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include "module_form.c"

struct timeout {
	unsigned long val;
};

static const char *vars[] = {
	"ACTION", "DEVPATH", "PHYSDEVPATH", "PHYSDEVDRIVER",
};

static int send_variables(void)
{
	struct sockaddr_un addr;
	const char *var;
	unsigned int i;
	int retval = 1;
	int s;

	addr.sun_family = AF_LOCAL;
	strcpy(addr.sun_path, "/tmp/hotplug.socket");

	if ((s = socket(PF_LOCAL, SOCK_STREAM, 0)) == -1) {
		dbg("could not open socket.");
		goto exit;
	}

	if (connect(s, (const struct sockaddr *)&addr, SUN_LEN(&addr)) == -1) {
		dbg("could not connect socket.");
		goto exit;
	}

	for (i = 0; i < sizeof(vars) / sizeof(vars[0]); i++)
		if ((var = getenv(vars[i])))
			write(s, var, strlen(var) + 1);

	retval = 0;

exit:
	if (s != -1)
		close(s);

	return retval;
}

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

static bool devpath_to_pathname(char *devpath, char *pathname, size_t size)
{
	const char *str;

	str = basename(devpath);
	if (!str)
		return false;

	snprintf(pathname, size, "/dev/%s", str);
	return true;
}

static bool devpath_to_mountpoint(char *devpath, char *mountpoint, size_t size)
{
	const char *str;

	str = basename(devpath);
	if (!str)
		return false;

	snprintf(mountpoint, size, "/autofs/%s", str);
	return true;
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

static const char *pidfile_name(const char dir[], const char filename[])
{
	static char pathname[FILENAME_MAX];

	snprintf(pathname, FILENAME_MAX, "/var/run/%s/%s.pid", dir, filename);

	return pathname;
}

static int bdpoll_exec(char pathname[])
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
		argv[1] = pathname;
		argv[2] = NULL;
		if (execvp(argv[0], argv) == -1)
			perror(argv[0]);
		return -1;
	} else {
		filename = pidfile_name("bdpoll", basename(pathname));
		return pidfile_write(filename, pid);
	}
}

static int bdpoll_kill(char pathname[])
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
	char *devpath;
	const char *minor, *major;
	char pathname[FILENAME_MAX];
	char mountpoint[FILENAME_MAX];
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

	if (!devpath_to_pathname(devpath, pathname, sizeof(pathname))) {
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

	if (!devpath_to_mountpoint(devpath, mountpoint, sizeof(mountpoint))) {
		dbg("could not get mount point.");
		goto exit;
	}

	if (chdir(mountpoint) == 0) {
		chdir("/");
		send_variables();
	}

	retval = 0;
exit:
	return retval;
}

static int hotplug_remove(void)
{
	char *devpath;
	char pathname[FILENAME_MAX];
	int retval = 1;

	devpath = getenv("DEVPATH");
	if (!devpath) {
		dbg("missing DEVPATH environment variable, aborting.");
		goto exit;
	}

	if (!devpath_to_pathname(devpath, pathname, sizeof(pathname))) {
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

	send_variables();

	retval = 0;
exit:
	return retval;
}

main(block);


