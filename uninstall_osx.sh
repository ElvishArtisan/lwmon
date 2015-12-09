#!/bin/sh

# uninstall_osx.sh
#
# (C) Copyright 2014 Fred Gleason <fredg@paravelsystems.com>
#     All Rights Reserved
#

echo "This will completely uninstall the lwmon package!"
echo
read -a RESP -p "Do you want to continue? (y/N) "
echo
if [ -z $RESP ] ; then
  echo "Cancelled!"
  exit 0
fi
if [ $RESP != "y" -a $RESP != "Y" ] ; then
  echo "Cancelled!"
  exit 0
fi

echo "Uninstalling lwmon..."
rm -rf /Library/lwmon
rm -f /usr/bin/lwaddr
rm -f /usr/bin/lwcp
rm -f /usr/bin/lwrp
pkgutil --forget com.paravelsystems.lwmon
echo "done."
