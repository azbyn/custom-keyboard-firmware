#include <stdint.h>

#include <bsp/board_api.h>
#include <tusb.h>

#include "usb_hid.h"
#include "usb_cdc.h"

void tud_hid_report_complete_cb(uint8_t instance, uint8_t const* report, uint16_t len) {
    UsbHid::getInstance().tud_hid_report_complete_cb(instance, report, len);
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id,
        hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) {
    return UsbHid::getInstance().tud_hid_get_report_cb(instance, report_id, report_type, buffer, reqlen);
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {
    return UsbHid::getInstance().tud_hid_set_report_cb(instance, report_id, report_type, buffer, bufsize);
}

// Called when data arrives on the CDC OUT endpoint
void tud_cdc_rx_cb(uint8_t itf) {
    UsbCdc::getInstance().tud_cdc_rx_cb(itf);
}

RepeatReportType UsbHid::getRepeatReportType() const {
    return KeyboardStateMachine::getStateSnapshot().repeatReportType;
}




#define _PID_MAP(itf, n)  ( (CFG_TUD_##itf) << (n) )
#define USB_PID           (0x4000 | _PID_MAP(CDC, 0) | _PID_MAP(MSC, 1) | _PID_MAP(HID, 2) | \
                           _PID_MAP(MIDI, 3) | _PID_MAP(VENDOR, 4) )

constexpr uint16_t USB_VID = 0x415A; // vendor id
constexpr uint16_t USB_BCD = 0x0200; // ie usb2.0
// ----------------------------------------------------------------
// Endpoint numbers
// ----------------------------------------------------------------
#define EP_CDC_NOTIF    0x81    // CDC notification IN
#define EP_CDC_OUT      0x02    // CDC data OUT
#define EP_CDC_IN       0x82    // CDC data IN
#define EP_HID          0x83    // HID IN  (keyboard only needs IN)

//--------------------------------------------------------------------+
// String Descriptors
//--------------------------------------------------------------------+

// String Descriptor Index
enum {
  STRID_LANGID = 0,
  STRID_MANUFACTURER,
  STRID_PRODUCT,
  STRID_SERIAL,
  STRID_CDC_INTERFACE,
  STRID_HID_INTERFACE,
};

// array of pointer to string descriptors
constexpr char const *string_desc_arr[] = {
  (const char[]) { 0x09, 0x04 }, // 0: is supported language is English (0x0409)
  "Azbyn",                       // 1: Manufacturer
  "Custom keyboard",             // 2: Product
  NULL,                          // 3: Serials will use unique ID if possible
  "Keyboard Config Serial",      // 4: CDC interface name
  "Keyboard",                    // 5: HID interface name
};

//--------------------------------------------------------------------+
// Device Descriptors
//--------------------------------------------------------------------+
constexpr tusb_desc_device_t const desc_device = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = USB_BCD,
    .bDeviceClass       = TUSB_CLASS_MISC,
    .bDeviceSubClass    = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol    = MISC_PROTOCOL_IAD,

    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,

    .idVendor           = USB_VID,
    .idProduct          = USB_PID,
    .bcdDevice          = 0x0100,

    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,

    .bNumConfigurations = 0x01
};

// Invoked when received GET DEVICE DESCRIPTOR
// Application return pointer to descriptor
uint8_t const * tud_descriptor_device_cb(void) {
  return (uint8_t const *) &desc_device;
}

//--------------------------------------------------------------------+
// HID Report Descriptor
//--------------------------------------------------------------------+

