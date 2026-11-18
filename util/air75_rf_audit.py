#!/usr/bin/env python3
# Copyright 2026 QMK
# SPDX-License-Identifier: GPL-2.0-or-later
"""Native Air75 regression tests using production RF and QMK function bodies.

UART, clocks, GPIO and USB/connection state are simulated, not physical devices.
Each case runs in a fresh process. Core weak hooks and keyboard overrides link
from separate translation units. Structures and buffers come from actual source.
"""
import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
KEYBOARD = ROOT / 'keyboards/nuphy/air75_v2/ansi'
BASELINE = '9c8d758b1c1029878212e2e105c4c150a31e0893'


def function(source, name, optional=False):
    match = re.search(r'^[A-Za-z_][^\n;{}]*\b' + name + r'\([^;{}]*?\)\s*\{.*?^}', source, re.M | re.S)
    if match is None:
        if optional:
            return ''
        raise ValueError(f'Cannot locate production function {name}')
    return match[0] + '\n'


def structure(source, name):
    match = re.search(r'typedef struct\s*\{[^}]*\}\s*(?:PACKED )?' + name + ';', source)
    if match is None:
        raise ValueError(f'Cannot locate production structure {name}')
    return match[0] + '\n'


DECLARATIONS = r'''
#define NKRO_ENABLE
#define NO_ACTION_ONESHOT
#define USB_PROTOCOL_BOOT 0
#define USB_PROTOCOL_REPORT 1
#define NRF_WAKEUP_PIN 0
#define NRF_RESET_PIN 1
#define dprintf(...) ((void)0)
#define dprint(...) ((void)0)
extern bool debug_keyboard;
extern DEV_INFO_STRUCT dev_info;
extern report_keyboard_t *keyboard_report;
extern report_nkro_t *nkro_report;
extern bool f_uart_ack, f_rf_read_data_ok, f_rf_sts_sysc_ok, f_rf_new_adv_ok;
extern bool f_rf_reset, f_rf_hand_ok, f_goto_sleep, f_send_channel, f_dial_sw_init_ok, f_wakeup_prepare;
extern uint8_t host_mode, rf_blink_cnt;
extern uint16_t rf_link_show_time, rf_linking_time, no_act_time;
extern host_driver_t rf_host_driver, *m_host_driver;
typedef struct {bool nkro;} test_keymap_config_t;
extern test_keymap_config_t keymap_config;
uint8_t usb_device_state_get_protocol(void);
bool bluetooth_can_send_nkro(void);
uint32_t timer_read32(void);
uint32_t timer_elapsed32(uint32_t start);
void wait_ms(uint32_t delay);
void wait_us(uint32_t delay);
void gpio_write_pin_low(uint8_t pin);
void gpio_write_pin_high(uint8_t pin);
void uart_transmit(const uint8_t *data, uint16_t size);
void UART_Send_Bytes(uint8_t *data, uint32_t size);
uint8_t get_checksum(uint8_t *data, uint8_t size);
uint8_t uart_send_cmd(uint8_t cmd, uint8_t ack, uint8_t delay);
void uart_send_report(uint8_t cmd, uint8_t *data, uint8_t size);
void uart_send_report_func(void);
void break_all_key(void);
void host_set_driver(host_driver_t *driver);
host_driver_t *host_get_driver(void);
bool host_can_send_nkro(void);
bool host_can_send_nkro_for_driver(host_driver_t *driver, bool default_capability);
void host_keyboard_send(report_keyboard_t *report);
void host_nkro_send(report_nkro_t *report);
void add_key_to_report(uint8_t key);
void del_key_from_report(uint8_t key);
void clear_keys_from_report(void);
void send_keyboard_report(void);
void clear_mods(void);
void clear_weak_mods(void);
void add_mods(uint8_t mods);
void clear_keyboard(void);
void clear_keyboard_but_mods(void);
void clear_keyboard_but_mods_and_keys(void);
static inline void clear_keys(void) {clear_keys_from_report();}
void test_active_host(connection_host_t host);
'''


