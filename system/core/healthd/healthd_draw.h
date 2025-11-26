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

#ifndef HEALTHD_DRAW_H
#define HEALTHD_DRAW_H

#include <linux/input.h>
#include <minui/minui.h>

#include "animation.h"

using namespace android;

class HealthdDraw {
 public:
  virtual ~HealthdDraw();

  // Redraws screen.
  void redraw_screen(const animation* batt_anim, GRSurface* surf_unknown);

  // According to the index of Direct Rendering Manager,
  // Blanks screen if true, unblanks if false.
  virtual void blank_screen(bool blank, int drm);

  // Rotate screen.
  virtual void rotate_screen(int drm);

  // Detect dual display
  virtual bool has_multiple_connectors();

  static std::unique_ptr<HealthdDraw> Create(animation *anim);

 protected:
  virtual void clear_screen();

  // returns the last y-offset of where the surface ends.
  virtual int draw_surface_centered(GRSurface* surface);
  // Negative x or y coordinates center text.
  virtual int draw_text(const GRFont* font, int x, int y, const char* str);

  // Negative x or y coordinates position the text away from the opposite edge
  // that positive ones do.
  virtual void determine_xy(const animation::text_field& field,
                            const int length, int* x, int* y);

  // Draws battery animation, if it exists.
  virtual void draw_battery(const animation* anim);
  // Draws clock text, if animation contains text_field data.
  virtual void draw_clock(const animation* anim);
  // Draws battery percentage text if animation contains text_field data.
  virtual void draw_percent(const animation* anim);
  // Draws charger->surf_unknown or basic text.
  virtual void draw_unknown(GRSurface* surf_unknown);

/**
   * @brief Desenha a data e hora atuais do sistema.
   *
   * Obtém e formata a data/hora, posicionando-a abaixo da porcentagem de bateria.
   * A cor do texto varia com base no campo de porcentagem.
   *
   * @param anim Estrutura de dados da animação.
   */
  virtual void draw_date(const animation* anim);
  
  /**
   * @brief Desenha o cabeçalho principal no topo da tela (Ex: DevTITANS).
   *
   * Posicionado no topo da tela, usando a cor Verde.
   *
   * @param anim Estrutura de dados da animação.
   */
  virtual void draw_header(const animation* anim);

  /**
   * @brief Desenha o subcabeçalho logo abaixo do cabeçalho principal (Ex: 2025/1).
   *
   * Posicionado logo abaixo do header e usando a cor Vermelha.
   *
   * @param anim Estrutura de dados da animação.
   */
  virtual void draw_subheader(const animation* anim); // NOVO MÉTODO ADICIONADO

  /**
   * @brief Desenha a versão do software no rodapé (bottom) da tela, usando a cor Azul.
   *
   * @param anim Estrutura de dados da animação.
   */
  virtual void draw_version(const animation* anim);

  // Pixel sizes of characters for default font.
  int char_width_;
  int char_height_;

  // Width and height of screen in pixels.
  int screen_width_;
  int screen_height_;

  // Device screen is split vertically.
  const bool kSplitScreen;
  // Pixels to offset graphics towards center split.
  const int kSplitOffset;

  // system text font, may be nullptr
  const GRFont* sys_font;

  // true if minui init'ed OK, false if minui init failed
  bool graphics_available;

 private:
  // Configures font using given animation.
  HealthdDraw(animation* anim);
};

#endif  // HEALTHD_DRAW_H