constexpr uint8_t desc_hid_report_6key[] = {
    HID_USAGE_PAGE ( HID_USAGE_PAGE_DESKTOP     )                    ,
    HID_USAGE      ( HID_USAGE_DESKTOP_KEYBOARD )                    ,
    HID_COLLECTION ( HID_COLLECTION_APPLICATION )                    ,
        /* Report ID  */
        HID_REPORT_ID(REPORT_ID_KEYBOARD         )
        /* 8 bits Modifier Keys (Shift, Control, Alt) */ 
        HID_USAGE_PAGE ( HID_USAGE_PAGE_KEYBOARD )                     ,
            HID_USAGE_MIN    ( 224                                    )  ,
            HID_USAGE_MAX    ( 231                                    )  ,
            HID_LOGICAL_MIN  ( 0                                      )  ,
            HID_LOGICAL_MAX  ( 1                                      )  ,
            HID_REPORT_COUNT ( 8                                      )  ,
            HID_REPORT_SIZE  ( 1                                      )  ,
            HID_INPUT        ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE )  ,
            /* 8 bit reserved */ 
            HID_REPORT_COUNT ( 1                                      )  ,
            HID_REPORT_SIZE  ( 8                                      )  ,
            HID_INPUT        ( HID_CONSTANT                           )  ,
        /* Output 5-bit LED Indicator Kana | Compose | ScrollLock | CapsLock | NumLock */ 
        HID_USAGE_PAGE  ( HID_USAGE_PAGE_LED                   )       ,
            HID_USAGE_MIN    ( 1                                       ) ,
            HID_USAGE_MAX    ( 5                                       ) ,
            HID_REPORT_COUNT ( 5                                       ) ,
            HID_REPORT_SIZE  ( 1                                       ) ,
            HID_OUTPUT       ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE  ) ,
            /* led padding */ 
            HID_REPORT_COUNT ( 1                                       ) ,
            HID_REPORT_SIZE  ( 3                                       ) ,
            HID_OUTPUT       ( HID_CONSTANT                            ) ,
        /* 6-byte Keycodes */ 
        HID_USAGE_PAGE ( HID_USAGE_PAGE_KEYBOARD )                     ,
            HID_USAGE_MIN    ( 0                                   )     ,
            HID_USAGE_MAX_N  ( 255, 2                              )     ,
            HID_LOGICAL_MIN  ( 0                                   )     ,
            HID_LOGICAL_MAX_N( 255, 2                              )     ,
            HID_REPORT_COUNT ( 6               )     ,
            HID_REPORT_SIZE  ( 8               )     ,
            HID_INPUT        ( HID_DATA | HID_ARRAY | HID_ABSOLUTE )     ,
        HID_COLLECTION_END ,
        TUD_HID_REPORT_DESC_CONSUMER( HID_REPORT_ID(REPORT_ID_CONSUMER_CONTROL )),

    //TUD_HID_REPORT_DESC_KEYBOARD( HID_REPORT_ID(REPORT_ID_KEYBOARD         )),
    /*TUD_HID_REPORT_DESC_MOUSE   ( HID_REPORT_ID(REPORT_ID_MOUSE            )),
    TUD_HID_REPORT_DESC_GAMEPAD ( HID_REPORT_ID(REPORT_ID_GAMEPAD          ))*/
};

constexpr uint8_t desc_hid_report_nkro[] = {
    HID_USAGE_PAGE ( HID_USAGE_PAGE_DESKTOP     )                    ,
    HID_USAGE      ( HID_USAGE_DESKTOP_KEYBOARD )                    ,
    HID_COLLECTION ( HID_COLLECTION_APPLICATION )                    ,
        /* Report ID  */
        HID_REPORT_ID(REPORT_ID_KEYBOARD         )
        /* 8 bits Modifier Keys (Shift, Control, Alt) */ 
        HID_USAGE_PAGE ( HID_USAGE_PAGE_KEYBOARD )                     ,
            HID_USAGE_MIN    ( 224                                    )  ,
            HID_USAGE_MAX    ( 231                                    )  ,
            HID_LOGICAL_MIN  ( 0                                      )  ,
            HID_LOGICAL_MAX  ( 1                                      )  ,
            HID_REPORT_COUNT ( 8                                      )  ,
            HID_REPORT_SIZE  ( 1                                      )  ,
            HID_INPUT        ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE )  ,
            /* 8 bit reserved */ 
            HID_REPORT_COUNT ( 1                                      )  ,
            HID_REPORT_SIZE  ( 8                                      )  ,
            HID_INPUT        ( HID_CONSTANT                           )  ,
        /* Output 5-bit LED Indicator Kana | Compose | ScrollLock | CapsLock | NumLock */ 
        HID_USAGE_PAGE  ( HID_USAGE_PAGE_LED                   )       ,
            HID_USAGE_MIN    ( 1                                       ) ,
            HID_USAGE_MAX    ( 5                                       ) ,
            HID_REPORT_COUNT ( 5                                       ) ,
            HID_REPORT_SIZE  ( 1                                       ) ,
            HID_OUTPUT       ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE  ) ,
            /* led padding */ 
            HID_REPORT_COUNT ( 1                                       ) ,
            HID_REPORT_SIZE  ( 3                                       ) ,
            HID_OUTPUT       ( HID_CONSTANT                            ) ,
        /* Bitmap of  Keycodes */ 
        HID_USAGE_PAGE ( HID_USAGE_PAGE_KEYBOARD )                     ,
            HID_USAGE_MIN    ( 0                                   )     ,
            HID_USAGE_MAX    ( UsbHid::keysNumKeys             )     ,

            // HID_USAGE_MAX_N  ( 255, 2                              )     ,
            HID_LOGICAL_MIN  ( 0                                   )     ,
            HID_LOGICAL_MAX  ( 1                              )     ,
            HID_REPORT_COUNT ( UsbHid::keysNumKeys+1                )     ,
            HID_REPORT_SIZE  ( 1 /*8*/            )     ,
            HID_INPUT        ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE )     ,
