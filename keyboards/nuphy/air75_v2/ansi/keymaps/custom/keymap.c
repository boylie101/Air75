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

#include QMK_KEYBOARD_H
#include "mousekey.h"

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {

// layer Mac
[0] = LAYOUT_ansi_84(
	KC_ESC, 	KC_BRID,  	KC_BRIU,  	MAC_TASK, 	MAC_SEARCH, MAC_VOICE,  MAC_DND,  	KC_MPRV,  	KC_MPLY,  	KC_MNXT, 	KC_MUTE, 	KC_VOLD, 	KC_VOLU, 	MAC_PRTA,	KC_INS,		KC_DEL,
	KC_GRV, 	KC_1,   	KC_2,   	KC_3,  		KC_4,   	KC_5,   	KC_6,   	KC_7,   	KC_8,   	KC_9,  		KC_0,   	KC_MINS,	KC_EQL, 				KC_BSPC,	KC_PGUP,
	KC_TAB, 	KC_Q,   	KC_W,   	KC_E,  		KC_R,   	KC_T,   	KC_Y,   	KC_U,   	KC_I,   	KC_O,  		KC_P,   	KC_LBRC,	KC_RBRC, 				KC_BSLS,	KC_PGDN,
	KC_CAPS,	KC_A,   	KC_S,   	KC_D,  		KC_F,   	KC_G,   	KC_H,   	KC_J,   	KC_K,   	KC_L,  		KC_SCLN,	KC_QUOT, 	 						KC_ENT,		KC_HOME,
	KC_LSFT,				KC_Z,   	KC_X,   	KC_C,  		KC_V,   	KC_B,   	KC_N,   	KC_M,   	KC_COMM,	KC_DOT,		KC_SLSH,				KC_RSFT,	KC_UP,		KC_END,
	KC_LCTL,	KC_LALT,	KC_LGUI,										KC_SPC, 							KC_RGUI,	MO(1),   	KC_RCTL,				KC_LEFT,	KC_DOWN,    KC_RGHT),

// layer Mac Fn
[1] = LAYOUT_ansi_84(
	_______, 	KC_F1,  	KC_F2,  	KC_F3, 		KC_F4,  	KC_F5,  	KC_F6,  	KC_F7,  	KC_F8,  	KC_F9, 		KC_F10, 	KC_F11, 	KC_F12, 	MAC_PRT,	_______,	_______,
	_______, 	LNK_BLE1,  	LNK_BLE2,  	LNK_BLE3,  	LNK_RF,   	_______,   	_______,   	_______,   	_______,   	_______,  	_______,   	_______,	_______, 				_______,	_______,
	_______, 	_______,   	_______,   	_______,  	DEV_RESET,   	RGB_TEST,   	_______,   	_______,   	_______,   	_______,  	_______,   	_______,		_______, 			KC_BSLS,	_______,
	_______,	MO(4),   	SLEEP_MODE,   	_______,  	_______,   	_______,   	_______,	_______,   	_______,   	_______,  	_______,	_______, 	 						_______,	_______,
	_______,				_______,   	_______,   	_______,  	_______,	BAT_SHOW,	BAT_NUM,	MS_WIGL, 		RGB_SPD,	RGB_SPI,	_______,				_______,	RGB_VAI,	_______,
	_______,	_______,	_______,										_______, 							_______,	MO(1),   	_______,				RGB_MOD,	RGB_VAD,    RGB_HUI),

// layer win
[2] = LAYOUT_ansi_84(
	KC_ESC, 	KC_F1,  	KC_F2,  	KC_F3, 		KC_F4,  	KC_F5,  	KC_F6,  	KC_F7,  	KC_F8,  	KC_F9, 		KC_F10, 	KC_F11, 	KC_F12, 	KC_PSCR,	KC_INS,		KC_DEL,
	KC_GRV, 	KC_1,   	KC_2,   	KC_3,  		KC_4,   	KC_5,   	KC_6,   	KC_7,   	KC_8,   	KC_9,  		KC_0,   	KC_MINS,	KC_EQL, 				KC_BSPC,	KC_PGUP,
	KC_TAB, 	KC_Q,   	KC_W,   	KC_E,  		KC_R,   	KC_T,   	KC_Y,   	KC_U,   	KC_I,   	KC_O,  		KC_P,   	KC_LBRC,	KC_RBRC, 				KC_BSLS,	KC_PGDN,
	KC_CAPS,	KC_A,   	KC_S,   	KC_D,  		KC_F,   	KC_G,   	KC_H,   	KC_J,   	KC_K,   	KC_L,  		KC_SCLN,	KC_QUOT, 	 						KC_ENT,		KC_HOME,
	KC_LSFT,				KC_Z,   	KC_X,   	KC_C,  		KC_V,   	KC_B,   	KC_N,   	KC_M,   	KC_COMM,	KC_DOT,		KC_SLSH,				KC_RSFT,	KC_UP,		KC_END,
	KC_LCTL,	KC_LGUI,	KC_LALT,										KC_SPC, 							KC_RALT,	MO(3),   	KC_RCTL,				KC_LEFT,	KC_DOWN,    KC_RGHT),

// layer win Fn
[3] = LAYOUT_ansi_84(
	_______, 	KC_BRID,   	KC_BRIU,    _______,  	_______,   	_______,   	_______,   	KC_MPRV,   	KC_MPLY,   	KC_MNXT,  	KC_MUTE, 	KC_VOLD, 	KC_VOLU,	_______,	_______,	_______,
	_______, 	LNK_BLE1,  	LNK_BLE2,  	LNK_BLE3,  	LNK_RF,   	_______,   	_______,   	_______,   	_______,   	_______,  	_______,   	_______,	_______, 				_______,	_______,
	_______, 	_______,   	_______,   	_______,  	DEV_RESET,   	RGB_TEST,   	_______,   	_______,   	_______,   	_______,  	_______,   	_______,		_______, 			KC_BSLS,	_______,
	_______,	MO(4),   	SLEEP_MODE,   	_______,  	_______,   	_______,   	_______,	_______,   	_______,   	_______,  	_______,	_______, 	 						_______,	_______,
	_______,				_______,   	_______,   	_______,  	_______,	BAT_SHOW,	BAT_NUM,	MS_WIGL, 		RGB_SPD,	RGB_SPI,	_______,				_______,	RGB_VAI,	_______,
	_______,	_______,	_______,										_______, 							_______,	MO(3),   	_______,				RGB_MOD,	RGB_VAD,    RGB_HUI),

// layer 4
[4] = LAYOUT_ansi_84(
	_______, 	_______,  	_______,  	_______, 	_______,  	_______,  	_______,  	_______,  	_______,  	_______, 	_______, 	_______, 	_______, 	_______,	_______,	_______,
	_______, 	_______,   	_______,   	_______,  	_______,   	_______,   	_______,   	_______,   	_______,   	_______,  	_______,   	_______,	_______, 				_______,	_______,
	_______, 	_______,   	_______,   	_______,  	_______,   	_______,   	_______,   	_______,   	_______,   	_______,  	_______,   	_______,	_______, 				_______,	_______,
	_______,	_______,   	_______,   	_______,  	_______,   	_______,   	_______,   	_______,   	_______,   	_______,  	_______,	_______, 	 						_______,	_______,
	_______,				_______,   	_______,   	_______,  	_______,   	_______,   	_______,   	_______,   	SIDE_SPD,	SIDE_SPI,	_______,				_______,	SIDE_VAI,	_______,
	_______,	_______,	_______,										_______, 							_______,	MO(4),   	_______,				SIDE_MOD,	SIDE_VAD,   SIDE_HUI)
};


