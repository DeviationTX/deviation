#ifdef PAGEDEF
PAGEDEF(PAGEID_MAIN,     PAGE_MainInit,        PAGE_MainEvent,        PAGE_MainExit,    _tr_noop("Main page"))
PAGEDEF(PAGEID_MIXER,    PAGE_MixerInit,       PAGE_MixerEvent,       NULL,             _tr_noop("Mixer"))
PAGEDEF(PAGEID_MODEL,    PAGE_ModelInit,       PAGE_ModelEvent,       NULL,             _tr_noop("Model setup"))
PAGEDEF(PAGEID_TIMER,    PAGE_TimerInit,       PAGE_TimerEvent,       NULL,             _tr_noop("Timers"))
PAGEDEF(PAGEID_TELEMCFG, PAGE_TelemconfigInit, PAGE_TelemconfigEvent, NULL,             _tr_noop("Telemetry config"))
PAGEDEF(PAGEID_TRIM,     PAGE_TrimInit,        PAGE_TrimEvent,        NULL,             _tr_noop("Trims"))
#if DATALOG_ENABLED
PAGEDEF(PAGEID_DATALOG,  PAGE_DatalogInit,     PAGE_DatalogEvent,     NULL,             _tr_noop("Datalog"))
#endif
PAGEDEF(PAGEID_MAINCFG,  PAGE_MainLayoutInit,  PAGE_MainLayoutEvent,  PAGE_MainLayoutExit, _tr_noop("Main page config"))
PAGEDEF(PAGEID_TXCFG,    PAGE_TxConfigureInit, PAGE_TxConfigureEvent, NULL,             _tr_noop("Transmitter config"))
PAGEDEF(PAGEID_TELEMMON, PAGE_TelemtestInit,   PAGE_TelemtestEvent,   NULL,             _tr_noop("Telemetry monitor"))
PAGEDEF(PAGEID_CHANMON,  PAGE_ChantestInit,    PAGE_ChantestEvent,    PAGE_ChantestExit,_tr_noop("Channel monitor"))
PAGEDEF(PAGEID_INPUTMON, PAGE_InputtestInit,   PAGE_ChantestEvent,    PAGE_ChantestExit,_tr_noop("Input monitor"))
PAGEDEF(PAGEID_BTNMON,   PAGE_ButtontestInit,  PAGE_ChantestEvent,    PAGE_ChantestExit,_tr_noop("Button monitor"))
#ifdef ENABLE_SCANNER
PAGEDEF(PAGEID_SCANNER,  PAGE_ScannerInit,     PAGE_ScannerEvent,     PAGE_ScannerExit, _tr_noop("Scanner"))
#endif
PAGEDEF(PAGEID_USB,      PAGE_USBInit,         PAGE_USBEvent,         PAGE_USBExit,     _tr_noop("USB"))
#if HAS_RTC
PAGEDEF(PAGEID_RTC,      PAGE_RTCInit,         PAGE_RTCEvent,         NULL,             _tr_noop("Real Time Clock"))
#endif
PAGEDEF(PAGEID_SPLASH,   PAGE_SplashInit,      PAGE_SplashEvent,      PAGE_SplashExit,  _tr_noop("Welcome"))
PAGEDEF(PAGEID_SETTIMER, PAGE_SetTimerInit,    NULL,                  NULL,             _tr_noop("Set permanent timer"))
/* Simple */
#if !defined(NO_STANDARD_GUI)
PAGEDEF(PAGEID_MODELMENU, PAGE_ModelMenuInit,  NULL,                  NULL,             _tr_noop("Model menu"))
PAGEDEF(PAGEID_REVERSE,  PAGE_ReverseInit,     NULL,                  NULL,             _tr_noop("Reverse"))
PAGEDEF(PAGEID_DREXP,    PAGE_DrExpInit ,      PAGE_DrExpCurvesEvent, NULL,             _tr_noop("D/R & Exp"))
PAGEDEF(PAGEID_SUBTRIM,  PAGE_SubtrimInit,     NULL,                  NULL,             _tr_noop("Subtrim"))
PAGEDEF(PAGEID_TRAVELADJ,PAGE_TravelAdjInit,   NULL,                  NULL,             _tr_noop("Travel adjust"))
PAGEDEF(PAGEID_THROCURVES,PAGE_ThroCurvesInit, PAGE_CurvesEvent,      NULL,             _tr_noop("Throttle curves"))
PAGEDEF(PAGEID_PITCURVES,PAGE_PitCurvesInit,   PAGE_CurvesEvent,      NULL,             _tr_noop("Pitch curves"))
PAGEDEF(PAGEID_THROHOLD, PAGE_ThroHoldInit,    NULL,                  NULL,             _tr_noop("Throttle hold"))
PAGEDEF(PAGEID_GYROSENSE,PAGE_GyroSenseInit,   NULL,                  NULL,             _tr_noop("Gyro sense"))
PAGEDEF(PAGEID_SWASH,    PAGE_SwashInit,       NULL,                  NULL,             _tr_noop("Swash"))
PAGEDEF(PAGEID_FAILSAFE, PAGE_FailSafeInit,    NULL,                  NULL,             _tr_noop("Fail-safe"))
PAGEDEF(PAGEID_SWITCHASSIGN,PAGE_SwitchAssignInit, NULL,              NULL,             _tr_noop("Switch assignment"))
#endif /* NO_STANDARD_GUI */
#endif /* PAGEDEF */
