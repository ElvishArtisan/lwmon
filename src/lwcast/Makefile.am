## automake.am
##
## Makefile for lwcast(1)
##
## (C) Copyright 2017-2021 Fred Gleason <fredg@paravelsystems.com>
##
##   This program is free software; you can redistribute it and/or modify
##   it under the terms of the GNU General Public License version 2 as
##   published by the Free Software Foundation.
##
##   This program is distributed in the hope that it will be useful,
##   but WITHOUT ANY WARRANTY; without even the implied warranty of
##   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
##   GNU General Public License for more details.
##
##   You should have received a copy of the GNU General Public
##   License along with this program; if not, write to the Free Software
##   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
##
##
## Use automake to process this into a Makefile.in

AM_CPPFLAGS = -Wall -DPREFIX=\"$(prefix)\" -std=c++11 -fPIC -Wno-strict-aliasing @QT5_CFLAGS@
MOC = @QT_MOC@
LUPDATE = @QT_LUPDATE@
LRELEASE = @QT_LRELEASE@

# The dependency for qt's Meta Object Compiler (moc)
moc_%.cpp:	%.h
	$(MOC) $< -o $@

bin_PROGRAMS = lwcast

dist_lwcast_SOURCES = addaddressdialog.cpp addaddressdialog.h\
                      cmdswitch.cpp cmdswitch.h\
                      lwcast.cpp lwcast.h\
                      subscriptionmodel.cpp subscriptionmodel.h

nodist_lwcast_SOURCES = moc_addaddressdialog.cpp\
                        moc_lwcast.cpp\
                        moc_subscriptionmodel.cpp

lwcast_LDADD = @QT5_LIBS@

CLEANFILES = *~\
             moc_*\
             *.obj\
             *.idb\
             *.pdb\
             *ilk

MAINTAINERCLEANFILES = *~\
                       Makefile.in
