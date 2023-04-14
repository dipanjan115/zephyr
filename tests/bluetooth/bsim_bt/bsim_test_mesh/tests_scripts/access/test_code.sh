#!/usr/bin/env bash
# Copyright 2022 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

source $(dirname "${BASH_SOURCE[0]}")/../../_mesh_test.sh

# RunTest mesh_access_extended_model_subscription_capacity access_sub_capacity_ext_model
RunTest multi_node_test_setup code_tx_node_1 code_tx_node_2 code_rx_node_3 code_rx_node_4
