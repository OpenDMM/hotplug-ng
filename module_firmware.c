/*
 * module_firmware.c
 *
 * Loads a firmware based on the firmware hotplug environment variables.
 *
 * Copyright (C) 2001,2005 Greg Kroah-Hartman <greg@kroah.com>
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
#include <dirent.h>
#include <sys/mman.h>
#include "module_form.c"

static int fw_open(const char *fmt, const char *str, int flags)
{
	char path[PATH_MAX];
	int fd;

	snprintf(path, PATH_MAX, fmt, str);
	path[PATH_MAX - 1] = '\0';

	fd = open(path, flags);
	if (fd == -1) {
		dbg("can't open '%s': %s", path, strerror(errno));
		return -1;
	}

	return fd;
}

static int fw_write(int fd, const char *buf, size_t count)
{
	size_t off;
	ssize_t ret;

	for (off = 0; off < count; off += ret) {
		ret = write(fd, &buf[off], count - off);
		if (ret < 0) {
			dbg("write failed: %s", strerror(errno));
			return -1;
		}
	}

	return count;
}

static int add(void)
{
	char *devpath_env;
	char *firmware_env;
	int load_fd = -1;
	int src_fd = -1;
	int dst_fd = -1;
	void *src_ptr = MAP_FAILED;
	struct stat st;
	int ret = 0;

	devpath_env = getenv("DEVPATH");
	firmware_env = getenv("FIRMWARE");
	dbg("DEVPATH='%s', FIRMWARE = '%s'", devpath_env, firmware_env);
	if ((devpath_env == NULL) ||
	    (firmware_env == NULL)) {
		dbg("missing an environment variable, aborting.");
		return 1;
	}

	load_fd = fw_open("/sys%s/loading", devpath_env, O_WRONLY);
	src_fd = fw_open("/lib/firmware/%s", firmware_env, O_RDONLY);
	dst_fd = fw_open("/sys%s/data", devpath_env, O_WRONLY);
	if ((load_fd == -1) || (src_fd == -1) || (dst_fd == -1))
		goto err;

	if (fstat(src_fd, &st) == -1) {
		dbg("stat failed: %s", strerror(errno));
		goto err;
	}

	src_ptr = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, src_fd, 0);
	if (src_ptr == MAP_FAILED) {
		dbg("mmap failed: %s", strerror(errno));
		goto err;
	}

	if (fw_write(load_fd, "1", 1) == -1)
		goto err;
	if (fw_write(dst_fd, src_ptr, st.st_size) == -1)
		goto err;
	if (fw_write(load_fd, "0", 1) == -1)
		goto err;

	goto cleanup;
err:
	ret = 1;
	if (load_fd != -1)
		fw_write(load_fd, "-1", 2);
cleanup:
	if (src_ptr != MAP_FAILED)
		munmap(src_ptr, st.st_size);
	if (dst_fd != -1)
		close(dst_fd);
	if (src_fd != -1)
		close(src_fd);
	if (load_fd != -1)
		close(load_fd);

	return ret;
}

main(firmware);

