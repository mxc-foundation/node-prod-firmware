#include <stdio.h>

#include <ad_gpadc.h>
#include <ad_ble.h>
#include <ad_nvms.h>
#include <ble_l2cap.h>
#include <ble_mgr.h>
#include <dis.h>
#include <dlg_suota.h>
#include <ias.h>
#include <lls.h>
#include <sys_watchdog.h>
#include <tps.h>
#include <util/list.h>
#include "sw_version.h"

#include "lmic/lmic.h"
#include "lmic/hal.h"
#include "ble.h"

//#define DEBUG

/*
 * 0x01 == BLE_APP_NOTIFY_MASK
 */
#define ALERT_TMO_NOTIF		0x02
#define ADV_TMO_NOTIF		0x04
#define UPDATE_CONN_PARAM_NOTIF	0x08

struct device {
	struct device	*next;
	bd_address_t	 addr;
};

typedef struct {
	void		*next;
	OS_TIMER	 param_timer;
	OS_TASK		 current_task;
	uint16_t	 conn_idx;
	bool		 expired;
} conn_dev_t;

#define STATUS_BLE_ON		0x01
#define STATUS_SUOTA_ONGOING	0x02
PRIVILEGED_DATA static uint8_t	 status;
PRIVILEGED_DATA static OS_TIMER	 adv_tim;
PRIVILEGED_DATA static OS_TIMER	 alert_tim;
PRIVILEGED_DATA static void	*reconnection_list;
PRIVILEGED_DATA static void	*param_connections;

static const uint8_t adv_data[] = {
	0x07, GAP_DATA_TYPE_UUID16_LIST_INC,
	0x03, 0x18, // = 0x1803 (LLS UUID)
	0x02, 0x18, // = 0x1802 (IAS UUID)
	0xF5, 0xFE, // = 0xFEF5 (DIALOG SUOTA UUID)
};

INITIALISED_PRIVILEGED_DATA static char	device_name[] = "MatchX 00000";
INITIALISED_PRIVILEGED_DATA static char	serial_number[] = "00000";

static const dis_device_info_t	dis_info = {
	.manufacturer	  = "MatchX GmbH",
	.model_number	  = "MX1731",
	.serial_number  = serial_number,
	.hw_revision	  = "Rev.A",
	.fw_revision	  = "1.0",
	.sw_revision	  = BLACKORCA_SW_VERSION,
};

bool
ble_is_suota_ongoing()
{
	return !!(status & STATUS_SUOTA_ONGOING);
}

static bool
suota_ready_cb(void)
{
	status |= STATUS_SUOTA_ONGOING;
#ifdef DEBUG
	printf("suota ongoing\r\n");
#endif
	return true;
}

static void
suota_status_cb(uint8_t status, uint8_t error_code)
{
	(void)error_code;
#ifdef DEBUG
	printf("suota status %d\r\n", status);
#endif
	if (status != SUOTA_ERROR)
		return;
}

static const suota_callbacks_t	suota_cb = {
	.suota_ready	= suota_ready_cb,
	.suota_status	= suota_status_cb,
};

static void
alert_tim_cb(OS_TIMER timer)
{
	OS_TASK	task = (OS_TASK)OS_TIMER_GET_TIMER_ID(timer);

	OS_TASK_NOTIFY(task, ALERT_TMO_NOTIF, OS_NOTIFY_SET_BITS);
}

static void
adv_tim_cb(OS_TIMER timer)
{
	OS_TASK	task = (OS_TASK)OS_TIMER_GET_TIMER_ID(timer);

	OS_TASK_NOTIFY(task, ADV_TMO_NOTIF, OS_NOTIFY_SET_BITS);
}

static void
ias_alert_cb(uint16_t conn_idx, uint8_t level)
{
	(void)conn_idx;
	(void)level;
#ifdef DEBUG
	printf("ias alert %d for conn %d\r\n", level, conn_idx);
#endif
}

static void
lls_alert_cb(uint16_t conn_idx, const bd_address_t *address, uint8_t level)
{
	(void)conn_idx;
	(void)address;
#ifdef DEBUG
	printf("lls alert %d for conn %d\r\n", level, conn_idx);
#endif
	if (level == 0)
		return;
	//XXX list_add
	OS_TIMER_RESET(alert_tim, OS_TIMER_FOREVER);
	OS_TIMER_RESET(adv_tim, OS_TIMER_FOREVER);
	ble_gap_adv_intv_set(BLE_ADV_INTERVAL_FROM_MS(20),
	    BLE_ADV_INTERVAL_FROM_MS(30));
	ble_gap_adv_stop();
}

static void
do_alert(uint8_t level)
{
	(void)level;
#ifdef DEBUG
	printf("do_alert %d\r\n", level);
#endif
}

static bool
conn_params_match(const void *elem, const void *ud)
{
	return ((conn_dev_t *)elem)->conn_idx == (uint16_t)(uint32_t)ud;
}

