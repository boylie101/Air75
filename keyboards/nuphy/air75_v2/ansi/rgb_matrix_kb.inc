// Keyboard-specific backlight effects. Keep only the built-in solid-color
// effect enabled alongside these two effects so RM_NEXT cycles exactly three.
RGB_MATRIX_EFFECT(NUPHY_COLOR_CYCLE)
RGB_MATRIX_EFFECT(NUPHY_KEYPRESS_RIPPLE)

#ifdef RGB_MATRIX_CUSTOM_EFFECT_IMPLS

static uint32_t nuphy_cycle_started;
static uint32_t nuphy_ripple_started;
static uint32_t nuphy_ripple_last_keypress;
static uint8_t  nuphy_ripple_active;
static uint8_t  nuphy_ripple_x;
static uint8_t  nuphy_ripple_y;

void nuphy_rgb_note_keypress(uint8_t row, uint8_t col) {
    if (rgb_matrix_get_mode() != RGB_MATRIX_CUSTOM_NUPHY_KEYPRESS_RIPPLE) {
        return;
    }

    uint32_t now = timer_read32();
    const uint32_t spread_ms = 450;
    const uint32_t hold_ms = 2800;
    const uint32_t fade_ms = 1200;
    const uint32_t total_ms = spread_ms + hold_ms + fade_ms;

    // While the keys are still lit, new presses only restart the off timer.
    // The original key remains the ripple origin and the spread is not replayed.
    if (nuphy_ripple_active && timer_elapsed32(nuphy_ripple_last_keypress) < total_ms) {
        nuphy_ripple_last_keypress = now;
        return;
    }

    uint8_t led;
    if (rgb_matrix_map_row_column_to_led(row, col, &led)) {
        nuphy_ripple_x = g_led_config.point[led].x;
        nuphy_ripple_y = g_led_config.point[led].y;
        nuphy_ripple_started = now;
        nuphy_ripple_last_keypress = now;
        nuphy_ripple_active = true;
    }
}

// Shared by the automatic color cycle and RGB_COL_NEXT (Fn + Right).
// Keep these as HSV so solid color and the cycle render the same palette.
hsv_t nuphy_cycle_colors[] = {
    {68, 118, 149}, // Sage green backlight
    {15, 255, 255}, // Orange
    {128, 255, 255}, // Cyan
    {0, 0, 255},     // White
    {0, 255, 255},   // User-adjustable color
};
static uint8_t nuphy_selected_color = 0xff;

void nuphy_cycle_color_next(void) {
    if (nuphy_selected_color == 0xff) {
        hsv_t current = rgb_matrix_get_hsv();
        nuphy_selected_color = (current.s < 10) ? 3 : (current.h < 5 || current.h > 250) ? 3 :
                               (current.h < 35) ? 1 : (current.h > 100 && current.h < 160) ? 2 : 0;
    }
    nuphy_selected_color = (nuphy_selected_color + 1) % ARRAY_SIZE(nuphy_cycle_colors);
    rgb_matrix_mode(RGB_MATRIX_SOLID_COLOR);
    hsv_t color = nuphy_cycle_colors[nuphy_selected_color];
    rgb_matrix_sethsv(color.h, color.s, color.v);
}

void nuphy_adjust_user_color(uint8_t component) {
    hsv_t *color = &nuphy_cycle_colors[4];
    nuphy_selected_color = 4;
    if (component == 0) {
        color->h++;
    } else if (component == 1) {
        color->s++;
    } else {
        color->v++;
    }
    if (nuphy_selected_color == 4 && rgb_matrix_get_mode() == RGB_MATRIX_SOLID_COLOR) {
        rgb_matrix_sethsv_noeeprom(color->h, color->s, color->v);
    }
}

static bool NUPHY_COLOR_CYCLE(effect_params_t *params) {
    RGB_MATRIX_USE_LIMITS(led_min, led_max);
    const uint32_t segment_ms = 30000;
    if (params->init) {
        nuphy_cycle_started = timer_read32();
    }

    uint32_t elapsed = timer_elapsed32(nuphy_cycle_started);
    uint8_t  color_index = (elapsed / segment_ms) % ARRAY_SIZE(nuphy_cycle_colors);
    uint8_t  next_index = (color_index + 1) % ARRAY_SIZE(nuphy_cycle_colors);
    uint8_t  blend = (elapsed % segment_ms) * 255 / segment_ms;
    rgb_t    first = rgb_matrix_hsv_to_rgb(nuphy_cycle_colors[color_index]);
    rgb_t    second = rgb_matrix_hsv_to_rgb(nuphy_cycle_colors[next_index]);
    rgb_t    rgb = {
        .r = (first.r * (255 - blend) + second.r * blend) / 255,
        .g = (first.g * (255 - blend) + second.g * blend) / 255,
        .b = (first.b * (255 - blend) + second.b * blend) / 255,
    };

    for (uint8_t i = led_min; i < led_max; i++) {
        RGB_MATRIX_TEST_LED_FLAGS();
        rgb_matrix_set_color(i, rgb.r, rgb.g, rgb.b);
    }
    return rgb_matrix_check_finished_leds(led_max);
}

static bool NUPHY_KEYPRESS_RIPPLE(effect_params_t *params) {
    RGB_MATRIX_USE_LIMITS(led_min, led_max);
    const uint32_t spread_ms = 450;
    const uint32_t hold_ms = 2800;
    const uint32_t fade_ms = 1200;
    const uint32_t total_ms = spread_ms + hold_ms + fade_ms;

    if (params->init) {
        nuphy_ripple_active = false;
    }

    uint32_t spread_elapsed = nuphy_ripple_active ? timer_elapsed32(nuphy_ripple_started) : total_ms;
    uint32_t idle_elapsed = nuphy_ripple_active ? timer_elapsed32(nuphy_ripple_last_keypress) : total_ms;
    uint16_t wave_radius = spread_elapsed < spread_ms ? (spread_elapsed * 280 / spread_ms) : 280;
    uint16_t brightness = 255;
    if (idle_elapsed >= hold_ms) {
        brightness = idle_elapsed >= hold_ms + fade_ms ? 0 : (hold_ms + fade_ms - idle_elapsed) * 255 / fade_ms;
    }

    rgb_t base = rgb_matrix_hsv_to_rgb(rgb_matrix_config.hsv);
    for (uint8_t i = led_min; i < led_max; i++) {
        RGB_MATRIX_TEST_LED_FLAGS();
        uint16_t distance = abs((int16_t)g_led_config.point[i].x - nuphy_ripple_x) +
                            abs((int16_t)g_led_config.point[i].y - nuphy_ripple_y);
        uint16_t led_brightness = (spread_elapsed < spread_ms && distance > wave_radius) ? 0 : brightness;
        rgb_matrix_set_color(i, base.r * led_brightness / 255,
                             base.g * led_brightness / 255,
                             base.b * led_brightness / 255);
    }
    return rgb_matrix_check_finished_leds(led_max);
}

#endif // RGB_MATRIX_CUSTOM_EFFECT_IMPLS
