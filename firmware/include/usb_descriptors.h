#ifndef USB_DESCRIPTORS_H_
#define USB_DESCRIPTORS_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------------------------------------------
// HID REPORT DEFINITIONS (USB-IF Power Device Class 0x84 / 0x85)
//--------------------------------------------------------------------

#define REPORT_ID_UPS_STATUS        0x01
#define REPORT_ID_UPS_CONFIG        0x02

// PresentStatus bitfield masks
#define UPS_STATUS_AC_PRESENT       (1 << 0) // 0x85, 0xD0
#define UPS_STATUS_CHARGING         (1 << 1) // 0x85, 0x44
#define UPS_STATUS_DISCHARGING      (1 << 2) // 0x85, 0x45
#define UPS_STATUS_FULLY_CHARGED    (1 << 3) // 0x85, 0x46
#define UPS_STATUS_BELOW_LIMIT      (1 << 4) // 0x85, 0x42 (Warning < 20%)
#define UPS_STATUS_SHUTDOWN_IMMINENT (1 << 5) // 0x84, 0x69 (Critical < 5%, initiate shutdown)
#define UPS_STATUS_BATTERY_PRESENT  (1 << 6) // 0x85, 0xD1
#define UPS_STATUS_FAULT            (1 << 7) // 0x85, 0x4B (Fault / Over-temp)

// UPS Input Report (Sent periodically and on state change)
typedef struct __attribute__((packed)) {
    uint8_t report_id;          // REPORT_ID_UPS_STATUS (0x01)
    uint8_t present_status;     // Bitfield (see UPS_STATUS_* above)
    uint8_t remaining_capacity; // 0 - 100 % (0x85, 0x66)
    uint16_t run_time_to_empty; // Seconds remaining (0x85, 0x68)
    uint16_t voltage_mv;        // Battery pack voltage in mV (0x85, 0xBB)
} hid_ups_status_report_t;

// UPS Feature Report (Static / Config values)
typedef struct __attribute__((packed)) {
    uint8_t report_id;          // REPORT_ID_UPS_CONFIG (0x02)
    uint16_t config_voltage_mv; // Nominal voltage: 7400 mV (0x84, 0x40)
    uint16_t design_capacity_mah; // Design capacity: 3000 mAh (0x85, 0x67)
} hid_ups_config_report_t;

// Configuration descriptor endpoint numbers
enum {
    ITF_NUM_HID = 0,
    ITF_NUM_CDC,
    ITF_NUM_CDC_DATA,
    ITF_NUM_TOTAL
};

#define EPNUM_HID_IN      0x81
#define EPNUM_CDC_NOTIF   0x82
#define EPNUM_CDC_OUT     0x02
#define EPNUM_CDC_IN      0x83

// Descriptor size calculations
#define CONFIG_TOTAL_LEN  (9 + 9 + 9 + 7 + 9 + 5 + 5 + 4 + 7 + 7)

extern const uint8_t hid_report_descriptor[];
extern const uint16_t hid_report_descriptor_len;

#ifdef __cplusplus
}
#endif

#endif /* USB_DESCRIPTORS_H_ */