// Mouse wiggler: a small horizontal nudge every five seconds.
static bool     mouse_wiggler_enabled        = false;
static bool     mouse_wiggler_return_pending = false;
static uint32_t mouse_wiggler_timer          = 0;
static uint32_t mouse_wiggler_return_timer   = 0;

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case MS_WIGL:
            if (record->event.pressed) {
                mouse_wiggler_enabled = !mouse_wiggler_enabled;
                mouse_wiggler_timer = timer_read32();

                /*
                 * If disabled during the short movement interval,
                 * immediately return the pointer to its starting position.
                 */
                if (!mouse_wiggler_enabled && mouse_wiggler_return_pending) {
                    report_mouse_t report = mousekey_get_report();
                    report.x -= 5;
                    host_mouse_send(&report);
                    mouse_wiggler_return_pending = false;
                }
            }
            return false;
        default:
            return true;
    }
}

void housekeeping_task_user(void) {
    if (!mouse_wiggler_enabled) {
        return;
    }

    if (mouse_wiggler_return_pending) {
        if (timer_elapsed32(mouse_wiggler_return_timer) >= 50) {
            report_mouse_t report = mousekey_get_report();
            report.x -= 5;
            host_mouse_send(&report);
            mouse_wiggler_return_pending = false;
        }
        return;
    }

    if (timer_elapsed32(mouse_wiggler_timer) >= 5000) {
        report_mouse_t report = mousekey_get_report();
        report.x += 5;
        host_mouse_send(&report);
        mouse_wiggler_return_timer = timer_read32();
        mouse_wiggler_timer = mouse_wiggler_return_timer;
        mouse_wiggler_return_pending = true;
    }
}

