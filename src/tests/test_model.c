#include "CuTest.h"

void TestModelLoadSave(CuTest *t)
{
    struct Model ValidateModel;

    chdir(FILESYSTEM_DIR);
    CONFIG_ResetModel();
    Model.fixed_id = 0xFEEDFEED;
    memcpy(&ValidateModel, &Model, sizeof(Model));

    CONFIG_WriteModel(1);
    CONFIG_ReadModel(1);
    CuAssertIntEquals(t, Model.fixed_id, 0xFEEDFEED);
    CuAssertTrue(t, memcmp(&ValidateModel, &Model, sizeof(Model)) == 0);
}


void TestModelLoadSaveWithLoc(CuTest *t)
{
    struct Model ValidateModel;

    chdir(FILESYSTEM_DIR);
    CONFIG_ReadLang(2);

    CONFIG_ResetModel();
    Model.fixed_id = 0xFEEDFEED;
    memcpy(&ValidateModel, &Model, sizeof(Model));

    CONFIG_WriteModel(2);
    CONFIG_ReadModel(2);
    CuAssertIntEquals(t, Model.fixed_id, 0xFEEDFEED);
    CuAssertTrue(t, memcmp(&ValidateModel, &Model, sizeof(Model)) == 0);
}

void TestModelChange(CuTest *t)
{
    CONFIG_ResetModel();
    Model.train_sw += 1;

    CuAssertTrue(t, CONFIG_IsModelChanged());
}