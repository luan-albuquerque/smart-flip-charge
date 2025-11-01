/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <android-base/stringprintf.h>
#include <batteryservice/BatteryService.h>
#include <cutils/klog.h>
#include <math.h>

#include "healthd_draw.h"

#if !defined(__ANDROID_VNDK__)
#include "charger.sysprop.h"
#endif

#define LOGE(x...) KLOG_ERROR("charger", x);
#define LOGW(x...) KLOG_WARNING("charger", x);
#define LOGV(x...) KLOG_DEBUG("charger", x);

static bool get_split_screen() {
#if !defined(__ANDROID_VNDK__)
    return android::sysprop::ChargerProperties::draw_split_screen().value_or(false);
#else
    return false;
#endif
}

static int get_split_offset() {
#if !defined(__ANDROID_VNDK__)
    int64_t value = android::sysprop::ChargerProperties::draw_split_offset().value_or(0);
#else
    int64_t value = 0;
#endif
    if (value < static_cast<int64_t>(std::numeric_limits<int>::min())) {
        value = std::numeric_limits<int>::min();
    }
    if (value > static_cast<int64_t>(std::numeric_limits<int>::max())) {
        value = std::numeric_limits<int>::max();
    }
    return static_cast<int>(value);
}

HealthdDraw::HealthdDraw(animation* anim)
    : kSplitScreen(get_split_screen()), kSplitOffset(get_split_offset()) {
    graphics_available = true;
    sys_font = gr_sys_font();
    if (sys_font == nullptr) {
        LOGW("No system font, screen fallback text not available\n");
    } else {
        gr_font_size(sys_font, &char_width_, &char_height_);
    }

    screen_width_ = gr_fb_width() / (kSplitScreen ? 2 : 1);
    screen_height_ = gr_fb_height();

    int res;
    if (!anim->text_clock.font_file.empty() &&
        (res = gr_init_font(anim->text_clock.font_file.c_str(), &anim->text_clock.font)) < 0) {
        LOGE("Could not load time font (%d)\n", res);
    }
    if (!anim->text_percent.font_file.empty() &&
        (res = gr_init_font(anim->text_percent.font_file.c_str(), &anim->text_percent.font)) < 0) {
        LOGE("Could not load percent font (%d)\n", res);
    }
}

HealthdDraw::~HealthdDraw() {}

void HealthdDraw::draw_date(const animation* anim) {
    if (!graphics_available) return;

    const animation::text_field& percent_field = anim->text_clock;
    if (percent_field.font == nullptr) return;

    time_t rawtime;
    time(&rawtime);
    tm* time_info = localtime(&rawtime);

    static constexpr char DATETIME_FORMAT[] = "%d/%m/%Y %H:%M";
    static constexpr int DATETIME_LENGTH = 20;
    char datetime_str[DATETIME_LENGTH];

    size_t length = strftime(datetime_str, DATETIME_LENGTH, DATETIME_FORMAT, time_info);
    if (length != DATETIME_LENGTH - 1) return;

    int x, y;
    determine_xy(percent_field, length, &x, &y);

    y += percent_field.font->char_height + 10;

    gr_color(255, 255, 255, 255);
    draw_text(percent_field.font, x, y, datetime_str);
}

void HealthdDraw::draw_version(const animation* /*anim*/) {
    if (!graphics_available || sys_font == nullptr) return;

    const char* version_text = "v1.0.0";

    // Mede o comprimento do texto em pixels
    int str_len_px = gr_measure(sys_font, version_text);

    // Centraliza horizontalmente
    int x = (screen_width_ - str_len_px) / 2;

    // Posiciona 10px acima da borda inferior
    int y = screen_height_ - char_height_ - 10;

    // Define cor azul
    gr_color(0, 123, 255, 255);

    // Desenha o texto com a fonte do sistema (sempre disponível)
    draw_text(sys_font, x, y, version_text);
}

