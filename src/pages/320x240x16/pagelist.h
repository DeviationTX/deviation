#ifdef PAGEDEF

#ifndef MENUID_MAIN
    #define MENUID_MAIN   (1 << 4)
    #define MENUID_MODEL  (2 << 4)
    #define MENUID_TX     (3 << 4)
#endif
//                  Menu ID     CONDITION
#define MAIN_MENU   MENUID_MAIN  |  (MIXER_STANDARD | MIXER_ADVANCED)        // Top-level menu, always shown
#define MODEL_MENU  MENUID_MODEL |  (MIXER_STANDARD | MIXER_ADVANCED)        // Model-menu, always shown
#define STDGUI_MENU MENUID_MODEL |  MIXER_STANDARD        // Model-menu, only shown for standard-gui
#define ADVGUI_MENU MENUID_MODEL |  MIXER_ADVANCED        // Model-menu, only shown for advanced-gui
#define TX_MENU     MENUID_TX    |  (MIXER_STANDARD | MIXER_ADVANCED)        // Transmitter-menu, always shown

//The following pages are not part of the menu system
//---------------------------------------------------

PAGEDEF(PAGEID_MAIN,     PAGE_MainInit,        PAGE_MainEvent,        PAGE_MainExit,      0,           _tr_noop("Main page"))
PAGEDEF(PAGEID_MENU,     PAGE_MenuInit,        NULL,                  NULL,               0,           _tr_noop("Main menu"))
#if HAS_PERMANENT_TIMER
PAGEDEF(PAGEID_SETTIMER, PAGE_SetTimerInit,    NULL,                  PAGE_SetTimerExit,  0,           _tr_noop("Set permanent timer"))
#endif
#if HAS_RTC
PAGEDEF(PAGEID_RTC,      PAGE_RTCInit,         PAGE_RTCEvent,         NULL,               0,           _tr_noop("Real Time Clock"))
#endif
#if HAS_TOUCH
PAGEDEF(PAGEID_TOUCH,    PAGE_TouchInit,       PAGE_TouchEvent,       NULL,               0,           _tr_noop("Touch Test"))
#endif
//Main menu
//---------
PAGEDEF(PAGEID_MODELMNU, PAGE_ModelMenuInit,   NULL,                  NULL,               MAIN_MENU,   _tr_noop("Model menu"))
PAGEDEF(PAGEID_TXMENU,   PAGE_TxMenuInit,      NULL,                  NULL,               MAIN_MENU,   _tr_noop("Transmitter menu"))
PAGEDEF(PAGEID_USB,      PAGE_USBInit,         PAGE_USBEvent,         PAGE_USBExit,       MAIN_MENU,   _tr_noop("USB"))
#if DEBUG_WINDOW_SIZE
PAGEDEF(PAGEID_DEBUGLOG, PAGE_DebuglogInit,    PAGE_DebuglogEvent,    NULL,               MAIN_MENU,   _tr_noop("Debuglog"))
#endif

//Model menu
//----------
PAGEDEF(PAGEID_MODEL,    PAGE_ModelInit,       PAGE_ModelEvent,       NULL,               MODEL_MENU,  _tr_noop("Model setup"))
PAGEDEF(PAGEID_MIXER,    PAGE_MixerInit,       NULL,                  NULL,               ADVGUI_MENU, _tr_noop("Mixer"))
#if HAS_STANDARD_GUI
PAGEDEF(PAGEID_REVERSE,  PAGE_ReverseInit,     NULL,                  NULL,               STDGUI_MENU, _tr_noop("Reverse"))
PAGEDEF(PAGEID_DREXP,    PAGE_DrExpInit ,      PAGE_DrExpCurvesEvent, NULL,               STDGUI_MENU, _tr_noop("D/R & Exp"))
PAGEDEF(PAGEID_SUBTRIM,  PAGE_SubtrimInit,     NULL,                  NULL,               STDGUI_MENU, _tr_noop("Subtrim"))
PAGEDEF(PAGEID_TRAVELADJ,PAGE_TravelAdjInit,   NULL,                  NULL,               STDGUI_MENU, _tr_noop("Travel adjust"))
PAGEDEF(PAGEID_THROCURVES,PAGE_ThroCurvesInit, PAGE_CurvesEvent,      NULL,               STDGUI_MENU, _tr_noop("Throttle curves"))
PAGEDEF(PAGEID_PITCURVES,PAGE_PitCurvesInit,   PAGE_CurvesEvent,      NULL,               STDGUI_MENU, _tr_noop("Pitch curves"))
PAGEDEF(PAGEID_THROHOLD, PAGE_ThroHoldInit,    NULL,                  NULL,               STDGUI_MENU, _tr_noop("Throttle hold"))
PAGEDEF(PAGEID_GYROSENSE,PAGE_GyroSenseInit,   NULL,                  NULL,               STDGUI_MENU, _tr_noop("Gyro sense"))
PAGEDEF(PAGEID_SWASH,    PAGE_SwashInit,       NULL,                  NULL,               STDGUI_MENU, _tr_noop("Swash"))
PAGEDEF(PAGEID_FAILSAFE, PAGE_FailSafeInit,    NULL,                  NULL,               STDGUI_MENU, _tr_noop("Fail-safe"))
PAGEDEF(PAGEID_SWITCHASSIGN,PAGE_SwitchAssignInit, NULL,              NULL,               STDGUI_MENU, _tr_noop("Switch assignment"))
#endif /* HAS_STANDARD_GUI */
PAGEDEF(PAGEID_TIMER,    PAGE_TimerInit,       PAGE_TimerEvent,       NULL,               MODEL_MENU,  _tr_noop("Timers"))
PAGEDEF(PAGEID_TELEMCFG, PAGE_TelemconfigInit, PAGE_TelemconfigEvent, NULL,               MODEL_MENU,  _tr_noop("Telemetry config"))
PAGEDEF(PAGEID_TRIM,     PAGE_TrimInit,        NULL,                  NULL,               ADVGUI_MENU, _tr_noop("Trims"))
#if HAS_DATALOG
PAGEDEF(PAGEID_DATALOG,  PAGE_DatalogInit,     PAGE_DatalogEvent,     NULL,               MODEL_MENU,  _tr_noop("Datalog"))
#endif
PAGEDEF(PAGEID_MAINCFG,  PAGE_MainLayoutInit,  PAGE_MainLayoutEvent,  PAGE_MainLayoutExit,MODEL_MENU,  _tr_noop("Main page config"))

