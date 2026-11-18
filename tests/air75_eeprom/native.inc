/* Copyright 2026 QMK
 * SPDX-License-Identifier: GPL-2.0-or-later
 * Included after actual production loader, controls and renderer function bodies.
 * EEPROM, RGB setters, time, LED transport and unrelated indicators are simulated.
 */
_Static_assert(sizeof(keyboard_config_t) == EECONFIG_KB_DATA_SIZE, "EEPROM layout must remain unchanged");
static keyboard_config_t defaults;
static uint64_t          digest = 14695981039346656037ULL;

static void hash_bytes(const void *data, size_t size) {
    const uint8_t *bytes = data;
    for (size_t i = 0; i < size; ++i) {
        digest ^= bytes[i];
        digest *= 1099511628211ULL;
    }
}
static void prepare(keyboard_config_t input) {
    stored = input;
    reads = writes = rgb_calls = 0;
    /* Match initial runtime values even after preceding scenarios changed them. */
    side_mode   = 0;
    side_light  = 3;
    side_speed  = 2;
    side_rgb    = 1;
    side_colour = side_play_point = side_play_cnt = 0;
    side_play_timer                               = now;
}
static void render(void) {
    for (unsigned i = 0; i < 3; ++i) {
        now += 100;
        side_led_show();
        hash_bytes(side_leds, sizeof(side_leds));
    }
}
static void check_loaded(keyboard_config_t expected, unsigned expected_writes) {
    assert(reads == 1 && writes == expected_writes && rgb_calls == 0);
    assert(memcmp(&keyboard_config, &expected, sizeof(expected)) == 0);
    assert(memcmp(&stored, &expected, sizeof(expected)) == 0);
    assert(side_mode == expected.ee_side_mode && side_light == expected.ee_side_light);
    assert(side_speed == expected.ee_side_speed && side_rgb == expected.ee_side_rgb && side_colour == expected.ee_side_colour);
    hash_bytes(&keyboard_config, sizeof(keyboard_config));
    render();
}
static uint8_t *field(keyboard_config_t *config, unsigned which) {
    switch (which) {
        case 0:
            return &config->ee_side_mode;
        case 1:
            return &config->ee_side_light;
        case 2:
            return &config->ee_side_speed;
        case 3:
            return &config->ee_side_rgb;
        case 4:
            return &config->ee_side_colour;
        case 5:
            return &config->sleep_enable;
        default:
            abort();
    }
}
int main(int argc, char **argv) {
    assert(argc == 2);
    device_reset_init();
    defaults = keyboard_config; /* Repair defaults must match actual factory-reset policy. */
    if (strcmp(argv[1], "valid") == 0) {
        unsigned cases = 0;
        for (unsigned mode = 0; mode <= SIDE_OFF; ++mode)
            for (unsigned light = 0; light < ARRAY_SIZE(side_light_table); ++light)
                for (unsigned speed = 0; speed < ARRAY_SIZE(side_speed_table[0]); ++speed)
                    for (unsigned rgb = 0; rgb <= 1; ++rgb)
                        for (unsigned colour = 0; colour < ARRAY_SIZE(colour_lib); ++colour)
                            for (unsigned sleep = 0; sleep <= 1; ++sleep) {
                                keyboard_config_t input = {0xAD, mode, light, speed, rgb, colour, sleep, 0xA6};
                                prepare(input);
                                loading_eeprom_data();
                                check_loaded(input, 0);
                                ++cases;
                            }
        printf("PASS %u valid configurations; digest=%016llx\n", cases, (unsigned long long)digest);
    } else if (strcmp(argv[1], "repairs") == 0) {
        const unsigned limits[] = {SIDE_OFF, ARRAY_SIZE(side_light_table) - 1, ARRAY_SIZE(side_speed_table[0]) - 1, 1, ARRAY_SIZE(colour_lib) - 1, 1};
        unsigned       cases    = 0;
        for (unsigned which = 0; which < ARRAY_SIZE(limits); ++which)
            for (unsigned value = 0; value <= UINT8_MAX; ++value) {
                keyboard_config_t input    = {0xAD, SIDE_MIX, 5, 4, 0, 3, 0, 0xA6};
                *field(&input, which)      = value;
                keyboard_config_t expected = input;
                bool              invalid  = value > limits[which];
                if (invalid) *field(&expected, which) = *field(&defaults, which);
                prepare(input);
                loading_eeprom_data();
                check_loaded(expected, invalid);
                /* Simulate another boot from the repaired EEPROM: no repeat write. */
                prepare(stored);
                loading_eeprom_data();
                check_loaded(expected, 0);
                ++cases;
            }
        printf("PASS %u single-field byte values, each followed by a reboot; valid fields and reserved byte preserved\n", cases);
        for (unsigned marker = 0; marker <= UINT8_MAX; ++marker) {
            if (marker == 0xAD) continue;
            prepare((keyboard_config_t){marker, 255, 255, 255, 255, 255, 255, 0xA6});
            loading_eeprom_data();
            keyboard_config_t expected = defaults;
            expected.retain1           = 0xA6;
            assert(reads == 1 && writes == 1 && rgb_calls == 3);
            assert(memcmp(&stored, &expected, sizeof(expected)) == 0);
            render();
        }
        puts("PASS 255 invalid-marker first-boot cases; existing defaults and main RGB initialization preserved");
        prepare((keyboard_config_t){0xAD, 255, 255, 255, 255, 255, 255, 0xA6});
        loading_eeprom_data();
        keyboard_config_t expected = defaults;
        expected.retain1           = 0xA6;
        check_loaded(expected, 1);
        prepare((keyboard_config_t){0xAD, SIDE_STATIC, 4, 2, 0, 3, 0, 0xA6});
        loading_eeprom_data();
        side_light_control(1); /* Normal controls can save brightness 5; keep it valid. */
        assert(stored.ee_side_light == 5);
        prepare(stored);
        loading_eeprom_data();
        expected = stored;
        check_loaded(expected, 0);
        side_speed_control(1);
        side_colour_control(1);
        side_mode_control(1);
        assert(writes == 3);
        puts("PASS mixed corruption and normal controls, including persisted brightness 5");
    } else {
        keyboard_config_t input = {0xAD, SIDE_STATIC, 4, 2, 0, 0, 1, 0xA6};
        if (strcmp(argv[1], "speed") == 0)
            input.ee_side_speed = 255;
        else if (strcmp(argv[1], "colour") == 0)
            input.ee_side_colour = 255;
        else {
            assert(strcmp(argv[1], "brightness") == 0);
            input.ee_side_light = 255;
        }
        prepare(input);
        loading_eeprom_data();
        render(); /* Original source fails here under bounds/ASan instrumentation. */
    }
    return 0;
}
