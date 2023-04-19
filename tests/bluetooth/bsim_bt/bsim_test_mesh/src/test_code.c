/*
 * Copyright (c) 2021 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "mesh_test.h"
#include "mesh/net.h"
#include "mesh/access.h"
#include "mesh/foundation.h"

#define LOG_MODULE_NAME test_code

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#define UNICAST_ADDR1 0x0001
#define UNICAST_ADDR2 0x0002
#define UNICAST_ADDR3 0x0003
#define UNICAST_ADDR4 0x0004

#define WAIT_TIME   10	/*seconds*/
#define TX_INTERVAL 200 /*miliseconds*/

#define TEST_MODEL_ID_1 0x2a2a
#define TEST_MODEL_ID_2 0x2b2b

#define TEST_MESSAGE_OP_1 BT_MESH_MODEL_OP_1(0x11)
#define TEST_MESSAGE_OP_2 BT_MESH_MODEL_OP_1(0x12)

static int model1_init(struct bt_mesh_model *model)
{
	return 0;
}
static int model2_init(struct bt_mesh_model *model)
{
	return 0;
}

static int test_msg_handler(struct bt_mesh_model *model, struct bt_mesh_msg_ctx *ctx,
			    struct net_buf_simple *buf)
{
	LOG_DBG("msg rx model id: %u", model->id);
	return 0;
}

static uint8_t dev_key[16] = {0xdd};
static uint8_t app_key[16] = {0xaa};
static uint8_t net_key[16] = {0xcc};
static struct bt_mesh_prov prov;

static const struct bt_mesh_model_cb test_model1_cb = {
	.init = model1_init,
};

static const struct bt_mesh_model_cb test_model2_cb = {
	.init = model2_init,
};

static const struct bt_mesh_model_op model_op1[] = {{TEST_MESSAGE_OP_1, 0, test_msg_handler},
						    BT_MESH_MODEL_OP_END};

static const struct bt_mesh_model_op model_op2[] = {{TEST_MESSAGE_OP_2, 0, test_msg_handler},
						    BT_MESH_MODEL_OP_END};

static struct bt_mesh_cfg_cli cfg_cli;

/* do not change model sequence. it will break pointer arithmetic. */
static struct bt_mesh_model models[] = {
	BT_MESH_MODEL_CFG_SRV,
	BT_MESH_MODEL_CFG_CLI(&cfg_cli),
	BT_MESH_MODEL_CB(TEST_MODEL_ID_1, model_op1, NULL, NULL, &test_model1_cb),
	BT_MESH_MODEL_CB(TEST_MODEL_ID_2, model_op2, NULL, NULL, &test_model2_cb),
};

static struct bt_mesh_model vnd_models[] = {};

static struct bt_mesh_elem elems[] = {
	BT_MESH_ELEM(0, models, vnd_models),
};

static const struct bt_mesh_comp local_comp = {
	.elem = elems,
	.elem_count = ARRAY_SIZE(elems),
};

static void provision(uint16_t addr)
{
	int err;

	err = bt_mesh_provision(net_key, 0, 0, 0, addr, dev_key);
	if (err) {
		FAIL("Provisioning failed (err %d)", err);
		return;
	}
}

static void common_configure(uint16_t addr)
{
	uint8_t status;
	int err;
	// uint16_t model_ids[] = {TEST_MODEL_ID_1};
	uint16_t model_ids[] = {TEST_MODEL_ID_1, TEST_MODEL_ID_2};

	err = bt_mesh_cfg_cli_app_key_add(0, addr, 0, 0, app_key, &status);
	if (err || status) {
		FAIL("AppKey add failed (err %d, status %u)", err, status);
		return;
	}

	// err = bt_mesh_cfg_cli_mod_app_bind(0, addr, addr, 0, TEST_MODEL_ID_1, &status);
	// if (err || status) {
	// 	FAIL("Model %#4x bind failed (err %d, status %u)", TEST_MODEL_ID_1, err, status);
	// 	return;
	// }

	for (int i = 0; i < ARRAY_SIZE(model_ids); i++) {
		err = bt_mesh_cfg_cli_mod_app_bind(0, addr, addr, 0, model_ids[i], &status);
		if (err || status) {
			FAIL("Model %#4x bind failed (err %d, status %u)", model_ids[i], err,
			     status);
			return;
		}
	}

	// err = bt_mesh_cfg_cli_net_transmit_set(0, addr, BT_MESH_TRANSMIT(2, 20), &status);
	// if (err || status != BT_MESH_TRANSMIT(2, 20)) {
	// 	FAIL("Net transmit set failed (err %d, status %u)", err, status);
	// 	return;
	// }
}

static struct k_work_delayable delayed_work_N1N3;
static struct k_work_delayable delayed_work_N1N4;
static struct k_work_delayable delayed_work_N2N3;

