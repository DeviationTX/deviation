
static void _lcd_backlight_init()
{
    rcc_periph_clock_enable(get_rcc_from_pin(BACKLIGHT_TIM.pin));
    GPIO_setup_input(BACKLIGHT_TIM.pin, ITYPE_FLOAT);

    // Configure Backlight PWM
    rcc_periph_clock_enable(get_rcc_from_port(BACKLIGHT_TIM.tim));
    timer_set_mode(BACKLIGHT_TIM.tim, TIM_CR1_CKD_CK_INT,
                   TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
    timer_set_period(BACKLIGHT_TIM.tim, 0x2CF);
    timer_set_prescaler(BACKLIGHT_TIM.tim, 0);
    timer_generate_event(BACKLIGHT_TIM.tim, TIM_EGR_UG);
    timer_set_oc_mode(BACKLIGHT_TIM.tim, TIM_OCx(BACKLIGHT_TIM.ch), TIM_OCM_PWM1);
    timer_enable_oc_preload(BACKLIGHT_TIM.tim, TIM_OCx(BACKLIGHT_TIM.ch));

    timer_set_oc_polarity_high(BACKLIGHT_TIM.tim, TIM_OCx(BACKLIGHT_TIM.ch));
    timer_enable_oc_output(BACKLIGHT_TIM.tim, TIM_OCx(BACKLIGHT_TIM.ch));

    timer_enable_preload(BACKLIGHT_TIM.tim);
}

static void _lcd_backlight_brightness(unsigned brightness)
{
    timer_disable_counter(BACKLIGHT_TIM.tim);
    if (brightness == 0) {
        // Turn off Backlight
        GPIO_setup_input(BACKLIGHT_TIM.pin, ITYPE_FLOAT);
    } else if (brightness > 9) {
        // Turn on Backlight full
        GPIO_setup_output(BACKLIGHT_TIM.pin, OTYPE_PUSHPULL);
        GPIO_pin_set(BACKLIGHT_TIM.pin);
    } else {
        GPIO_setup_output_af(BACKLIGHT_TIM.pin, OTYPE_PUSHPULL, BACKLIGHT_TIM.tim);
        u32 duty_cycle = 720 * brightness / 10;
        timer_set_oc_value(BACKLIGHT_TIM.tim, TIM_OCx(BACKLIGHT_TIM.ch), duty_cycle);
        timer_enable_counter(BACKLIGHT_TIM.tim);
    }
}