// Transmitter menu
//-----------------
PAGEDEF(PAGEID_TXCFG,    PAGE_TxConfigureInit, NULL,                  NULL,               TX_MENU,     _tr_noop("Transmitter config"))
PAGEDEF(PAGEID_CHANMON,  PAGE_ChantestInit,    PAGE_ChantestEvent,    PAGE_ChantestExit,  TX_MENU,     _tr_noop("Channel monitor"))
PAGEDEF(PAGEID_TELEMMON, PAGE_TelemtestInit,   PAGE_TelemtestEvent,   NULL,               TX_MENU,     _tr_noop("Telemetry monitor"))
PAGEDEF(PAGEID_RANGE,    PAGE_RangeInit,       NULL,	              PAGE_RangeExit,     TX_MENU,     _tr_noop("Range Test"))
PAGEDEF(PAGEID_INPUTMON, PAGE_InputtestInit,   PAGE_ChantestEvent,    PAGE_ChantestExit,  TX_MENU,     _tr_noop("Input monitor"))
PAGEDEF(PAGEID_BTNMON,   PAGE_ButtontestInit,  PAGE_ChantestEvent,    PAGE_ChantestExit,  TX_MENU,     _tr_noop("Button monitor"))
#if HAS_SCANNER
PAGEDEF(PAGEID_SCANNER,  PAGE_ScannerInit,     PAGE_ScannerEvent,     PAGE_ScannerExit,   TX_MENU,     _tr_noop("Scanner"))
#endif

//These pages should not be lisetd for quickpages
PAGEDEF(PAGEID_SPLASH,   PAGE_SplashInit,      PAGE_SplashEvent,      PAGE_SplashExit,    0,           _tr_noop("Welcome"))
PAGEDEF(PAGEID_EDITLIMIT, PAGE_EditLimitsInit, NULL,                  NULL,               0,           "")
PAGEDEF(PAGEID_MIXTEMPL, PAGE_MixTemplateInit, PAGE_MixTemplateEvent, NULL,               0,           "")
PAGEDEF(PAGEID_EDITCURVE, PAGE_EditCurvesInit, NULL,                  NULL,               0,           "")
PAGEDEF(PAGEID_LOADSAVE, PAGE_LoadSaveInit,    NULL,                  NULL,               0,           "")
PAGEDEF(PAGEID_REORDER,  PAGE_ReorderInit,     NULL,                  NULL,               0,           "")
PAGEDEF(PAGEID_LANGUAGE, PAGE_LanguageInit,    NULL,                  NULL,               0,           _tr_noop("Select Language"))
PAGEDEF(PAGEID_CALIB,    PAGE_CalibInit,       NULL,                  NULL,               0,           "")
PAGEDEF(PAGEID_TRIMEDIT, PAGE_TrimEditInit,    NULL,                  NULL,               0,           "")
PAGEDEF(PAGEID_TGLEDIT,  PAGE_ToggleEditInit,  NULL,                  PAGE_ToggleEditExit,0,           "")
PAGEDEF(PAGEID_PROTOCFG, PAGE_ModelProtoInit,  NULL,                  NULL,               0,           "")
PAGEDEF(PAGEID_TYPECFG,  PAGE_ModelConfigInit, NULL,                  NULL,               0,           "")
PAGEDEF(PAGEID_TRAINCFG, PAGE_TrainConfigInit, NULL,                  NULL,               0,           "")


#endif /* PAGEDEF */
