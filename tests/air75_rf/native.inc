/* Copyright 2026 QMK
 * SPDX-License-Identifier: GPL-2.0-or-later
 * Included after production function bodies by air75_rf_audit.py.
 * Hardware, clocks, USB state and connection state are simulated.
 */
bool                 debug_keyboard;
DEV_INFO_STRUCT      dev_info = {.link_mode = LINK_RF_24, .rf_state = RF_CONNECT};
bool                 f_uart_ack, f_rf_read_data_ok, f_rf_sts_sysc_ok, f_rf_new_adv_ok;
bool                 f_rf_reset, f_rf_hand_ok, f_goto_sleep, f_send_channel, f_dial_sw_init_ok = true;
uint8_t              host_mode = HOST_RF_TYPE, rf_blink_cnt;
uint16_t             rf_link_show_time, rf_linking_time, no_act_time;
test_keymap_config_t keymap_config = {true};
report_keyboard_t    keyboard_state, *keyboard_report = &keyboard_state;
report_nkro_t        nkro_state, *nkro_report         = &nkro_state;
static uint8_t       protocol = USB_PROTOCOL_REPORT;
static uint32_t      now;
static bool          bt_nkro;
static unsigned      usb_byte_count, usb_bit_count;
static void          usb_keyboard(report_keyboard_t *report) {
    (void)report;
    ++usb_byte_count;
}
static void usb_nkro(report_nkro_t *report) {
    (void)report;
    ++usb_bit_count;
}
static host_driver_t usb_driver = {.send_keyboard = usb_keyboard, .send_nkro = usb_nkro};
host_driver_t        bt_driver;
host_driver_t       *m_host_driver = &usb_driver;
uint8_t              usb_device_state_get_protocol(void) {
    return protocol;
}
bool bluetooth_can_send_nkro(void) {
    return bt_nkro;
}
uint32_t timer_read32(void) {
    return now;
}
uint32_t timer_elapsed32(uint32_t start) {
    return now - start;
}
void wait_ms(uint32_t delay) {
    (void)delay;
}
void wait_us(uint32_t delay) {
    (void)delay;
}
void gpio_write_pin_low(uint8_t pin) {
    (void)pin;
}
void gpio_write_pin_high(uint8_t pin) {
    (void)pin;
}
static struct {
    uint8_t  bytes[UART_MAX_LEN];
    uint16_t size;
} frames[4096];
static unsigned frame_count;
void            uart_transmit(const uint8_t *data, uint16_t size) {
    assert(frame_count < 4096 && size <= UART_MAX_LEN);
    memcpy(frames[frame_count].bytes, data, size);
    frames[frame_count++].size = size;
}
static void bit(unsigned key, bool pressed) {
    assert(key < NKRO_REPORT_BITS * 8);
    if (pressed)
        nkro_report->bits[key / 8] |= 1u << (key % 8);
    else
        nkro_report->bits[key / 8] &= ~(1u << (key % 8));
}
static void state(void) {
    uart_send_report_nkro(nkro_report);
}
static bool encoded(unsigned key) {
    for (unsigned i = 2; i < 8; ++i)
        if (bytekb_report_buf[i] == key) return true;
    return key < 120 && (uart_bit_report_buf[1 + key / 8] & (1u << (key % 8)));
}
static void six(void) {
    for (unsigned key = 4; key < 10; ++key)
        bit(key, true);
    state();
}
static void dump_frames(void) {
    for (unsigned i = 0; i < frame_count; ++i) {
        const uint8_t *p = frames[i].bytes;
        assert(p[0] == UART_HEAD && p[2] == 1 && frames[i].size == p[3] + 5);
        assert(p[4 + p[3]] == get_checksum((uint8_t *)p + 4, p[3]));
        assert(p[3] == (p[1] == CMD_RPT_BIT_KB ? 16 : 8));
        if (p[1] == CMD_RPT_BIT_KB) assert(p[4] == 0);
        for (unsigned j = 0; j < frames[i].size; ++j)
            printf("%02x", p[j]);
        puts("");
    }
}
int main(int argc, char **argv) {
    assert(argc == 2);
    host_set_driver(&rf_host_driver);
    unsigned test = (unsigned)strtoul(argv[1], NULL, 10);
    switch (test) {
        case 0:
            for (unsigned key = 4; key < 120; ++key)
                bit(key, true);
            nkro_report->mods = 0xff;
            state();
            for (unsigned key = 4; key < 120; ++key)
                assert(encoded(key));
            memset(nkro_report, 0, sizeof(*nkro_report));
            state();
            for (unsigned key = 4; key < 120; ++key)
                assert(!encoded(key));
            assert(bytekb_report_buf[0] == 0);
            dump_frames();
            break;
        case 1: /* Every byte slot accepts a high usage. */
            for (unsigned slot = 0; slot < 6; ++slot) {
                bit(0x87 + slot, true);
                state();
                assert(bytekb_report_buf[2 + slot] == 0x87 + slot);
            }
            for (unsigned slot = 0; slot < 6; ++slot) {
                bit(0x87 + slot, false);
                state();
                assert(!encoded(0x87 + slot));
            }
            dump_frames();
            break;
        case 2: /* Last transmitted bitmap position. */
            six();
            bit(0x77, true);
            state();
            assert(uart_bit_report_buf[15] == 0x80 && encoded(0x77));
            bit(0x77, false);
            state();
            assert(!encoded(0x77));
            dump_frames();
            break;
        case 3: /* Unsupported overflow must not schedule E2. */
        case 10: {
            six();
            frame_count  = 0;
            unsigned key = test == 3 ? 0x78 : 0x87;
            bit(key, true);
            state();
            assert(!encoded(key));
            bit(key, false);
            state();
#if BASELINE_ENCODER
            assert(frame_count == 6 && f_bit_kb_act);
#else
            assert(frame_count == 0 && !f_bit_kb_act);
            assert(sizeof(uart_bit_report_buf) == 16);
#endif
            assert(sizeof(bitkb_report_buf) >= NKRO_REPORT_BITS + 1);
            break;
        }
        case 4: /* Modifiers travel only in E1. */
            six();
            bit(10, true);
            state();
            frame_count       = 0;
            nkro_report->mods = 0x22;
            state();
            nkro_report->mods = 0;
            state();
            assert(frame_count == 6 && uart_bit_report_buf[0] == 0);
            for (unsigned i = 0; i < frame_count; ++i)
                assert(frames[i].bytes[1] == CMD_RPT_BYTE_KB);
            dump_frames();
            break;
        case 5: /* Mixed supported and unsupported changes. */
            six();
            frame_count = 0;
            bit(0x77, true);
            bit(0x78, true);
            bit(0xef, true);
            state();
            assert(encoded(0x77) && !encoded(0x78) && !encoded(0xef));
            bit(0x77, false);
            bit(0x78, false);
            bit(0xef, false);
            state();
            assert(frame_count == 6);
            dump_frames();
            break;
        case 6:
            bit(4, true);
            bit(0xef, true);
            state();
            assert(encoded(4) && encoded(0xef));
            bit(4, false);
            bit(0xef, false);
            state();
            dump_frames();
            break;
        case 7: /* Overflow is not promoted when a byte slot becomes free. */
            six();
            bit(10, true);
            state();
            bit(4, false);
            state();
            assert(bytekb_report_buf[2] == 0 && encoded(10));
            dump_frames();
            break;
        case 8:
            for (unsigned key = 4; key < 12; ++key)
                bit(key, true);
            state();
            assert(bytekb_report_buf[2] == 4 && bytekb_report_buf[7] == 9);
            assert(uart_bit_report_buf[2] == 0x0c);
            dump_frames();
            break;
        case 9:
            keyboard_report->mods     = 0x81;
            keyboard_report->reserved = 0xff;
            for (unsigned i = 0; i < 6; ++i)
                keyboard_report->keys[i] = 0x87 + i;
            uart_send_report_keyboard(keyboard_report);
            assert(keyboard_report->reserved == 0 && bytekb_report_buf[0] == 0x81);
            dump_frames();
            break;
        case 11: {
            six();
            bit(10, true);
            state();
            unsigned before = frame_count;
            state();
            assert(frame_count == before);
            dump_frames();
            break;
        }
        default:
            assert(false);
    }
    return 0;
}
