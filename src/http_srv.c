/*
 * http_srv.c
 *
 *  Created on: 01.01.2018
 *      Author: dk
 */

#include "http_srv.h"

#include <mgos.h>
#include <mgos_sys_config.h>
#include <mgos_http_server.h>
#include <mongoose.h>
#include "midi_file.hpp"

static struct mg_serve_http_opts s_http_server_opts;

static void handle_save(struct mg_connection *nc, struct http_message *hm) {
//	/* Get form variables and store settings values */
//	//mg_get_http_var(&hm->body, "setting1", client_id, sizeof(client_id));
//	for (int i = 0; i < WIFI_STA_MAX; i++) {
//		char l_ssid[16], l_pwd[16];
//		snprintf(l_ssid, sizeof(l_ssid), "wifi_ssid_%d", i);
//		snprintf(l_pwd, sizeof(l_pwd), "wifi_pwd_%d", i);
//		mg_get_http_var(&hm->body, l_ssid, (char *) wifi_sta_config[i].sta.ssid, sizeof(wifi_sta_config[i].sta.ssid));
//		mg_get_http_var(&hm->body, l_pwd, (char *) wifi_sta_config[i].sta.password,
//				sizeof(wifi_sta_config[i].sta.password));
//	}
//	mg_get_http_var(&hm->body, "wifi_ap_ssid", (char *) wifi_ap_config.ap.ssid, sizeof(wifi_ap_config.ap.ssid));
//	mg_get_http_var(&hm->body, "wifi_ap_pwd", (char *) wifi_ap_config.ap.password, sizeof(wifi_ap_config.ap.password));
//
//	nvs_write();
//	ESP_LOGI("http_srv", "Settings updated");
//	/* Send response */
//	mg_printf(nc, "HTTP/1.1 200 OK\r\nContent-Length: %lu\r\n\r\n%.*s", (unsigned long) hm->body.len,
//			(int) hm->body.len, hm->body.p);
}

static void handle_play(struct mg_connection *nc, struct http_message *hm) {
	char filename[32];
	mg_get_http_var(&hm->body, "filename", filename, sizeof(filename));
	LOG(LL_INFO, ("%p %s", nc, filename));
	xTaskCreate(midi_file_play_task, "midi_file_play_task", 4096,
			filename, 20, &midi_file_play_handle);

	/* Send response */
	mg_printf(nc, "HTTP/1.1 200 OK\r\nContent-Length: %lu\r\n\r\n%.*s", (unsigned long) hm->body.len,
			(int) hm->body.len, hm->body.p);
	nc->flags |= MG_F_SEND_AND_CLOSE;
}

static void handle_ssi_call(struct mg_connection *nc, const char *param) {
	LOG(LL_INFO, ("%p %s", nc, param));
	if (strcmp(param, "wifi_sta_ssid") == 0) {
		mg_printf_html_escape(nc, "%s", mgos_sys_config_get_wifi_sta_ssid());
	} else if (strcmp(param, "wifi_sta_pass") == 0) {
		mg_printf_html_escape(nc, "%s", mgos_sys_config_get_wifi_sta_pass());
	}
}
//
static void ev_handler(struct mg_connection *nc, int ev, void *ev_data, void *user_data) {
	struct http_message *hm = (struct http_message *) ev_data;

	switch (ev) {
	case MG_EV_HTTP_REQUEST:
		if (mg_vcmp(&hm->uri, "/save") == 0) {
			handle_save(nc, hm); /* Handle RESTful call */
		} else if (mg_vcmp(&hm->uri, "/play") == 0) {
			handle_play(nc, hm);
		} else {
			LOG(LL_INFO, ("%p %.*s %.*s", nc, (int) hm->method.len, hm->method.p,
							(int) hm->uri.len, hm->uri.p));
			mg_serve_http(nc, ev_data, s_http_server_opts);
		}
		break;
	case MG_EV_SSI_CALL:
		handle_ssi_call(nc, ev_data);
		break;
	default:
		break;
	}
	(void) user_data;

}

void http_server_init() {
	s_http_server_opts.document_root = mgos_sys_config_get_http_document_root();
	s_http_server_opts.hidden_file_pattern =
			mgos_sys_config_get_http_hidden_files();
	s_http_server_opts.auth_domain = mgos_sys_config_get_http_auth_domain();
	s_http_server_opts.global_auth_file = mgos_sys_config_get_http_auth_file();
	s_http_server_opts.index_files = "index.shtml";

	mgos_register_http_endpoint("/", ev_handler, NULL);
}