def sources(rf, baseline=False, no_override=False):
    ansi = (KEYBOARD / 'ansi.h').read_text()
    report = (ROOT / 'tmk_core/protocol/report.h').read_text()
    host = (ROOT / 'tmk_core/protocol/host.c').read_text()
    action = (ROOT / 'quantum/action.c').read_text()
    action_util = (ROOT / 'quantum/action_util.c').read_text()
    report_c = (ROOT / 'tmk_core/protocol/report.c').read_text()
    connection = (ROOT / 'quantum/connection/connection.h').read_text()
    header = '#pragma once\n#include <assert.h>\n#include <stdbool.h>\n#include <stdint.h>\n#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>\n#define PACKED __attribute__((packed))\n'
    header += '\n'.join(re.findall(r'^#define (?:NKRO_REPORT_BITS|KEYBOARD_REPORT_KEYS)\s+.*$', report, re.M)) + '\n'
    header += '\n'.join(re.findall(r'^#define (?:LINK_\w+|RF_\w+|CMD_\w+|UART_\w+|HOST_\w+|FUNC_VALID_LEN|POWER_DOWN_DELAY)\s+.*$', ansi, re.M)) + '\n'
    header += re.search(r'typedef enum\s*\{.*?\} TYPE_RX_STATE;', ansi, re.S)[0] + '\n'
    header += re.search(r'enum hid_report_ids\s*\{.*?\};', report, re.S)[0] + '\n'
    header += re.search(r'typedef enum connection_host_t\s*\{.*?\} connection_host_t;', connection, re.S)[0] + '\n'
    header += '\n'.join(re.findall(r'^typedef int8_t mouse_(?:xy|hv)_report_t;', report, re.M)) + '\n'
    header += ''.join(structure(report, n) for n in ('report_keyboard_t', 'report_nkro_t', 'report_mouse_t', 'report_extra_t'))
    header += ''.join(structure(ansi, n) for n in ('DEV_INFO_STRUCT', 'USART_MGR_STRUCT'))
    header += re.search(r'typedef struct\s*\{[^}]*\} host_driver_t;', (ROOT / 'tmk_core/protocol/host_driver.h').read_text())[0] + '\n'
    header += DECLARATIONS
    core = '#include "fixture.h"\nstatic host_driver_t *driver;\nstatic uint8_t real_mods, weak_mods;\n'
    core += '#ifdef CONNECTION_ENABLE\nstatic connection_host_t active_host = CONNECTION_HOST_USB;\nextern host_driver_t bt_driver;\n#endif\n'
    core += 'void test_active_host(connection_host_t host) {\n#ifdef CONNECTION_ENABLE\nactive_host = host;\n#else\n(void)host;\n#endif\n}\n'
    core += ''.join(function(host, n) for n in ('host_set_driver', 'host_get_driver', 'host_get_active_driver', 'host_can_send_nkro', 'host_keyboard_send', 'host_nkro_send'))
    core += function(host, 'host_can_send_nkro_for_driver', optional=True)
    core += ''.join(function(report_c, n) for n in ('add_key_byte', 'del_key_byte', 'add_key_bit', 'del_key_bit', 'add_key_to_report', 'del_key_from_report', 'clear_keys_from_report'))
    core += ''.join(function(action_util, n) for n in ('get_mods_for_report', 'send_6kro_report', 'send_nkro_report', 'send_keyboard_report', 'clear_mods', 'clear_weak_mods', 'add_mods'))
    core += ''.join(function(action, n) for n in ('clear_keyboard', 'clear_keyboard_but_mods', 'clear_keyboard_but_mods_and_keys'))
    native = '#include "fixture.h"\n'
    native += '\n'.join(re.findall(r'^#define RX_\w+\s+.*$', rf, re.M)) + '\nUSART_MGR_STRUCT Usart_Mgr;\n'
    native += '\n'.join(re.findall(r'^(?:static )?(?:uint8_t|uint16_t|uint32_t|bool)\s+\w+(?:\[[^\]]+\])?\s*(?:=[^;]+)?;', rf, re.M)) + '\n'
    native += function(rf, 'rf_keyboard_state_active', optional=True)
    native += ''.join(
        function(rf, n) for n in (
            'uart_auto_nkro_send', 'uart_send_report_func', 'uart_send_report_keyboard', 'uart_send_report_nkro', 'uart_send_consumer_report', 'uart_send_system_report', 'uart_send_mouse_report', 'get_checksum', 'UART_Send_Bytes', 'uart_send_report',
            'RF_Protocol_Receive', 'uart_send_cmd', 'dev_sts_sync'
        )
    )
    driver_source = (KEYBOARD / 'rf_driver.c').read_text()
    native += 'static uint8_t rf_keyboard_leds(void);\nstatic void rf_send_keyboard(report_keyboard_t *report);\nstatic void rf_send_nkro(report_nkro_t *report);\nstatic void rf_send_mouse(report_mouse_t *report);\nstatic void rf_send_extra(report_extra_t *report);\n'
    native += re.search(r'^host_driver_t\s+rf_host_driver\s*=.*?;', driver_source, re.M | re.S)[0] + '\n'
    native += ''.join(function(driver_source, n) for n in ('rf_keyboard_leds', 'rf_send_keyboard', 'rf_send_nkro', 'rf_send_mouse', 'rf_send_extra'))
    if not no_override:
        native += function(driver_source, 'host_can_send_nkro_for_driver', optional=True)
    native += ''.join(function((KEYBOARD / 'ansi.c').read_text(), n) for n in ('break_all_key', 'switch_dev_link'))
    native += f'\n#define BASELINE_ENCODER {int(baseline)}\n#define NO_DRIVER_OVERRIDE {int(no_override)}\n'
    native += (ROOT / 'tests/air75_rf/native.inc').read_text()
    return header, core, native


