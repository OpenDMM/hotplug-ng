/*
 * module_block_e2.c
 *
 * Sends some hotplug variables to enigma2
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
#include <sys/socket.h>
#include <sys/un.h>
#include "module_form.c"

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

static inline int hotplug_add(void)
{
	return send_variables();
}

static inline int hotplug_remove(void)
{
	return send_variables();
}

main(block);

