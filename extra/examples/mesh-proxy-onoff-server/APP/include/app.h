#ifndef APP_H
#define APP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "MESH_LIB.h"
#include "config.h"
#include "HAL.h"

#define APP_USER_EVT (1 << 0)

void App_Init(void);

void blemesh_on_sync(const struct bt_mesh_comp *app_comp);

#ifdef __cplusplus
}
#endif

#endif
