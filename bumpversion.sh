#! /bin/bash
VERSION=`grep AC_INIT < configure.ac | awk -F',' '{print $2}'`
FIRST=`echo $VERSION | awk -F'.' '{print $1}'`
SECOND=`echo $VERSION | awk -F'.' '{print $2}'`
THIRD=`echo $VERSION | awk -F'.' '{print $3}'`
NEXTTHIRD=`expr ${THIRD} + 1`
export DEBEMAIL=gt1@sanger.ac.uk
export DEBFULLNAME="German Tischler"

function cleanup
{
	if [ ! -z "${COMMITFILE}" ] ; then
		if [ -f "${COMMITFILE}" ] ; then
			rm -f "${COMMITFILE}"
		fi
	fi
}

COMMITFILE=commit_msg_$$.txt

trap cleanup EXIT SIGINT SIGTERM

# make sure we have the latest version
git checkout experimental
git pull

# create commit log message
joe "${COMMITFILE}"

if [ ! -s "${COMMITFILE}" ] ; then
	echo "Empty commit log, aborting"
	exit 1
fi

# update to next minor version
awk -v first=${FIRST} -v second=${SECOND} -v third=${THIRD} '/^AC_INIT/ {gsub(first"."second"."third,first"."second"."third+1);print} ; !/^AC_INIT/{print}' < configure.ac | \
	awk -v first=${FIRST} -v second=${SECOND} -v third=${THIRD} '/^LIBRARY_VERSION=/ {gsub("="first"."third"."second,"="first":"third+1":"second);print} ; !/^LIBRARY_VERSION=/{print}' \
	> configure.ac.tmp
mv configure.ac.tmp configure.ac

# update change log
CHANGELOG=ChangeLog dch --distribution unstable -v ${FIRST}.${SECOND}.${NEXTTHIRD}-1

# commit files
git add configure.ac ChangeLog

git commit -F "${COMMITFILE}"
git push

# switch to experimental debian branch
git checkout experimental-debian
git pull
git merge experimental

# create change log message
pushd debian
dch --distribution unstable -v ${FIRST}.${SECOND}.${NEXTTHIRD}-1 "New upstream version ${FIRST}.${SECOND}.${NEXTTHIRD}"
dch --release ""
popd
sed -i  -e "s/[0-9][0-9]*\.[0-9][0-9]*\.[0-9][0-9]*/${FIRST}.${SECOND}.${NEXTTHIRD}/g" debian/libmaus0.install

git add debian/changelog
git add debian/libmaus0.install

git commit -F "${COMMITFILE}"

git push

# back to experimental branch
git checkout experimental

TAG=libmaus_experimental_${FIRST}_${SECOND}_${NEXTTHIRD}
git tag -a ${TAG} -m "libmaus experimental version ${FIRST}_${SECOND}_${NEXTTHIRD}"
git push origin ${TAG}

exit 0
