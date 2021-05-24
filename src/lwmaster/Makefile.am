## automake.am
##
## Makefile for lwmaster(8)
##
## (C) Copyright 2020 Fred Gleason <fredg@paravelsystems.com>
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

AM_CFLAGS = -Wall -DPREFIX=\"$(prefix)\" -Wno-strict-aliasing

install-exec-am:	
	mkdir -p $(DESTDIR)@prefix@/sbin
	cp lwmaster $(DESTDIR)@prefix@/sbin/lwmaster
	chmod 4755 $(DESTDIR)@prefix@/sbin/lwmaster

uninstall-local:	
	rm -f $(DESTDIR)@prefix@/sbin/lwmaster


sbin_PROGRAMS = lwmaster

dist_lwmaster_SOURCES = lwmaster.c lwmaster.h

CLEANFILES = *~\
             *.buildinfo\
             *.changes\
             *.ddeb\
             *.deb\
             *.dsc\
             *ilk\
             *.obj\
             *.idb\
             *.pdb\
             *.tar.gz\
             *.tar.xz\
             moc_*

MAINTAINERCLEANFILES = *~\
                       Makefile.in
