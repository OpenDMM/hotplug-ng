# Makefile for hotplug-ng
#
# Copyright (C) 2003-2005 Greg Kroah-Hartman <greg@kroah.com>
# Copyright (C) 2007 Andreas Oberritter
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

# Set the following to control the use of syslog
# Set it to `false' to remove all logging
USE_LOG = true

# Set the following to `true' to log the debug
# and make a unstripped, unoptimized  binary.
# Leave this set to `false' for production use.
DEBUG = false

VERSION =	002b
ROOT =		hotplug
BDPOLL =	bdpoll
MODULE_IEEE1394 = 	module_ieee1394
MODULE_USB = 	module_usb
MODULE_PCI = 	module_pci
MODULE_SCSI = 	module_scsi
MODULE_FIRMWARE =	module_firmware
MODULE_BLOCK =	module_block

RELEASE_NAME =	$(ROOT)-ng-$(VERSION)

MODULE_ALL =	$(MODULE_IEEE1394) $(MODULE_USB) $(MODULE_PCI) $(MODULE_SCSI) $(MODULE_FIRMWARE) $(MODULE_BLOCK)

DESTDIR =

# override this to make hotplug look in a different location for it's files
prefix =
exec_prefix =	${prefix}
etcdir =	${prefix}/etc
sbindir =	${exec_prefix}/sbin
mandir =	${prefix}/usr/share/man
hotplugdir =	${etcdir}/hotplug.d

INSTALL = /usr/bin/install -c
INSTALL_PROGRAM = ${INSTALL}
INSTALL_DATA  = ${INSTALL} -m 644

# If you are running a cross compiler, you may want to set this
# to something more interesting, like "arm-linux-".  If you want
# to compile vs uClibc, that can be done here as well.
CROSS = #/usr/i386-linux-uclibc/usr/bin/i386-uclibc-
CC = $(CROSS)gcc
LD = $(CROSS)gcc
AR = $(CROSS)ar
STRIP = $(CROSS)strip
RANLIB = $(CROSS)ranlib

CFLAGS := -pipe -Wall -Wextra
CPPFLAGS := -D_GNU_SOURCE
LDFLAGS := -Wl,-warn-common

ifeq ($(strip $(USE_LOG)),true)
	CFLAGS  += -DUSE_LOG
endif

# if DEBUG is enabled, then we do not strip or optimize
ifeq ($(strip $(DEBUG)),true)
	CFLAGS  += -O0 -ggdb -DDEBUG
else
	CFLAGS  += -Os -fomit-frame-pointer
	LDFLAGS += -s
endif

all: $(ROOT) $(BDPOLL) $(MODULE_ALL) $(GEN_CONFIGS)

HOTPLUG_OBJS =	\
	hotplug_util.o \
	udev_sysdeps.o \
	udev_sysfs.o \
	udev_utils_string.o

OBJS = \
	hotplug.a

HEADERS = \
	hotplug_version.h	\
	module_form.c		\
	logging.h		\
	list.h

hotplug.a: $(HOTPLUG_OBJS)
	rm -f $@
	$(AR) cq $@ $(HOTPLUG_OBJS)
	$(RANLIB) $@

# header files automatically generated
GEN_HEADERS =	hotplug_version.h

# Rules on how to create the generated header files
hotplug_version.h:
	@echo \#define HOTPLUG_VERSION		\"$(VERSION)\" > $@
	@echo \#define HOTPLUG_DIR		\"$(hotplugdir)\" >> $@


$(HOTPLUG_OBJS):	$(GEN_HEADERS)
$(OBJS):		$(GEN_HEADERS)
$(ROOT).o:		$(GEN_HEADERS)
$(BDPOLL).o:		$(GEN_HEADERS)
$(MODULE_IEEE1394).o:	$(GEN_HEADERS)
$(MODULE_USB).o:	$(GEN_HEADERS)
$(MODULE_PCI).o:	$(GEN_HEADERS)
$(MODULE_SCSI).o:	$(GEN_HEADERS)
$(MODULE_FIRMWARE).o:	$(GEN_HEADERS)
$(MODULE_BLOCK).o:	$(GEN_HEADERS)

$(ROOT): $(OBJS) $(HEADERS)

$(BDPOLL): $(OBJS) $(HEADERS)

$(MODULE_IEEE1394): $(OBJS) $(HEADERS)

$(MODULE_USB): $(OBJS) $(HEADERS)

$(MODULE_PCI): $(OBJS) $(HEADERS)

$(MODULE_SCSI): $(OBJS) $(HEADERS)

$(MODULE_FIRMWARE): $(OBJS) $(HEADERS)

$(MODULE_BLOCK): $(OBJS) $(HEADERS)

clean:
	-find . \( -not -type d \) -and \( -name '*~' -o -name '*.[oas]' \) -type f -print \
	 | xargs rm -f
	-rm -f core $(ROOT) $(BDPOLL) $(MODULE_ALL) $(GEN_HEADERS) $(GEN_CONFIGS)