void HealthdDraw::draw_header(const animation* /*anim*/) {
    if (!graphics_available || sys_font == nullptr) return;

    const char* header_text = "DevTitans2025/1";

    // Mede comprimento do texto em pixels
    int str_len_px = gr_measure(sys_font, header_text);

    // Centraliza horizontalmente
    int x = (screen_width_ - str_len_px) / 2;

    // Posição fixa no topo da tela (10px abaixo da borda superior)
    int y = 10;

    // Cor branca
    gr_color(255, 255, 255, 255);

    // Desenha o texto com fonte garantida
    draw_text(sys_font, x, y, header_text);
}

void HealthdDraw::redraw_screen(const animation* batt_anim, GRSurface* surf_unknown) {
    if (!graphics_available) return;
    clear_screen();

    if (batt_anim->cur_status == BATTERY_STATUS_UNKNOWN || batt_anim->cur_level < 0 ||
        batt_anim->num_frames == 0)
        draw_unknown(surf_unknown);
    else
        draw_battery(batt_anim);
    gr_flip();
}

void HealthdDraw::blank_screen(bool blank, int drm) {
    if (!graphics_available) return;
    gr_fb_blank(blank, drm);
}

void HealthdDraw::rotate_screen(int drm) {
    if (!graphics_available) return;
    if (drm == 0)
        gr_rotate(GRRotation::RIGHT);
    else
        gr_rotate(GRRotation::NONE);
}

bool HealthdDraw::has_multiple_connectors() {
    return graphics_available && gr_has_multiple_connectors();
}

void HealthdDraw::clear_screen(void) {
    if (!graphics_available) return;
    gr_color(0, 0, 0, 255);
    gr_clear();
}

int HealthdDraw::draw_surface_centered(GRSurface* surface) {
    if (!graphics_available) return 0;

    int w = gr_get_width(surface);
    int h = gr_get_height(surface);
    int x = (screen_width_ - w) / 2 + kSplitOffset;
    int y = (screen_height_ - h) / 2;

    gr_blit(surface, 0, 0, w, h, x, y);
    if (kSplitScreen) {
        x += screen_width_ - 2 * kSplitOffset;
        gr_blit(surface, 0, 0, w, h, x, y);
    }

    return y + h;
}

int HealthdDraw::draw_text(const GRFont* font, int x, int y, const char* str) {
    if (!graphics_available) return 0;
    int str_len_px = gr_measure(font, str);

    if (x < 0) x = (screen_width_ - str_len_px) / 2;
    if (y < 0) y = (screen_height_ - char_height_) / 2;
    gr_text(font, x + kSplitOffset, y, str, false);
    if (kSplitScreen) gr_text(font, x - kSplitOffset + screen_width_, y, str, false);

    return y + char_height_;
}

void HealthdDraw::determine_xy(const animation::text_field& field,
                               const int length, int* x, int* y) {
    *x = field.pos_x;
    screen_width_ = gr_fb_width() / (kSplitScreen ? 2 : 1);
    screen_height_ = gr_fb_height();

    int str_len_px = length * field.font->char_width;
    if (field.pos_x == CENTER_VAL) {
        *x = (screen_width_ - str_len_px) / 2;
    } else if (field.pos_x >= 0) {
        *x = field.pos_x;
    } else {
        *x = screen_width_ + field.pos_x - str_len_px - kSplitOffset;
    }

    *y = field.pos_y;
    if (field.pos_y == CENTER_VAL) {
        *y = (screen_height_ - field.font->char_height) / 2;
    } else if (field.pos_y >= 0) {
        *y = field.pos_y;
    } else {
        *y = screen_height_ + field.pos_y - field.font->char_height;
    }
}

void HealthdDraw::draw_clock(const animation* anim) {
    static constexpr char CLOCK_FORMAT[] = "%H:%M";
    static constexpr int CLOCK_LENGTH = 6;

    const animation::text_field& field = anim->text_clock;
    if (!graphics_available || field.font == nullptr) return;

    time_t rawtime;
    time(&rawtime);
    tm* time_info = localtime(&rawtime);

    char clock_str[CLOCK_LENGTH];
    size_t length = strftime(clock_str, CLOCK_LENGTH, CLOCK_FORMAT, time_info);
    if (length != CLOCK_LENGTH - 1) return;

    int x, y;
    determine_xy(field, length, &x, &y);

    gr_color(field.color_r, field.color_g, field.color_b, field.color_a);
    draw_text(field.font, x, y, clock_str);
}

