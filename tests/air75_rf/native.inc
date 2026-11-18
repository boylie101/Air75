/* Copyright 2026 QMK
 * SPDX-License-Identifier: GPL-2.0-or-later
 * Included after production function bodies by air75_rf_audit.py.
 * Hardware, clocks, USB state and connection state are simulated.
 */
bool                 debug_keyboard;
DEV_INFO_STRUCT      dev_info = {.link_mode = LINK_RF_24, .rf_state = RF_CONNECT};
bool                 f_uart_ack, f_rf_read_data_ok, f_rf_sts_sysc_ok, f_rf_new_adv_ok;
bool                 f_rf_reset, f_rf_hand_ok, f_goto_sleep, f_send_channel, f_dial_sw_init_ok = true;
bool                 f_wakeup_prepare;
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
static void down(uint8_t key) {
    add_key_to_report(key);
    send_keyboard_report();
}
static void up(uint8_t key) {
    del_key_from_report(key);
    send_keyboard_report();
}
static void ordinary_keys(void) {
    for (uint8_t key = 4; key <= 10; ++key)
        down(key);
}
static void poll(uint16_t inactivity, uint32_t elapsed) {
    no_act_time = inactivity;
    now += elapsed;
    uart_send_report_func();
}
static void warm(void) {
    for (unsigned i = 0; i < 3; ++i)
        poll(0, 51);
    frame_count = 0;
}
static void status_frame(uint8_t link, uint8_t state_value, bool valid) {
    uint8_t packet[] = {UART_HEAD, CMD_RF_STS_SYSC, 0, 5, link, state_value, 0, 0, 80, 0};
    for (unsigned i = 4; i < 9; ++i)
        packet[9] += packet[i];
    if (!valid) packet[9] ^= 0xff;
    memcpy(Usart_Mgr.RXDBuf, packet, sizeof(packet));
    Usart_Mgr.RXDLen   = sizeof(packet);
    Usart_Mgr.RXDState = RX_Done;
    RF_Protocol_Receive();
}
static void status(uint8_t state_value) {
    status_frame(dev_info.link_mode, state_value, true);
}
static void receive_raw(const uint8_t *packet, uint8_t size) {
    memcpy(Usart_Mgr.RXDBuf, packet, size);
    Usart_Mgr.RXDLen      = size;
    Usart_Mgr.RXDState    = RX_Done;
    Usart_Mgr.RXDOverTime = 9;
    f_uart_ack            = false;
    sync_lost             = 7;
    RF_Protocol_Receive();
    assert(Usart_Mgr.RXDLen == 0 && Usart_Mgr.RXDState == RX_Idle && Usart_Mgr.RXDOverTime == 0);
}
static void receive_packet(uint8_t command, const uint8_t *payload, uint8_t size, bool ack_only) {
    uint8_t packet[UART_MAX_LEN] = {UART_HEAD, command, ack_only ? 0xa0 : 0, size};
    assert(size <= UART_MAX_LEN - 5);
    if (size) memcpy(packet + 4, payload, size);
    for (uint8_t i = 0; i < size; ++i)
        packet[4 + size] += packet[4 + i];
    receive_raw(packet, ack_only ? 3 : size + 5);
}
static unsigned reports(uint8_t command) {
    unsigned count = 0;
    for (unsigned i = 0; i < frame_count; ++i)
        if (frames[i].bytes[1] == command) ++count;
    return count;
}
static void snapshot(void) {
    assert(reports(CMD_RPT_BYTE_KB) == 3 && reports(CMD_RPT_BIT_KB) == 3);
    for (unsigned i = 0; i < frame_count; ++i) {
        uint8_t *p = frames[i].bytes;
        if (p[1] == CMD_RPT_BYTE_KB) assert(p[3] == 8 && memcmp(p + 4, bytekb_report_buf, 8) == 0);
        if (p[1] == CMD_RPT_BIT_KB) assert(p[3] == 16 && p[4] == 0 && memcmp(p + 4, uart_bit_report_buf, 16) == 0);
    }
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
        case 20:
        case 21:
        case 22:
        case 23:
        case 24:
        case 25:
        case 26:
        case 27: {
            unsigned mode      = test - 20;
            bool     radio     = mode & 4;
            protocol           = (mode & 2) ? USB_PROTOCOL_REPORT : USB_PROTOCOL_BOOT;
            keymap_config.nkro = mode & 1;
            host_set_driver(radio ? &rf_host_driver : &usb_driver);
            bool capability = (radio && !NO_DRIVER_OVERRIDE) || protocol == USB_PROTOCOL_REPORT;
            assert(host_can_send_nkro() == capability);
            bool nkro = capability && keymap_config.nkro;
            ordinary_keys();
            up(10);
            down(10);
            add_mods(0x22);
            send_keyboard_report();
            assert((nkro ? nkro_report->mods : keyboard_report->mods) == 0x22);
            assert((nkro_report->bits[1] & 4) == (nkro ? 4 : 0));
            if (radio) {
                assert(encoded(10) == nkro);
                assert(bytekb_report_buf[0] == 0x22 && uart_bit_report_buf[0] == 0);
                assert(usb_byte_count == 0 && usb_bit_count == 0);
            } else {
                assert(frame_count == 0);
                assert(nkro ? usb_bit_count > 0 && usb_byte_count == 0 : usb_byte_count > 0 && usb_bit_count == 0);
            }
            clear_keyboard();
            assert((nkro ? nkro_report->mods : keyboard_report->mods) == 0);
            if (radio)
                for (unsigned i = 0; i < sizeof(uart_bit_report_buf); ++i)
                    assert(uart_bit_report_buf[i] == 0);
            break;
        }
        case 28:
            ordinary_keys();
            assert(encoded(10));
            switch_dev_link(LINK_USB);
            assert(host_get_driver() == &usb_driver && !encoded(10));
            protocol = USB_PROTOCOL_BOOT;
            assert(!host_can_send_nkro());
            ordinary_keys();
            switch_dev_link(LINK_RF_24);
            dev_info.rf_state = RF_CONNECT;
            assert(host_get_driver() == &rf_host_driver && host_can_send_nkro());
            ordinary_keys();
            assert(encoded(10));
            clear_keyboard();
            assert(!encoded(10));
            break;
        case 29:
        case 30:
            test_active_host(CONNECTION_HOST_BLUETOOTH);
            bt_nkro  = test == 30;
            protocol = USB_PROTOCOL_BOOT;
            assert(host_can_send_nkro() == bt_nkro);
            protocol = USB_PROTOCOL_REPORT;
            assert(host_can_send_nkro() == bt_nkro);
            break;
        case 31:
            test_active_host(CONNECTION_HOST_NONE);
            assert(!host_can_send_nkro());
            ordinary_keys();
            assert(frame_count == 0);
            break;
        case 32:
            test_active_host(CONNECTION_HOST_USB);
            protocol = USB_PROTOCOL_BOOT;
            assert(host_can_send_nkro());
            host_set_driver(&usb_driver);
            assert(!host_can_send_nkro());
            break;
        case 40: /* Exact conditional sequence, with actual QMK report/clear paths. */
            ordinary_keys();
            assert(encoded(10));
            protocol = USB_PROTOCOL_BOOT;
            up(10);
            down(10);
            protocol = USB_PROTOCOL_REPORT;
            break_all_key();
            assert((uart_bit_report_buf[2] & 4) == (NO_DRIVER_OVERRIDE ? 4 : 0));
            frame_count = 0;
            now         = 51;
            uart_send_report_func();
            for (unsigned i = 0; i < frame_count; ++i) {
                if (frames[i].bytes[1] == CMD_RPT_BIT_KB) assert((frames[i].bytes[6] & 4) == (NO_DRIVER_OVERRIDE ? 4 : 0));
            }
            break;
        case 41: /* Ordinary transport switching remains a clear, not a held-state replay. */
            ordinary_keys();
            add_mods(0x22);
            send_keyboard_report();
            switch_dev_link(LINK_USB);
            switch_dev_link(LINK_RF_24);
            dev_info.rf_state = RF_CONNECT;
            now               = 51;
            uart_send_report_func();
            for (unsigned i = 0; i < sizeof(uart_bit_report_buf); ++i)
                assert(uart_bit_report_buf[i] == 0);
            for (unsigned i = 0; i < sizeof(bytekb_report_buf); ++i)
                assert(bytekb_report_buf[i] == 0);
            break;
        case 42: /* Supported NKRO toggle sequence clears before changing mode. */
            ordinary_keys();
            clear_keyboard();
            keymap_config.nkro = false;
            ordinary_keys();
            assert(!encoded(10));
            clear_keyboard();
            keymap_config.nkro = true;
            ordinary_keys();
            assert(encoded(10));
            clear_keyboard();
            assert(!encoded(10));
            break;
        case 60: /* Long-held modifiers and overflow outlive the recent-activity window. */
            ordinary_keys();
            add_mods(0x22);
            send_keyboard_report();
            warm();
            poll(4001, 51);
            snapshot();
            assert(encoded(10) && bytekb_report_buf[0] == 0x22);
            break;
        case 61: /* All releases occur offline; reconnect after the refresh window. */
            ordinary_keys();
            warm();
            status(RF_DISCONNECT);
            poll(4001, 1);
            clear_keyboard();
            poll(4001, 30000);
            frame_count = 0;
            status(RF_CONNECT);
            poll(4002, 1);
            snapshot();
            assert(!encoded(10));
            for (unsigned i = 0; i < 8; ++i)
                assert(bytekb_report_buf[i] == 0);
            break;
        case 62:
            ordinary_keys();
            warm();
            status(RF_DISCONNECT);
            poll(4001, 51);
            status(RF_CONNECT);
            frame_count = 0;
            poll(5000, 1);
            snapshot();
            assert(encoded(10));
            break;
        case 63: /* An idle all-up keyboard still explicitly clears a recovered module. */
            status(RF_DISCONNECT);
            poll(5000, 51);
            status(RF_CONNECT);
            poll(5000, 1);
            snapshot();
            for (unsigned i = 0; i < sizeof(uart_bit_report_buf); ++i)
                assert(uart_bit_report_buf[i] == 0);
            break;
        case 64: /* Disconnect and reconnect status can arrive between housekeeping polls. */
            ordinary_keys();
            warm();
            status(RF_DISCONNECT);
            status(RF_CONNECT);
            poll(4001, 1);
            snapshot();
            break;
        case 65: /* Local module reset must wait for a fresh matching status payload. */
            status(RF_CONNECT);
            ordinary_keys();
            warm();
            f_rf_reset = true;
            now += 200;
            dev_sts_sync();
            frame_count = 0;
            poll(4001, 51);
            assert(frame_count == 0);
            Usart_Mgr.RXDBuf[0] = UART_HEAD;
            Usart_Mgr.RXDBuf[1] = CMD_RF_STS_SYSC;
            Usart_Mgr.RXDBuf[2] = 0xa0;
            Usart_Mgr.RXDLen    = 3;
            Usart_Mgr.RXDState  = RX_Done;
            RF_Protocol_Receive();
            poll(4001, 51);
            assert(frame_count == 0);
            status(RF_CONNECT);
            poll(4001, 1);
            snapshot();
            assert(encoded(10));
            break;
        case 66: /* Sender emits held-state snapshots; downstream forwarding is not modelled. */
            ordinary_keys();
            warm();
            poll(4001, 51);
            snapshot();
            frame_count = 0;
            poll(5001, 51);
            snapshot();
            break;
        case 67: /* Idle recovery is bounded; it does not keep all-up traffic alive forever. */
            for (unsigned i = 0; i < 3; ++i)
                poll(4001, 51);
            assert(reports(CMD_RPT_BYTE_KB) == 9 && reports(CMD_RPT_BIT_KB) == 9);
            frame_count = 0;
            poll(4001, 51);
            assert(frame_count == 0);
            break;
        case 68: /* Preserve sleep; resume can replay current state. */
            ordinary_keys();
            warm();
            f_goto_sleep = true;
            poll(0, 51);
            assert(frame_count == 0);
            f_goto_sleep     = false;
            f_wakeup_prepare = true;
            poll(0, 51);
            assert(frame_count == 0);
            f_wakeup_prepare = false;
            poll(4001, 1);
            snapshot();
            break;
        case 69:
            status(RF_DISCONNECT);
            ordinary_keys();
            frame_count = 0;
            poll(0, 51);
            assert(frame_count == 0);
            status(RF_CONNECT);
            poll(0, 1);
            snapshot();
            assert(encoded(10));
            break;
        case 70:
            keymap_config.nkro = false;
            down(4);
            add_mods(0x81);
            send_keyboard_report();
            warm();
            status(RF_DISCONNECT);
            poll(4001, 51);
            up(4);
            clear_mods();
            send_keyboard_report();
            status(RF_CONNECT);
            frame_count = 0;
            poll(5001, 1);
            snapshot();
            for (unsigned i = 0; i < 8; ++i)
                assert(bytekb_report_buf[i] == 0);
            break;
        case 71:
            keymap_config.nkro = false;
            down(4);
            warm();
            poll(4001, 51);
            assert(reports(CMD_RPT_BYTE_KB) == 3 && bytekb_report_buf[2] == 4);
            break;
        case 72: {
            ordinary_keys();
            warm();
            status(RF_DISCONNECT);
            poll(4001, 1);
            up(4);
            up(5);
            up(10);
            down(11);
            add_mods(0x22);
            send_keyboard_report();
            uint8_t expected[8];
            memcpy(expected, bytekb_report_buf, 8);
            status(RF_CONNECT);
            frame_count = 0;
            poll(5000, 1);
            snapshot();
            assert(memcmp(expected, bytekb_report_buf, 8) == 0 && !encoded(4) && !encoded(5) && !encoded(10) && encoded(11));
            break;
        }
        case 73: /* Simulate dropping every immediate all-up copy; refresh sends the release. */
            ordinary_keys();
            warm();
            clear_keyboard();
            frame_count = 0;
            poll(0, 51);
            snapshot();
            assert(!encoded(10));
            break;
        case 74:
            ordinary_keys();
            warm();
            switch_dev_link(LINK_BT_1);
            status(RF_CONNECT);
            frame_count = 0;
            poll(4001, 1);
            snapshot();
            assert(!encoded(10));
            for (unsigned i = 0; i < 8; ++i)
                assert(bytekb_report_buf[i] == 0);
            break;
        case 75:
            ordinary_keys();
            warm();
            f_rf_reset = true;
            now += 200;
            dev_sts_sync();
            frame_count = 0;
            status_frame(dev_info.link_mode, RF_CONNECT, false);
            poll(4001, 51);
            assert(frame_count == 0);
            status_frame(LINK_BT_2, RF_CONNECT, true);
            poll(4001, 51);
            assert(frame_count == 0);
            status(RF_CONNECT);
            poll(4001, 1);
            snapshot();
            break;
        case 80: { /* A status ACK cannot consume stale fields or count link mismatches. */
            DEV_INFO_STRUCT expected = dev_info;
            dev_info.rf_state        = RF_DISCONNECT;
            dev_info.rf_battery      = 42;
            expected                 = dev_info;
            Usart_Mgr.RXDBuf[4]      = dev_info.link_mode;
            Usart_Mgr.RXDBuf[5]      = RF_CONNECT;
            Usart_Mgr.RXDBuf[6]      = 2;
            Usart_Mgr.RXDBuf[7]      = 1;
            Usart_Mgr.RXDBuf[8]      = 97;
            receive_packet(CMD_RF_STS_SYSC, NULL, 0, true);
            assert(f_uart_ack && sync_lost == 0 && !f_rf_sts_sysc_ok);
            assert(memcmp(&dev_info, &expected, sizeof(expected)) == 0);
            Usart_Mgr.RXDBuf[4] = LINK_BT_2;
            for (unsigned i = 0; i < 8; ++i)
                receive_packet(CMD_RF_STS_SYSC, NULL, 0, true);
            assert(!f_send_channel && !f_rf_sts_sysc_ok);
            break;
        }
        case 81: { /* A configuration ACK cannot copy bytes or select a stale profile. */
            memset(func_tab, 0x5c, sizeof(func_tab));
            Usart_Mgr.RXDBuf[8]      = LINK_BT_2;
            Usart_Mgr.RXDBuf[9]      = LINK_BT_3;
            Usart_Mgr.RXDBuf[10]     = LINK_BT_1;
            DEV_INFO_STRUCT expected = dev_info;
            receive_packet(CMD_READ_DATA, NULL, 0, true);
            assert(f_uart_ack && sync_lost == 0 && !f_rf_read_data_ok);
            assert(memcmp(&dev_info, &expected, sizeof(expected)) == 0);
            for (unsigned i = 0; i < sizeof(func_tab); ++i)
                assert(func_tab[i] == 0x5c);
            break;
        }
        case 82:   /* Preserve payload-free handshake, pairing and suspend ACKs. */
        case 83: { /* Preserve the same notifications in validated data frames. */
            const uint8_t commands[] = {CMD_HAND, CMD_NEW_ADV, CMD_24G_SUSPEND, 0x99};
            for (unsigned i = 0; i < sizeof(commands); ++i) {
                f_rf_hand_ok = f_rf_new_adv_ok = f_goto_sleep = false;
                receive_packet(commands[i], NULL, 0, test == 82);
                assert(f_uart_ack && sync_lost == 0);
                assert(f_rf_hand_ok == (commands[i] == CMD_HAND));
                assert(f_rf_new_adv_ok == (commands[i] == CMD_NEW_ADV));
                assert(f_goto_sleep == (commands[i] == CMD_24G_SUSPEND));
                assert(!f_rf_sts_sysc_ok && !f_rf_read_data_ok);
            }
            break;
        }
        case 84: { /* Valid status and configuration payloads still update their state. */
            uint8_t payload[FUNC_VALID_LEN] = {LINK_RF_24, RF_CONNECT, 2, 0, 83};
            receive_packet(CMD_RF_STS_SYSC, payload, 5, false);
            assert(f_uart_ack && sync_lost == 0 && f_rf_sts_sysc_ok);
            assert(dev_info.rf_state == RF_CONNECT && dev_info.rf_led == 2 && dev_info.rf_battery == 83);
            payload[4] = LINK_BT_2;
            payload[5] = LINK_BT_3;
            payload[6] = LINK_BT_1;
            receive_packet(CMD_READ_DATA, payload, sizeof(payload), false);
            assert(f_uart_ack && sync_lost == 0 && f_rf_read_data_ok);
            assert(dev_info.link_mode == LINK_BT_2 && dev_info.rf_channel == LINK_BT_3 && dev_info.ble_channel == LINK_BT_1);
            assert(memcmp(func_tab, payload, sizeof(payload)) == 0);
            break;
        }
        case 85: { /* Invalid ACK markers and incomplete prefixes remain rejected. */
            uint8_t packet[] = {UART_HEAD, CMD_HAND, 0, 0};
            receive_raw(packet, 3);
            assert(!f_uart_ack && sync_lost == 7 && !f_rf_hand_ok);
            packet[2] = 0xa0;
            receive_raw(packet, 2);
            assert(!f_uart_ack && sync_lost == 7 && !f_rf_hand_ok);
            receive_raw(packet, 4);
            assert(!f_uart_ack && sync_lost == 7 && !f_rf_hand_ok);
            packet[0] = 0;
            receive_raw(packet, 3);
            assert(!f_uart_ack && sync_lost == 7 && !f_rf_hand_ok);
            break;
        }
        case 86: { /* ACK handling does not weaken payload length or checksum validation. */
            uint8_t         payload[FUNC_VALID_LEN] = {0};
            DEV_INFO_STRUCT expected                = dev_info;
            receive_packet(CMD_RF_STS_SYSC, payload, 4, false);
            assert(!f_uart_ack && sync_lost == 7 && !f_rf_sts_sysc_ok);
            receive_packet(CMD_READ_DATA, payload, FUNC_VALID_LEN - 1, false);
            assert(!f_uart_ack && sync_lost == 7 && !f_rf_read_data_ok);
            status_frame(dev_info.link_mode, RF_DISCONNECT, false);
            assert(!f_uart_ack && sync_lost == 7 && !f_rf_sts_sysc_ok);
            assert(memcmp(&dev_info, &expected, sizeof(expected)) == 0);
            break;
        }
        case 87: { /* An ACK cannot finish a data request; the following payload can. */
            uint8_t payload[FUNC_VALID_LEN] = {LINK_RF_24, RF_CONNECT, 0, 0, 83};
            receive_packet(CMD_RF_STS_SYSC, NULL, 0, true);
            assert(!f_rf_sts_sysc_ok);
            receive_packet(CMD_RF_STS_SYSC, payload, 5, false);
            assert(f_rf_sts_sysc_ok && dev_info.rf_battery == 83);
            receive_packet(CMD_READ_DATA, NULL, 0, true);
            assert(!f_rf_read_data_ok);
            payload[4] = LINK_RF_24;
            receive_packet(CMD_READ_DATA, payload, sizeof(payload), false);
            assert(f_rf_read_data_ok && dev_info.link_mode == LINK_RF_24);
            break;
        }
        case 88: /* Reset recovery requires fresh status data, not a stale status ACK. */
            ordinary_keys();
            warm();
            f_rf_reset = true;
            now += 200;
            dev_sts_sync();
            dev_info.rf_state   = RF_DISCONNECT;
            Usart_Mgr.RXDBuf[4] = LINK_RF_24;
            Usart_Mgr.RXDBuf[5] = RF_CONNECT;
            frame_count         = 0;
            receive_packet(CMD_RF_STS_SYSC, NULL, 0, true);
            assert(!f_rf_sts_sysc_ok && dev_info.rf_state == RF_DISCONNECT);
            poll(4001, 51);
            assert(frame_count == 0);
            status(RF_CONNECT);
            poll(4001, 1);
            snapshot();
            break;
        default:
            assert(false);
    }
    return 0;
}