static void set_key_led(uint8_t row, uint8_t col, uint8_t red, uint8_t green, uint8_t blue) {
    uint8_t led_index;
    if (rgb_matrix_map_row_column_to_led(row, col, &led_index)) {
        rgb_matrix_set_color(led_index, red, green, blue);
    }
}

bool rgb_matrix_indicators_user(void) {
    const uint8_t white_r = 128, white_g = 128, white_b = 128;
    const uint8_t purple_r = 128, purple_g = 0,   purple_b = 255;
    const uint8_t red_r    = 255, red_g    = 0,   red_b    = 0;
    const uint8_t blue_r   = 0,   blue_g   = 64,  blue_b   = 255;
    const uint8_t green_r  = 0,   green_g  = 255, green_b  = 0;
    const uint8_t orange_r = 255, orange_g = 128, orange_b = 0;

    // Fn + M is orange whenever either Fn layer is active; stays orange when wiggler is enabled.
    if (mouse_wiggler_enabled || layer_state_is(1) || layer_state_is(3)) {
        set_key_led(4, 8, orange_r, orange_g, orange_b);
    }

    if (!(layer_state_is(1) || layer_state_is(3))) {
        return true;
    }

    // Standard Fn functions.
    for (uint8_t col = 1; col <= 12; col++) {
        set_key_led(0, col, white_r, white_g, white_b);
    }
    set_key_led(2, 14, white_r, white_g, white_b); // Mac print
    set_key_led(3, 1, purple_r, purple_g, purple_b);  // Fn + A: extra layer
    set_key_led(3, 2, red_r, red_g, red_b); // Fn + S: sleep toggle
    set_key_led(2, 4, red_r, red_g, red_b);           // Fn + R: reset (hold 3 seconds)
    set_key_led(2, 5, white_r, white_g, white_b);  // Fn + T: RGB test mode
    set_key_led(2, 13, blue_r, blue_g, blue_b);          // Fn + backslash: backslash / pipe (blue)
    set_key_led(4, 9, white_r, white_g, white_b);  // RGB speed
    set_key_led(4, 10, white_r, white_g, white_b); // RGB speed
    set_key_led(4, 14, white_r, white_g, white_b); // RGB value
    set_key_led(5, 13, white_r, white_g, white_b); // RGB mode
    set_key_led(5, 14, white_r, white_g, white_b); // RGB value
    set_key_led(5, 15, white_r, white_g, white_b); // RGB hue

    // Wireless selection and battery functions.
    set_key_led(1, 1, blue_r, blue_g, blue_b);       // Bluetooth 1
    set_key_led(1, 2, blue_r, blue_g, blue_b);       // Bluetooth 2
    set_key_led(1, 3, blue_r, blue_g, blue_b);       // Bluetooth 3
    set_key_led(1, 4, red_r, red_g, red_b);          // 2.4 GHz
    set_key_led(4, 6, green_r, green_g, green_b);    // Fn + B: battery display
    set_key_led(4, 7, green_r, green_g, green_b);    // Fn + N: battery numbers

    // Fn + M: orange while held; persistent orange when the wiggler is enabled.
    set_key_led(4, 8, orange_r, orange_g, orange_b);

    // The extra layer contains the side-light controls.
    if (layer_state_is(4)) {
        set_key_led(4, 9, white_r, white_g, white_b);
        set_key_led(4, 10, white_r, white_g, white_b);
        set_key_led(4, 14, white_r, white_g, white_b);
        set_key_led(5, 13, white_r, white_g, white_b);
        set_key_led(5, 14, white_r, white_g, white_b);
        set_key_led(5, 15, white_r, white_g, white_b);
    }

    return true;
}
