#include <nds.h>
#include <math.h>
#include <stdio.h>

#include "entries.h"
#include "bgsub.h"
#include "sliders.h"

#define HALF_WIDTH 128
#define HALF_HEIGHT 96

#define HUE_RESOLUTION 32
#define SAT_RESOLUTION 1

#define SLIDER_HUE_Y 162
#define SLIDER_SAT_X 224

int max(int x, int y) {
  if (x > y) return x;
  return y;
}

int min(int x, int y) {
  if (x > y) return y;
  return x;
}

void init_sub_bg() {
    int subBg = bgInitSub(3, BgType_Bmp8, BgSize_B8_256x256, 1, 0);
    bgSetPriority(subBg, 3);
    dmaCopy(bgsubBitmap, bgGetGfxPtr(subBg), bgsubBitmapLen);
    dmaCopy(bgsubPal, BG_PALETTE_SUB, bgsubPalLen);
}

u16 *slider_hue_gfx;
u16 *slider_sat_gfx;

void copy_slider_gfx(int ishue, int selected, u16* gfx) {
  u8* src = (u8*)slidersTiles + (2 * ishue + selected) * 16 * 16;
  dmaCopy(src, gfx, 16 * 16);
}

void set_slider_gfx(int entry, int x, int y, u16* gfx) {
  oamSet(&oamSub, entry, x, y, 0, 0,
         SpriteSize_16x16, SpriteColorFormat_256Color, gfx, -1,
         false, false, false, false, false);
}

void init_sliders_gfx() {
  dmaCopy(slidersPal, SPRITE_PALETTE_SUB, slidersPalLen);

  // Hue slider
  slider_hue_gfx = oamAllocateGfx(&oamSub, SpriteSize_16x16, SpriteColorFormat_256Color);
  copy_slider_gfx(1, 0, slider_hue_gfx);

  // Saturation slider
  slider_sat_gfx = oamAllocateGfx(&oamSub, SpriteSize_16x16, SpriteColorFormat_256Color);
  copy_slider_gfx(0, 0, slider_sat_gfx);
}

int get_pixel(int x, int y) {
  return VRAM_A[y * 256 + x];
}

void set_pixel(int x, int y, int c) {
  VRAM_A[y * 256 + x] = c;
}

void clear_screen() {
  dmaCopy(VRAM_B, VRAM_A, 256 * 192 * 2);
}

// 0 <= h < 360
int hsv_to_rgb(int h, float v, float s) {
  float c = v * s;
  float h1 = (float)h / 60;
  while(h1 > 2) { h1 -= 2; }
  float x = c * (1 - fabs(h1 - 1));
  float r1 = 0;
  float g1 = 0;
  float b1 = 0;
  if(h < 60) { r1 = c; g1 = x; }
  else if(h < 120) { r1 = x; g1 = c; }
  else if(h < 180) { g1 = c; b1 = x; }
  else if(h < 240) { g1 = x; b1 = c; }
  else if(h < 300) { r1 = x; b1 = c; }
  else { r1 = c; b1 = x; }
  float m = v - c;
  return RGB15((int)((r1 + m) * 31), (int)((g1 + m) * 31), (int)((b1 + m) * 31));
}

void draw_wheel(int hueres, int satres) {
  for(s16 a = 0; a < DEGREES_IN_CIRCLE; a += (32 * hueres)) {
    if(angleToDegrees(a) < 0) break;

    for(int r = 0; r < 70; r+=satres) {
      int c = hsv_to_rgb(angleToDegrees(a), 1, (float)r/70);

      for(int i = 0; i < hueres; i++) {
        s16 x = cosLerp(a + 32 * i);
        s16 y = sinLerp(a + 32 * i);

        for(int j = 0; j < satres; j++) {
          set_pixel(fixedToInt((r + j) * x, 12) + HALF_WIDTH,
                    fixedToInt((r + j) * y, 12) + HALF_HEIGHT,
                    c);
        }
      }
    }
  }
}

int main() {
  /* lcdSwap(); */

  // Main screen
  videoSetMode(MODE_FB0);
  vramSetBankA(VRAM_A_LCD);

  // Sub screen (touch screen)
  videoSetModeSub(MODE_5_2D);
  vramSetBankC(VRAM_C_SUB_BG);
  vramSetBankD(VRAM_D_SUB_SPRITE);
  init_sub_bg();
  oamInit(&oamSub, SpriteMapping_1D_32, false);
  init_sliders_gfx();

  int slider_hue_x = 120;
  int slider_sat_y = 92;

  draw_wheel(HUE_RESOLUTION, slider_sat_y / 50);

  touchPosition touch;

  while(1) {
    scanKeys();
    int held = keysHeld();

    if(held & KEY_TOUCH) {
      touchRead(&touch);

      if(abs(slider_hue_x + 8 - touch.px) < 16 && abs(SLIDER_HUE_Y + 8 - touch.py) < 16) {
        copy_slider_gfx(1, 1, slider_hue_gfx);
        slider_hue_x = max(40, min(200, touch.px - 8));
      } else {
        copy_slider_gfx(1, 0, slider_hue_gfx);
      }

      if(abs(SLIDER_SAT_X + 8 - touch.px) < 16 && abs(slider_sat_y + 8 - touch.py) < 16) {
        copy_slider_gfx(0, 1, slider_sat_gfx);
        slider_sat_y = max(40, min(140, touch.py - 8));
      } else {
        copy_slider_gfx(0, 0, slider_sat_gfx);
      }

    } else {
      copy_slider_gfx(1, 0, slider_hue_gfx);
      copy_slider_gfx(0, 0, slider_sat_gfx);
    }

    set_slider_gfx(SLIDER_HUE_ENTRY, slider_hue_x, SLIDER_HUE_Y, slider_hue_gfx);
    set_slider_gfx(SLIDER_SAT_ENTRY, SLIDER_SAT_X, slider_sat_y, slider_sat_gfx);

    /* draw_wheel(HUE_RESOLUTION, SAT_RESOLUTION); */

    // Gfx update
    swiWaitForVBlank();
    oamUpdate(&oamSub);
  }
}
