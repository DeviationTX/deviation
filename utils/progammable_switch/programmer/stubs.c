
const char DeviationVersion[33] = "";
volatile unsigned char priority_ready = 0;
volatile unsigned char PrevXferComplete;

void TELEMETRY_SetUpdated(int telem) {
    (void)telem;
}
void TELEMETRY_SetType(int type) {
    (void)type;
}

void medium_priority_cb() {}

void PROTOCOL_SetSwitch(int module)
{
    (void)module;
}
