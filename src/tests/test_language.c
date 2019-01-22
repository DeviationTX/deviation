#include "CuTest.h"

void TestLanguage(CuTest *t)
{
    static const char input[] = "Configure";
    const char *loc;

    // for default language, input == loc
    CONFIG_ReadLang(0);
    CuAssertStrEquals(t, _tr(input), input);

    // loc string is not equals to input
    CONFIG_ReadLang(1);
    CuAssertIntEquals(t, 1, Transmitter.language);
    CuAssertTrue(t, strcmp(_tr(input), input) != 0);
    loc = _tr(input);

    // After switch language, the text should be different
    CONFIG_ReadLang(2);
    CuAssertIntEquals(t, 2, Transmitter.language);
    CuAssertTrue(t, strcmp(_tr(input), loc) != 0);

    CuAssertStrEquals(t, "ok1", _tr("ok1"));
}

void TestV1Language(CuTest *t)
{
    const char name[] = "language/lang.tst";
    FILE *fh;
    fh = fopen(name, "w");
    fprintf(fh,
        "Test\n"
        ":test\n"
        "abcd\n"
        ":ok\n"
        "ko\n");
    fclose(fh);

    ReadLang(name);
    CuAssertTrue(t, table_size > 0);
    CuAssertTrue(t, lookupmap[0].hash < lookupmap[1].hash);
    CuAssertStrEquals(t, "abcd", _tr("test"));
    CuAssertStrEquals(t, "ko", _tr("ok"));
    CuAssertStrEquals(t, "ok1", _tr("ok1"));
}
