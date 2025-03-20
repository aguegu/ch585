#include "app_generic_onoff_model.h"

static void generic_onoff_status(struct bt_mesh_model *model,
                             struct bt_mesh_msg_ctx *ctx) {
  NET_BUF_SIMPLE_DEFINE(msg, 32);
  int err;

  bt_mesh_model_msg_init(&msg, BLE_MESH_MODEL_OP_GEN_ONOFF_STATUS);

  if (((struct bt_mesh_generic_onoff_server *)(model->user_data))->onReadState) {
    net_buf_simple_add_u8(&msg, ((struct bt_mesh_generic_onoff_server *)(model->user_data))->onReadState());
  }

  APP_DBG("ttl: 0x%02x dst: 0x%04x", ctx->recv_ttl, ctx->recv_dst);
  APP_DBG("msg len: 0x%02x: %02x %02x %02x", msg.len, msg.data[0], msg.data[1], msg.data[2]);

  ctx->send_ttl = BLE_MESH_TTL_DEFAULT;

  err = bt_mesh_model_send(model, ctx, &msg, NULL, NULL);
  if (err) {
    APP_DBG("send status failed: %d", err);
  }
}

static void gen_onoff_get(struct bt_mesh_model *model,
                          struct bt_mesh_msg_ctx *ctx,
                          struct net_buf_simple *buf) {
  APP_DBG(" ");

  generic_onoff_status(model, ctx);
}

static void gen_onoff_set(struct bt_mesh_model *model,
                          struct bt_mesh_msg_ctx *ctx,
                          struct net_buf_simple *buf) {
  // APP_DBG("ttl: 0x%02x dst: 0x%04x rssi: %d", ctx->recv_ttl, ctx->recv_dst, ctx->recv_rssi);

  ((struct bt_mesh_generic_onoff_server *)(model->user_data))->onWriteState(buf->data[0]);
  generic_onoff_status(model, ctx);
}

static void gen_onoff_set_unack(struct bt_mesh_model *model,
                                struct bt_mesh_msg_ctx *ctx,
                                struct net_buf_simple *buf) {
  ((struct bt_mesh_generic_onoff_server *)(model->user_data))->onWriteState(buf->data[0]);
}

const struct bt_mesh_model_op gen_onoff_op[] = {
  { BLE_MESH_MODEL_OP_GEN_ONOFF_GET, 0, gen_onoff_get },
  { BLE_MESH_MODEL_OP_GEN_ONOFF_SET, 2, gen_onoff_set },
  { BLE_MESH_MODEL_OP_GEN_ONOFF_SET_UNACK, 2, gen_onoff_set_unack },
  BLE_MESH_MODEL_OP_END,
};

void generic_onoff_status_publish(struct bt_mesh_model *model) {
  struct bt_mesh_msg_ctx ctx = {
    .net_idx = bt_mesh_app_key_find(model->pub->key)->net_idx,
    .app_idx = model->pub->key,
    .addr = model->pub->addr,
    .send_ttl = BLE_MESH_TTL_DEFAULT,
  };

  generic_onoff_status(model, &ctx);
}
