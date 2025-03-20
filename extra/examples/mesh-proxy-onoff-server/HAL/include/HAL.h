#ifndef __HAL_H
#define __HAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "config.h"
#include "RTC.h"

#define HAL_REG_INIT_EVENT    0x2000

extern tmosTaskID halTaskID;

extern void HAL_Init(void);

extern tmosEvents HAL_ProcessEvent(tmosTaskID task_id, tmosEvents events);

extern void CH58x_BLEInit(void);

extern uint16_t HAL_GetInterTempValue(void);

extern void Lib_Calibration_LSI(void);


#ifdef __cplusplus
}
#endif

#endif
