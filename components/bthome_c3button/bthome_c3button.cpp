#include "bthome_c3button.h"
#include <cstring>
#include "esp_bt.h"
#include "esp_attr.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace bthome_c3btn {

static const char *const TAG = "bthome_c3btn";
static constexpr uint8_t NUM_BUTTONS = 6;
static const char LOCAL_NAME[] = "C3 Btn";  // <= 6 chars to fit in 31 bytes

static RTC_DATA_ATTR uint8_t rtc_packet_id;
static RTC_DATA_ATTR uint32_t rtc_magic;

static volatile bool g_cmd_done = false;
static bool g_ready = false;
static bool g_adv_on = false;
static uint8_t g_buf[4 + 32];

static void vhci_ready_cb() {}
static int vhci_rx_cb(uint8_t *data, uint16_t len) {
  if (len >= 2 && data[0] == 0x04 && (data[1] == 0x0E || data[1] == 0x0F)) g_cmd_done = true;
  return 0;
}
static esp_vhci_host_callback_t g_cb = {vhci_ready_cb, vhci_rx_cb};

static bool hci_cmd(uint16_t opcode, const uint8_t *params, uint8_t plen) {
  g_buf[0] = 0x01;
  g_buf[1] = opcode & 0xFF;
  g_buf[2] = opcode >> 8;
  g_buf[3] = plen;
  if (plen) memcpy(&g_buf[4], params, plen);

  int64_t t0 = esp_timer_get_time();
  while (!esp_vhci_host_check_send_available()) {
    if (esp_timer_get_time() - t0 > 50000) { ESP_LOGW(TAG, "HCI busy (0x%04X)", opcode); return false; }
    vTaskDelay(1);
  }
  g_cmd_done = false;
  esp_vhci_host_send_packet(g_buf, plen + 4);
  t0 = esp_timer_get_time();
  while (!g_cmd_done) {
    if (esp_timer_get_time() - t0 > 100000) { ESP_LOGW(TAG, "HCI timeout (0x%04X)", opcode); return false; }
    vTaskDelay(1);
  }
  return true;
}

static bool begin() {
  if (g_ready) return true;
  if (rtc_magic != 0xB7B70002) {
    rtc_magic = 0xB7B70002;
    rtc_packet_id = (uint8_t) esp_random();
  }
  esp_bt_controller_config_t cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
  esp_err_t err = esp_bt_controller_init(&cfg);
  if (err != ESP_OK) { ESP_LOGE(TAG, "controller init: %s", esp_err_to_name(err)); return false; }
  err = esp_bt_controller_enable(ESP_BT_MODE_BLE);
  if (err != ESP_OK) { ESP_LOGE(TAG, "controller enable: %s", esp_err_to_name(err)); return false; }
  esp_vhci_host_register_callback(&g_cb);
  esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, ESP_PWR_LVL_P9);

  hci_cmd(0x0C03, nullptr, 0);  // HCI Reset
  // 20 ms interval, ADV_NONCONN_IND, public addr, all channels
  const uint8_t p[15] = {0x20, 0x00, 0x20, 0x00, 0x03, 0x00, 0x00,
                         0, 0, 0, 0, 0, 0, 0x07, 0x00};
  if (!hci_cmd(0x2006, p, sizeof(p))) return false;
  g_ready = true;
  return true;
}

void send_buttons(uint8_t mask) {
  if (!begin()) return;
  rtc_packet_id++;

  uint8_t ad[31] = {0};
  uint8_t i = 0;
  ad[i++] = 0x02; ad[i++] = 0x01; ad[i++] = 0x06;
  const uint8_t nlen = sizeof(LOCAL_NAME) - 1;
  ad[i++] = nlen + 1; ad[i++] = 0x09;
  memcpy(&ad[i], LOCAL_NAME, nlen); i += nlen;
  const uint8_t len_pos = i++;
  ad[i++] = 0x16; ad[i++] = 0xD2; ad[i++] = 0xFC;
  ad[i++] = 0x44;                              // BTHome v2, trigger-based
  ad[i++] = 0x00; ad[i++] = rtc_packet_id;     // packet id
  for (uint8_t b = 0; b < NUM_BUTTONS; b++) {
    ad[i++] = 0x3A;
    ad[i++] = (mask & (1u << b)) ? 0x01 : 0x00;
  }
  ad[len_pos] = i - len_pos - 1;

  uint8_t p[32] = {0};
  p[0] = i;
  memcpy(&p[1], ad, i);
  hci_cmd(0x2008, p, sizeof(p));
  if (!g_adv_on) {
    uint8_t en = 1;
    if (hci_cmd(0x200A, &en, 1)) g_adv_on = true;
  }
  ESP_LOGI(TAG, "Advertised mask 0x%02X (pid %u)", mask, rtc_packet_id);
}

void end() {
  if (g_adv_on) { 
    uint8_t en = 0; 
    hci_cmd(0x200A, &en, 1); 
    g_adv_on = false; 
  }
  if (g_ready) { 
    esp_bt_controller_disable(); 
    esp_bt_controller_deinit();  // Shut down the BT PHY/MAC
    g_ready = false; 
  }
}

}  // namespace bthome_c3btn