def build(directory, name, rf, analyze, connection=False, **kwargs):
    path = directory / name
    path.mkdir()
    for filename, source in zip(('fixture.h', 'core.c', 'native.c'), sources(rf, **kwargs)):
        (path / filename).write_text(source)
    flags = ['-std=c11', '-Wall', '-Wextra', '-Werror', '-g']
    if connection:
        flags += ['-DCONNECTION_ENABLE', '-DBLUETOOTH_ENABLE']
    compiler = os.environ.get('CC', 'clang')
    if analyze:
        for filename in ('core.c', 'native.c'):
            subprocess.run([compiler, '--analyze', *flags, '-Xanalyzer', '-analyzer-werror', str(path / filename), '-o', str(path / (filename + '.plist'))], check=True)
    binary = path / 'audit'
    subprocess.run([compiler, *flags, '-fsanitize=address,undefined', '-fno-sanitize-recover=all', str(path / 'core.c'), str(path / 'native.c'), '-o', str(binary)], check=True)
    return binary


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--analyze', action='store_true')
    parser.add_argument('--stage', choices=('encoder', 'transport', 'recovery', 'all'), default='all')
    args = parser.parse_args()
    rf = (KEYBOARD / 'rf.c').read_text()
    with tempfile.TemporaryDirectory(prefix='air75-rf-tests-') as temp:
        directory = Path(temp)
        current = build(directory, 'current', rf, args.analyze)
        count = 0
        for case in range(12):
            subprocess.run([str(current), str(case)], check=True, stdout=subprocess.DEVNULL)
            count += 1
        old_rf = subprocess.check_output(['git', 'show', BASELINE + ':keyboards/nuphy/air75_v2/ansi/rf.c'], cwd=ROOT, text=True)
        old = build(directory, 'baseline', old_rf, args.analyze, baseline=True)
        for case in (0, 1, 2, 4, 5, 6, 7, 8, 9, 11):
            assert subprocess.check_output([str(current), str(case)]) == subprocess.check_output([str(old), str(case)]), case
            count += 1
        print(f'PASS {count} encoder checks (12 current cases; 10 byte-for-byte comparisons with pre-hardening encoder)')
        if args.stage != 'encoder':
            default = build(directory, 'default-hook', rf, args.analyze, no_override=True)
            connected = build(directory, 'connection', rf, args.analyze, connection=True)
            for binary in (current, default):
                for case in range(20, 28):
                    subprocess.run([str(binary), str(case)], check=True)
            subprocess.run([str(current), '28'], check=True)
            for case in range(29, 33):
                subprocess.run([str(connected), str(case)], check=True)
            print('PASS 21 transport checks: USB/RF x BOOT/REPORT x six-key/NKRO, default weak hook, switching, built-in Bluetooth/NONE')
            for case in (40, 41, 42):
                subprocess.run([str(current), str(case)], check=True)
            subprocess.run([str(default), '40'], check=True)
            print('PASS 4 state-clearing checks: conditional failure reproduced without RF override, absent with override; normal switching and mode toggle clear correctly')
            if args.stage in ('recovery', 'all'):
                for case in range(60, 76):
                    subprocess.run([str(current), str(case)], check=True)
                print('PASS 16 recovery checks: long holds, offline releases, reconnects, module/status resets, missed snapshots, six-key mode, sleep and channel clearing')


if __name__ == '__main__':
    main()
