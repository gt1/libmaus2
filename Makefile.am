EXTRA_DIST = configure GPLv2 GPLv3
SUBDIRS = src @TESTDIR@
ACLOCAL_AMFLAGS=-I m4

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libmaus2.pc libmaus2lsf.pc libmaus2digests.pc libmaus2seqchksumsfactory.pc libmaus2fftw.pc @LIBMAUSIRODSPC@

doc_DATA = ChangeLog

install-data-hook:
	cd ${DESTDIR}${docdir} && mv ChangeLog changelog && gzip -f -n -9 changelog
