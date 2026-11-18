#!/usr/bin/env python3
# Copyright 2026 QMK
# SPDX-License-Identifier: GPL-2.0-or-later
"""Test actual Air75 EEPROM loading and side renderers with simulated hardware."""

import argparse
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile

from air75_rf_audit import function, structure

ROOT = Path(__file__).resolve().parents[1]
KEYBOARD = ROOT / 'keyboards/nuphy/air75_v2/ansi'
BASELINE = '48ca65f4dcdd2f87870bd94ced1adf99362dd329'

STUBS = r'''
static keyboard_config_t keyboard_config, stored;
static bool f_bat_hold;
static uint32_t now;
static unsigned reads, writes, rgb_calls;
static uint32_t timer_read32(void) {return now;}
static uint32_t timer_elapsed32(uint32_t start) {return now - start;}
static uint32_t eeconfig_read_kb_datablock(void *data, uint32_t offset, uint32_t length) {
    assert(offset == 0 && length == sizeof(stored));
    memcpy(data, &stored, length); ++reads; return length;
}
static uint32_t eeconfig_update_kb_datablock(const void *data, uint32_t offset, uint32_t length) {
    assert(offset == 0 && length == sizeof(stored));
    memcpy(&stored, data, length); ++writes; return length;
}
static void rgb_matrix_enable(void) {++rgb_calls;}
static void rgb_matrix_mode(uint8_t mode) {assert(mode == RGB_MATRIX_SOLID_COLOR); ++rgb_calls;}
static void rgb_matrix_sethsv(uint8_t h, uint8_t s, uint8_t v) {assert(h == 68 && s == 118 && v == 149); ++rgb_calls;}
static void rgb_matrix_set_speed(uint8_t speed) {(void)speed; ++rgb_calls;}
static void side_ws2812_setleds(rgb_led_t *leds, uint16_t size) {assert(leds == side_leds && size == SIDE_LED_NUM);}
static void bat_led_show(void) {}
static void sleep_sw_led_show(void) {}
static void sys_sw_led_show(void) {}
static void sys_led_show(void) {}
static void rf_led_show(void) {}
'''


def sources(reference=None):
    def read(name):
        path = KEYBOARD / name
        if reference:
            return subprocess.check_output(['git', 'show', reference + ':' + str(path.relative_to(ROOT))], cwd=ROOT, text=True)
        return path.read_text()

    ansi, header, side = read('ansi.c'), read('ansi.h'), read('side.c')
    count = re.search(r'^#define EECONFIG_KB_DATA_SIZE\s+(\d+)', read('config.h'), re.M)[1]
    speed_step = json.loads(read('keyboard.json'))['rgb_matrix']['speed_steps']
    text = '#include <assert.h>\n#include <stdbool.h>\n#include <stdint.h>\n#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>\n'
    text += '#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))\n#define RGB_MATRIX_SOLID_COLOR 1\n'
    text += f'#define RGB_MATRIX_SPD_STEP {speed_step}\n#define EECONFIG_KB_DATA_SIZE {count}\n'
    text += 'typedef struct {uint8_t r, g, b;} rgb_led_t;\n'
    text += structure(header, 'keyboard_config_t')
    text += read('side_table.h').replace('#pragma once', '')
    text += side[side.index('#define SIDE_BRIGHT_MAX'):side.index('extern DEV_INFO_STRUCT')]
    text += STUBS
    text += function(side, 'side_config_sanitize', optional=True)
    text += function(ansi, 'loading_eeprom_data')
    for name in (
        'side_rgb_set_color', 'side_rgb_refresh', 'side_light_control', 'side_speed_control', 'side_colour_control', 'side_mode_control', 'light_point_playing', 'count_rgb_light', 'side_wave_mode_show', 'side_spectrum_mode_show', 'side_breathe_mode_show',
        'side_static_mode_show', 'side_off_mode_show', 'device_reset_init', 'side_led_show'
    ):
        text += function(side, name)
    text += (ROOT / 'tests/air75_eeprom/native.inc').read_text()
    return text


def build(directory, name, reference, analyze):
    path = directory / name
    source = path.with_suffix('.c')
    source.write_text(sources(reference))
    flags = ['-std=c11', '-Wall', '-Wextra', '-Werror', '-g']
    compiler = os.environ.get('CC', 'clang')
    if analyze:
        subprocess.run([compiler, '--analyze', *flags, '-Xanalyzer', '-analyzer-werror', str(source), '-o', str(path.with_suffix('.plist'))], check=True)
    subprocess.run([compiler, *flags, '-fsanitize=address,undefined', '-fno-sanitize-recover=all', str(source), '-o', str(path)], check=True)
    return path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--analyze', action='store_true')
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix='air75-eeprom-') as directory:
        directory = Path(directory)
        current = build(directory, 'current', None, args.analyze)
        old = build(directory, 'baseline', BASELINE, args.analyze)
        current_valid = subprocess.check_output([str(current), 'valid'], text=True)
        assert current_valid == subprocess.check_output([str(old), 'valid'], text=True)
        print(current_valid.strip() + '; identical settings/render output to pre-fix loader')
        subprocess.run([str(current), 'repairs'], check=True)
        for field in ('speed', 'colour', 'brightness'):
            result = subprocess.run([str(old), field], capture_output=True, text=True)
            assert result.returncode != 0 and ('out of bounds' in result.stderr or 'buffer-overflow' in result.stderr), (field, result.stderr)
            subprocess.run([str(current), field], check=True)
        print('PASS: all three original out-of-bounds reproductions fail under sanitizers before the fix and pass after it')


if __name__ == '__main__':
    main()
