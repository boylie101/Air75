/*
Copyright 2023 @ Nuphy <https://nuphy.com/>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ansi.h"
#include "usb_main.h"
#include "rf_driver.h"

void nuphy_rgb_note_keypress(uint8_t row, uint8_t col);
void nuphy_cycle_color_next(void);
void nuphy_adjust_user_color(uint8_t component);

static uint16_t rgb_adjust_keycode;
static uint32_t rgb_adjust_repeat_timer;

keyboard_config_t keyboard_config;
DEV_INFO_STRUCT dev_info = {
    .rf_battery = 100,
    .link_mode  = LINK_USB,
    .rf_state   = RF_IDLE,
};

bool f_uart_ack         = 0;
bool f_bat_hold         = 0;
bool f_sys_show         = 0;
bool f_sleep_show       = 0;
bool f_send_channel     = 0;
bool f_dial_sw_init_ok  = 0;
bool f_rf_sw_press      = 0;
bool f_dev_reset_press  = 0;
bool f_rgb_test_press   = 0;
bool f_bat_num_show     = 0;
bool f_rf_hand_ok       = 0;
bool f_goto_sleep       = 0;
bool f_rf_read_data_ok  = 0;
bool f_rf_sts_sysc_ok   = 0;
bool f_rf_new_adv_ok    = 0;
bool f_rf_reset         = 0;
bool f_wakeup_prepare   = 0;

uint16_t       rf_linking_time       = 0;
uint16_t       rf_link_show_time     = 0;
uint8_t        rf_blink_cnt          = 0;
uint16_t       no_act_time           = 0;
uint8_t        rf_sw_temp            = 0;
uint16_t       dev_reset_press_delay = 0;
uint16_t       rf_sw_press_delay     = 0;
uint16_t rgb_test_press_delay        = 0;
uint8_t        host_mode             = 0;
host_driver_t *m_host_driver         = 0;

extern bool               f_rf_new_adv_ok;
extern report_keyboard_t *keyboard_report;
extern report_nkro_t *nkro_report;
extern uint8_t            bitkb_report_buf[32];
extern uint8_t            bytekb_report_buf[8];
extern uint8_t            side_mode;
extern uint8_t            side_light;
extern uint8_t            side_speed;
extern uint8_t            side_rgb;
extern uint8_t            side_colour;

void    dev_sts_sync(void);
void    rf_uart_init(void);
void    rf_device_init(void);
void    uart_send_report_func(void);
void    uart_receive_pro(void);
uint8_t uart_send_cmd(uint8_t cmd, uint8_t ack_cnt, uint8_t delayms);
void    uart_send_report(uint8_t report_type, uint8_t *report_buf, uint8_t report_size);
void    side_speed_control(uint8_t dir);
void    side_light_control(uint8_t dir);
void    side_colour_control(uint8_t dir);
void    side_mode_control(uint8_t dir);
void    side_led_show(void);
void    sleep_handle(void);
void    num_led_show(void);
void    rgb_test_show(void);
void    device_reset_show(void);
void    device_reset_init(void);

/**
 * @brief  Initialize the GPIO pins.
 */
void gpio_init(void) {
    /* Enable the LED power drivers. */
    gpio_set_pin_output(DRIVER_LED_CS_PIN);
    gpio_set_pin_output(DRIVER_SIDE_CS_PIN);
    gpio_write_pin_low(DRIVER_LED_CS_PIN);
    gpio_write_pin_low(DRIVER_SIDE_CS_PIN);
    /* Set the side LED data pin low. */
    gpio_set_pin_output(DRIVER_SIDE_PIN);
    gpio_write_pin_low(DRIVER_SIDE_PIN);
    /* Configure the RF module pins. */
    gpio_set_pin_output(NRF_WAKEUP_PIN);
    gpio_write_pin_high(NRF_WAKEUP_PIN);
    gpio_set_pin_input_high(NRF_TEST_PIN);
    /* Reset the RF module. */
    gpio_set_pin_output(NRF_RESET_PIN);
    gpio_write_pin_low(NRF_RESET_PIN);
    wait_ms(50);
    gpio_write_pin_high(NRF_RESET_PIN);
    /* Configure the mode-switch pins. */
    gpio_set_pin_input_high(DEV_MODE_PIN);
    gpio_set_pin_input_high(SYS_MODE_PIN);
    /* Enable the LED power supply. */
    gpio_set_pin_output(DC_BOOST_PIN);
    gpio_write_pin_high(DC_BOOST_PIN);
}

