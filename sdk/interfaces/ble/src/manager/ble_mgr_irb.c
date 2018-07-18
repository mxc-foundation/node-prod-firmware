/**
 ****************************************************************************************
 *
 * @file ble_mgr_irb.c
 *
 * @brief BLE IRB handlers
 *
 * Copyright (C) 2015. Dialog Semiconductor, unpublished work. This computer
 * program includes Confidential, Proprietary Information and is a Trade Secret of
 * Dialog Semiconductor. All use, disclosure, and/or reproduction is prohibited
 * unless authorized in writing. All Rights Reserved.
 *
 * <black.orca.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */

#include "ble_mgr.h"
#include "ble_mgr_irb.h"
#include "ble_mgr_irb_common.h"
#include "ble_mgr_irb_gap.h"
#include "ble_mgr_irb_gatts.h"
#include "ble_mgr_irb_gattc.h"
#include "ble_mgr_irb_l2cap.h"

static const irb_ble_handler_t h_common[IRB_BLE_GET_IDX(IRB_BLE_LAST_COMMON)] = {
        irb_ble_handler_stack_api_msg,
        irb_ble_handler_register_cmd,
#ifndef BLE_STACK_PASSTHROUGH_MODE
        irb_ble_handler_enable_cmd,
        irb_ble_handler_reset_cmd,
        irb_ble_handler_read_tx_power,
#endif
};

#ifndef BLE_STACK_PASSTHROUGH_MODE
static const irb_ble_handler_t h_gap[IRB_BLE_GET_IDX(IRB_BLE_LAST_GAP)] = {
        irb_ble_handler_gap_address_set_cmd,
        irb_ble_handler_gap_device_name_set_cmd,
        irb_ble_handler_gap_appearance_set_cmd,
        irb_ble_handler_gap_ppcp_set_cmd,
        irb_ble_handler_gap_adv_start_cmd,
        irb_ble_handler_gap_adv_stop_cmd,
        irb_ble_handler_gap_adv_data_set_cmd,
        irb_ble_handler_gap_scan_start_cmd,
        irb_ble_handler_gap_scan_stop_cmd,
        irb_ble_handler_gap_connect_cmd,
        irb_ble_handler_gap_connect_cancel_cmd,
        irb_ble_handler_gap_disconnect_cmd,
        irb_ble_handler_gap_conn_rssi_get_cmd,
        irb_ble_handler_gap_role_set_cmd,
        irb_ble_handler_gap_mtu_size_set_cmd,
        irb_ble_handler_gap_channel_map_set_cmd,
        irb_ble_handler_gap_conn_param_update_cmd,
        irb_ble_handler_gap_conn_param_update_reply_cmd,
        irb_ble_handler_gap_pair_cmd,
        irb_ble_handler_gap_pair_reply_cmd,
        irb_ble_handler_gap_passkey_reply_cmd,
        irb_ble_handler_gap_unpair_cmd,
        irb_ble_handler_gap_set_sec_level_cmd,
#if (dg_configBLE_SKIP_LATENCY_API == 1)
        irb_ble_handler_gap_skip_latency_cmd,
#endif /* (dg_configBLE_SKIP_LATENCY_API == 1) */
        irb_ble_handler_gap_data_length_set_cmd,
#if (dg_configBLE_SECURE_CONNECTIONS == 1)
        irb_ble_handler_gap_numeric_reply_cmd,
#endif /* (dg_configBLE_SECURE_CONNECTIONS == 1) */
};

static const irb_ble_handler_t h_gatts[IRB_BLE_GET_IDX(IRB_BLE_LAST_GATTS)] = {
        irb_ble_handler_gatts_service_add_cmd,
        irb_ble_handler_gatts_service_add_include_cmd,
        irb_ble_handler_gatts_service_add_characteristic_cmd,
        irb_ble_handler_gatts_service_add_descriptor_cmd,
        irb_ble_handler_gatts_service_register_cmd,
        irb_ble_handler_gatts_service_enable_cmd,
        irb_ble_handler_gatts_service_disable_cmd,
        irb_ble_handler_gatts_service_characteristic_get_prop_cmd,
        irb_ble_handler_gatts_service_characteristic_set_prop_cmd,
        irb_ble_handler_gatts_get_value_cmd,
        irb_ble_handler_gatts_set_value_cmd,
        irb_ble_handler_gatts_read_cfm_cmd,
        irb_ble_handler_gatts_write_cfm_cmd,
        irb_ble_handler_gatts_prepare_write_cfm_cmd,
        irb_ble_handler_gatts_send_event_cmd,
        irb_ble_handler_gatts_service_changed_ind_cmd,
};

static const irb_ble_handler_t h_gattc[IRB_BLE_GET_IDX(IRB_BLE_LAST_GATTC)] = {
        irb_ble_handler_gattc_browse_cmd,
        irb_ble_handler_gattc_discover_svc_cmd,
        irb_ble_handler_gattc_discover_include_cmd,
        irb_ble_handler_gattc_discover_char_cmd,
        irb_ble_handler_gattc_discover_desc_cmd,
        irb_ble_handler_gattc_read_cmd,
        irb_ble_handler_gattc_write_generic_cmd,
        irb_ble_handler_gattc_write_execute_cmd,
        irb_ble_handler_gattc_exchange_mtu_cmd,
};

static const irb_ble_handler_t h_l2cap[IRB_BLE_GET_IDX(IRB_BLE_LAST_L2CAP)] = {
        irb_ble_handler_l2cap_listen_cmd,
        irb_ble_handler_l2cap_stop_listen_cmd,
        irb_ble_handler_l2cap_connect_cmd,
        irb_ble_handler_l2cap_disconnect_cmd,
        irb_ble_handler_l2cap_add_credits_cmd,
        irb_ble_handler_l2cap_send_cmd,
};
#endif

static const irb_ble_handler_t* handlers[IRB_BLE_CAT_LAST] = {
        h_common,
#ifndef BLE_STACK_PASSTHROUGH_MODE
        h_gap,
        h_gatts,
        h_gattc,
        h_l2cap,
#endif
};

static const uint8_t handlers_num[IRB_BLE_CAT_LAST] = {

        IRB_BLE_GET_IDX(IRB_BLE_LAST_COMMON),
#ifndef BLE_STACK_PASSTHROUGH_MODE
        IRB_BLE_GET_IDX(IRB_BLE_LAST_GAP),
        IRB_BLE_GET_IDX(IRB_BLE_LAST_GATTS),
        IRB_BLE_GET_IDX(IRB_BLE_LAST_GATTC),
        IRB_BLE_GET_IDX(IRB_BLE_LAST_L2CAP),
#endif
};

bool ble_irb_handle_msg(OS_IRB *irb)
{
        irb_ble_hdr_t *hdr;
        uint8_t cat, idx;
        const irb_ble_handler_t *h;

        hdr = (irb_ble_hdr_t *) irb->ptr_buf;
        cat = IRB_BLE_GET_CAT(hdr->op_code);
        idx = IRB_BLE_GET_IDX(hdr->op_code);

        /* Make sure the message has a valid category and ID */
        OS_ASSERT(cat < IRB_BLE_CAT_LAST);
        OS_ASSERT(idx < handlers_num[cat]);
	(void)handlers_num;

        h = handlers[cat];
        h += idx;

        if (!(*h)) {
                /* No handler provided */
                return false;
        }

        (*h)(irb);

        return true;
}