static bool
device_match_addr(const void *elem, const void *ud)
{
	return memcmp(&((const struct device *)elem)->addr, ud,
	    sizeof(bd_address_t)) == 0;
}

static void
conn_params_timer_cb(OS_TIMER timer)
{
	conn_dev_t	*conn_dev;

	conn_dev = (conn_dev_t *)OS_TIMER_GET_TIMER_ID(timer);
	conn_dev = list_find(param_connections, conn_params_match,
	    (const void *)(uint32_t)conn_dev->conn_idx);
	if (conn_dev) {
		conn_dev->expired = true;
		OS_TASK_NOTIFY(conn_dev->current_task, UPDATE_CONN_PARAM_NOTIF,
		    OS_NOTIFY_SET_BITS);
	}
}

static void
handle_evt_gap_connected(ble_evt_gap_connected_t *evt)
{
	struct device	*dev;
	conn_dev_t	*conn_dev;

	conn_dev = OS_MALLOC(sizeof(*conn_dev));
	if (conn_dev) {
		conn_dev->conn_idx = evt->conn_idx;
		conn_dev->expired = false;
		conn_dev->current_task = OS_GET_CURRENT_TASK();
		conn_dev->param_timer = OS_TIMER_CREATE("conn_param",
		    OS_MS_2_TICKS(5000), OS_TIMER_FAIL, (uint32_t)conn_dev,
		    conn_params_timer_cb);
		list_append(&param_connections, conn_dev);
		OS_TIMER_START(conn_dev->param_timer, OS_TIMER_FOREVER);
	}
	dev = list_unlink(&reconnection_list, device_match_addr,
	    &evt->peer_address);
	if (dev) {
		do_alert(0);
		list_free(&reconnection_list, NULL, NULL);
		OS_TIMER_STOP(alert_tim, OS_TIMER_FOREVER);
		OS_TIMER_STOP(adv_tim, OS_TIMER_FOREVER);
		ble_gap_adv_intv_set(BLE_ADV_INTERVAL_FROM_MS(1000),
		    BLE_ADV_INTERVAL_FROM_MS(1500));
		ble_gap_adv_stop();
	}
}

static void
handle_evt_gap_disconnected(ble_evt_gap_disconnected_t *evt)
{
	conn_dev_t	*conn_dev;

	conn_dev = list_unlink(&param_connections, conn_params_match,
	    (const void *)(uint32_t)evt->conn_idx);
	if (conn_dev) {
		OS_TIMER_DELETE(conn_dev->param_timer, OS_TIMER_FOREVER);
		OS_FREE(conn_dev);
	}
	ble_gap_adv_intv_set(BLE_ADV_INTERVAL_FROM_MS(20),
	    BLE_ADV_INTERVAL_FROM_MS(30));
	ble_gap_adv_stop();
	OS_TIMER_START(adv_tim, OS_TIMER_FOREVER);
}

static void
handle_evt_gap_adv_completed(void)
{
	if (!(status & STATUS_SUOTA_ONGOING))
		ble_gap_adv_start(GAP_CONN_MODE_UNDIRECTED);
}

static void
handle_evt_gap_pair_req(ble_evt_gap_pair_req_t *evt)
{
	ble_gap_pair_reply(evt->conn_idx, true, evt->bond);
}

static void
ble_param_init(void)
{
	uint8_t	eui[8];

	os_getDevEui(eui);
	if (snprintf(serial_number, sizeof(serial_number), "%05d",
		    (eui[1] << 8) | eui[0]) >= (int)sizeof(serial_number)) {
		return;
	}
	memcpy(device_name + sizeof(device_name) - sizeof(serial_number),
	    serial_number, sizeof(serial_number));
}

