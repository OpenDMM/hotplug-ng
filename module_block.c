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
#include <fcntl.h>
#include <sys/stat.h>
#include "module_form.c"

static const char *devpath_to_pathname(const char *devpath)
{
	static char pathname[256];
	const char *str = strrchr(devpath, '/');

	if (!str)
		return NULL;

	snprintf(pathname, sizeof(pathname), "/dev%s", str);

	return pathname;
}

static int add(void)
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

	unlink(pathname);

	dev = (atoi(major) << 8) | atoi(minor);
	if (mknod(pathname, S_IFBLK | S_IRUSR | S_IWUSR, dev) == -1) {
		dbg("mknod: %s", strerror(errno));
		goto exit;
	}

exit:
	return retval;
}

main(block);

