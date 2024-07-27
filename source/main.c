#include <nds.h>
#include <math.h>
#include <stdio.h>

#define HALF_WIDTH 128
#define HALF_HEIGHT 96

void set_pixel(int x, int y, int c) {
  VRAM_A[(y * 256 + x)] = c;
}

void clear_screen() {
  dmaCopy(VRAM_B, VRAM_A, 256 * 192 * 2);
}

// TODO Fixed point ?
/* int32 hf = inttof32(h); */
/*  hf = divf32(hf, inttof32(60)); */
/*  while(hf > inttof32(2)) { hf -= 2; } */
/*  hf -= 1; */
/*  if(hf < inttof32(0)) { hf = - hf; } */

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
  return RGB15((int)((r1 + m) * 16), (int)((g1 + m) * 16), (int)((b1 + m) * 16));
}

void draw_wheel() {
  for(s16 a = 0; a < DEGREES_IN_CIRCLE; a += 32) {
    if(angleToDegrees(a) < 0) continue;

    s16 x = cosLerp(a);
    s16 y = sinLerp(a);
    for(int r = 0; r < 70; r++) {
      set_pixel(fixedToInt(r * x, 12) + HALF_WIDTH,
                fixedToInt(r * y, 12) + HALF_HEIGHT,
                hsv_to_rgb(angleToDegrees(a), 1, (float)r/70));
    }
  }
}

int main() {
  lcdSwap();

  // Main screen (touch screen)
  videoSetMode(MODE_FB0);
  vramSetBankA(VRAM_A_LCD);

  /* // Sub screen */
  videoSetModeSub(MODE_5_2D);
  vramSetBankC(VRAM_C_SUB_BG);
  /* vramSetBankD(VRAM_D_SUB_SPRITE); */
  consoleDemoInit();
  /* oamInit(&oamSub, SpriteMapping_1D_32, true); */

  clear_screen();
  draw_wheel();

  while(1) {
    scanKeys();

    // Gfx update
    swiWaitForVBlank();
    /* oamUpdate(&oamMain); */
  }
}