static void
ble_task_func(void *params)
{
	uint8_t		scan_rsp[BLE_SCAN_RSP_LEN_MAX];
	ble_service_t	*suota;
#if dg_configUSE_WDOG
	int8_t		wdog_id;

	(void)params;
	wdog_id = sys_watchdog_register(false);
#endif
#ifdef DEBUG
	printf("ble init\r\n");
#endif
	ble_param_init();
	ble_mgr_init();
	ble_peripheral_start();
	ble_register_app();
	ble_gap_mtu_size_set(512);
	ble_gap_device_name_set(device_name, ATT_PERM_READ);
	scan_rsp[0] = sizeof(device_name) - 1 + 1;
	scan_rsp[1] = GAP_DATA_TYPE_LOCAL_NAME;
	memcpy(scan_rsp + 2, device_name, sizeof(device_name) - 1);
	ble_service_add(ias_init(ias_alert_cb));
	ble_service_add(lls_init(lls_alert_cb));
	tps_init(0);
	suota = suota_init(&suota_cb);
	ble_service_add(suota);
	dis_init(NULL, &dis_info);
	alert_tim = OS_TIMER_CREATE("lls", OS_MS_2_TICKS(15000), OS_TIMER_FAIL,
	    (void *)OS_GET_CURRENT_TASK(), alert_tim_cb);
	adv_tim = OS_TIMER_CREATE("adv", OS_MS_2_TICKS(30000), OS_TIMER_FAIL,
	    (void *)OS_GET_CURRENT_TASK(), adv_tim_cb);
	ble_gap_adv_intv_set(BLE_ADV_INTERVAL_FROM_MS(20),
	    BLE_ADV_INTERVAL_FROM_MS(30));
	ble_gap_adv_data_set(sizeof(adv_data), adv_data,
	    sizeof(device_name) - 1 + 2, scan_rsp);
	ble_gap_adv_start(GAP_CONN_MODE_UNDIRECTED);
	OS_TIMER_START(adv_tim, OS_TIMER_FOREVER);
	for (;;) {
		uint32_t	notif;

#ifdef DEBUG
		printf("loop\r\n");
#endif
		sys_watchdog_notify(wdog_id);
		sys_watchdog_suspend(wdog_id);
		OS_TASK_NOTIFY_WAIT(0, OS_TASK_NOTIFY_ALL_BITS, &notif,
		    OS_TASK_NOTIFY_FOREVER);
		sys_watchdog_notify_and_resume(wdog_id);
#ifdef DEBUG
		printf("notif %#lx\r\n", notif);
#endif
		if (notif & BLE_APP_NOTIFY_MASK) {
			ble_evt_hdr_t	*hdr;

			if ((hdr = ble_get_event(false)) != NULL) {
				if (!ble_service_handle_event(hdr)) {
#ifdef DEBUG
					printf("evt %#x\r\n", hdr->evt_code);
#endif
					switch (hdr->evt_code) {
					case BLE_EVT_GAP_CONNECTED:
#ifdef DEBUG
						printf("gap connected\r\n");
#endif
						handle_evt_gap_connected((ble_evt_gap_connected_t *)hdr);
						break;
					case BLE_EVT_GAP_DISCONNECTED:
#ifdef DEBUG
						printf("gap disconnected\r\n");
#endif
						handle_evt_gap_disconnected((ble_evt_gap_disconnected_t *)hdr);
						break;
					case BLE_EVT_GAP_ADV_COMPLETED:
#ifdef DEBUG
						printf("gap adv completed\r\n");
#endif
						handle_evt_gap_adv_completed();
						break;
					case BLE_EVT_GAP_PAIR_REQ:
#ifdef DEBUG
						printf("gap pair req\r\n");
#endif
						handle_evt_gap_pair_req((ble_evt_gap_pair_req_t *)hdr);
						break;
					case BLE_EVT_L2CAP_CONNECTED:
					case BLE_EVT_L2CAP_DISCONNECTED:
					case BLE_EVT_L2CAP_DATA_IND:
#ifdef DEBUG
						printf("l2cap %d\r\n", hdr->evt_code & 0xff);
#endif
						suota_l2cap_event(suota, hdr);
						break;
					default:
#ifdef DEBUG
						printf("default event\r\n");
#endif
						ble_handle_event_default(hdr);
						break;
					}
				}
				OS_FREE(hdr);
			}
			if (ble_has_event()) {
#ifdef DEBUG
				printf("has event\r\n");
#endif
				OS_TASK_NOTIFY(OS_GET_CURRENT_TASK(),
				    BLE_APP_NOTIFY_MASK, OS_NOTIFY_SET_BITS);
			}
		}
		if (notif & ALERT_TMO_NOTIF) {
#ifdef DEBUG
			printf("alert tmo\r\n");
#endif
			do_alert(0);
			//XXX list_free
		}
		if (notif & ADV_TMO_NOTIF) {
#ifdef DEBUG
			printf("adv tmo\r\n");
#endif
			ble_gap_adv_intv_set(BLE_ADV_INTERVAL_FROM_MS(1000),
			    BLE_ADV_INTERVAL_FROM_MS(1500));
			ble_gap_adv_stop();
		}
		if (notif & UPDATE_CONN_PARAM_NOTIF) {
			conn_dev_t *conn_dev = param_connections;

			if (conn_dev && conn_dev->expired) {
				param_connections = conn_dev->next;
				OS_TIMER_DELETE(conn_dev->param_timer,
				    OS_TIMER_FOREVER);
				OS_FREE(conn_dev);
				if (param_connections) {
					OS_TASK_NOTIFY(OS_GET_CURRENT_TASK(),
					    UPDATE_CONN_PARAM_NOTIF,
					    OS_NOTIFY_SET_BITS);
				}
			}
		}
	}
}

void
ble_on(void)
{
	PRIVILEGED_DATA static OS_TASK	ble_handle;

	if (status & STATUS_BLE_ON)
		return;
	OS_TASK_CREATE("BLE & SUOTA", ble_task_func, (void *)0,
	    1024, OS_TASK_PRIORITY_NORMAL + 1, ble_handle);
	status |= STATUS_BLE_ON;
}