/**
 * @brief  Process long key presses.
 */
void long_press_key(void) {
    static uint32_t long_press_timer = 0;

    if (timer_elapsed32(long_press_timer) < 100) {
        return;
    }
    long_press_timer = timer_read32();

    // Open a new RF device
    if (f_rf_sw_press) {
        rf_sw_press_delay++;
        if (rf_sw_press_delay >= RF_LONG_PRESS_DELAY) {
            f_rf_sw_press = 0;

            dev_info.link_mode   = rf_sw_temp;
            dev_info.rf_channel  = rf_sw_temp;
            dev_info.ble_channel = rf_sw_temp;

            uint8_t timeout = 5;
            while (timeout--) {
                uart_send_cmd(CMD_NEW_ADV, 0, 1);
                wait_ms(20);
                uart_receive_pro();
                if (f_rf_new_adv_ok) {
                    break;
                }
            }
        }
    } else {
        rf_sw_press_delay = 0;
    }

    // Restore the device to factory settings.
    if (f_dev_reset_press) {
        dev_reset_press_delay++;
        if (dev_reset_press_delay >= DEV_RESET_PRESS_DELAY) {
            f_dev_reset_press = 0;

            if (dev_info.link_mode != LINK_USB) {
                if (dev_info.link_mode != LINK_RF_24) {
                    dev_info.link_mode      = LINK_BT_1;
                    dev_info.ble_channel    = LINK_BT_1;
                    dev_info.rf_channel     = LINK_BT_1;
                }
            } else {
                dev_info.ble_channel = LINK_BT_1;
                dev_info.rf_channel  = LINK_BT_1;
            }

            uart_send_cmd(CMD_SET_LINK, 10, 10);
            wait_ms(500);
            uart_send_cmd(CMD_CLR_DEVICE, 10, 10);

            eeconfig_init();
            device_reset_show();
            device_reset_init();

            if (dev_info.sys_sw_state == SYS_SW_MAC) {
                default_layer_set(1 << 0);
                keymap_config.nkro = 0;
            } else {
                default_layer_set(1 << 2);
                keymap_config.nkro = 1;
            }
        }
    } else {
        dev_reset_press_delay = 0;
    }

    // Enter the RGB test mode
    if (f_rgb_test_press) {
        rgb_test_press_delay++;
        if (rgb_test_press_delay >= RGB_TEST_PRESS_DELAY) {
            f_rgb_test_press = 0;
            rgb_test_show();
        }
    } else {
        rgb_test_press_delay = 0;
    }
}

/**
 * @brief  Release all keys, clear keyboard report.
 */
void break_all_key(void) {
    uint8_t report_buf[16];
    bool    nkro_temp = keymap_config.nkro;

    clear_weak_mods();
    clear_mods();
    clear_keyboard();

    keymap_config.nkro = 1;
    memset(nkro_report, 0, sizeof(report_nkro_t));
    host_nkro_send(nkro_report);
    wait_ms(10);

    keymap_config.nkro = 0;
    memset(keyboard_report, 0, sizeof(report_keyboard_t));
    host_keyboard_send(keyboard_report);
    wait_ms(10);

    keymap_config.nkro = nkro_temp;

    if (dev_info.link_mode != LINK_USB) {
        memset(report_buf, 0, 16);
        uart_send_report(CMD_RPT_BIT_KB, report_buf, 16);
        wait_ms(10);
        uart_send_report(CMD_RPT_BYTE_KB, report_buf, 8);
        wait_ms(10);
    }

    memset(bitkb_report_buf, 0, sizeof(bitkb_report_buf));
    memset(bytekb_report_buf, 0, sizeof(bytekb_report_buf));
}

/**
 * @brief  Switch the device connection mode.
 * @param mode The connection mode to select.
 */
void switch_dev_link(uint8_t mode) {
    if (mode > LINK_USB) {
        return;
    }

    break_all_key();

    dev_info.link_mode = mode;

    dev_info.rf_state = RF_IDLE;
    f_send_channel    = 1;

    if (mode == LINK_USB) {
        host_mode = HOST_USB_TYPE;
        host_set_driver(m_host_driver);
        rf_link_show_time = 0;
    } else {
        host_mode = HOST_RF_TYPE;

        host_set_driver(&rf_host_driver);
    }
}

/**
 * @brief  Scan the mode switches.
 */
