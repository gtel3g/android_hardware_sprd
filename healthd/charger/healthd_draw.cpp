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

#include "healthd_draw.h"

#define LOGE(x...) KLOG_ERROR("charger", x);
#define LOGV(x...) KLOG_DEBUG("charger", x);

HealthdDraw::HealthdDraw(animation* anim)
  : kSplitScreen(HEALTHD_DRAW_SPLIT_SCREEN),
    kSplitOffset(HEALTHD_DRAW_SPLIT_OFFSET) {
  gr_init();
  gr_font_size(gr_sys_font(), &char_width_, &char_height_);

  screen_width_ = gr_fb_width() / (kSplitScreen ? 2 : 1);
  screen_height_ = gr_fb_height();

  int res;
  if (!anim->text_clock.font_file.empty() &&
      (res = gr_init_font(anim->text_clock.font_file.c_str(),
                          &anim->text_clock.font)) < 0) {
    LOGE("Could not load time font (%d)\n", res);
  }
  if (!anim->text_percent.font_file.empty() &&
      (res = gr_init_font(anim->text_percent.font_file.c_str(),
                          &anim->text_percent.font)) < 0) {
    LOGE("Could not load percent font (%d)\n", res);
  }
}

HealthdDraw::~HealthdDraw() {}

void HealthdDraw::redraw_screen(const animation* batt_anim, GRSurface* surf_unknown) {
  clear_screen();

  /* try to display *something* */
  if (batt_anim->cur_level < 0 || batt_anim->num_frames == 0)
    draw_unknown(surf_unknown);
  else
    draw_battery(batt_anim);
  gr_flip();
}

void HealthdDraw::blank_screen(bool blank) { gr_fb_blank(blank); }

void HealthdDraw::clear_screen(void) {
  gr_color(0, 0, 0, 255);
  gr_clear();
}

int HealthdDraw::draw_surface_centered(GRSurface* surface) {
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
  int str_len_px = gr_measure(font, str);

  if (x < 0) x = (screen_width_ - str_len_px) / 2;
  if (y < 0) y = (screen_height_ - char_height_) / 2;
  gr_text(font, x + kSplitOffset, y, str, false /* bold */);
  if (kSplitScreen)
    gr_text(font, x - kSplitOffset + screen_width_, y, str, false /* bold */);

  return y + char_height_;
}

