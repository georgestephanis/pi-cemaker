#ifndef MOCK_TUSB_H_
#define MOCK_TUSB_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CFG_TUD_ENDPOINT0_SIZE 64
#define CFG_TUD_HID_EP_BUFSIZE 64

#define TUSB_DESC_DEVICE 0x01
#define TUSB_DESC_CONFIGURATION 0x02
#define TUSB_DESC_STRING 0x03
#define TUSB_DESC_INTERFACE 0x04
#define TUSB_DESC_ENDPOINT 0x05
#define HID_ITF_PROTOCOL_NONE 0

typedef struct __attribute__((packed)) {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint16_t bcdUSB;
    uint8_t  bDeviceClass;
    uint8_t  bDeviceSubClass;
    uint8_t  bDeviceProtocol;
    uint8_t  bMaxPacketSize0;
    uint16_t idVendor;
    uint16_t idProduct;
    uint16_t bcdDevice;
    uint8_t  iManufacturer;
    uint8_t  iProduct;
    uint8_t  iSerialNumber;
    uint8_t  bNumConfigurations;
} tusb_desc_device_t;

#define TUD_CONFIG_DESCRIPTOR(config_num, _itfcount, _stridx, _total_len, _attribute, _power_ma) \
    9, TUSB_DESC_CONFIGURATION, ((_total_len) & 0xFF), (((_total_len) >> 8) & 0xFF), (_itfcount), (config_num), (_stridx), (_attribute), ((_power_ma)/2)

#define TUD_HID_DESCRIPTOR(_itfnum, _stridx, _boot_protocol, _report_desc_len, _epin, _epsize, _ep_interval) \
    9, TUSB_DESC_INTERFACE, (_itfnum), 0, 1, 0x03, 0x00, (_boot_protocol), (_stridx), \
    9, 0x21, 0x11, 0x01, 0x00, 1, 0x22, ((_report_desc_len) & 0xFF), (((_report_desc_len) >> 8) & 0xFF), \
    7, TUSB_DESC_ENDPOINT, (_epin), 0x03, ((_epsize) & 0xFF), (((_epsize) >> 8) & 0xFF), (_ep_interval)

#define TUD_CDC_DESCRIPTOR(_itfnum, _stridx, _ep_notif, _ep_notif_size, _ep_out, _ep_in, _ep_size) \
    9, TUSB_DESC_INTERFACE, (_itfnum), 0, 1, 0x02, 0x02, 0x00, (_stridx), \
    5, 0x24, 0x00, 0x10, 0x01, \
    5, 0x24, 0x01, 0x00, ((_itfnum)+1), \
    4, 0x24, 0x02, 0x02, \
    5, 0x24, 0x06, (_itfnum), ((_itfnum)+1), \
    7, TUSB_DESC_ENDPOINT, (_ep_notif), 0x03, ((_ep_notif_size) & 0xFF), (((_ep_notif_size) >> 8) & 0xFF), 16, \
    9, TUSB_DESC_INTERFACE, ((_itfnum)+1), 0, 2, 0x0A, 0x00, 0x00, 0, \
    7, TUSB_DESC_ENDPOINT, (_ep_out), 0x02, ((_ep_size) & 0xFF), (((_ep_size) >> 8) & 0xFF), 0, \
    7, TUSB_DESC_ENDPOINT, (_ep_in), 0x02, ((_ep_size) & 0xFF), (((_ep_size) >> 8) & 0xFF), 0

static inline bool tud_hid_ready(void) { return true; }
static inline bool tud_hid_report(uint8_t report_id, void const* report, uint8_t len) {
    (void)report_id; (void)report; (void)len; return true;
}
static inline bool tud_cdc_connected(void) { return false; }
static inline void tud_cdc_write_str(const char* str) { (void)str; }
static inline void tud_cdc_write_flush(void) {}
static inline bool tud_cdc_available(void) { return false; }
static inline uint32_t tud_cdc_read(void* buffer, uint32_t bufsize) {
    (void)buffer; (void)bufsize; return 0;
}
static inline void tusb_init(void) {}
static inline void tud_task(void) {}

typedef enum {
    HID_REPORT_TYPE_INVALID = 0,
    HID_REPORT_TYPE_INPUT,
    HID_REPORT_TYPE_OUTPUT,
    HID_REPORT_TYPE_FEATURE
} hid_report_type_t;

#ifdef __cplusplus
}
#endif

#endif /* MOCK_TUSB_H_ */
