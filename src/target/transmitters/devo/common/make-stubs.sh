#!/usr/bin/env bash

# Auto generate the stubs
FILES=`find . -name '*.h' | sort`
echo '
/* This is auto-generated code. Edit at your own peril. */

'

cat $FILES | grep '^MODULE_CALLTYPE' |
    sed -e 's/^MODULE_CALLTYPE/extern/' \
        -e 's/(.*$/();/' \
        -e 's/[us][0-9][0-9]*/void/' \
        -e 's/int /void /'

echo \
'

void PROTO_Stubs(int idx)
{
    if (! idx)
        return;
'
cat $FILES | grep '^MODULE_CALLTYPE' |
    sed -e 's/^MODULE_CALLTYPE//' \
        -e 's/(.*$/();/' \
        -e 's/[us][0-9][0-9]*//' \
        -e 's/int//' \
        -e 's/void//'

echo \
'
}

'
