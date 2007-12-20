#!/usr/bin/make -f
#
# Makefile for hotplug-ng
#
# Copyright (C) 2007 Andreas Oberritter
# Copyright (C) 2003-2005 Greg Kroah-Hartman <greg@kroah.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
#

###############################################################################

CC = $(CROSS_COMPILE)gcc
CFLAGS ?= -pipe -Wall -Wextra -Wno-unused -Os -fomit-frame-pointer
LDFLAGS ?= -Wl,-warn-common -s
INSTALL ?= /usr/bin/install

CPPFLAGS += -DUDEVMONITOR
CPPFLAGS += -DUDEVTRIGGER

# Set the following to control the use of syslog
# Unset it to remove all logging
CPPFLAGS += -DUSE_LOG

# Set the following to log the debug.
# Leave this unset for production use.
CPPFLAGS += -DDEBUG

###############################################################################

hotplug_bin = hotplug
hotplug_links = bdpoll
hotplug_objs = \
	bdpoll.o \
	hotplug_basename.o hotplug_devpath.o hotplug_pidfile.o \
	hotplug_setenv.o hotplug_socket.o hotplug_timeout.o hotplug_util.o \
	module_block.o module_firmware.o module_ieee1394.o \
	module_pci.o module_scsi.o module_usb.o \
	udev_sysdeps.o udev_sysfs.o udev_utils.o udev_utils_string.o

ifneq ($(findstring -DUDEVMONITOR,$(CPPFLAGS)),)
hotplug_links += udevmonitor
hotplug_objs += udevmonitor.o
endif
ifneq ($(findstring -DUDEVTRIGGER,$(CPPFLAGS)),)
hotplug_links += udevtrigger
hotplug_objs += udevtrigger.o
endif

all: $(hotplug_bin)

udev_version.h: .svn/entries
	./gen_udev_version.sh > $@

$(hotplug_objs): udev_version.h

$(hotplug_bin): $(hotplug_objs)

clean:
	$(RM) $(hotplug_bin) $(hotplug_objs) udev_version.h

install: $(hotplug_bin)
	$(INSTALL) -d $(DESTDIR)/sbin
	$(INSTALL) -m755 $^ $(DESTDIR)/sbin
	$(foreach link,$(hotplug_links),ln -sf $(hotplug_bin) $(DESTDIR)/sbin/$(link);)

uninstall:
	$(foreach file,$(hotplug_bin) $(hotplug_links),$(RM) $(DESTDIR)/sbin/$(file);)

