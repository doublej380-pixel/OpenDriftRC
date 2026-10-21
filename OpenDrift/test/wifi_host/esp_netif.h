#pragma once
using esp_err_t=int;
constexpr int ESP_OK=0, ESP_ERR_ESP_NETIF_DHCP_ALREADY_STOPPED=1,
    ESP_ERR_ESP_NETIF_DHCP_ALREADY_STARTED=2;
using esp_netif_t=int;
enum esp_netif_dhcp_status_t { ESP_NETIF_DHCP_INIT, ESP_NETIF_DHCP_STARTED };
inline esp_netif_t* esp_netif_get_handle_from_ifkey(const char*) { static int n; return &n; }
inline int esp_netif_dhcps_get_status(esp_netif_t*,esp_netif_dhcp_status_t* s) { *s=ESP_NETIF_DHCP_STARTED; return ESP_OK; }
inline int esp_netif_dhcps_start(esp_netif_t*) { return ESP_OK; }
inline int esp_netif_dhcps_stop(esp_netif_t*) { return ESP_OK; }
