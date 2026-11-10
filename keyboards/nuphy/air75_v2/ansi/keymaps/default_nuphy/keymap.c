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

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {

// layer Linux
[0] = LAYOUT_75_ansi(
	KC_ESC,	KC_BRID,	KC_BRIU,	LGUI(KC_D),	LGUI(KC_E),	LGUI(S(KC_M)),	LGUI(S(KC_D)),	KC_MPRV,	KC_MPLY,	KC_MNXT,	KC_MUTE,	KC_VOLD,	KC_VOLU,	S(KC_PSCR),	KC_INS,	KC_DEL,
	KC_GRV, 	KC_1,   	KC_2,   	KC_3,  		KC_4,   	KC_5,   	KC_6,   	KC_7,   	KC_8,   	KC_9,  		KC_0,   	KC_MINS,	KC_EQL, 				KC_BSPC,	KC_PGUP,
	KC_TAB, 	KC_Q,   	KC_W,   	KC_E,  		KC_R,   	KC_T,   	KC_Y,   	KC_U,   	KC_I,   	KC_O,  		KC_P,   	KC_LBRC,	KC_RBRC, 				KC_BSLS,	KC_PGDN,
	KC_CAPS,	KC_A,   	KC_S,   	KC_D,  		KC_F,   	KC_G,   	KC_H,   	KC_J,   	KC_K,   	KC_L,  		KC_SCLN,	KC_QUOT, 	 						KC_ENT,		KC_HOME,
	KC_LSFT,				KC_Z,   	KC_X,   	KC_C,  		KC_V,   	KC_B,   	KC_N,   	KC_M,   	KC_COMM,	KC_DOT,		KC_SLSH,				KC_RSFT,	KC_UP,		KC_END,
	KC_LCTL,	KC_LGUI,	KC_LALT,										KC_SPC, 							KC_RGUI,	MO(1),   	KC_RCTL,				KC_LEFT,	KC_DOWN,    KC_RGHT),

// layer Linux Fn
[1] = LAYOUT_75_ansi(
	_______,	KC_F1,	KC_F2,	KC_F3,	KC_F4,	KC_F5,	KC_F6,	KC_F7,	KC_F8,	KC_F9,	KC_F10,	KC_F11,	KC_F12,	S(KC_PSCR),	_______,	_______,
	_______, 	LNK_BLE1,  	LNK_BLE2,  	LNK_BLE3,  	LNK_RF,   	_______,   	_______,   	_______,   	_______,   	_______,  	_______,   	_______,	_______, 				_______,	_______,
	_______, 	_______,   	_______,   	_______,  	_______,   	_______,   	_______,   	_______,   	_______,   	_______,  	_______,   	DEV_RESET,	_______, 			BAT_SHOW,	_______,
	_______,	_______,RGB_SAT_UP,	_______,	_______,	_______,RGB_HUE_UP,	_______,	_______,	MO(4),	_______,	_______,	_______,	_______,
	_______,SLEEP_MODE,	_______,	RGB_TEST,RGB_VAL_UP,	BAT_NUM,	_______,	_______,	RM_SPDD,	RM_SPDU,	_______,	_______,	RM_VALU,	_______,
	_______,	_______,	_______,										_______, 							_______,	MO(1),   	_______,				RM_NEXT,	RM_VALD,    RGB_COL_NEXT),

// layer Windows
[2] = LAYOUT_75_ansi(
	KC_ESC,	KC_BRID,	KC_BRIU,	KC_MPRV,	KC_MPLY,	KC_MNXT,	KC_MUTE,	KC_VOLD,	KC_VOLU,	_______,	_______,	_______,	_______,	_______,	KC_INS,	KC_DEL,
	KC_GRV, 	KC_1,   	KC_2,   	KC_3,  		KC_4,   	KC_5,   	KC_6,   	KC_7,   	KC_8,   	KC_9,  		KC_0,   	KC_MINS,	KC_EQL, 				KC_BSPC,	KC_PGUP,
	KC_TAB, 	KC_Q,   	KC_W,   	KC_E,  		KC_R,   	KC_T,   	KC_Y,   	KC_U,   	KC_I,   	KC_O,  		KC_P,   	KC_LBRC,	KC_RBRC, 				KC_BSLS,	KC_PGDN,
	KC_CAPS,	KC_A,   	KC_S,   	KC_D,  		KC_F,   	KC_G,   	KC_H,   	KC_J,   	KC_K,   	KC_L,  		KC_SCLN,	KC_QUOT, 	 						KC_ENT,		KC_HOME,
	KC_LSFT,				KC_Z,   	KC_X,   	KC_C,  		KC_V,   	KC_B,   	KC_N,   	KC_M,   	KC_COMM,	KC_DOT,		KC_SLSH,				KC_RSFT,	KC_UP,		KC_END,
	KC_LCTL,	KC_LGUI,	KC_LALT,										KC_SPC, 							KC_RALT,	MO(3),   	KC_RCTL,				KC_LEFT,	KC_DOWN,    KC_RGHT),

// layer Windows Fn
[3] = LAYOUT_75_ansi(
	_______,	KC_F1,	KC_F2,	LGUI(KC_D),	LGUI(KC_E),	LGUI(LALT(KC_K)),	LGUI(KC_N),	KC_F7,	KC_F8,	KC_F9,	KC_F10,	KC_F11,	KC_F12,	KC_PSCR,	_______,	_______,
	_______, 	LNK_BLE1,  	LNK_BLE2,  	LNK_BLE3,  	LNK_RF,   	_______,   	_______,   	_______,   	_______,   	_______,  	_______,   	_______,	_______, 				_______,	_______,
	_______, 	_______,   	_______,   	_______,  	_______,   	_______,   	_______,   	_______,   	_______,   	_______,  	_______,   	DEV_RESET,	_______, 			BAT_SHOW,	_______,
	_______,	_______,RGB_SAT_UP,	_______,	_______,	_______,RGB_HUE_UP,	_______,	_______,	MO(4),	_______,	_______,	_______,	_______,
	_______,SLEEP_MODE,	_______,	RGB_TEST,RGB_VAL_UP,	BAT_NUM,	_______,	_______,	RM_SPDD,	RM_SPDU,	_______,	_______,	RM_VALU,	_______,
	_______,	_______,	_______,										_______, 							_______,	MO(3),   	_______,				RM_NEXT,	RM_VALD,    RGB_COL_NEXT),

// layer 4
[4] = LAYOUT_75_ansi(
	_______, 	_______,  	_______,  	_______, 	_______,  	_______,  	_______,  	_______,  	_______,  	_______, 	_______, 	_______, 	_______, 	_______,	_______,	_______,
	_______, 	_______,   	_______,   	_______,  	_______,   	_______,   	_______,   	_______,   	_______,   	_______,  	_______,   	_______,	_______, 				_______,	_______,
	_______, 	_______,   	_______,   	_______,  	_______,   	_______,   	_______,   	_______,   	_______,   	_______,  	_______,   	_______,	_______, 				_______,	_______,
	_______,	_______,   	_______,   	_______,  	_______,   	_______,   	_______,   	_______,   	_______,   	_______,  	_______,	_______, 	 						_______,	_______,
	_______,				_______,   	_______,   	_______,  	_______,   	_______,   	_______,   	_______,   	SIDE_SPD,	SIDE_SPI,	_______,				_______,	SIDE_VAI,	_______,
	_______,	_______,	_______,										_______, 							_______,	MO(4),   	_______,				SIDE_MOD,	SIDE_VAD,   SIDE_HUI)
};


static void set_key_led(uint8_t row, uint8_t col, uint8_t red, uint8_t green, uint8_t blue) {
    uint8_t led_index;
    if (rgb_matrix_map_row_column_to_led(row, col, &led_index)) {
        rgb_matrix_set_color(led_index, red, green, blue);
    }
}

bool rgb_matrix_indicators_user(void) {
    if (!layer_state_is(1) && !layer_state_is(3)) {
        return true;
    }

    set_key_led(4, 2, 255, 0, 0);     // Fn + Z: Sleep Mode
    set_key_led(3, 6, 128, 128, 128); // Fn + H: hue
    set_key_led(3, 2, 128, 128, 128); // Fn + S: saturation
    set_key_led(4, 5, 128, 128, 128); // Fn + V: value
    return true;
}
