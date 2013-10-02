#! /bin/bash
VERSION=`grep <configure.in "AC_INIT" | perl -p -e "s/.*AC_INIT\(//" | awk -F ',' '{print $2}'`
DATE=`date +"%Y%m%d%H%M%S"`
RELEASE=libmaus-${VERSION}-release-${DATE}
git checkout master
git checkout -b ${RELEASE}-branch master
autoreconf -i -f
ADDFILES="INSTALL Makefile.in aclocal.m4 autom4te.cache config.guess config.h.in config.sub configure depcomp install-sh ltmain.sh m4/libtool.m4 m4/ltoptions.m4 m4/ltsugar.m4 m4/ltversion.m4 m4/lt~obsolete.m4 missing src/Makefile.in"
git add ${ADDFILES}
git commit -m "Release ${RELEASE}"
git tag ${RELEASE}
git push origin ${RELEASE}
git checkout master
git branch -D ${RELEASE}-branch
