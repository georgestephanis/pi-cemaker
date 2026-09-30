#include "tusb.h"
#include "usb_descriptors.h"

//--------------------------------------------------------------------+
// HID Report Descriptor (USB-IF Power Device 0x84 / Battery System 0x85)
//--------------------------------------------------------------------+

const uint8_t hid_report_descriptor[] = {
    0x05, 0x84,       // Usage Page (Power Device)
    0x09, 0x01,       // Usage (Power Device)
    0xA1, 0x01,       // Collection (Application)
    
    // --- Input Report 1: Real-Time UPS Status (7 bytes total with ID) ---
    0x85, REPORT_ID_UPS_STATUS, //   Report ID (1)
    
    // PresentStatus Bitmask (1 byte)
    0x09, 0x02,       //   Usage (PresentStatus)
    0xA1, 0x02,       //   Collection (Logical)
    0x05, 0x85,       //     Usage Page (Battery System)
    0x09, 0xD0,       //     Usage (ACPresent)                     - Bit 0
    0x09, 0x44,       //     Usage (Charging)                      - Bit 1
    0x09, 0x45,       //     Usage (Discharging)                   - Bit 2
    0x09, 0x46,       //     Usage (FullyCharged)                  - Bit 3
    0x09, 0x42,       //     Usage (BelowRemainingCapacityLimit)   - Bit 4
    0x05, 0x84,       //     Usage Page (Power Device)
    0x09, 0x69,       //     Usage (ShutdownImminent)              - Bit 5
    0x05, 0x85,       //     Usage Page (Battery System)
    0x09, 0xD1,       //     Usage (BatteryPresent)                - Bit 6
    0x09, 0x4B,       //     Usage (NeedReplacement / Fault)       - Bit 7
    0x15, 0x00,       //     Logical Minimum (0)
    0x25, 0x01,       //     Logical Maximum (1)
    0x75, 0x01,       //     Report Size (1)
    0x95, 0x08,       //     Report Count (8)
    0x81, 0x02,       //     Input (Data, Variable, Absolute)
    0xC0,             //   End Collection
    
    // RemainingCapacity (0 - 100 %) (1 byte)
    0x05, 0x85,       //   Usage Page (Battery System)
    0x09, 0x66,       //   Usage (RemainingCapacity)
    0x15, 0x00,       //   Logical Minimum (0)
    0x25, 0x64,       //   Logical Maximum (100)
    0x75, 0x08,       //   Report Size (8)
    0x95, 0x01,       //   Report Count (1)
    0x81, 0x02,       //   Input (Data, Variable, Absolute)
    
    // RunTimeToEmpty (Seconds) (2 bytes)
    0x09, 0x68,       //   Usage (RunTimeToEmpty)
    0x15, 0x00,       //   Logical Minimum (0)
    0x27, 0xFF, 0xFF, 0x00, 0x00, // Logical Maximum (65535)
    0x75, 0x10,       //   Report Size (16)
    0x95, 0x01,       //   Report Count (1)
    0x81, 0x02,       //   Input (Data, Variable, Absolute)
    
    // Voltage (Millivolts) (2 bytes)
    0x09, 0xBB,       //   Usage (Voltage)
    0x15, 0x00,       //   Logical Minimum (0)
    0x27, 0xFF, 0xFF, 0x00, 0x00, // Logical Maximum (65535)
    0x75, 0x10,       //   Report Size (16)
    0x95, 0x01,       //   Report Count (1)
    0x81, 0x02,       //   Input (Data, Variable, Absolute)

    // --- Feature Report 2: Static Device Configuration ---
    0x85, REPORT_ID_UPS_CONFIG, //   Report ID (2)
    
    // ConfigVoltage (Nominal 7400 mV)
    0x05, 0x84,       //   Usage Page (Power Device)
    0x09, 0x40,       //   Usage (ConfigVoltage)
    0x15, 0x00,       //   Logical Minimum (0)
    0x27, 0xFF, 0xFF, 0x00, 0x00, // Logical Maximum (65535)
    0x75, 0x10,       //   Report Size (16)
    0x95, 0x01,       //   Report Count (1)
    0xB1, 0x02,       //   Feature (Data, Variable, Absolute)
    
    // DesignCapacity (3000 mAh)
    0x05, 0x85,       //   Usage Page (Battery System)
    0x09, 0x67,       //   Usage (DesignCapacity)
    0x15, 0x00,       //   Logical Minimum (0)
    0x27, 0xFF, 0xFF, 0x00, 0x00, // Logical Maximum (65535)
    0x75, 0x10,       //   Report Size (16)
    0x95, 0x01,       //   Report Count (1)
    0xB1, 0x02,       //   Feature (Data, Variable, Absolute)

    0xC0              // End Collection
};

