#include "app.h"

__attribute__((aligned(4))) uint32_t MEM_BUF[BLE_MEMHEAP_SIZE / 4];

__HIGH_CODE
__attribute__((noinline)) void Main_Circulation() {
  while (1) {
    TMOS_SystemProcess();
  }
}

uint8_t bt_mesh_lib_init(void) {
  uint8_t ret;

  ret = RF_RoleInit();

#if ((CONFIG_BLE_MESH_PROXY) || (CONFIG_BLE_MESH_PB_GATT) ||                   \
     (CONFIG_BLE_MESH_OTA))
  ret = GAPRole_PeripheralInit();
#endif /* PROXY || PB-GATT || OTA */

  MeshTimer_Init();
  MeshDeamon_Init();
  ble_sm_alg_ecc_init();

  return ret;
}

int main(void) {
  HSECFG_Capacitance(HSECap_18p);
  SetSysClock(CLK_SOURCE_HSE_PLL_62_4MHz);

#ifdef DEBUG

  GPIOA_SetBits(GPIO_Pin_9);
  GPIOA_ModeCfg(GPIO_Pin_8, GPIO_ModeIN_PU);
  GPIOA_ModeCfg(GPIO_Pin_9, GPIO_ModeOut_PP_5mA);
  UART1_DefInit();

#endif
  APP_DBG(VER_LIB);
  APP_DBG(VER_MESH_LIB);
  APP_DBG(VER_MESH_FILE);
  CH58x_BLEInit();
  HAL_Init();
  bt_mesh_lib_init();
  App_Init();
  Main_Circulation();
}
