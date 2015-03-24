
void MODULE_CSN(int module, int set)
{
    if (set) {
        PROTOSPI_pin_set(MODULE_ENABLE[module]);
    } else {
        PROTOSPI_pin_clear(MODULE_ENABLE[module]);
    }
}
int MULTIMOD_SwitchCommand(int module, int command)
{
    switch(command) {
        case TXRX_OFF:
        case TX_EN:
        case RX_EN:
            PACTL_SetTxRxMode(command);
            break;
        case CLEAR_PIN_ENABLE:
            PACTL_SetNRF24L01_CE(0);
            break;
        case SET_PIN_ENABLE:
            PACTL_SetNRF24L01_CE(1);
            break;
        case CHANGE_MODULE:
            PACTL_SetSwitch(module);
            break;
    }
    return 1;
}

