#ifndef APP_GENERIC_ONOFF_MODEL_H
#define APP_GENERIC_ONOFF_MODEL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "MESH_LIB.h"

extern const struct bt_mesh_model_op gen_onoff_op[];

struct bt_mesh_generic_onoff_server {
  BOOL (*onReadState)();
  void (*onWriteState)(BOOL state);
};

void generic_onoff_status_publish(struct bt_mesh_model *model);

#ifdef __cplusplus
}
#endif

#endif
