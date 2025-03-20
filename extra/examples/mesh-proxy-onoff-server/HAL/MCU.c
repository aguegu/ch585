#include "HAL.h"

tmosTaskID halTaskID;
uint32_t g_LLE_IRQLibHandlerLocation;

void Lib_Calibration_LSI(void) {
  Calibration_LSI(Level_64);
}

#if(defined(BLE_SNV)) && (BLE_SNV == TRUE)

uint32_t Lib_Read_Flash(uint32_t addr, uint32_t num, uint32_t *pBuf) {
  EEPROM_READ(addr, pBuf, num * 4);
  return 0;
}

uint32_t Lib_Write_Flash(uint32_t addr, uint32_t num, uint32_t *pBuf) {
  EEPROM_ERASE(addr, num * 4);
  EEPROM_WRITE(addr, pBuf, num * 4);
  return 0;
}

#endif

void CH58x_BLEInit(void) {
  bleConfig_t cfg;
  if (tmos_memcmp(VER_LIB, VER_FILE, strlen(VER_FILE)) == FALSE) {
    PRINT("head file error...\n");
    while(1);
  }

  SysTick_Config(SysTick_LOAD_RELOAD_Msk);
  PFIC_DisableIRQ(SysTick_IRQn);

  g_LLE_IRQLibHandlerLocation = (uint32_t)LLE_IRQLibHandler;
  PFIC_SetPriority(BLEL_IRQn, 0xF0);
  tmos_memset(&cfg, 0, sizeof(bleConfig_t));
  cfg.MEMAddr = (uint32_t)MEM_BUF;
  cfg.MEMLen = (uint32_t)BLE_MEMHEAP_SIZE;
  cfg.BufMaxLen = (uint32_t)BLE_BUFF_MAX_LEN;
  cfg.BufNumber = (uint32_t)BLE_BUFF_NUM;
  cfg.TxNumEvent = (uint32_t)BLE_TX_NUM_EVENT;
  cfg.TxPower = (uint32_t)BLE_TX_POWER;

#if(defined(BLE_SNV)) && (BLE_SNV == TRUE)
  if((BLE_SNV_ADDR + BLE_SNV_BLOCK * BLE_SNV_NUM) > (0x78000 - FLASH_ROM_MAX_SIZE))
  {
      PRINT("SNV config error...\n");
      while(1);
  }
  cfg.SNVAddr = (uint32_t)BLE_SNV_ADDR;
  cfg.SNVBlock = (uint32_t)BLE_SNV_BLOCK;
  cfg.SNVNum = (uint32_t)BLE_SNV_NUM;
  cfg.readFlashCB = Lib_Read_Flash;
  cfg.writeFlashCB = Lib_Write_Flash;
#endif

  cfg.ConnectNumber = (PERIPHERAL_MAX_CONNECTION & 3) | (CENTRAL_MAX_CONNECTION << 2);
  cfg.srandCB = SYS_GetSysTickCnt;


  cfg.tsCB = HAL_GetInterTempValue; // 根据温度变化校准RF和内部RC( 大于7摄氏度 )
  cfg.rcCB = Lib_Calibration_LSI; // 内部32K时钟校准

  uint8_t MacAddr[6];
  GetMACAddress(MacAddr);
  for (uint8_t i = 0; i < 6; i++) {
    cfg.MacAddr[i] = MacAddr[i]; // 使用芯片mac地址
  }

  if (!cfg.MEMAddr || cfg.MEMLen < 4 * 1024) {
    while(1);
  }

  // BLE_Lib 占用了VTF Interrupt 2号和3号
  uint8_t i = BLE_LibInit(&cfg);
  if (i) {
    PRINT("LIB init error code: %x ...\n", i);
    while(1);
  }
}

tmosEvents HAL_ProcessEvent(tmosTaskID task_id, tmosEvents events) {
  uint8_t *msgPtr;

  if (events & SYS_EVENT_MSG) {
    msgPtr = tmos_msg_receive(task_id);
    if (msgPtr) {
      tmos_msg_deallocate(msgPtr);
    }
    return events ^ SYS_EVENT_MSG;
  }

  if (events & HAL_REG_INIT_EVENT) {
    Lib_Calibration_LSI(); // 校准内部RC
    tmos_start_task(halTaskID, HAL_REG_INIT_EVENT, MS1_TO_SYSTEM_TIME(BLE_CALIBRATION_PERIOD));
    return events ^ HAL_REG_INIT_EVENT;
  }

  return 0;
}

void HAL_Init() {
  halTaskID = TMOS_ProcessEventRegister(HAL_ProcessEvent);
  HAL_TimeInit();
  tmos_start_task(halTaskID, HAL_REG_INIT_EVENT, 800);
}

uint16_t HAL_GetInterTempValue(void) {
  uint8_t  sensor, channel, config, tkey_cfg;
  uint16_t adc_data;

  tkey_cfg = R8_TKEY_CFG;
  sensor = R8_TEM_SENSOR;
  channel = R8_ADC_CHANNEL;
  config = R8_ADC_CFG;
  ADC_InterTSSampInit();
  R8_ADC_CONVERT |= RB_ADC_START;
  while(R8_ADC_CONVERT & RB_ADC_START);
  adc_data = R16_ADC_DATA;
  R8_TEM_SENSOR = sensor;
  R8_ADC_CHANNEL = channel;
  R8_ADC_CFG = config;
  R8_TKEY_CFG = tkey_cfg;
  return (adc_data);
}