#if (MAX_KEY_BITMAP % 8) != 0
            /* hid padding */ 
            HID_REPORT_COUNT ( 1                                       ) ,
            HID_REPORT_SIZE  ( (7 - (UsbHid::keysNumKeys%8))           ) ,
            HID_INPUT       ( HID_CONSTANT                            ) ,
#endif
        HID_COLLECTION_END ,
        TUD_HID_REPORT_DESC_CONSUMER( HID_REPORT_ID(REPORT_ID_CONSUMER_CONTROL )),

    //TUD_HID_REPORT_DESC_KEYBOARD( HID_REPORT_ID(REPORT_ID_KEYBOARD         )),
    /*TUD_HID_REPORT_DESC_MOUSE   ( HID_REPORT_ID(REPORT_ID_MOUSE            )),
    TUD_HID_REPORT_DESC_GAMEPAD ( HID_REPORT_ID(REPORT_ID_GAMEPAD          ))*/
};

// Invoked when received GET HID REPORT DESCRIPTOR
// Application return pointer to descriptor
// Descriptor contents must exist long enough for transfer to complete
uint8_t const * tud_hid_descriptor_report_cb(uint8_t instance) {
    (void) instance;
    return KeyboardStateMachine::getStateSnapshot().nkro ?
        desc_hid_report_nkro : desc_hid_report_6key;
}

//--------------------------------------------------------------------+
// Configuration Descriptor
//--------------------------------------------------------------------+
constexpr size_t expectedCurrCons = 100;//mA

template<typename T, typename... Args>
auto make_array(Args&&... args) -> std::array<T, sizeof...(Args)> {
    return {static_cast<T>(args)...};
}

namespace ConfigurationDescriptorWithCdc {
    enum {
        ITF_NUM_CDC = 0,
        ITF_NUM_CDC_DATA,   // CDC requires two interfaces (control + data)
        ITF_NUM_HID,
        ITF_NUM_TOTAL
    };

    static constexpr size_t CONFIG_TOTAL_LEN = (TUD_CONFIG_DESC_LEN +TUD_CDC_DESC_LEN + TUD_HID_DESC_LEN);

#define M(descriptor) {\
    TUD_CONFIG_DESCRIPTOR(\
        1, /* Config number*/\
        ITF_NUM_TOTAL, /*interface count*/\
        0, /*string index,*/\
        CONFIG_TOTAL_LEN, /* total length, */\
        TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, /*attr*/ \
        expectedCurrCons/*power in mA*/ \
    ),\
    TUD_CDC_DESCRIPTOR(\
        ITF_NUM_CDC, /*ift_num*/ \
        STRID_CDC_INTERFACE,/*strid*/ \
        EP_CDC_NOTIF, /*ep_notif*/ \
        8, /*ep_notif_size,*/ \
        EP_CDC_OUT, /*ep_out*/ \
        EP_CDC_IN, /*ep_in*/ \
        64 /*ep_size*/ \
    ),\
    TUD_HID_DESCRIPTOR(\
        ITF_NUM_HID, /* Interface number, */ \
        STRID_HID_INTERFACE, /*string index,*/ \
        HID_ITF_PROTOCOL_NONE, /*protocol*/ \
        sizeof(descriptor), /*report descriptor len,*/ \
        EP_HID, /* EP In address,*/ \
        CFG_TUD_HID_EP_BUFSIZE,/*size*/ \
        1/* polling interval in ms*/ \
    )}