void dial_sw_scan(void) {
    uint8_t         dial_scan       = 0;
    static uint8_t  dial_save       = 0xf0;
    static uint8_t  debounce        = 0;
    static uint32_t dial_scan_timer = 0;
    static bool     f_first         = true;

    if (!f_first) {
        if (timer_elapsed32(dial_scan_timer) < 20) {
            return;
        }
    }
    dial_scan_timer = timer_read32();

    gpio_set_pin_input_high(DEV_MODE_PIN);
    gpio_set_pin_input_high(SYS_MODE_PIN);

    if (gpio_read_pin(DEV_MODE_PIN)) {
        dial_scan |= 0X01;
    }
    if (gpio_read_pin(SYS_MODE_PIN)) {
        dial_scan |= 0X02;
    }

    if (dial_save != dial_scan) {
        break_all_key();

        no_act_time     = 0;
        rf_linking_time = 0;

        dial_save         = dial_scan;
        debounce          = 25;
        f_dial_sw_init_ok = 0;
        return;
    } else if (debounce) {
        debounce--;
        return;
    }

    if (dial_scan & 0x01) {
        if (dev_info.link_mode != LINK_USB) {
            switch_dev_link(LINK_USB);
        }
    } else {
        if (dev_info.link_mode != dev_info.rf_channel) {
            switch_dev_link(dev_info.rf_channel);
        }
    }

    if (dial_scan & 0x02) {
        if (dev_info.sys_sw_state != SYS_SW_MAC) {
            f_sys_show = 1;
            default_layer_set(1 << 0);
            dev_info.sys_sw_state = SYS_SW_MAC;
            keymap_config.nkro    = 0;
            break_all_key();
        }
    } else {
        if (dev_info.sys_sw_state != SYS_SW_WIN) {
            f_sys_show = 1;
            default_layer_set(1 << 2);
            dev_info.sys_sw_state = SYS_SW_WIN;
            keymap_config.nkro    = 1;
            break_all_key();
        }
    }

    if (f_dial_sw_init_ok == 0) {
        f_dial_sw_init_ok = 1;
        f_first           = false;

        if (dev_info.link_mode != LINK_USB) {
            host_set_driver(&rf_host_driver);
        }
    }
}

/**
 * @brief  Read the mode switches during startup.
 */
void dial_sw_fast_scan(void) {
    uint8_t dial_scan_dev = 0;
    uint8_t dial_scan_sys = 0;
    uint8_t dial_check_dev = 0;
    uint8_t dial_check_sys = 0;
    uint8_t debounce = 0;

    gpio_set_pin_input_high(DEV_MODE_PIN);
    gpio_set_pin_input_high(SYS_MODE_PIN);

    // Debounce to get a stable state
    for (debounce = 0; debounce < 10; debounce++) {
        dial_scan_dev = 0;
        dial_scan_sys = 0;
        if (gpio_read_pin(DEV_MODE_PIN)) {
            dial_scan_dev = 0x01;
        } else {
            dial_scan_dev = 0;
        }
        if (gpio_read_pin(SYS_MODE_PIN)) {
            dial_scan_sys = 0x01;
        } else {
            dial_scan_sys = 0;
        }
        if ((dial_scan_dev != dial_check_dev) || (dial_scan_sys != dial_check_sys)) {
            dial_check_dev = dial_scan_dev;
            dial_check_sys = dial_scan_sys;
            debounce = 0;
        }
        wait_ms(1);
    }

    // Select the wireless or USB connection mode.
    if (dial_scan_dev) {
        if (dev_info.link_mode != LINK_USB) {
            switch_dev_link(LINK_USB);
        }
    } else {
        if (dev_info.link_mode != dev_info.rf_channel) {
            switch_dev_link(dev_info.rf_channel);
        }
    }

    // Select the Windows or macOS key layout.
    if (dial_scan_sys) {
        if (dev_info.sys_sw_state != SYS_SW_MAC) {
            default_layer_set(1 << 0);
            dev_info.sys_sw_state = SYS_SW_MAC;
            keymap_config.nkro    = 0;
            break_all_key();
        }
    } else {
        if (dev_info.sys_sw_state != SYS_SW_WIN) {
            //f_sys_show = 1;
            default_layer_set(1 << 2);
            dev_info.sys_sw_state = SYS_SW_WIN;
            keymap_config.nkro    = 1;
            break_all_key();
        }
    }
}

/**
 * @brief  Update keyboard timers.
 */
