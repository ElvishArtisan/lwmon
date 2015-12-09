#!/bin/sh

# make_pkg.sh
#
# Make an OS X Installation package.
#
# (C) Copyright 2014 Fred Gleason <fredg@paravelsystems.com>
#     All Rights Reserved.
#

function ApplyLibraryPaths
{
    TARGET=$1

    install_name_tool -change $QTDIR/lib/QtCore.framework/Versions/4/QtCore /Library/lwmon/QtCore.framework/Versions/4/QtCore $TARGET
    install_name_tool -change $QTDIR/lib/QtGui.framework/Versions/4/QtGui /Library/lwmon/QtGui.framework/Versions/4/QtGui $TARGET
    install_name_tool -change $QTDIR/lib/QtNetwork.framework/Versions/4/QtNetwork /Library/lwmon/QtNetwork.framework/Versions/4/QtNetwork $TARGET
}


ROOT_DIR=tmp

#
# Create Environment
#
rm -rf $ROOT_DIR
mkdir $ROOT_DIR

#
# Create tree
#
mkdir -p $ROOT_DIR/Library/lwmon

cp -a $QTDIR/lib/QtCore.framework $ROOT_DIR/Library/lwmon/
ApplyLibraryPaths $ROOT_DIR/Library/lwmon/QtCore.framework/Versions/4/QtCore
cp -a $QTDIR/lib/QtGui.framework $ROOT_DIR/Library/lwmon/
ApplyLibraryPaths $ROOT_DIR/Library/lwmon/QtGui.framework/Versions/4/QtGui
cp -a $QTDIR/lib/QtNetwork.framework $ROOT_DIR/Library/lwmon/
ApplyLibraryPaths $ROOT_DIR/Library/lwmon/QtNetwork.framework/Versions/4/QtNetwork

cp uninstall_osx.sh $ROOT_DIR/Library/lwmon/uninstall.sh

mkdir -p $ROOT_DIR/usr/bin
cp src/lwmon $ROOT_DIR/usr/bin
ApplyLibraryPaths $ROOT_DIR/usr/bin/lwmon
ln -s /usr/bin/lwmon $ROOT_DIR/usr/bin/lwaddr
ln -s /usr/bin/lwmon $ROOT_DIR/usr/bin/lwcp
ln -s /usr/bin/lwmon $ROOT_DIR/usr/bin/lwrp

if test -f docs/lwaddr.1 ; then
  mkdir -p $ROOT_DIR/usr/share/man/man1
  cp docs/lwaddr.1 $ROOT_DIR/usr/share/man/man1/
  cp docs/lwcp.1 $ROOT_DIR/usr/share/man/man1/
  cp docs/lwrp.1 $ROOT_DIR/usr/share/man/man1/
fi

pkgbuild --identifier com.paravelsystems.lwmon --root $ROOT_DIR $ROOT_DIR/com.paravelsystems.lwmon.pkg

#
# Build the Product Archive
#
productbuild --synthesize --package $ROOT_DIR/com.paravelsystems.lwmon.pkg $ROOT_DIR/Distribution.xml

productbuild --distribution $ROOT_DIR/Distribution.xml --package-path $ROOT_DIR ./lwmon.pkg

#
# Clean Up
#
rm -rf $ROOT_DIR
