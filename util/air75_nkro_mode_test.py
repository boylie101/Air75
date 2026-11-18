#!/usr/bin/env python3
# Copyright 2026 QMK
# SPDX-License-Identifier: GPL-2.0-or-later
"""Exercise actual Air75 slider/startup/reset functions with stubbed hardware."""

import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
KEYBOARD = ROOT / 'keyboards/nuphy/air75_v2/ansi'


def function(source, name):
    match = re.search(r'^(?:void|bool) ' + name + r'\([^;]*?\)\s*\{.*?^}', source, re.M | re.S)
    if match is None:
        raise ValueError(f'Cannot locate production function {name}')
    return match[0] + '\n'


STUBS = r'''
static DEV_INFO_STRUCT dev_info;
static struct {bool nkro;} keymap_config;
static bool f_rf_sw_press, f_dev_reset_press, f_rgb_test_press, f_rf_new_adv_ok;
static bool f_dial_sw_init_ok, f_sys_show;
static uint16_t rf_sw_press_delay, dev_reset_press_delay, rgb_test_press_delay;
static uint16_t no_act_time, rf_linking_time;
static uint8_t rf_sw_temp, rf_host_driver;
static uint32_t now, selected_layer;
static unsigned clears, resets;
static bool mac_position;
#define DEV_MODE_PIN 0
#define SYS_MODE_PIN 1
static uint32_t timer_read32(void) {return now;}
static uint32_t timer_elapsed32(uint32_t start) {return now - start;}
static void wait_ms(uint32_t delay) {(void)delay;}
static void gpio_set_pin_input_high(uint8_t pin) {(void)pin;}
static bool gpio_read_pin(uint8_t pin) {return pin == DEV_MODE_PIN || mac_position;}
static void uart_send_cmd(uint8_t cmd, uint8_t ack, uint8_t delay) {(void)cmd; (void)ack; (void)delay;}
static void uart_receive_pro(void) {}
static void device_reset_show(void) {}
static void device_reset_init(void) {}
static void rgb_test_show(void) {}
static void eeconfig_init(void) {keymap_config.nkro = false; ++resets;}
static void default_layer_set(uint32_t layer) {selected_layer = layer;}
static void break_all_key(void) {++clears;}
static void switch_dev_link(uint8_t mode) {dev_info.link_mode = mode;}
static void host_set_driver(void *driver) {(void)driver;}
'''

CHECKS = r'''
static void check_mode(bool mac) {
    assert(dev_info.sys_sw_state == (mac ? SYS_SW_MAC : SYS_SW_WIN));
    assert(selected_layer == (mac ? 1u : 4u));
    assert(keymap_config.nkro == (mac ? EXPECT_MAC_NKRO : true));
}
int main(int argc, char **argv) {
    assert(argc == 2);
    dev_info.link_mode = LINK_USB;
    mac_position = argv[1][0] != '1' && argv[1][0] != '4';
    switch (argv[1][0]) {
        case '0':
        case '1':
            dial_sw_fast_scan(); check_mode(mac_position); assert(clears == 1);
            break;
        case '2':
            mac_position = false; dial_sw_fast_scan(); check_mode(false);
            mac_position = true;
            for (unsigned i = 0; i < 40; ++i) {now += 21; dial_sw_scan();}
            check_mode(true); assert(f_dial_sw_init_ok);
            mac_position = false;
            for (unsigned i = 0; i < 40; ++i) {now += 21; dial_sw_scan();}
            check_mode(false);
            break;
        case '3':
        case '4':
            dev_info.sys_sw_state = mac_position ? SYS_SW_MAC : SYS_SW_WIN;
            f_dev_reset_press = true; dev_reset_press_delay = DEV_RESET_PRESS_DELAY - 1;
            now = 101; long_press_key();
            check_mode(mac_position); assert(resets == 1 && !f_dev_reset_press);
            break;
        default: assert(false);
    }
    return 0;
}
'''


def main():
    ansi = (KEYBOARD / 'ansi.c').read_text()
    header = (KEYBOARD / 'ansi.h').read_text()
    policy = re.search(r'^#ifndef AIR75_MAC_MODE_NKRO\n.*?^#endif', ansi, re.M | re.S)
    assert policy is not None
    defines = '\n'.join(re.findall(r'^#define (?:RF_\w+|CMD_\w+|LINK_\w+|SYS_\w+|DEV_RESET_PRESS_DELAY|RGB_TEST_PRESS_DELAY)\s+.*$', header, re.M))
    types = re.search(r'typedef struct\s*\{[^}]*\}\s*DEV_INFO_STRUCT;', header)[0]
    functions = ''.join(function(ansi, name) for name in ('long_press_key', 'dial_sw_scan', 'dial_sw_fast_scan'))
    custom = '#include "' + str(KEYBOARD / 'keymaps/custom/config.h') + '"\n'
    variants = [('legacy', '', False), ('custom', custom, True), ('custom_6kro_override', custom + '#undef AIR75_MAC_MODE_NKRO\n#define AIR75_MAC_MODE_NKRO false\n', False)]
    with tempfile.TemporaryDirectory(prefix='air75-nkro-mode-') as temp:
        for name, config, expected in variants:
            source = '#include <assert.h>\n#include <stdbool.h>\n#include <stdint.h>\n' + config
            source += policy[0] + '\n#define EXPECT_MAC_NKRO ' + str(expected).lower() + '\n'
            source += defines + '\n' + types + STUBS + functions + CHECKS
            path = Path(temp) / name
            path.with_suffix('.c').write_text(source)
            subprocess.run([os.environ.get('CC', 'clang'), '-std=c11', '-Wall', '-Wextra', '-Werror', '-fsanitize=undefined', '-fno-sanitize-recover=all', str(path.with_suffix('.c')), '-o', str(path)], check=True)
            for case in range(5):
                subprocess.run([str(path), str(case)], check=True)
            print(f'PASS {name}: Mac/Windows startup, live slider transitions, Mac/Windows factory reset')


if __name__ == '__main__':
    main()
