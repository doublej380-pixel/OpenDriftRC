#pragma once
#include "esp_netif.h"
#include "esp_wifi.h"
struct esp_netif_sta_list_t { int num=0; };
inline int esp_netif_get_sta_list(wifi_sta_list_t*,esp_netif_sta_list_t* n) { n->num=0; return ESP_OK; }
