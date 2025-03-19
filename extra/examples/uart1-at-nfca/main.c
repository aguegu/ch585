#include <stdio.h>
#include "CH58x_common.h"
#include "wch_nfca_mifare_classic.h"
#include "wch_nfca_pcd_bsp.h"
#include "uart1.h"
#include "ssd1306.h"

volatile uint32_t jiffies = 0;

void delayInJiffy(uint32_t t) {
  uint32_t start = jiffies;
  while (t) {
    if (jiffies != start) {
      t--;
      start++;
    }
  }
}

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

uint16_t nfcReadBlocks(uint8_t *dest, uint8_t blockCount) { // 1 block = 16 bytes; 1K bytes = 64 blocks = 16 sectors
  uint16_t err = PCD_NO_ERROR;
  for (uint8_t i = 0; i < blockCount; i++) {
    if (i % 4 == 0) {
      if (i >> 2) {
        err = PcdAuthState(PICC_AUTHENT1A, i, (uint8_t []){ 0xD3, 0xF7, 0xD3, 0xF7, 0xD3, 0xF7 }, picc_uid);
      } else {
        err = PcdAuthState(PICC_AUTHENT1A, i, (uint8_t []){ 0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5 }, picc_uid);
      }
      if (err) break;
    }
    err = PcdRead(i);
    if (err) break;
    memcpy(dest + 16 * i, g_nfca_pcd_recv_buf, 16);
  }
  return err;
}

uint8_t nfcParseBlocks(uint8_t *src, uint8_t blockCount) {
  uint8_t sectorCount = blockCount >> 2;
  for (uint8_t i = 0; i < sectorCount; i++) {
    uint8_t *p = src + i * 64;

    if (i) {  // NDEF Sectors
      if (p[0] != 0x03) return 1;
      if (p[2 + p[1]] != 0xfe) return 1;
      if (memcmp(p + 54, (uint8_t []){ 0x7f, 0x07, 0x88 }, 3)) return 1;
    } else {  // MAD Sector
      if (memcmp(p + 54, (uint8_t []){ 0x78, 0x77, 0x88 }, 3)) return 1;
    }
  }
  return 0;
}

uint8_t nfcParseNDefText(uint8_t *src, char *p, uint8_t capacity) {
  if (src[2] != 0xd1) return 1;
  if (src[3] != 0x01) return 1; // Type Length == 1
  if (src[5] != 'T') return 1;  // Type Field == 'T'
  if (src[6] != 0x02) return 1; // the length of language code == 2, and in UTF8
  if (memcmp(src + 7, "en", 2)) return 1;

  uint8_t len = src[4] - src[6] - 1;

  if (len > capacity) {
    len = capacity;
  }

  memcpy(p, src + 7 + src[6], len);
  p[len] = '\0';
  return 0;
}

typedef struct {
  uint8_t pwmChannel;
  uint8_t pinPositive;
  uint8_t pinNegative;
  uint8_t pwmChannelIndex;
} MOTOR;

#define MOTOR_LEN 2

const MOTOR motors[MOTOR_LEN] = {
  { .pwmChannel = CH_PWM6, .pwmChannelIndex = 2, .pinPositive = GPIO_Pin_0, .pinNegative = GPIO_Pin_1 },
  { .pwmChannel = CH_PWM8, .pwmChannelIndex = 4, .pinPositive = GPIO_Pin_6, .pinNegative = GPIO_Pin_7 },
};

