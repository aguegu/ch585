#include "CH58x_common.h"
#include "uart1.h"
#include "at.h"
#include "ssd1306.h"

volatile uint32_t jiffies = 0;

void handleAT(uint8_t * payload, uint8_t len) {
  sendOK();
}

void handleATMAC(uint8_t * payload, uint8_t len) {
  uint8_t mac[6];
  GetMACAddress(mac);
  for (uint8_t i = 6; i--;) {
    printf("%02X", mac[i]);
  }
  sendOK();
}

void handleATID(uint8_t * payload, uint8_t len) {
  uint8_t id[8];
  FLASH_EEPROM_CMD(CMD_GET_UNIQUE_ID, 0, id, 0);
  for (uint8_t i = 0; i < 8; i++) {
    printf("%02X", id[i]);
  }
  sendOK();
}

void handleATRESET(uint8_t * payload, uint8_t len) {
  sendOK();
  uart1FlushTx();
  SYS_ResetExecute();
}

void handleATECHO(uint8_t * payload, uint8_t len) {
  for (uint8_t i = 0; i < len; i++) {
    printf("%02X", payload[i]);
  }
  sendOK();
}

void handleATSC(uint8_t * payload, uint8_t len) {
  for (uint8_t slaveAddress = 0x03; slaveAddress < 0x78; slaveAddress++) {
    while (I2C_GetFlagStatus(I2C_FLAG_BUSY) != RESET);

    I2C_GenerateSTART(ENABLE);
    while(!I2C_CheckEvent(I2C_EVENT_MASTER_MODE_SELECT));

    I2C_Send7bitAddress(slaveAddress << 1, I2C_Direction_Transmitter);
    while (!I2C_CheckEvent(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) && !I2C_GetFlagStatus(I2C_FLAG_AF));

    if (I2C_GetFlagStatus(I2C_FLAG_AF)) {
      I2C_ClearFlag(I2C_FLAG_AF);
    }

    BOOL acked = I2C_GetFlagStatus(I2C_FLAG_TXE);
    I2C_GenerateSTOP(ENABLE);

    if (acked) {
      printf("%02X", slaveAddress);
    }
  }

  sendOK();
}

void handleATTR(uint8_t * payload, uint8_t len) {
  if (len < 2) {
    return sendError();
  }

  I2C_GenerateSTART(ENABLE);
  while (!I2C_CheckEvent(I2C_EVENT_MASTER_MODE_SELECT)); // 0x00030001

  I2C_Send7bitAddress(payload[0] << 1, I2C_Direction_Transmitter);
  while (!I2C_CheckEvent(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) && !I2C_GetFlagStatus(I2C_FLAG_AF));  // 0x00070082

  if (I2C_GetFlagStatus(I2C_FLAG_AF)) {
    I2C_ClearFlag(I2C_FLAG_AF);
    I2C_GenerateSTOP(ENABLE);
    return sendError();
  }

  if (len > 2) {
    uint8_t writeCount = len - 2;
    uint8_t *toWrite = payload + 1;
    while (writeCount--) {
      I2C_SendData(*toWrite++);
      while (!I2C_CheckEvent(I2C_EVENT_MASTER_BYTE_TRANSMITTED));  // 0x00070084
    }
  }

  uint8_t readCount = payload[len - 1];

  if (readCount) {
    I2C_GenerateSTART(ENABLE);  // restart
    while (!I2C_CheckEvent(I2C_EVENT_MASTER_MODE_SELECT)); // 0x00030001

    I2C_Send7bitAddress(payload[0] << 1, I2C_Direction_Receiver);
    while (!I2C_CheckEvent(I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED)); // 0x00030002

    while (readCount--) {
      I2C_AcknowledgeConfig(readCount);
      while (!I2C_CheckEvent(I2C_EVENT_MASTER_BYTE_RECEIVED)); // 0x00030040
      printf("%02X", I2C_ReceiveData());
    }
  }

  I2C_GenerateSTOP(ENABLE);
  while (I2C_GetFlagStatus(I2C_FLAG_BUSY) != RESET); // 0x00000000

  sendOK();
}

void handleATSHOW(uint8_t * payload, uint8_t len) {
  payload[len] = 0;
  ssdPutString(payload, 0, 0);
  ssdRefresh();
  sendOK();
}

void handleATCELSIUS(uint8_t * payload, uint8_t len) {
  uint16_t buff[16];
  ADC_InterTSSampInit();

  ADC_ExcutSingleConver();  // drop initial measurement
  for (uint8_t i = 0; i < 16; i++) {
    buff[i] = ADC_ExcutSingleConver();
  }
  for(uint8_t i = 0; i < 16; i++) {
    uint32_t C25 = 0;
    C25 = (*((PUINT32)ROM_CFG_TMP_25C));
    printf("%d %d %d \n", adc_to_temperature_celsius(buff[i]), buff[i], C25);
  }

  ADC_DisableTSPower();
  sendOK();
}