spotless: clean

install-man:
	$(INSTALL_DATA) -D hotplug.8 $(DESTDIR)$(mandir)/man8/hotplug.8

uninstall-man:
	- rm $(mandir)/man8/hotplug.8

install: all install-man
	$(INSTALL) -d $(DESTDIR)$(hotplugdir)
	$(INSTALL) -d $(DESTDIR)$(hotplugdir)/ieee1394
	$(INSTALL) -d $(DESTDIR)$(hotplugdir)/usb
	$(INSTALL) -d $(DESTDIR)$(hotplugdir)/pci
	$(INSTALL) -d $(DESTDIR)$(hotplugdir)/scsi
	$(INSTALL) -d $(DESTDIR)$(hotplugdir)/firmware
	$(INSTALL) -d $(DESTDIR)$(hotplugdir)/block
	$(INSTALL) -d $(DESTDIR)/var/run/bdpoll
	$(INSTALL_PROGRAM) -D $(ROOT) $(DESTDIR)$(sbindir)/$(ROOT)
	$(INSTALL_PROGRAM) -D $(BDPOLL) $(DESTDIR)$(sbindir)/$(BDPOLL)
	$(INSTALL_PROGRAM) -D $(MODULE_IEEE1394) $(DESTDIR)$(sbindir)/$(MODULE_IEEE1394)
	$(INSTALL_PROGRAM) -D $(MODULE_USB) $(DESTDIR)$(sbindir)/$(MODULE_USB)
	$(INSTALL_PROGRAM) -D $(MODULE_PCI) $(DESTDIR)$(sbindir)/$(MODULE_PCI)
	$(INSTALL_PROGRAM) -D $(MODULE_SCSI) $(DESTDIR)$(sbindir)/$(MODULE_SCSI)
	$(INSTALL_PROGRAM) -D $(MODULE_FIRMWARE) $(DESTDIR)$(sbindir)/$(MODULE_FIRMWARE)
	$(INSTALL_PROGRAM) -D $(MODULE_BLOCK) $(DESTDIR)$(sbindir)/$(MODULE_BLOCK)
	- ln -f -s $(sbindir)/$(MODULE_IEEE1394) $(DESTDIR)$(hotplugdir)/ieee1394/$(MODULE_IEEE1394).hotplug
	- ln -f -s $(sbindir)/$(MODULE_USB) $(DESTDIR)$(hotplugdir)/usb/$(MODULE_USB).hotplug
	- ln -f -s $(sbindir)/$(MODULE_PCI) $(DESTDIR)$(hotplugdir)/pci/$(MODULE_PCI).hotplug
	- ln -f -s $(sbindir)/$(MODULE_SCSI) $(DESTDIR)$(hotplugdir)/scsi/$(MODULE_SCSI).hotplug
	- ln -f -s $(sbindir)/$(MODULE_FIRMWARE) $(DESTDIR)$(hotplugdir)/firmware/$(MODULE_FIRMWARE).hotplug
	- ln -f -s $(sbindir)/$(MODULE_BLOCK) $(DESTDIR)$(hotplugdir)/block/$(MODULE_BLOCK).hotplug

uninstall: uninstall-man
	- rm $(sbindir)/$(ROOT)
	- rm $(sbindir)/$(BDPOLL)
	- rm $(sbindir)/$(MODULE_IEEE1394)
	- rm $(sbindir)/$(MODULE_USB)
	- rm $(sbindir)/$(MODULE_PCI)
	- rm $(sbindir)/$(MODULE_SCSI)
	- rm $(sbindir)/$(MODULE_FIRMWARE)
	- rm $(sbindir)/$(MODULE_BLOCK)
	- rm $(hotplugdir)/ieee1394/$(MODULE_IEEE1394).hotplug
	- rm $(hotplugdir)/usb/$(MODULE_USB).hotplug
	- rm $(hotplugdir)/pci/$(MODULE_PCI).hotplug
	- rm $(hotplugdir)/scsi/$(MODULE_SCSI).hotplug
	- rm $(hotplugdir)/firmware/$(MODULE_FIRMWARE).hotplug
	- rm $(hotplugdir)/block/$(MODULE_BLOCK).hotplug
	- rmdir $(hotplugdir)/ieee1394
	- rmdir $(hotplugdir)/usb
	- rmdir $(hotplugdir)/pci
	- rmdir $(hotplugdir)/scsi
	- rmdir $(hotplugdir)/firmware
	- rmdir $(hotplugdir)/block
	- rmdir $(hotplugdir)

test: all
	@ cd test && ./udev-test.pl

bin-tarball:
	$(RM) -r hotplug-ng-$(VERSION)
	$(MAKE) install DESTDIR=hotplug-ng-$(VERSION)
	tar -czf hotplug-ng-$(VERSION).tar.gz hotplug-ng-$(VERSION)

