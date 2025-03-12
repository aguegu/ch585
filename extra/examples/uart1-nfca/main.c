#include <stdio.h>
#include "CH58x_common.h"
#include "wch_nfca_mifare_classic.h"
#include "wch_nfca_pcd_bsp.h"

volatile uint32_t jiffies = 0;

int _write(int fd, char *buf, int size) {
  for (int i = 0; i < size; i++) {
    while (R8_UART1_TFC == UART_FIFO_SIZE);
    R8_UART1_THR = *buf++;
  }
  return size;
}

void delayInJiffy(uint32_t t) {
  uint32_t start = jiffies;
  while (t) {
    if (jiffies != start) {
      t--;
      start++;
    }
  }
}

uint8_t default_key[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint8_t picc_uid[4];

uint16_t sys_get_vdd(void) {
    uint8_t  sensor, channel, config, tkey_cfg;
    uint16_t adc_data;

    tkey_cfg = R8_TKEY_CFG;
    sensor = R8_TEM_SENSOR;
    channel = R8_ADC_CHANNEL;
    config = R8_ADC_CFG;

    R8_TKEY_CFG &= ~RB_TKEY_PWR_ON;
    R8_ADC_CHANNEL = CH_INTE_VBAT;
    R8_ADC_CFG = RB_ADC_POWER_ON | RB_ADC_BUF_EN | (0 << 4);    /* 使用-12dB模式 */
    R8_ADC_CONVERT &= ~RB_ADC_PGA_GAIN2;
    R8_ADC_CONVERT |= (3 << 4);                                 /* 7个Tadc */
    R8_ADC_CONVERT |= RB_ADC_START;
    while (R8_ADC_CONVERT & RB_ADC_START);
    adc_data = R16_ADC_DATA;

    R8_TEM_SENSOR = sensor;
    R8_ADC_CHANNEL = channel;
    R8_ADC_CFG = config;
    R8_TKEY_CFG = tkey_cfg;
    return (adc_data);
}

int main() {
  PWR_DCDCCfg(ENABLE);
  HSECFG_Capacitance(HSECap_18p);
  SetSysClock(CLK_SOURCE_HSE_PLL_62_4MHz);
  SysTick_Config(GetSysClock() / 60); // 60Hz

  GPIOA_SetBits(GPIO_Pin_9);
  GPIOA_ModeCfg(GPIO_Pin_8, GPIO_ModeIN_PU);      // RXD1
  GPIOA_ModeCfg(GPIO_Pin_9, GPIO_ModeOut_PP_5mA); // TXD1
  UART1_DefInit();

  nfca_pcd_init();
  nfca_pcd_lpcd_calibration();

  uint16_t res;
  uint16_t adc_vdd;
  int vdd_value;

  adc_vdd = sys_get_vdd();
  vdd_value = ADC_VoltConverSignalPGA_MINUS_12dB(adc_vdd);
  printf("vdd_value: %d\n", vdd_value);
  if (vdd_value > 3400) {
    nfca_pcd_set_out_drv(NFCA_PCD_DRV_CTRL_LEVEL0);
    printf("LV0\n");
  } else if (vdd_value > 3000) {
    nfca_pcd_set_out_drv(NFCA_PCD_DRV_CTRL_LEVEL1);
    printf("LV1\n");
  } else if (vdd_value > 2600) {
    nfca_pcd_set_out_drv(NFCA_PCD_DRV_CTRL_LEVEL2);
    printf("LV2\n");
  } else {
    nfca_pcd_set_out_drv(NFCA_PCD_DRV_CTRL_LEVEL3);
    printf("LV3\n");
  }

  while(1) {
    nfca_pcd_start();

#if 1   /* 置1先进行超低功耗检卡，对天线信号幅度影响小的设备可能会无法唤醒 */
    if (nfca_pcd_lpcd_check() == 0) {
      printf("NO CARD\n");
      goto next_loop;
    }
    printf("CARD DETECT\n");
#endif
    mDelaymS(5);   /* 手机等模拟卡设备需要长时间的连续波唤醒其卡功能，普通实体卡 1 ms 即可 */

#if NFCA_PCD_USE_NFC_CTR_PIN
    nfca_pcd_ctr_handle();  /* 对天线信号进行检测，使用NFC CTR引脚控制幅度 */
#endif

    res = PcdRequest(PICC_REQALL);
    printf("PICC_REQALL: %d\n", res);
    if (res == 0x0004) {
      res = PcdAnticoll(PICC_ANTICOLL1);
      if (res == PCD_NO_ERROR) {
        picc_uid[0] = g_nfca_pcd_recv_buf[0];
        picc_uid[1] = g_nfca_pcd_recv_buf[1];
        picc_uid[2] = g_nfca_pcd_recv_buf[2];
        picc_uid[3] = g_nfca_pcd_recv_buf[3];
        printf("uid: %02x %02x %02x %02x\n", picc_uid[0], picc_uid[1], picc_uid[2], picc_uid[3]);

        res = PcdSelect(PICC_ANTICOLL1, picc_uid);
        if (res == PCD_NO_ERROR) {
          printf("\nselect OK, SAK:%02x\n", g_nfca_pcd_recv_buf[0]);
          res = PcdAuthState(PICC_AUTHENT1A, 0, default_key, picc_uid);

          printf("PICC_AUTHENT1A: %d\n", res);

          if (res != PCD_NO_ERROR) {
              goto nfc_exit;
          }

          for (uint8_t i = 0; i < 4; i++) {
            res = PcdRead(i);
            if (res != PCD_NO_ERROR) {
              printf("ERR: 0x%x\n", res);
              goto nfc_exit;
            }
            printf("block %02d: ", i);
            for (uint8_t j = 0; j < 16; j++) {
              printf("%02x ", g_nfca_pcd_recv_buf[j]);
            }
            printf("\n");
          }
  nfc_exit:
          PcdHalt();
        }
      }
    }
next_loop:
    nfca_pcd_stop();
    mDelaymS(500);
  }
  return 0;
}

__INTERRUPT
__HIGH_CODE
void SysTick_Handler(void) {
  jiffies++;
  SysTick->SR = 0;
}