void handleATADC(uint8_t * payload, uint8_t len) {
  uint16_t buff[4], sum = 0;

  ADC_ChannelCfg(6);
  ADC_ExcutSingleConver();//时间足够时建议再次转换并丢弃首次ADC数据
  for (uint8_t i = 0; i < 4; i++) {
    buff[i] = ADC_ExcutSingleConver();
  }
  for (uint8_t i = 0; i < 4; i++) {
    sum += buff[i];
  }
  printf("%04X", sum >> 2);
  sendOK();
}

const static CommandHandler atHandlers[] = {
  { "AT", TRUE, handleAT },
  { "AT+MAC", TRUE, handleATMAC },
  { "AT+ID", TRUE, handleATID },
  { "AT+RESET", TRUE, handleATRESET },
  { "AT+ECHO=", FALSE, handleATECHO },

  { "AT+SC", TRUE, handleATSC },
  { "AT+TR=", FALSE, handleATTR },

  { "AT+SHOW=", FALSE, handleATSHOW },

  { "AT+CELSIUS", TRUE, handleATCELSIUS },

  { "AT+ADC", TRUE, handleATADC },

  { NULL, TRUE, NULL }  // End marker
};

BOOL athandler() {
  static uint8_t command[256];
  static uint8_t l = 0;
  static uint8_t content[128];
  BOOL LFrecevied = FALSE;

  while (uart1RxAvailable()) {
    uint8_t temp = uart1RxGet();
    if (temp == '\n') {
      LFrecevied = TRUE;
      break;
    } else {
      command[l++] = temp;
    }
  }

  if (LFrecevied) {
    BOOL handled = FALSE;
    if (command[l - 1] == '\r') {
      command[l - 1] = 0;

      for (uint8_t i = 0; atHandlers[i].command != NULL; i++) {
        if (atHandlers[i].isEqual && strcmp(command, atHandlers[i].command) == 0) {
          atHandlers[i].handler(NULL, 0);
          handled = TRUE;
          break;
        }

        if (!atHandlers[i].isEqual && startsWith(command, atHandlers[i].command)) {
          uint8_t len = genPayload(command + strlen(atHandlers[i].command), content);
          atHandlers[i].handler(content, len);
          handled = TRUE;
          break;
        }
      }
    }

    if (!handled) {
      sendError();
    }

    l = 0;
    uart1FlushTx();
  }
  return LFrecevied;
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


int main() {
  HSECFG_Capacitance(HSECap_18p);
  SetSysClock(CLK_SOURCE_HSE_PLL_62_4MHz);
  SysTick_Config(GetSysClock() / 60); // 60Hz

  uint16_t buff[4], sum = 0;
  char s[16];

  GPIOA_SetBits(GPIO_Pin_9);
  GPIOA_ModeCfg(GPIO_Pin_8, GPIO_ModeIN_PU);      // RXD1
  GPIOA_ModeCfg(GPIO_Pin_9, GPIO_ModeOut_PP_5mA); // TXD1

  GPIOPinRemap(ENABLE, RB_PIN_I2C); // I2C remap, the default i2c is on USB
  GPIOB_ModeCfg(GPIO_Pin_20, GPIO_ModeIN_PU); // i2c SDA
  GPIOB_ModeCfg(GPIO_Pin_21, GPIO_ModeIN_PU); // i2c SCL

  I2C_Init(I2C_Mode_I2C, 400000, I2C_DutyCycle_16_9, I2C_Ack_Enable, I2C_AckAddr_7bit, 0x00);

  uart1Init();

  ssdInit();

  GPIOA_ModeCfg(GPIO_Pin_3, GPIO_ModeIN_Floating);
  ADC_ExtSingleChSampInit(SampleFreq_8_or_4, ADC_PGA_0);

  ssdPutString("Hello.", 0, 0);
  ssdRefresh();

  while (1) {
    if (!athandler()) {
      __WFI();
    }
    sum = 0;
    ADC_ChannelCfg(6);
    ADC_ExcutSingleConver();//时间足够时建议再次转换并丢弃首次ADC数据
    for (uint8_t i = 0; i < 4; i++) {
      buff[i] = ADC_ExcutSingleConver();
    }
    for (uint8_t i = 0; i < 4; i++) {
      sum += buff[i];
    }
    // printf("%04X", sum >> 2);
    ssdClear();
    sprintf(s, "%d", sum >> 2);
    ssdPutString(s, 0, 0);
    ssdRefresh();

    delayInJiffy(6);
  }
}

__INTERRUPT
__HIGH_CODE
void SysTick_Handler(void) {
  jiffies++;
  SysTick->SR = 0;
}
