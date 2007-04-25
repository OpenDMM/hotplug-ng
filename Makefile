# Makefile for hotplug-ng
#
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

# Set the following to control the use of syslog
# Set it to `false' to remove all logging
USE_LOG = true

# Set the following to `true' to log the debug
# and make a unstripped, unoptimized  binary.
# Leave this set to `false' for production use.
DEBUG = false

# Set this to compile with Security-Enhanced Linux support.
USE_SELINUX = false

VERSION =	002
ROOT =		hotplug
MODULE_IEEE1394 = 	module_ieee1394
MODULE_USB = 	module_usb
MODULE_PCI = 	module_pci
MODULE_SCSI = 	module_scsi

RELEASE_NAME =	$(ROOT)-ng-$(VERSION)

MODULE_ALL =	$(MODULE_IEEE1394) $(MODULE_USB) $(MODULE_PCI) $(MODULE_SCSI)

DESTDIR =

KERNEL_DIR = /lib/modules/${shell uname -r}/build

# override this to make hotplug look in a different location for it's files
prefix =
exec_prefix =	${prefix}
etcdir =	${prefix}/etc
sbindir =	${exec_prefix}/sbin
usrbindir =	${exec_prefix}/usr/bin
mandir =	${prefix}/usr/share/man
hotplugdir =	${etcdir}/hotplug.d
srcdir = .

INSTALL = /usr/bin/install -c
INSTALL_PROGRAM = ${INSTALL}
INSTALL_DATA  = ${INSTALL} -m 644
INSTALL_SCRIPT = ${INSTALL_PROGRAM}

# Comment out this line to build with something other 
# than the local version of klibc
#USE_KLIBC = true

# make the build silent (well, at least the hotplug part)  Set this
# to something else to make it noisy again.
V=false

# set up PWD so that older versions of make will work with our build.
PWD = $(shell pwd)

# If you are running a cross compiler, you may want to set this
# to something more interesting, like "arm-linux-".  If you want
# to compile vs uClibc, that can be done here as well.
CROSS = #/usr/i386-linux-uclibc/usr/bin/i386-uclibc-
CC = $(CROSS)gcc
LD = $(CROSS)gcc
AR = $(CROSS)ar
STRIP = $(CROSS)strip
RANLIB = $(CROSS)ranlib
HOSTCC = gcc

export CROSS CC AR STRIP RANLIB CFLAGS LDFLAGS LIB_OBJS ARCH_LIB_OBJS CRT0

# code taken from uClibc to determine the current arch
ARCH := ${shell $(CC) -dumpmachine | sed -e s'/-.*//' -e 's/i.86/i386/' -e 's/sparc.*/sparc/' \
	-e 's/arm.*/arm/g' -e 's/m68k.*/m68k/' -e 's/powerpc/ppc/g'}

# code taken from uClibc to determine the gcc include dir
GCCINCDIR := ${shell LC_ALL=C $(CC) -print-search-dirs | sed -ne "s/install: \(.*\)/\1include/gp"}

# code taken from uClibc to determine the libgcc.a filename
GCC_LIB := $(shell $(CC) -print-libgcc-file-name )

# use '-Os' optimization if available, else use -O2
OPTIMIZATION := ${shell if $(CC) -Os -S -o /dev/null -xc /dev/null >/dev/null 2>&1; \
		then echo "-Os"; else echo "-O2" ; fi}

# check if compiler option is supported
cc-supports = ${shell if $(CC) ${1} -S -o /dev/null -xc /dev/null > /dev/null 2>&1; then echo "$(1)"; fi;}

WARNINGS := -Wall -fno-builtin -Wchar-subscripts -Wpointer-arith -Wstrict-prototypes -Wsign-compare
WARNINGS += $(call cc-supports,-Wno-pointer-sign)
WARNINGS += $(call cc-supports,-Wdeclaration-after-statement)

CFLAGS := -pipe

ifeq ($(strip $(USE_LOG)),true)
	CFLAGS  += -DUSE_LOG
endif

# if DEBUG is enabled, then we do not strip or optimize
ifeq ($(strip $(DEBUG)),true)
	CFLAGS  += -O1 -g -DDEBUG -D_GNU_SOURCE
	LDFLAGS += -Wl,-warn-common
	STRIPCMD = /bin/true -Since_we_are_debugging
else
	CFLAGS  += $(OPTIMIZATION) -fomit-frame-pointer -D_GNU_SOURCE
	LDFLAGS += -s -Wl,-warn-common
	STRIPCMD = $(STRIP) -s --remove-section=.note --remove-section=.comment
endif

