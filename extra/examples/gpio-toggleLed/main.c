#include "CH58x_common.h"

int main() {
  HSECFG_Capacitance(HSECap_18p);
  SetSysClock(CLK_SOURCE_HSE_PLL_62_4MHz);

  GPIOB_SetBits(GPIO_Pin_9);
  GPIOB_ModeCfg(GPIO_Pin_17, GPIO_ModeOut_PP_5mA);

  while(1) {
    GPIOB_InverseBits(GPIO_Pin_17);
    DelayMs(1000);
  }
}
