#!/bin/sh

REV=`svn info | awk '/^Revision:/ { print $2 }'`
URL=`svn info | awk '/^URL:/ { print $2 }'`

echo "#define UDEV_VERSION \"r$REV ($URL)\""