constexpr uint8_t const desc_configuration_nkro[] = M(desc_hid_report_nkro);
constexpr uint8_t const desc_configuration_6kro[] = M(desc_hid_report_6key);

#undef M

};


namespace ConfigurationDescriptorWithoutCdc {
    enum {
        ITF_NUM_HID,
        ITF_NUM_TOTAL
    };
    static constexpr size_t CONFIG_TOTAL_LEN = (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN);

#define M(descriptor) {\
    TUD_CONFIG_DESCRIPTOR(\
        1, /* Config number*/ \
        ITF_NUM_TOTAL, /*interface count,*/ \
        0, /*string index,*/ \
        CONFIG_TOTAL_LEN, /* total length,*/ \
        TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, /*attr*/ \
        expectedCurrCons/*power in mA*/ \
    ),\
\
    TUD_HID_DESCRIPTOR(\
        ITF_NUM_HID, /* Interface number, */ \
        STRID_HID_INTERFACE, /*string index,*/ \
        HID_ITF_PROTOCOL_NONE, /*protocol*/ \
        sizeof(descriptor), /*report descriptor len,*/ \
        EP_HID, /* EP In address,*/ \
        CFG_TUD_HID_EP_BUFSIZE,/*size*/ \
        1/* polling interval in ms*/ \
    )}
    
constexpr uint8_t const desc_configuration_nkro[] = M(desc_hid_report_nkro);
constexpr uint8_t const desc_configuration_6kro[] = M(desc_hid_report_6key);
#undef M
};


#if TUD_OPT_HIGH_SPEED
// Per USB specs: high speed capable device must report device_qualifier and other_speed_configuration

// other speed configuration
uint8_t desc_other_speed_config[CONFIG_TOTAL_LEN];

// device qualifier is mostly similar to device descriptor since we don't change configuration based on speed
tusb_desc_device_qualifier_t const desc_device_qualifier = {
    .bLength            = sizeof(tusb_desc_device_qualifier_t),
    .bDescriptorType    = TUSB_DESC_DEVICE_QUALIFIER,
    .bcdUSB             = USB_BCD,

    // .bDeviceClass       = 0x00,
    // .bDeviceSubClass    = 0x00,
    // .bDeviceProtocol    = 0x00,

    .bDeviceClass       = TUSB_CLASS_MISC,
    .bDeviceSubClass    = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol    = MISC_PROTOCOL_IAD,

    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
    .bNumConfigurations = 0x01,
    .bReserved          = 0x00
};

// Invoked when received GET DEVICE QUALIFIER DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long enough for transfer to complete.
// device_qualifier descriptor describes information about a high-speed capable device that would
// change if the device were operating at the other speed. If not highspeed capable stall this request.
uint8_t const* tud_descriptor_device_qualifier_cb(void){
    return (uint8_t const*) &desc_device_qualifier;
}

// Invoked when received GET OTHER SEED CONFIGURATION DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long enough for transfer to complete
// Configuration descriptor in the other speed e.g if high speed then this is for full speed and vice versa
uint8_t const* tud_descriptor_other_speed_configuration_cb(uint8_t index) {
    (void) index; // for multiple configurations

    // other speed config is basically configuration with type = OHER_SPEED_CONFIG
    memcpy(desc_other_speed_config, desc_configuration, CONFIG_TOTAL_LEN);
    desc_other_speed_config[1] = TUSB_DESC_OTHER_SPEED_CONFIG;

    // this example use the same configuration for both high and full speed mode
    return desc_other_speed_config;
}