# If we are using our version of klibc, then we need to build, link it, and then
# link udev against it statically.
# Otherwise, use glibc and link dynamically.
ifeq ($(strip $(USE_KLIBC)),true)
	KLIBC_BASE	= $(PWD)/klibc
	KLIBC_DIR	= $(KLIBC_BASE)/klibc
	INCLUDE_DIR	:= $(KLIBC_BASE)/include
	LINUX_INCLUDE_DIR	:= $(KERNEL_DIR)/include
	include $(KLIBC_DIR)/arch/$(ARCH)/MCONFIG
	ARCH_LIB_OBJS	= $(KLIBC_DIR)/libc.a
	CRT0 = $(KLIBC_DIR)/crt0.o
	LIBC = $(ARCH_LIB_OBJS) $(LIB_OBJS) $(CRT0)
	CFLAGS += $(WARNINGS) -nostdinc				\
		$(OPTFLAGS) $(REQFLAGS)				\
		-D__KLIBC__ -fno-builtin-printf			\
		-I$(INCLUDE_DIR)				\
		-I$(INCLUDE_DIR)/arch/$(ARCH)			\
		-I$(INCLUDE_DIR)/bits$(BITSIZE)			\
		-I$(GCCINCDIR)					\
		-I$(LINUX_INCLUDE_DIR)
	LIB_OBJS =
	LDFLAGS = --static --nostdlib -nostartfiles -nodefaultlibs
else
	WARNINGS += -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations
	CRT0 =
	LIBC =
	CFLAGS += $(WARNINGS) -I$(GCCINCDIR)
	LIB_OBJS = -lc
	LDFLAGS =
endif

ifeq ($(strip $(USE_SELINUX)),true)
	CFLAGS += -DUSE_SELINUX
	LIB_OBJS += -lselinux
endif

CFLAGS += 	-I$(PWD)/libsysfs/sysfs \
		-I$(PWD)/libsysfs

all: $(ROOT) $(MODULE_ALL) $(GEN_CONFIGS)

$(ARCH_LIB_OBJS) : $(CRT0)

$(CRT0):
	@if [ ! -r klibc/linux ]; then \
		ln -f -s $(KERNEL_DIR) klibc/linux; \
	fi
	$(MAKE) -C klibc SUBDIRS=klibc TESTS=

SYSFS_OBJS = \
	libsysfs/sysfs_class.o	\
	libsysfs/sysfs_device.o	\
	libsysfs/sysfs_dir.o	\
	libsysfs/sysfs_driver.o	\
	libsysfs/sysfs_utils.o	\
	libsysfs/dlist.o

SYSFS = $(PWD)/libsysfs/sysfs.a

HOTPLUG_OBJS =	\
	hotplug_util.o

OBJS = \
	hotplug.a		\
	libsysfs/sysfs.a

HEADERS = \
	hotplug_version.h	\
	module_form.c		\
	logging.h		\
	list.h

ifeq ($(strip $(V)),false)
	QUIET=@./ccdv
	HOST_PROGS=ccdv
else
	QUIET=
	HOST_PROGS=
endif

hotplug.a: $(HOTPLUG_OBJS)
	rm -f $@
	$(QUIET) $(AR) cq $@ $(HOTPLUG_OBJS)
	$(QUIET) $(RANLIB) $@

libsysfs/sysfs.a: $(SYSFS_OBJS)
	rm -f $@
	$(QUIET) $(AR) cq $@ $(SYSFS_OBJS)
	$(QUIET) $(RANLIB) $@

# header files automatically generated
GEN_HEADERS =	hotplug_version.h

ccdv:
	@echo "Building ccdv"
	@$(HOSTCC) -O1 ccdv.c -o ccdv

# Rules on how to create the generated header files
hotplug_version.h:
	@echo "Creating $@"
	@echo \#define HOTPLUG_VERSION		\"$(VERSION)\" > $@
	@echo \#define HOTPLUG_DIR		\"$(hotplugdir)\" >> $@


$(HOTPLUG_OBJS):	$(GEN_HEADERS) $(HOST_PROGS)
$(SYSFS_OBJS):		$(HOST_PROGS)
$(OBJS):		$(GEN_HEADERS) $(HOST_PROGS)
$(ROOT).o:		$(GEN_HEADERS) $(HOST_PROGS)
$(MODULE_IEEE1394).o:	$(GEN_HEADERS) $(HOST_PROGS)
$(MODULE_USB).o:	$(GEN_HEADERS) $(HOST_PROGS)
$(MODULE_PCI).o:	$(GEN_HEADERS) $(HOST_PROGS)
$(MODULE_SCSI).o:	$(GEN_HEADERS) $(HOST_PROGS)

$(ROOT): $(LIBC) $(ROOT).o $(OBJS) $(HEADERS) 
	$(QUIET) $(LD) $(LDFLAGS) -o $@ $(CRT0) $(ROOT).o $(LIB_OBJS) $(ARCH_LIB_OBJS)
	$(QUIET) $(STRIPCMD) $@

