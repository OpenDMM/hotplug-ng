/*
 * bdpoll.c
 *
 * Polls a block device for partitioning changes.
 *
 * ~# bdpoll /dev/sda
 *
 * Copyright (C) 2007 Andreas Oberritter
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/mount.h>
#include <unistd.h>

enum bd_event {
	EV_NONE,
	EV_INSERTED,
	EV_REMOVED,
};

int main(int argc, char *argv[])
{
	enum bd_event event;
	int fd, present;

	if (argc != 2) {
		fprintf(stderr, "usage: %s <block device>\n", argv[0]);
		return 1;
	}

	present = 0;

	for (;;) {
		event = EV_NONE;
		fd = open(argv[1], O_RDONLY);
		if (fd < 0) {
			if (errno != ENOMEDIUM) {
				perror(argv[1]);
			} else if (present) {
				present = 0;
				event = EV_REMOVED;
			}
		} else {
			if (!present) {
				present = 1;
				event = EV_INSERTED;
			}
			close(fd);
		}

		switch (event) {
		case EV_INSERTED:
			printf("%s: media inserted\n", argv[1]);
			break;
		case EV_REMOVED:
			printf("%s: media removed\n", argv[1]);
			fd = open(argv[1], O_RDONLY | O_NONBLOCK);
			if (fd < 0) {
				perror(argv[1]);
				break;
			}
			ioctl(fd, BLKRRPART);
			close(fd);
			break;
		case EV_NONE:
			usleep(500 * 1000);
			break;
		}
	}

	return 0;
}

