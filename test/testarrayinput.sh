#! /bin/bash
SCRIPTDIR=`dirname "${BASH_SOURCE[0]}"`
pushd ${SCRIPTDIR}
SCRIPTDIR=`pwd`
popd

../src/testarrayinput
RET=$?

echo "Exiting with return code ${RET}"

exit ${RET}
