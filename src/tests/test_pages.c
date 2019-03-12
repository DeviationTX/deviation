#include "CuTest.h"

extern void AssertScreenshot(CuTest* t, const char* testname);

#define PAGEDEF(id, init, event, exit, menu, name) menu,
static int page_attr[] = {
    #include "pagelist.h"
};
#undef PAGEDEF

void TestAllPages(CuTest* t)
{
    char testname[256];
    objHEAD = NULL;
    CONFIG_ReadLang(0);
    CONFIG_LoadTx();
    CONFIG_ReadDisplay();
    CONFIG_ResetModel();
    CONFIG_ReadTemplate("heli_std.ini");
    Transmitter.audio_player = AUDIO_AUDIOFX;
    Transmitter.current_model = 1;

    PAGE_Init();
    for (int i = 0; i < PAGEID_LAST; i++) {
        if ((page_attr[i] & (MIXER_STANDARD | MIXER_ADVANCED))
                != (MIXER_STANDARD | MIXER_ADVANCED)) {
            // If this is a mixer specific page
            if (page_attr[i] & MIXER_STANDARD) {
                Model.mixer_mode = MIXER_STANDARD;
                STDMIXER_Preset();
            } else {
                Model.mixer_mode = MIXER_ADVANCED;
            }
        } else {
            Model.mixer_mode = MIXER_ADVANCED;
        }

        // Skip the pages which are not consistent across tests run
        if (i == PAGEID_DEBUGLOG ||
            i == PAGEID_USB ||
            i == PAGEID_SPLASH ||
            i == PAGEID_LANGUAGE ||
            i == PAGEID_VOICECFG)
            continue;

        if (pages[i].pageName == NULL || pages[i].pageName[0] == '\0')
            continue;

        snprintf(testname, sizeof(testname), "%s", pages[i].pageName);
        for (int j = 0; testname[j] != '\0'; j++) {
            if (testname[j] == ' ') testname[j] = '_';
            if (testname[j] == '&') testname[j] = '_';
            if (testname[j] == '/') testname[j] = '_';
        }
        PAGE_ChangeByID(i, 0);
        GUI_RefreshScreen();
        AssertScreenshot(t, testname);
    }
}
