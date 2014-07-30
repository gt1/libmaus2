EXTRA_DIST = configure GPLv2 GPLv3
SUBDIRS = src
ACLOCAL_AMFLAGS=-I m4

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libmaus.pc libmauslsf.pc

doc_DATA = ChangeLog

install-data-hook:
	cd ${DESTDIR}${docdir} && mv ChangeLog changelog && gzip -f -n -9 changelog