#endif // highspeed

// Invoked when received GET CONFIGURATION DESCRIPTOR
// Application return pointer to descriptor
// Descriptor contents must exist long enough for transfer to complete
uint8_t const * tud_descriptor_configuration_cb(uint8_t index) {
    (void) index; // for multiple configurations

    // This example use the same configuration for both high and full speed mode
    bool nkro = KeyboardStateMachine::getStateSnapshot().nkro;
    if (UsbCdc::getInstance().isCdcEnabled())
        return nkro ? ConfigurationDescriptorWithCdc::desc_configuration_nkro : ConfigurationDescriptorWithCdc::desc_configuration_6kro;
    return nkro ? ConfigurationDescriptorWithoutCdc::desc_configuration_nkro : ConfigurationDescriptorWithoutCdc::desc_configuration_6kro;
}

void tud_hid_set_protocol_cb(uint8_t instance, uint8_t protocol) {
    (void) instance;
    //can modify it the raw way, since it's called before the descriptor is requested
    if (protocol == HID_PROTOCOL_BOOT) 
        KeyboardStateMachine::getMutableStateSnapshot().nkro = false;
    if (protocol == HID_PROTOCOL_REPORT) 
        KeyboardStateMachine::getMutableStateSnapshot().nkro = true;
}

static uint16_t _desc_str[32 + 1];

// Invoked when received GET STRING DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long enough for transfer to complete
uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    //TODO if we get in that, set the mode to windows
    (void) langid;
    size_t chr_count;

    switch ( index ) {
        case STRID_LANGID:
            memcpy(&_desc_str[1], string_desc_arr[0], 2);
            chr_count = 1;
            break;

        case STRID_SERIAL:
            chr_count = board_usb_get_serial(_desc_str + 1, 32);
            break;

        default:
            // Note: the 0xEE index string is a Microsoft OS 1.0 Descriptors.
            // https://docs.microsoft.com/en-us/windows-hardware/drivers/usbcon/microsoft-defined-usb-descriptors

            if ( !(index < sizeof(string_desc_arr) / sizeof(string_desc_arr[0])) ) return NULL;
            if (index > 10)
                display_printf("W?DESC_%d;", index);

            const char *str = string_desc_arr[index];

            // Cap at max char
            chr_count = strlen(str);
            size_t const max_count = sizeof(_desc_str) / sizeof(_desc_str[0]) - 1; // -1 for string type
            if ( chr_count > max_count ) chr_count = max_count;

            // Convert ASCII string into UTF-16
            for ( size_t i = 0; i < chr_count; i++ ) {
                _desc_str[1 + i] = str[i];
            }
            break;
    }

    // first byte is length (including header), second byte is string type
    _desc_str[0] = (uint16_t) ((TUSB_DESC_STRING << 8) | (2 * chr_count + 2));

    return _desc_str;
}
void tud_mount_cb(void) {
    KeyboardStateMachine::getInstance().onUsbMount(true);
    UsbHid::getInstance().mountState = UsbHid::S_On;

    // KeyboardStateMachine::getInstance().onUsbSuspendMode(false);

    display_printf("mount");
}
void tud_umount_cb(void) {
    KeyboardStateMachine::getInstance().onUsbMount(false);
    display_printf("unmount");

    UsbHid::getInstance().mountState = UsbHid::S_Off;
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en) {
    KeyboardStateMachine::getInstance().onUsbSuspendMode(true);
    display_printf("SUSP_%d", remote_wakeup_en);

    UsbHid::getInstance().mountState = remote_wakeup_en ? UsbHid::S_SuspendedWakeable : UsbHid::S_SuspendedUnwakeable;
}
void tud_resume_cb(void) {
    KeyboardStateMachine::getInstance().onUsbSuspendMode(false);
    display_printf("RESUME");
    UsbHid::getInstance().mountState = tud_mounted() ? UsbHid::S_On : UsbHid::S_Unmounted;

    // KeyboardStateMachine::setDisplayState(DS_OFF);
}