int main() {
  PWR_DCDCCfg(ENABLE);
  HSECFG_Capacitance(HSECap_18p);
  SetSysClock(CLK_SOURCE_HSE_PLL_62_4MHz);
  SysTick_Config(GetSysClock() / 60); // 60Hz

  GPIOA_SetBits(GPIO_Pin_9);
  GPIOA_ModeCfg(GPIO_Pin_8, GPIO_ModeIN_PU);      // RXD1
  GPIOA_ModeCfg(GPIO_Pin_9, GPIO_ModeOut_PP_5mA); // TXD1
  uart1Init();

  ssdInit();

  ssdPutString("NFC Scanning ...", 0, 0);
  ssdRefresh();

  PWMX_CLKCfg(240);            // freq = 62.4M / 240 / 255 = 1019.6 Hz
  PWMX_CycleCfg(PWMX_Cycle_255);

  for (uint8_t i = 0; i < MOTOR_LEN; i++) {
    PWMX_ACTOUT(motors[i].pwmChannel, 0, High_Level, ENABLE);
    GPIOB_ResetBits(motors[i].pinNegative);
    GPIOB_ModeCfg(motors[i].pinNegative, GPIO_ModeOut_PP_5mA);
    GPIOB_ModeCfg(motors[i].pinPositive, GPIO_ModeOut_PP_5mA);
  }

  nfca_pcd_init();
  nfca_pcd_lpcd_calibration();

  uint16_t err;
  uint16_t adc_vdd;
  int vdd_value;

  uint8_t blocks[16 * 8];
  char str[16];

  adc_vdd = sys_get_vdd();
  vdd_value = ADC_VoltConverSignalPGA_MINUS_12dB(adc_vdd);
  printf("vdd_value: %d\n", vdd_value);
  if (vdd_value > 3400) {
    nfca_pcd_set_out_drv(NFCA_PCD_DRV_CTRL_LEVEL0);
  } else if (vdd_value > 3000) {
    nfca_pcd_set_out_drv(NFCA_PCD_DRV_CTRL_LEVEL1);
  } else if (vdd_value > 2600) {
    nfca_pcd_set_out_drv(NFCA_PCD_DRV_CTRL_LEVEL2);
  } else {
    nfca_pcd_set_out_drv(NFCA_PCD_DRV_CTRL_LEVEL3);
  }

  while(1) {
    nfca_pcd_start();

    mDelaymS(1);   /* 手机等模拟卡设备需要长时间的连续波唤醒其卡功能，普通实体卡 1 ms 即可 */

#if NFCA_PCD_USE_NFC_CTR_PIN
    nfca_pcd_ctr_handle();  /* 对天线信号进行检测，使用NFC CTR引脚控制幅度 */
#endif

    do {
      uint16_t atqa = PcdRequest(PICC_REQALL);

      printf("\nATQA: %04x\n", atqa);

      ssdClear();
      sprintf(str, "ATQA: %04x", atqa);
      ssdPutString(str, 0, 0);

      if (atqa != 0x0004) break;

      err = PcdAnticoll(PICC_ANTICOLL1);
      if (err) break;

      memcpy(picc_uid, g_nfca_pcd_recv_buf, 4);
      printf("UID: %02x:%02x:%02x:%02x\n", picc_uid[0], picc_uid[1], picc_uid[2], picc_uid[3]);
      sprintf(str, "UID: %02x%02x%02x%02x", picc_uid[0], picc_uid[1], picc_uid[2], picc_uid[3]);
      ssdPutString(str, 2, 0);

      err = PcdSelect(PICC_ANTICOLL1, picc_uid);
      if (err) break;

      printf("SAK: %02x %02x %02x\n", g_nfca_pcd_recv_buf[0], g_nfca_pcd_recv_buf[1], g_nfca_pcd_recv_buf[2]);

      err = nfcReadBlocks(blocks, 8);
      PcdHalt();

      for (uint16_t i = 0; i < 128; i++) {
        printf("%02x", blocks[i]);
        if ((i & 0x0f) == 0x0f) {
          printf("\n");
        }
      }

      if (!err && !nfcParseBlocks(blocks, 8)) {
        char s[33];
        memset(s, 0, 33);
        if (!nfcParseNDefText(blocks + 64, s, 33)) {
          printf("%s\n", s);
          ssdPutString(s, 4, 0);
          ssdPutString(s + 16, 6, 0);
        }

        if (strcmp(s, "Ahead") == 0) {
          *((volatile uint8_t *)((&R8_PWM4_DATA) + motors[0].pwmChannelIndex)) = 0xc0;
          R8_PWM_POLAR &= ~motors[0].pwmChannel;
          GPIOB_ResetBits(motors[0].pinNegative);

          *((volatile uint8_t *)((&R8_PWM4_DATA) + motors[1].pwmChannelIndex)) = 0xc0;
          R8_PWM_POLAR &= ~motors[1].pwmChannel;
          GPIOB_ResetBits(motors[1].pinNegative);
        }

        if (strcmp(s, "Backward") == 0) {
          *((volatile uint8_t *)((&R8_PWM4_DATA) + motors[0].pwmChannelIndex)) = 0xc0;
          R8_PWM_POLAR |= motors[0].pwmChannel;
          GPIOB_SetBits(motors[0].pinNegative);

          *((volatile uint8_t *)((&R8_PWM4_DATA) + motors[1].pwmChannelIndex)) = 0xc0;
          R8_PWM_POLAR |= motors[1].pwmChannel;
          GPIOB_SetBits(motors[1].pinNegative);
        }

        if (strcmp(s, "Left") == 0) {
          *((volatile uint8_t *)((&R8_PWM4_DATA) + motors[0].pwmChannelIndex)) = 0xc0;
          R8_PWM_POLAR |= motors[0].pwmChannel;
          GPIOB_SetBits(motors[0].pinNegative);

          *((volatile uint8_t *)((&R8_PWM4_DATA) + motors[1].pwmChannelIndex)) = 0xc0;
          R8_PWM_POLAR &= ~motors[1].pwmChannel;
          GPIOB_ResetBits(motors[1].pinNegative);
        }

        if (strcmp(s, "Right") == 0) {
          *((volatile uint8_t *)((&R8_PWM4_DATA) + motors[0].pwmChannelIndex)) = 0xc0;
          R8_PWM_POLAR &= ~motors[0].pwmChannel;
          GPIOB_ResetBits(motors[0].pinNegative);

          *((volatile uint8_t *)((&R8_PWM4_DATA) + motors[1].pwmChannelIndex)) = 0xc0;
          R8_PWM_POLAR |= motors[1].pwmChannel;
          GPIOB_SetBits(motors[1].pinNegative);
        }

        if (strcmp(s, "Stop") == 0) {
          *((volatile uint8_t *)((&R8_PWM4_DATA) + motors[0].pwmChannelIndex)) = 0x00;
          R8_PWM_POLAR &= ~motors[0].pwmChannel;
          GPIOB_ResetBits(motors[0].pinNegative);

          *((volatile uint8_t *)((&R8_PWM4_DATA) + motors[1].pwmChannelIndex)) = 0x00;
          R8_PWM_POLAR &= ~motors[1].pwmChannel;
          GPIOB_ResetBits(motors[1].pinNegative);
        }
      }

      ssdRefresh();
    } while (FALSE);

    nfca_pcd_stop();
    delayInJiffy(6);
  }
  return 0;
}

__INTERRUPT
__HIGH_CODE
void SysTick_Handler(void) {
  jiffies++;
  SysTick->SR = 0;
}
