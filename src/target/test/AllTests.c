#include <stdio.h>

#include "CuTest.h"

CuSuite* CuGetSuite(void);
CuSuite* CuStringGetSuite(void);

extern void MusicTest(CuTest*);

int RunAllTests(void)
{
	CuString *output = CuStringNew();
	CuSuite* suite = CuSuiteNew();

	CuSuiteAddSuite(suite, CuGetSuite());
	CuSuiteAddSuite(suite, CuStringGetSuite());

    SUITE_ADD_TEST(suite, MusicTest);

	CuSuiteRun(suite);
	CuSuiteSummary(suite, output);
	CuSuiteDetails(suite, output);
	printf("%s\n", output->buffer);
	return suite->failCount;
}

void PWR_Init(void)
{
	exit(RunAllTests());
}