$(MODULE_IEEE1394): $(LIBC) $(MODULE_IEEE1394).o $(OBJS) $(HEADERS) 
	$(QUIET) $(LD) $(LDFLAGS) -o $@ $(CRT0) $(MODULE_IEEE1394).o $(OBJS) $(LIB_OBJS) $(ARCH_LIB_OBJS)
	$(QUIET) $(STRIPCMD) $@

$(MODULE_USB): $(LIBC) $(MODULE_USB).o $(OBJS) $(HEADERS) 
	$(QUIET) $(LD) $(LDFLAGS) -o $@ $(CRT0) $(MODULE_USB).o $(OBJS) $(LIB_OBJS) $(ARCH_LIB_OBJS)
	$(QUIET) $(STRIPCMD) $@

$(MODULE_PCI): $(LIBC) $(MODULE_PCI).o $(OBJS) $(HEADERS) 
	$(QUIET) $(LD) $(LDFLAGS) -o $@ $(CRT0) $(MODULE_PCI).o $(OBJS) $(LIB_OBJS) $(ARCH_LIB_OBJS)
	$(QUIET) $(STRIPCMD) $@

$(MODULE_SCSI): $(LIBC) $(MODULE_SCSI).o $(OBJS) $(HEADERS) 
	$(QUIET) $(LD) $(LDFLAGS) -o $@ $(CRT0) $(MODULE_SCSI).o $(OBJS) $(LIB_OBJS) $(ARCH_LIB_OBJS)
	$(QUIET) $(STRIPCMD) $@


#.c.o:
#	$(CC) $(CFLAGS) $(DEFS) $(CPPFLAGS) -c -o $@ $<
.c.o:
	$(QUIET) $(CC) $(CFLAGS) -c -o $@ $<


clean:
	-find . \( -not -type d \) -and \( -name '*~' -o -name '*.[oas]' \) -type f -print \
	 | xargs rm -f 
	-rm -f core $(ROOT) $(MODULE_ALL) $(GEN_HEADERS) $(GEN_CONFIGS)
	-rm -f ccdv
	$(MAKE) -C klibc SUBDIRS=klibc clean

spotless: clean
	$(MAKE) -C klibc SUBDIRS=klibc spotless
	-rm -f klibc/linux

release: spotless
	git-tar-tree HEAD $(RELEASE_NAME) | gzip -9v > $(RELEASE_NAME).tar.gz
	@echo "$(RELEASE_NAME).tar.gz created"

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
	$(INSTALL_PROGRAM) -D $(ROOT) $(DESTDIR)$(sbindir)/$(ROOT)
	$(INSTALL_PROGRAM) -D $(MODULE_IEEE1394) $(DESTDIR)$(sbindir)/$(MODULE_IEEE1394)
	$(INSTALL_PROGRAM) -D $(MODULE_USB) $(DESTDIR)$(sbindir)/$(MODULE_USB)
	$(INSTALL_PROGRAM) -D $(MODULE_PCI) $(DESTDIR)$(sbindir)/$(MODULE_PCI)
	$(INSTALL_PROGRAM) -D $(MODULE_SCSI) $(DESTDIR)$(sbindir)/$(MODULE_SCSI)
	- ln -f -s $(sbindir)/$(MODULE_IEEE1394) $(DESTDIR)$(hotplugdir)/ieee1394/$(MODULE_IEEE1394).hotplug
	- ln -f -s $(sbindir)/$(MODULE_USB) $(DESTDIR)$(hotplugdir)/usb/$(MODULE_USB).hotplug
	- ln -f -s $(sbindir)/$(MODULE_PCI) $(DESTDIR)$(hotplugdir)/pci/$(MODULE_PCI).hotplug
	- ln -f -s $(sbindir)/$(MODULE_SCSI) $(DESTDIR)$(hotplugdir)/scsi/$(MODULE_SCSI).hotplug

uninstall: uninstall-man
	- rm $(sbindir)/$(ROOT)
	- rm $(sbindir)/$(MODULE_IEEE1394)
	- rm $(sbindir)/$(MODULE_USB)
	- rm $(sbindir)/$(MODULE_PCI)
	- rm $(sbindir)/$(MODULE_SCSI)
	- rm $(hotplugdir)/ieee1394/$(MODULE_IEEE1394).hotplug
	- rm $(hotplugdir)/usb/$(MODULE_USB).hotplug
	- rm $(hotplugdir)/pci/$(MODULE_PCI).hotplug
	- rm $(hotplugdir)/scsi/$(MODULE_SCSI).hotplug
	- rmdir $(hotplugdir)/ieee1394
	- rmdir $(hotplugdir)/usb
	- rmdir $(hotplugdir)/pci
	- rmdir $(hotplugdir)/scsi
	- rmdir $(hotplugdir)

test: all
	@ cd test && ./udev-test.pl
