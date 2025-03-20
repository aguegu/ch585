#include "HAL.h"

volatile uint32_t RTCTigFlag;

void RTC_SetTignTime(uint32_t time) {
  sys_safe_access_enable();
  R32_RTC_TRIG = time;
  sys_safe_access_disable();
  RTCTigFlag = 0;
}

__INTERRUPT
__HIGH_CODE
void RTC_IRQHandler(void) {
  R8_RTC_FLAG_CTRL = (RB_RTC_TMR_CLR | RB_RTC_TRIG_CLR);
  RTCTigFlag = 1;
}

__HIGH_CODE
static uint32_t SYS_GetClockValue(void) {
  volatile uint32_t i;

  do {
    i = R32_RTC_CNT_32K;
  } while(i != R32_RTC_CNT_32K);

  return (i);
}

__HIGH_CODE
static void SYS_SetPendingIRQ(void) {
  PFIC_SetPendingIRQ( RTC_IRQn );
}

void HAL_TimeInit(void) {
  bleClockConfig_t conf;

  sys_safe_access_enable();
  R8_CK32K_CONFIG &= ~(RB_CLK_OSC32K_XT | RB_CLK_XT32K_PON);
  sys_safe_access_disable();
  sys_safe_access_enable();
  R8_CK32K_CONFIG |= RB_CLK_INT32K_PON;
  sys_safe_access_disable();
  LSECFG_Current(LSE_RCur_100);
  Lib_Calibration_LSI();

  RTC_InitTime(2020, 1, 1, 0, 0, 0); //RTC时钟初始化当前时间

  tmos_memset(&conf, 0, sizeof(bleClockConfig_t));
  conf.ClockAccuracy = CLK_OSC32K ? 1000 : 50;
  conf.ClockFrequency = CAB_LSIFQ;
  conf.ClockMaxCount = RTC_MAX_COUNT;
  conf.getClockValue = SYS_GetClockValue;
  conf.SetPendingIRQ = SYS_SetPendingIRQ;

  TMOS_TimerInit(&conf);
}
