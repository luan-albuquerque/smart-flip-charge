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
        LOGW("draw_split_offset = %" PRId64 " overflow for an int; resetting to %d.\n", value,
             std::numeric_limits<int>::min());
        value = std::numeric_limits<int>::min();
    }
    if (value > static_cast<int64_t>(std::numeric_limits<int>::max())) {
        LOGW("draw_split_offset = %" PRId64 " overflow for an int; resetting to %d.\n", value,
             std::numeric_limits<int>::max());
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

void HealthdDraw::redraw_screen(const animation* batt_anim, GRSurface* surf_unknown) {
    if (!graphics_available) return;
    clear_screen();

    /* try to display *something* */
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

// support screen rotation for foldable phone
void HealthdDraw::rotate_screen(int drm) {
    if (!graphics_available) return;
    if (drm == 0)
        gr_rotate(GRRotation::RIGHT /* landscape mode */);
    else
        gr_rotate(GRRotation::NONE /* Portrait mode */);
}

// detect dual display
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

    LOGV("drawing surface %dx%d+%d+%d\n", w, h, x, y);
    gr_blit(surface, 0, 0, w, h, x, y);
    if (kSplitScreen) {
        x += screen_width_ - 2 * kSplitOffset;
        LOGV("drawing surface %dx%d+%d+%d\n", w, h, x, y);
        gr_blit(surface, 0, 0, w, h, x, y);
    }

    return y + h;
}

int HealthdDraw::draw_text(const GRFont* font, int x, int y, const char* str) {
    if (!graphics_available) return 0;
    int str_len_px = gr_measure(font, str);

    if (x < 0) x = (screen_width_ - str_len_px) / 2;
    if (y < 0) y = (screen_height_ - char_height_) / 2;
    gr_text(font, x + kSplitOffset, y, str, false /* bold */);
    if (kSplitScreen) gr_text(font, x - kSplitOffset + screen_width_, y, str, false /* bold */);

    return y + char_height_;
}

void HealthdDraw::determine_xy(const animation::text_field& field, const int length, int* x,
                               int* y) {
    *x = field.pos_x;
    screen_width_ = gr_fb_width() / (kSplitScreen ? 2 : 1);
    screen_height_ = gr_fb_height();

    int str_len_px = length * field.font->char_width;
    if (field.pos_x == CENTER_VAL) {
        *x = (screen_width_ - str_len_px) / 2;
    } else if (field.pos_x >= 0) {
        *x = field.pos_x;
    } else {  // position from max edge
        *x = screen_width_ + field.pos_x - str_len_px - kSplitOffset;
    }

    *y = field.pos_y;

    if (field.pos_y == CENTER_VAL) {
        *y = (screen_height_ - field.font->char_height) / 2;
    } else if (field.pos_y >= 0) {
        *y = field.pos_y;
    } else {  // position from max edge
        *y = screen_height_ + field.pos_y - field.font->char_height;
    }
}

void HealthdDraw::draw_clock(const animation* anim) {
    static constexpr char CLOCK_FORMAT[] = "%H:%M";
    static constexpr int CLOCK_LENGTH = 6;

    const animation::text_field& field = anim->text_clock;

    if (!graphics_available || field.font == nullptr || field.font->char_width == 0 ||
        field.font->char_height == 0)
        return;

    time_t rawtime;
    time(&rawtime);
    tm* time_info = localtime(&rawtime);

    char clock_str[CLOCK_LENGTH];
    size_t length = strftime(clock_str, CLOCK_LENGTH, CLOCK_FORMAT, time_info);
    if (length != CLOCK_LENGTH - 1) {
        LOGE("Could not format time\n");
        return;
    }

    int x, y;
    determine_xy(field, length, &x, &y);

    LOGV("drawing clock %s %d %d\n", clock_str, x, y);
    gr_color(field.color_r, field.color_g, field.color_b, field.color_a);
    draw_text(field.font, x, y, clock_str);
}

/**
 * @brief Desenha a data atual na tela de carregamento.
 *
 * A data é formatada como DD/MM/AAAA e desenhada abaixo do campo de porcentagem.
 *
 * @param anim Estrutura de animação para obter a fonte e a posição.
 */
void HealthdDraw::draw_date(const animation* anim) {
    // Usa o campo de porcentagem como base para o estilo de fonte.
    const animation::text_field& percent_field = anim->text_percent;
    if (percent_field.font == nullptr) return;

    // Obtém a data e hora local do sistema.
    time_t rawtime;
    time(&rawtime);
    tm* time_info = localtime(&rawtime);

    // Define o formato da data.
    static constexpr char DATETIME_FORMAT[] = "%d/%m/%Y";
    LOGE("draw_date: MONTOU A DATA");
    static constexpr int DATETIME_LENGTH = 11;
    char datetime_str[DATETIME_LENGTH];

    // Formata a data.
    size_t length = strftime(datetime_str, DATETIME_LENGTH, DATETIME_FORMAT, time_info);
    if (length < 1) return;

    int x, y;
    // Calcula posição X e Y inicial (geralmente centro da tela).
    determine_xy(percent_field, length, &x, &y);

    // POSICIONAMENTO: Move o texto para baixo, 50px abaixo da altura da fonte.
    y += percent_field.font->char_height + 50;

    // Cor: Branco total (255, 255, 255).
    gr_color(255, 255, 255, 255);

    // Desenha a data/hora formatada.
    draw_text(percent_field.font, x, y, datetime_str);
}

/**
 * @brief Desenha o cabeçalho principal ("DevTITANS") no TOPO da tela.
 *
 * Utiliza a mesma fonte do campo de porcentagem para manter o estilo visual desejado.
 *
 * @param anim Estrutura de animação.
 */
void HealthdDraw::draw_header(const animation* anim) {
    if (!graphics_available) return;

    // Obtém as propriedades (Fonte, Cores, Posição) do campo de porcentagem
    const animation::text_field& percent_field = anim->text_percent;
    if (percent_field.font == nullptr) return;

    std::string header_text = base::StringPrintf("DevTITANS");
    int length = header_text.size();

    int x, y;
    animation::text_field temp_field = percent_field;

    // POSICIONAMENTO PARA O TOPO:
    // Usa a altura da fonte do campo de porcentagem para o cálculo de margem (20px de topo).
    temp_field.pos_y = 20 + percent_field.font->char_height;

    // Calcula posição X e Y final usando o novo pos_y (topo).
    determine_xy(temp_field, length, &x, &y);

    // Define a cor VERDE ESMERALDA (R:0, G:179, B:13, A:255).
    gr_color(0, 179, 13, 255);

    // Desenha o texto usando a FONTE DO PERCENTUAL, conforme solicitado.
    draw_text(percent_field.font, x, y, header_text.c_str());
}

/**
 * @brief Desenha o subcabeçalho ("2025/1") em VERMELHO, logo abaixo do cabeçalho principal.
 *
 * @param anim Estrutura de animação.
 */
void HealthdDraw::draw_subheader(const animation* anim) {
    if (!graphics_available) return;

    const animation::text_field& percent_field = anim->text_percent;
    if (percent_field.font == nullptr) return;

    std::string subheader_text = base::StringPrintf("$ 2025/1 @");
    int length = subheader_text.size();

    int x, y;
    animation::text_field temp_field = percent_field;

    // POSICIONAMENTO: 20 (margem topo) + altura da fonte do cabeçalho + 10 (espaço) + altura da
    // fonte do subheader Usa a altura da fonte do percentual para o cálculo.
    temp_field.pos_y = 20 + (2 * percent_field.font->char_height) + 10;

    // Calcula a posição.
    determine_xy(temp_field, length, &x, &y);

    // Define a cor VERMELHA RUBY (R:185, G:30, B:60, A:255).
    gr_color(185, 30, 60, 255);

    // Desenha o texto usando a FONTE DO PERCENTUAL, conforme solicitado.
    draw_text(percent_field.font, x, y, subheader_text.c_str());
}

/**
 * @brief Desenha a string de versão no RODAPÉ da tela.
 *
 * @param anim Estrutura de animação.
 */
void HealthdDraw::draw_version(const animation* anim) {
    if (!graphics_available) return;

    const animation::text_field& percent_field = anim->text_percent;
    if (percent_field.font == nullptr) return;

    std::string version_str = base::StringPrintf("v2.7.3-final");
    int length = version_str.size();

    int x, y;
    animation::text_field temp_field = percent_field;

    // AJUSTE PARA O RODAPÉ: O valor NEGATIVO ativa o alinhamento inferior.
    // -20px de margem a partir da borda inferior.
    temp_field.pos_y = -20 - percent_field.font->char_height;

    determine_xy(temp_field, length, &x, &y);

    // Define a cor AZUL SAFIRA (R:20, G:90, B:200, A:255).
    gr_color(20, 90, 200, 255);

    // Desenha o texto da versão.
    draw_text(percent_field.font, x, y, version_str.c_str());
}

/**
 * @brief Desenha a porcentagem de bateria na tela de carregamento.
 *
 * Esta função calcula dinamicamente a cor da porcentagem com base no nível
 * atual da bateria (escala de 0% a 100%), usando interpolação linear (gradiente)
 * entre cores predefinidas.
 *
 * @param anim Ponteiro para a estrutura 'animation' contendo o nível de carga e status.
 */
void HealthdDraw::draw_percent(const animation* anim) {
    // Retorna imediatamente se o sistema gráfico não estiver inicializado.
    if (!graphics_available) return;

    // Obtém o nível de carga atual.
    int cur_level = anim->cur_level;

    // Se o status for CARGA COMPLETA, força o nível para 100%.
    if (anim->cur_status == BATTERY_STATUS_FULL) cur_level = 100;

    // Se o nível for inválido (negativo), retorna.
    if (cur_level < 0) return;

    // Obtém o campo de texto configurado para a porcentagem.
    const animation::text_field& field = anim->text_percent;
    // Garante que a fonte esteja carregada.
    if (field.font == nullptr) return;

    // Estrutura para definir os pontos de cor na escala da bateria.
    struct ColorPoint {
        int level;
        int r;
        int g;
        int b;
    };

    // Array estático de cores. O sistema fará interpolação entre esses pontos.
    static const ColorPoint colors[] = {
            {0, 255, 0, 0},     // Vermelho (0%)
            {15, 255, 69, 0},   // Laranja-Avermelhado (15%)
            {30, 255, 140, 0},  // Laranja-Escuro (30%)
            {45, 255, 165, 0},  // Laranja (45%)
            {50, 255, 255, 0},  // Amarelo (50%)
            {65, 178, 255, 0},  // Verde-Limão Amarelado (65%)
            {75, 127, 255, 0},  // Verde-Limão (75%)
            {85, 76, 255, 0},   // Verde-Claro (85%)
            {100, 0, 255, 0}    // Verde (100%)
    };

    // Variáveis para armazenar a cor final (interpolada).
    int r = 0, g = 0, b = 0;

    // Itera pelos pontos de cor para encontrar o intervalo correto.
    for (int i = 0; i < 8; i++) {
        // Verifica se o nível atual está entre o ponto 'i' e o ponto 'i+1'.
        if (cur_level >= colors[i].level && cur_level <= colors[i + 1].level) {
            // Calcula o fator de interpolação (t) (de 0.0 a 1.0) dentro do intervalo.
            float t = float(cur_level - colors[i].level) /
                      float(colors[i + 1].level - colors[i].level);

            // Interpola linearmente a cor Vermelho (R).
            r = colors[i].r + (int)((colors[i + 1].r - colors[i].r) * t);

            // Interpola linearmente a cor Verde (G).
            g = colors[i].g + (int)((colors[i + 1].g - colors[i].g) * t);

            // Interpola linearmente a cor Azul (B).
            b = colors[i].b + (int)((colors[i + 1].b - colors[i].b) * t);

            // Encerra o loop após encontrar e calcular a cor.
            break;
        }
    }

    // Formata o nível atual como uma string com o símbolo de porcentagem (ex: "50%").
    std::string str = base::StringPrintf("%d%%", cur_level);

    // Calcula a posição (X, Y) do texto com base no campo de alinhamento.
    int x, y;
    determine_xy(field, str.size(), &x, &y);

    // Define a cor final interpolada (R, G, B) com opacidade total (A:255).
    gr_color(r, g, b, field.color_a);

    // Desenha o texto da porcentagem na tela.
    draw_text(field.font, x, y, str.c_str());
}

/**
 * @brief Função principal de desenho da bateria. Chama todos os componentes.
 *
 * @param anim Estrutura de animação.
 */
void HealthdDraw::draw_battery(const animation* anim) {
    if (!graphics_available) return;

    // 1. TOPO: Desenha o cabeçalho principal
    draw_header(anim);

    // 2. SUBCABEÇALHO: Desenha o subcabeçalho
    draw_subheader(anim);

    // 3. CENTRO: Desenha a porcentagem e a data (que está logo abaixo da porcentagem)
    draw_percent(anim);
    // draw_date(anim);

    // 4. RODAPÉ: Desenha a versão
    draw_version(anim);
}

void HealthdDraw::draw_unknown(GRSurface* surf_unknown) {
    int y;
    if (surf_unknown) {
        draw_surface_centered(surf_unknown);
    } else if (sys_font) {
        gr_color(0xa4, 0xc6, 0x39, 255);
        y = draw_text(sys_font, -1, -1, "Charging!");
        draw_text(sys_font, -1, y + 25, "?\?/100");
    } else {
        LOGW("Charging, level unknown\n");
    }
}

std::unique_ptr<HealthdDraw> HealthdDraw::Create(animation* anim) {
    if (gr_init() < 0) {
        LOGE("gr_init failed\n");
        return nullptr;
    }
    return std::unique_ptr<HealthdDraw>(new HealthdDraw(anim));
}
