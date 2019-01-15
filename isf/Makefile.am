## Makefile.am -- Process this file with automake to produce Makefile.in
## Copyright (C) 2002 James Su
##
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2, or (at your option)
## any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

AUX_DIST                = $(ac_aux_dir)/config.guess \
			$(ac_aux_dir)/config.sub \
			$(ac_aux_dir)/compile \
			$(ac_aux_dir)/depcomp \
			$(ac_aux_dir)/install-sh \
			$(ac_aux_dir)/ltconfig \
			$(ac_aux_dir)/ltmain.sh \
			$(ac_aux_dir)/missing

EXTRA_DIST		= config.rpath \
			  mkinstalldirs \
			  bootstrap \
			  scim.spec \
			  intltool-extract.in \
			  intltool-merge.in \
			  intltool-update.in

AUTOMAKE_OPTIONS 	= gnu
SUBDIRS 		= ism/src ism/utils ism/modules ism/configs po/scim ism/extras ism/data ism/demos

MAINTAINERCLEANFILES 	= Makefile.in \
			  aclocal.m4 \
			  configure \
			  config.h.in \
			  $(AUX_DIST)

DISTCLEANFILES		= isf.pc scim.pc scim-gtkutils.pc intltool-extract intltool-merge intltool-update

CLEANFILES		= *.bak

ACLOCAL			= aclocal -I $(ac_aux_dir)/ism

pkgconfigdir            = $(libdir)/pkgconfig

if SCIM_BUILD_GTK_UTILS
GTK_UTILS_PKGCONFIG     = scim-gtkutils.pc
endif

pkgconfig_DATA          = isf.pc scim.pc \
			  $(GTK_UTILS_PKGCONFIG)

smackdir = /etc/smack/accesses.d/
smack_DATA = isf.rule org.tizen.isf-kbd-mode-changer.rule

kbdmodechangerxmldir = /usr/share/packages/
kbdmodechangerxml_DATA = org.tizen.isf-kbd-mode-changer.xml

if ISF_INSTALL_SYSTEMD_SCRIPT
SYSTEMD_SCRIPT = scim.service scim.socket
endif

# $(libdir) will be /usr/lib64 on x86_64 arch, so hard-coded.
systemdsystemunitdir = /usr/lib/systemd/system
systemdsystemunit_DATA = $(SYSTEMD_SCRIPT)

ACLOCAL_AMFLAGS = -I ism/m4

.PHONY: update-ChangeLog

update-ChangeLog:
	/bin/sh $(top_srcdir)/svn2cl.sh --authors=$(top_srcdir)/developers -f $(top_srcdir)/ChangeLog

uninstall-local:
	rm -rf @SCIM_MODULE_PATH@

install-data-hook:
if ISF_INSTALL_SYSTEMD_SCRIPT
	$(mkinstalldirs) $(DESTDIR)$(systemdsystemunitdir)/sockets.target.wants
	ln -sf ../scim.socket $(DESTDIR)$(systemdsystemunitdir)/sockets.target.wants/scim.socket
	$(mkinstalldirs) $(DESTDIR)$(systemdsystemunitdir)/multi-user.target.wants
	ln -sf ../scim.service $(DESTDIR)$(systemdsystemunitdir)/multi-user.target.wants/scim.service
endif