static void send_message_N1N3(struct k_work *work)
{
	static int count = 0;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = 0,
		.app_idx = 0,
		.addr = UNICAST_ADDR3,
		.send_rel = false,
		.send_ttl = 3,
	};

	BT_MESH_MODEL_BUF_DEFINE(buf, TEST_MESSAGE_OP_1, 0);

	bt_mesh_model_msg_init(&buf, TEST_MESSAGE_OP_1);
	bt_mesh_model_send(&models[2], &ctx, &buf, NULL, NULL);

	count++;

	if (count < 2) {
		k_work_reschedule(&delayed_work_N1N3, K_MSEC(TX_INTERVAL));
	}
}

static void send_message_N1N4(struct k_work *work)
{
	static int count = 0;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = 0,
		.app_idx = 0,
		.addr = UNICAST_ADDR4,
		.send_rel = false,
		.send_ttl = 3,
	};

	BT_MESH_MODEL_BUF_DEFINE(buf, TEST_MESSAGE_OP_1, 0);

	bt_mesh_model_msg_init(&buf, TEST_MESSAGE_OP_1);
	bt_mesh_model_send(&models[2], &ctx, &buf, NULL, NULL);

	count++;

	if (count < 2) {
		k_work_reschedule(&delayed_work_N1N4, K_MSEC(TX_INTERVAL));
	}
}

static void send_message_N2N3(struct k_work *work)
{
	static int count = 0;
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = 0,
		.app_idx = 0,
		.addr = UNICAST_ADDR3,
		.send_rel = false,
		.send_ttl = 3,
	};

	BT_MESH_MODEL_BUF_DEFINE(buf, TEST_MESSAGE_OP_2, 0);

	bt_mesh_model_msg_init(&buf, TEST_MESSAGE_OP_2);
	bt_mesh_model_send(&models[3], &ctx, &buf, NULL, NULL);

	count++;

	if (count < 2) {
		k_work_reschedule(&delayed_work_N2N3, K_MSEC(TX_INTERVAL));
	}
}

static void test_tx_node_1(void)
{
	bt_mesh_test_cfg_set(NULL, WAIT_TIME);
	bt_mesh_device_setup(&prov, &local_comp);
	provision(UNICAST_ADDR1);
	common_configure(UNICAST_ADDR1);
	LOG_INF(" ---- ## CONFIG DONE ## ");

	k_work_init_delayable(&delayed_work_N1N3, send_message_N1N3);
	k_work_reschedule(&delayed_work_N1N3, K_MSEC(TX_INTERVAL));

	k_work_init_delayable(&delayed_work_N1N4, send_message_N1N4);
	k_work_reschedule(&delayed_work_N1N4, K_MSEC(TX_INTERVAL));
	PASS();
}

static void test_tx_node_2(void)
{
	bt_mesh_test_cfg_set(NULL, WAIT_TIME);
	bt_mesh_device_setup(&prov, &local_comp);
	provision(UNICAST_ADDR2);
	common_configure(UNICAST_ADDR2);
	LOG_INF(" ---- ## CONFIG DONE ## ");

	k_work_init_delayable(&delayed_work_N2N3, send_message_N2N3);
	k_work_reschedule(&delayed_work_N2N3, K_MSEC(TX_INTERVAL));
	PASS();
}

static void test_rx_node_3(void)
{
	bt_mesh_test_cfg_set(NULL, WAIT_TIME);
	bt_mesh_device_setup(&prov, &local_comp);
	provision(UNICAST_ADDR3);
	common_configure(UNICAST_ADDR3);
	LOG_INF(" ---- ## CONFIG DONE ## ");

	PASS();
}

static void test_rx_node_4(void)
{
	bt_mesh_test_cfg_set(NULL, WAIT_TIME);
	bt_mesh_device_setup(&prov, &local_comp);
	provision(UNICAST_ADDR4);
	common_configure(UNICAST_ADDR4);
	LOG_INF(" ---- ## CONFIG DONE ## ");

	PASS();
}

/*Make a test case to tweak with the buffer*/

// static void test_queue_node(void)
// {

// }

#define TEST_CASE(role, name, description)                                                         \
	{                                                                                          \
		.test_id = "code_" #role "_" #name, .test_descr = description,                     \
		.test_tick_f = bt_mesh_test_timeout, .test_main_f = test_##role##_##name,          \
	}

static const struct bst_test_instance test_code[] = {
	TEST_CASE(tx, node_1, "Access: tx data of node 1"),
	TEST_CASE(tx, node_2, "Access: tx data of node 2"),
	TEST_CASE(rx, node_3, "Acess: rx data of node 3"),
	TEST_CASE(rx, node_4, "Acess: rx data of node 4"), BSTEST_END_MARKER};

struct bst_test_list *test_code_install(struct bst_test_list *tests)
{
	tests = bst_add_tests(tests, test_code);
	return tests;
};