#!/usr/bin/env bash

# Auto generate single AllTests file for CuTest.
# Searches through all *.c files in the current directory.
# Prints to stdout.
# Author: Asim Jalis
# Date: 01/08/2003
FILES=`find tests/ -name '*.c' | sort`
echo '

/* This is auto-generated code. Edit at your own peril. */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "CuTest.h"

'

cat $FILES | grep '^void Test' |
    sed -e 's/(.*$//' \
        -e 's/$/(CuTest*);/' \
        -e 's/^/extern /'

echo \
'

int RunAllTests(void)
{
    CuString *output = CuStringNew();
    CuSuite* suite = CuSuiteNew();

'
cat $FILES | grep '^void Test' |
    sed -e 's/^void //' \
        -e 's/(.*$//' \
        -e 's/^/    SUITE_ADD_TEST(suite, /' \
        -e 's/$/);/'

echo \
'
    CuSuiteRun(suite);
    CuSuiteSummary(suite, output);
    CuSuiteDetails(suite, output);
    printf("%s\\n", output->buffer);
    CuStringDelete(output);
    CuSuiteDelete(suite);
    return suite->failCount;
}

int main(void)
{
    chdir(FILESYSTEM_DIR);
    return RunAllTests();
}

'