void timer_pro(void) {
    static uint32_t interval_timer = 0;
    static bool     f_first        = true;

    if (f_first) {
        f_first        = false;
        interval_timer = timer_read32();
        m_host_driver  = host_get_driver();
    }

    if (timer_elapsed32(interval_timer) < 10) {
        return;
    } else {
        interval_timer = timer_read32();
    }

    if (rf_link_show_time < RF_LINK_SHOW_TIME) {
        rf_link_show_time++;
    }

    if (no_act_time < 0xffff) {
        no_act_time++;
    }

    if (rf_linking_time < 0xffff) {
        rf_linking_time++;
    }
}

/**
 * @brief  Load keyboard settings from EEPROM.
 */
void loading_eeprom_data(void) {
    eeconfig_read_kb_datablock(&keyboard_config, 0, sizeof(keyboard_config));
    if (keyboard_config.default_brightness_flag != 0xAD) {
        /* Updated lighting defaults: static sage green for key and stronger green side LEDs. */
        rgb_matrix_enable();
        rgb_matrix_mode(RGB_MATRIX_SOLID_COLOR);
        rgb_matrix_sethsv(68, 118, 149);
        side_mode = 2;
        side_light = 4;
        side_colour = 0;
        keyboard_config.default_brightness_flag = 0xAD;
        keyboard_config.ee_side_mode            = side_mode;
        keyboard_config.ee_side_light           = side_light;
        keyboard_config.ee_side_speed           = side_speed;
        keyboard_config.ee_side_rgb             = side_rgb;
        keyboard_config.ee_side_colour          = side_colour;
        keyboard_config.sleep_enable            = true;
        eeconfig_update_kb_datablock(&keyboard_config, 0, sizeof(keyboard_config));
    } else {
        side_mode   = keyboard_config.ee_side_mode;
        side_light  = keyboard_config.ee_side_light;
        side_speed  = keyboard_config.ee_side_speed;
        side_rgb    = keyboard_config.ee_side_rgb;
        side_colour = keyboard_config.ee_side_colour;
    }
}

