#ifndef _DEVOF12E_HARDWARE_H_
#define _DEVOF12E_HARDWARE_H_

#define I2C1_CFG ((struct i2c_config) { \
    .i2c = I2C1,                        \
    .scl_sca = {GPIOB, GPIO6 | GPIO7},  \
    .i2c_freq = 400000,                 \
    .dutycycle = I2C_CCR_DUTY_DIV2,     \
    .fastmode  = 1,                     \
    })
#define LCD_I2C_CFG I2C1_CFG
#endif