void HealthdDraw::draw_percent(const animation* anim) {
    if (!graphics_available) return;
    int cur_level = anim->cur_level;
    if (anim->cur_status == BATTERY_STATUS_FULL) cur_level = 100;
    if (cur_level < 0) return;

    const animation::text_field& field = anim->text_percent;
    if (field.font == nullptr) return;

    struct ColorPoint { int level; int r; int g; int b; };
    static const ColorPoint colors[] = {
        {0,   255,   0,   0},      // Red
        {15,  255,  69,   0},      // OrangeRed
        {30,  255, 140,   0},      // DarkOrange
        {45,  255, 165,   0},      // Orange
        {50,  255, 255,   0},      // Yellow
        {65,  255, 215,   0},      // Gold
        {75,  255, 255, 224},      // LightYellow
        {85,   60, 179, 113},      // MediumSeaGreen
        {100,   0, 255,   0}       // Green
    };

    int r = 0, g = 0, b = 0;
    for (int i = 0; i < 8; i++) {
        if (cur_level >= colors[i].level && cur_level <= colors[i+1].level) {
            float t = float(cur_level - colors[i].level) / float(colors[i+1].level - colors[i].level);
            r = colors[i].r + (int)((colors[i+1].r - colors[i].r) * t);
            g = colors[i].g + (int)((colors[i+1].g - colors[i].g) * t);
            b = colors[i].b + (int)((colors[i+1].b - colors[i].b) * t);
            break;
        }
    }

    std::string str = base::StringPrintf("%d%%", cur_level);
    int x, y;
    determine_xy(field, str.size(), &x, &y);

    gr_color(r, g, b, field.color_a);
    draw_text(field.font, x, y, str.c_str());
}

void HealthdDraw::draw_battery(const animation* anim) {
    
    if (!graphics_available) return;

    draw_battery_bar(anim->cur_level);
    draw_header(anim);
    draw_date(anim);
    draw_percent(anim);
    draw_version(anim);
}

/**
 * Draws a simple horizontal progress bar representing battery charge.
 */
void HealthdDraw::draw_battery_bar(int level) {
    if (!graphics_available) return;

    int bar_width = screen_width_ * 0.6;
    int bar_height = 30;
    int x = (screen_width_ - bar_width) / 2 + kSplitOffset;
    int y = (screen_height_ / 2) + (screen_height_ / 6);

    int filled_width = (bar_width * level) / 100;

    int r = 0, g = 0, b = 0;
    if (level <= 30) {
        r = 255; g = (255 * level) / 30; b = 0;
    } else if (level < 75) {
        g = 255; b = 0;
        float factor = (level - 30) / 45.0f;
        r = (int)(255 * (1.0f - factor));
    } else {
        r = 0; g = 255; b = 0;
    }

    gr_color(50, 50, 50, 255);
    gr_fill(x, y, bar_width, bar_height);

    gr_color(r, g, b, 255);
    gr_fill(x, y, filled_width, bar_height);
}

void HealthdDraw::draw_unknown(GRSurface* surf_unknown) {
    if (surf_unknown) {
        draw_surface_centered(surf_unknown);
    } else if (sys_font) {
        gr_color(0xa4, 0xc6, 0x39, 255);
        int y = draw_text(sys_font, -1, -1, "Charging!");
        draw_text(sys_font, -1, y + 25, "?\?/100");
    }
}

std::unique_ptr<HealthdDraw> HealthdDraw::Create(animation* anim) {
    if (gr_init() < 0) {
        LOGE("gr_init failed\n");
        return nullptr;
    }
    return std::unique_ptr<HealthdDraw>(new HealthdDraw(anim));
}
