#include <nds.h>

int main() {
  lcdSwap();

  // Main screen (touch screen)
  videoSetMode(MODE_FB0);
  vramSetBankA(VRAM_A_LCD);

  /* // Sub screen */
  videoSetModeSub(MODE_0_2D);
  vramSetBankC(VRAM_C_SUB_BG);
  /* oamInit(&oamSub, SpriteMapping_1D_32, true); */

  while(1) {
    scanKeys();

    for(int i = 0; i < 255*192; i++) {
      VRAM_A[i] = RGB15(0, i % 255, 0);
    }

    // Gfx update
    swiWaitForVBlank();
    oamUpdate(&oamMain);
  }
}
