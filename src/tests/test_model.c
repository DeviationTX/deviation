#include "CuTest.h"

extern u8 CONFIG_ReadModel_old(const char* file); 
extern u8 CONFIG_WriteModel_old(u8 model_num);

u8 CONFIG_ReadModel_new(const char* file) {
    clear_model(1);

    auto_map = 0;
    if (CONFIG_IniParse(file, ini_handler, &Model)) {
        printf("Failed to parse Model file: %s\n", file);
    }
    if (! ELEM_USED(Model.pagecfg2.elem[0]))
        CONFIG_ReadLayout("layout/default.ini");
    if(! PROTOCOL_HasPowerAmp(Model.protocol))
        Model.tx_power = TXPOWER_150mW;
    Model.radio = PROTOCOL_GetRadio(Model.protocol);
    MIXER_SetMixers(NULL, 0);
    if(auto_map)
        RemapChannelsForProtocol(EATRG0);
    if(! Model.name[0])
        sprintf(Model.name, "Model%d", 1);
    return 1;
}

const char* const names[] = {
// "../../tests/models/geniuscp.ini",

"../../tests/models/fx071.ini",

"../../tests/models/280qav.ini",
"../../tests/models/bixler2.ini",

"../../tests/models/yacht.ini",

"../../tests/models/4g6s.ini",
"../../tests/models/blade130x.ini",
"../../tests/models/nazath.ini",

"../../tests/models/apm.ini",
"../../tests/models/deltaray.ini",
"../../tests/models/trex150dfc.ini",

"../../tests/models/ardrone2.ini",
"../../tests/models/wltoys931.ini",
};

void TestNewAndOld(CuTest *t)
{
    struct Model ValidateModel;

    for (unsigned i = 0; i < ARRAYSIZE(names); i++) {
        const char *filename = names[i];
        printf("Test model: %s\n", filename);
        CuAssertTrue(t, CONFIG_ReadModel_old(filename));
        memcpy(&ValidateModel, &Model, sizeof(Model));
        CuAssertTrue(t, CONFIG_ReadModel_new(filename));
        printf("\tRead successfully\n", filename);

        CuAssertTrue(t, memcmp(&ValidateModel, &Model, sizeof(Model)) == 0);
        printf("\tRead result is identical\n", filename);

        CONFIG_WriteModel(1);
        CONFIG_ReadModel(1);
        CuAssertTrue(t, memcmp(&ValidateModel, &Model, sizeof(Model)) == 0);
        printf("\tWrite result is identical\n", filename);        
    }
}

void TestModelLoadSave(CuTest *t)
{
    struct Model ValidateModel;

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
