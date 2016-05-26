#! /bin/bash
SCRIPTDIR=`dirname "${BASH_SOURCE[0]}"`
pushd ${SCRIPTDIR}
SCRIPTDIR=`pwd`
popd

source ${SCRIPTDIR}/configure.gz.sh

configure_sh > configure
../src/testAIO
RET=$?
rm -f configure

echo "Exiting with code ${RET}"

exit ${RET}