/* QMK key event processing. */
bool process_record_kb(uint16_t keycode, keyrecord_t *record) {
    if (!process_record_user(keycode, record)) {
        return false;
    }
    if (record->event.pressed) {
        nuphy_rgb_note_keypress(record->event.key.row, record->event.key.col);
    }
    no_act_time = 0;
    switch (keycode) {
        case RGB_HUE_UP:
        case RGB_SAT_UP:
        case RGB_VAL_UP:
            if (record->event.pressed) {
                rgb_adjust_keycode = keycode;
                rgb_adjust_repeat_timer = timer_read32();
                nuphy_adjust_user_color(keycode == RGB_HUE_UP ? 0 : keycode == RGB_SAT_UP ? 1 : 2);
            } else if (rgb_adjust_keycode == keycode) {
                rgb_adjust_keycode = 0;
            }
            return false;

        case RF_DFU:
            if (record->event.pressed) {
                if (dev_info.link_mode != LINK_USB) {
                    return false;
                }
                uart_send_cmd(CMD_RF_DFU, 10, 20);
            }
            return false;

        case LNK_USB:
            if (record->event.pressed) {
                break_all_key();
            } else {
                dev_info.link_mode = LINK_USB;
                uart_send_cmd(CMD_SET_LINK, 10, 10);
            }
            return false;

        case LNK_RF:
            if (record->event.pressed) {
                if (dev_info.link_mode != LINK_USB) {
                    rf_sw_temp    = LINK_RF_24;
                    f_rf_sw_press = 1;
                    break_all_key();
                }
            } else if (f_rf_sw_press) {
                f_rf_sw_press = 0;

                if (rf_sw_press_delay < RF_LONG_PRESS_DELAY) {
                    dev_info.link_mode   = rf_sw_temp;
                    dev_info.rf_channel  = rf_sw_temp;
                    dev_info.ble_channel = rf_sw_temp;
                    uart_send_cmd(CMD_SET_LINK, 10, 20);
                }
            }
            return false;

        case LNK_BLE1:
            if (record->event.pressed) {
                if (dev_info.link_mode != LINK_USB) {
                    rf_sw_temp    = LINK_BT_1;
                    f_rf_sw_press = 1;
                    break_all_key();
                }
            } else if (f_rf_sw_press) {
                f_rf_sw_press = 0;

                if (rf_sw_press_delay < RF_LONG_PRESS_DELAY) {
                    dev_info.link_mode   = rf_sw_temp;
                    dev_info.rf_channel  = rf_sw_temp;
                    dev_info.ble_channel = rf_sw_temp;
                    uart_send_cmd(CMD_SET_LINK, 10, 20);
                }
            }
            return false;

        case LNK_BLE2:
            if (record->event.pressed) {
                if (dev_info.link_mode != LINK_USB) {
                    rf_sw_temp    = LINK_BT_2;
                    f_rf_sw_press = 1;
                    break_all_key();
                }
            } else if (f_rf_sw_press) {
                f_rf_sw_press = 0;

                if (rf_sw_press_delay < RF_LONG_PRESS_DELAY) {
                    dev_info.link_mode   = rf_sw_temp;
                    dev_info.rf_channel  = rf_sw_temp;
                    dev_info.ble_channel = rf_sw_temp;
                    uart_send_cmd(CMD_SET_LINK, 10, 20);
                }
            }
            return false;

        case LNK_BLE3:
            if (record->event.pressed) {
                if (dev_info.link_mode != LINK_USB) {
                    rf_sw_temp    = LINK_BT_3;
                    f_rf_sw_press = 1;
                    break_all_key();
                }
            } else if (f_rf_sw_press) {
                f_rf_sw_press = 0;

                if (rf_sw_press_delay < RF_LONG_PRESS_DELAY) {
                    dev_info.link_mode   = rf_sw_temp;
                    dev_info.rf_channel  = rf_sw_temp;
                    dev_info.ble_channel = rf_sw_temp;
                    uart_send_cmd(CMD_SET_LINK, 10, 20);
                }
            }
            return false;

        case SIDE_VAI:
            if (record->event.pressed) {
                side_light_control(1);
            }
            return false;

        case SIDE_VAD:
            if (record->event.pressed) {
                side_light_control(0);
            }
            return false;

        case SIDE_MOD:
            if (record->event.pressed) {
                side_mode_control(1);
            }
            return false;

        case SIDE_HUI:
            if (record->event.pressed) {
                side_colour_control(1);
            }
            return false;

        case RGB_COL_NEXT: {
            if (record->event.pressed) {
                nuphy_cycle_color_next();
            }
            return false;
        }

        case SIDE_SPI:
            if (record->event.pressed) {
                side_speed_control(1);
            }
            return false;

        case SIDE_SPD:
            if (record->event.pressed) {
                side_speed_control(0);
            }
            return false;

        case DEV_RESET:
            if (record->event.pressed) {
                f_dev_reset_press = 1;
                break_all_key();
            } else {
                f_dev_reset_press = 0;
            }
            return false;

        case SLEEP_MODE:
            if (record->event.pressed) {
                if (keyboard_config.sleep_enable) {
                    keyboard_config.sleep_enable = false;
                }
                else keyboard_config.sleep_enable = true;
                f_sleep_show       = 1;
                eeconfig_update_kb_datablock(&keyboard_config, 0, sizeof(keyboard_config));
            }
            return false;

        case BAT_SHOW:
            if (record->event.pressed) {
                f_bat_hold = !f_bat_hold;
            }
            return false;

        case BAT_NUM:
            if (record->event.pressed) {
                f_bat_num_show = 1;
            } else {
                f_bat_num_show = 0;
            }
            return false;

        case RGB_TEST:
            if (record->event.pressed) {
                f_rgb_test_press = 1;
            } else {
                f_rgb_test_press = 0;
            }
            return false;

        default:
            return true;
    }
}


bool rgb_matrix_indicators_kb(void)
{
    if (!rgb_matrix_indicators_user()) {
        return false;
    }
    if (f_bat_num_show) {
        num_led_show();
    }
    rgb_matrix_set_color(RGB_MATRIX_LED_COUNT-1, 0, 0, 0);
    return true;
}

/* QMK keyboard post-initialization. */
void keyboard_post_init_kb(void) {
    gpio_init();
    rf_uart_init();
    wait_ms(500);
    rf_device_init();

    break_all_key();
    dial_sw_fast_scan();
    loading_eeprom_data();
    keyboard_post_init_user();
}

/* QMK housekeeping task. */
void housekeeping_task_kb(void) {
    timer_pro();

    uart_receive_pro();

    uart_send_report_func();

    dev_sts_sync();

    long_press_key();

    dial_sw_scan();

    side_led_show();

    sleep_handle();

    if (rgb_adjust_keycode && timer_elapsed32(rgb_adjust_repeat_timer) >= 50) {
        rgb_adjust_repeat_timer = timer_read32();
        nuphy_adjust_user_color(rgb_adjust_keycode == RGB_HUE_UP ? 0 : rgb_adjust_keycode == RGB_SAT_UP ? 1 : 2);
    }
}