void HealthdDraw::determine_xy(const animation::text_field& field,
                               const int length, int* x, int* y) {
  *x = field.pos_x;

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

  if (field.font == nullptr || field.font->char_width == 0 ||
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

static void draw_segment_digit(int digit, int x, int y,
                               int w, int h, int t) {
  /*
   * Seven-segment layout:
   *
   *      A
   *    F   B
   *      G
   *    E   C
   *      D
   */
  static const unsigned char segments[10] = {
      0x3f, /* 0: A B C D E F */
      0x06, /* 1: B C */
      0x5b, /* 2: A B G E D */
      0x4f, /* 3: A B C D G */
      0x66, /* 4: F G B C */
      0x6d, /* 5: A F G C D */
      0x7d, /* 6: A F E D C G */
      0x07, /* 7: A B C */
      0x7f, /* 8 */
      0x6f, /* 9: A B C D F G */
  };

  if (digit < 0 || digit > 9) {
    return;
  }

  const unsigned char seg = segments[digit];
  const int mid = y + h / 2;

  /* A */
  if (seg & 0x01)
    gr_fill(x + t, y, x + w - t, y + t);

  /* B */
  if (seg & 0x02)
    gr_fill(x + w - t, y + t, x + w, mid);

  /* C */
  if (seg & 0x04)
    gr_fill(x + w - t, mid, x + w, y + h - t);

  /* D */
  if (seg & 0x08)
    gr_fill(x + t, y + h - t, x + w - t, y + h);

  /* E */
  if (seg & 0x10)
    gr_fill(x, mid, x + t, y + h - t);

  /* F */
  if (seg & 0x20)
    gr_fill(x, y + t, x + t, mid);

  /* G */
  if (seg & 0x40)
    gr_fill(x + t, mid - t / 2,
            x + w - t, mid + (t + 1) / 2);
}

static void draw_percent_sign(int x, int y, int w, int h, int t) {
  const int dot = t * 2;

  /* upper-left dot */
  gr_fill(x, y + t,
          x + dot, y + t + dot);

  /* lower-right dot */
  gr_fill(x + w - dot, y + h - t - dot,
          x + w, y + h - t);

  /*
   * Slash, drawn as a short staircase so it scales cleanly
   * without requiring antialiasing or bitmap resources.
   */
  const int steps = 7;

  for (int i = 0; i < steps; ++i) {
    const int sx =
        x + dot / 2 +
        i * (w - dot) / (steps - 1);

    const int sy =
        y + h - dot -
        i * (h - dot * 2) / (steps - 1);

    gr_fill(sx, sy, sx + t, sy + t);
  }
}

void HealthdDraw::draw_percent(const animation* anim) {
  int level = anim->cur_level;

  if (anim->cur_status == BATTERY_STATUS_FULL) {
    level = 100;
  }

  if (level < 0) {
    return;
  }

  if (level > 100) {
    level = 100;
  }

  const int short_side =
      screen_width_ < screen_height_ ? screen_width_ : screen_height_;

  /*
   * Percentage geometry is derived only from framebuffer dimensions.
   * No device-specific coordinates or bitmap fonts are required.
   */
  int digit_w = short_side * 60 / 1000;
  int digit_h = short_side * 105 / 1000;
  int thickness = short_side / 110;

  if (digit_w < 18)
    digit_w = 18;

  if (digit_h < 32)
    digit_h = 32;

  if (thickness < 3)
    thickness = 3;

  std::string number = base::StringPrintf("%d", level);

  const int digits = number.size();

  /*
   * Keep the percentage comfortably inside the battery body.
   *
   * draw_battery() uses 34% of the shortest framebuffer side for
   * battery width. Recreate that geometry here and reserve a safe
   * margin from the outline.
   */
  const int battery_w = short_side * 34 / 100;
  const int safe_margin = thickness * 5;
  const int max_text_w = battery_w - safe_margin * 2;

  int spacing = thickness * 2;
  int percent_w = digit_w * 3 / 4;

  int total_w =
      digits * digit_w +
      (digits - 1) * spacing +
      spacing * 2 +
      percent_w;

  /*
   * 100% is the widest possible string. If it approaches the battery
   * outline, scale the whole percentage geometry down proportionally.
   */
  if (total_w > max_text_w && max_text_w > 0) {
    const int old_total_w = total_w;

    digit_w = digit_w * max_text_w / old_total_w;
    digit_h = digit_h * max_text_w / old_total_w;
    thickness = thickness * max_text_w / old_total_w;

    if (digit_w < 14)
      digit_w = 14;

    if (digit_h < 26)
      digit_h = 26;

    if (thickness < 2)
      thickness = 2;

    spacing = thickness * 2;
    percent_w = digit_w * 3 / 4;

    total_w =
        digits * digit_w +
        (digits - 1) * spacing +
        spacing * 2 +
        percent_w;
  }

  int x =
      screen_width_ / 2 +
      kSplitOffset -
      total_w / 2;

  const int y =
      screen_height_ / 2 -
      digit_h / 2;

  /*
   * White remains readable over both the black empty area and
   * the green battery fill.
   */
  gr_color(255, 255, 255, 255);

  for (size_t i = 0; i < number.size(); ++i) {
    draw_segment_digit(number[i] - '0',
                       x, y,
                       digit_w, digit_h,
                       thickness);

    x += digit_w + spacing;
  }

  x += spacing;

  draw_percent_sign(x, y,
                    percent_w, digit_h,
                    thickness);

  LOGV("SPRD charger: vector percent level=%d size=%dx%d\n",
       level, total_w, digit_h);
}

void HealthdDraw::draw_battery(const animation* anim) {
  int level = anim->cur_level;

  if (anim->cur_status == BATTERY_STATUS_FULL) {
    level = 100;
  }

  if (level < 0) {
    level = 0;
  } else if (level > 100) {
    level = 100;
  }

  /*
   * Scale the charger UI from the shortest framebuffer side.
   * This keeps the same proportions in portrait and landscape modes
   * without device- or resolution-specific coordinates.
   */
  const int short_side =
      screen_width_ < screen_height_ ? screen_width_ : screen_height_;

  int body_w = short_side * 34 / 100;
  int body_h = short_side * 56 / 100;
  int stroke = short_side / 120;

  if (stroke < 3) {
    stroke = 3;
  }

  const int terminal_w = body_w / 3;
  const int terminal_h = stroke * 2;

  const int center_x = screen_width_ / 2 + kSplitOffset;

  const int left = center_x - body_w / 2;
  const int right = left + body_w;
  const int top = (screen_height_ - body_h) / 2;
  const int bottom = top + body_h;

  const int terminal_left = center_x - terminal_w / 2;
  const int terminal_right = center_x + terminal_w / 2;
  const int terminal_top = top - terminal_h;

  /*
   * White battery outline.
   */
  gr_color(255, 255, 255, 255);

  gr_fill(left, top, right, top + stroke);
  gr_fill(left, bottom - stroke, right, bottom);
  gr_fill(left, top, left + stroke, bottom);
  gr_fill(right - stroke, top, right, bottom);

  gr_fill(terminal_left, terminal_top,
          terminal_right, top);

  /*
   * Battery fill. Leave a small gap between the fill and outline.
   */
  const int padding = stroke * 2;

  const int inner_left = left + padding;
  const int inner_right = right - padding;
  const int inner_top = top + padding;
  const int inner_bottom = bottom - padding;

  const int inner_h = inner_bottom - inner_top;
  const int fill_h = inner_h * level / 100;

  if (fill_h > 0) {
    gr_color(0, 210, 100, 255);

    gr_fill(inner_left,
            inner_bottom - fill_h,
            inner_right,
            inner_bottom);
  }

  LOGV("SPRD charger: fb=%dx%d battery=%dx%d level=%d%%\n",
       screen_width_, screen_height_, body_w, body_h, level);

  /*
   * Keep the existing Lineage text handling for this first step.
   * It will be replaced by scalable SPRD text separately.
   */
  draw_clock(anim);
  draw_percent(anim);
}

void HealthdDraw::draw_unknown(GRSurface* surf_unknown) {
  int y;
  if (surf_unknown) {
    draw_surface_centered(surf_unknown);
  } else {
    gr_color(0xa4, 0xc6, 0x39, 255);
    y = draw_text(gr_sys_font(), -1, -1, "Charging!");
    draw_text(gr_sys_font(), -1, y + 25, "?\?/100");
  }
}
