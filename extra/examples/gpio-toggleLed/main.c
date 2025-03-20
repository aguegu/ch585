#include "CH58x_common.h"

int main() {
  HSECFG_Capacitance(HSECap_18p);
  SetSysClock(CLK_SOURCE_HSE_PLL_62_4MHz);

  GPIOB_ModeCfg(GPIO_Pin_2, GPIO_ModeOut_PP_5mA);
  GPIOB_ModeCfg(GPIO_Pin_3, GPIO_ModeOut_PP_5mA);

  GPIOB_InverseBits(GPIO_Pin_2);

  while(1) {
    GPIOB_InverseBits(GPIO_Pin_2);
    GPIOB_InverseBits(GPIO_Pin_3);
    DelayMs(100);
  }
}
