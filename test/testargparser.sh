#! /bin/bash
SCRIPTDIR=`dirname "${BASH_SOURCE[0]}"`
pushd ${SCRIPTDIR}
SCRIPTDIR=`pwd`
popd

function expectedoutput64
{
cat <<EOF
QXJnUGFyc2VyKHByb2duYW1lPXRlc3RhcmdwYXJzZXI7a3ZhcmdzPXsoSSx2YWx1ZSkobSw1ayko
c3BsaXRhLDUpfTtyZXN0YXJncz1bYTtiO2M7NTs2XSkKYXJnWzBdPWEKYXJnWzFdPWIKYXJnWzJd
PWMKYXJnWzNdPTUKYXJnWzRdPTYKNQo2CnZhbHVlCjUKNTEyMAo=
EOF
}

source ${SCRIPTDIR}/base64decode.sh

../src/testargparser -Ivalue --splita5 -m5k a b c 5 6 2>&1 | sed "s|progname=.*testargparser|progname=testargparser|" >testargparser.output
RET=$?

if test $RET -ne 0 ; then
	rm -f testargparser.output
	exit $RET
fi

expectedoutput64 | base64 ${BASE64DEC} > testargparser.output.ref

cmp -s testargparser.output testargparser.output.ref

RET=$?

rm -f testargparser.output testargparser.output.ref

echo "Exiting with code ${RET}"

exit ${RET}