const uint16_t hid_report_descriptor_len = sizeof(hid_report_descriptor);

// Invoked when received GET HID REPORT DESCRIPTOR
uint8_t const * tud_hid_descriptor_report_cb(uint8_t instance) {
    (void) instance;
    return hid_report_descriptor;
}

//--------------------------------------------------------------------+
// Device Descriptor
//--------------------------------------------------------------------+

tusb_desc_device_t const desc_device = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200, // USB 2.0
    .bDeviceClass       = 0x00,   // Miscellaneous device / per-interface
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,

    .idVendor           = 0xCafe, // Development PID/VID
    .idProduct          = 0x4002, // Pi-cemaker UPS
    .bcdDevice          = 0x0100,

    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,

    .bNumConfigurations = 0x01
};

uint8_t const * tud_descriptor_device_cb(void) {
    return (uint8_t const *) &desc_device;
}

//--------------------------------------------------------------------+
// Configuration Descriptor (Composite HID + CDC)
//--------------------------------------------------------------------+

uint8_t const desc_configuration[] = {
    // Config number, interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x80, 100),

    // Interface 0: HID Power Device
    // Interface number, string index, protocol, report descriptor len, EP In & Out address, size & polling interval
    TUD_HID_DESCRIPTOR(ITF_NUM_HID, 4, HID_ITF_PROTOCOL_NONE, sizeof(hid_report_descriptor),
                       EPNUM_HID_IN, CFG_TUD_HID_EP_BUFSIZE, 10),

    // Interface 1 & 2: CDC Serial
    // Interface number, string index, EP notification address and size, EP data address (out, in) and size
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC, 5, EPNUM_CDC_NOTIF, 8, EPNUM_CDC_OUT, EPNUM_CDC_IN, 64)
};

uint8_t const * tud_descriptor_configuration_cb(uint8_t index) {
    (void) index;
    return desc_configuration;
}

//--------------------------------------------------------------------+
// String Descriptors
//--------------------------------------------------------------------+

static char const* string_desc_arr [] = {
    (const char[]) { 0x09, 0x04 }, // 0: Supported language is English (0x0409)
    "Web3 Pi",                     // 1: Manufacturer
    "Pi-cemaker 2S UPS",           // 2: Product
    "PCM-000001",                  // 3: Serial Number
    "Pi-cemaker HID Power Device", // 4: HID Interface
    "Pi-cemaker Virtual COM Port"  // 5: CDC Interface
};

static uint16_t _desc_str[32];

uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void) langid;
    uint8_t chr_count;

    if (index == 0) {
        memcpy(&_desc_str[1], string_desc_arr[0], 2);
        chr_count = 1;
    } else {
        if (!(index < sizeof(string_desc_arr)/sizeof(string_desc_arr[0]))) return NULL;

        const char* str = string_desc_arr[index];
        chr_count = (uint8_t) strlen(str);
        if (chr_count > 31) chr_count = 31;

        for (uint8_t i = 0; i < chr_count; i++) {
            _desc_str[1+i] = str[i];
        }
    }

    _desc_str[0] = (uint16_t) ((TUSB_DESC_STRING << 8) | (2 * chr_count + 2));
    return _desc_str;
}
