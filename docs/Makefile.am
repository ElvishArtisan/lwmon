## automake.am
##
## docs/Automake.am for lwmon
##
##   (C) Copyright 2015-2023 Fred Gleason <fredg@paravelsystems.com>
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
## by Fred Gleason <fredg@paravelsystems.com>
##
## Use automake to process this into a Makefile.in

##
## Build Dependencies
##
%.html:	%.xml
	xsltproc -o $@ $(DOCBOOK_STYLESHEETS)/xhtml/docbook.xsl $<

%.pdf:	%.xml
	xsltproc $(DOCBOOK_STYLESHEETS)/fo/docbook.xsl $< | fop - -pdf $@

%.1:	%.xml
	xsltproc $(DOCBOOK_STYLESHEETS)/manpages/docbook.xsl $<

%.8:	%.xml
	xsltproc $(DOCBOOK_STYLESHEETS)/manpages/docbook.xsl $<

all-local:	lwaddr.1\
                lwcap.1\
                lwcast.1\
                lwcp.1\
                lwmaster.8\
                lwmastermon.1\
                lwmultcap.1\
		lwrp.1\
		lwrpdump.1\
                udpmon.1

man_MANS = lwaddr.1\
           lwcap.1\
           lwcast.1\
           lwcp.1\
           lwmaster.8\
           lwmastermon.1\
           lwmultcap.1\
           lwrp.1\
           lwrpdump.1\
           udpmon.1

EXTRA_DIST = lwaddr.1\
             lwaddr.xml\
             lwcap.1\
             lwcap.xml\
             lwcast.1\
             lwcast.xml\
             lwcp.1\
             lwcp.xml\
             lwmaster.8\
             lwmaster.xml\
             lwmastermon.1\
             lwmastermon.xml\
             lwmultcap.1\
             lwmultcap.xml\
             lwrp.1\
             lwrp.xml\
             lwrpdump.1\
             lwrpdump.xml\
             udpmon.1\
             udpmon.xml

CLEANFILES = *~
MAINTAINERCLEANFILES = *~\
                       *.1\
                       *.8\
                       *.html\
                       *.pdf\
                       aclocal.m4\
                       configure\
                       Makefile.in
