#pragma once
#include <stdint.h>
#include <tusb.h>

#include "utils.h"
#include "fixed_vector.h"

struct UsbCdc {
    // char commandBuffer[1024];
    // size_t cmdIdx = 0;
    FixedVector<char, 1024> commandBuffer;
    bool _cdcEnabled = false;
    UsbCdc() {}
public:
    static UsbCdc& getInstance(){
        static UsbCdc instance;
        return instance;
    }
    UsbCdc(const UsbCdc&) = delete;
    UsbCdc& operator=(const UsbCdc&) = delete;

    bool isCdcEnabled() const {return _cdcEnabled; }

    void setCdcEnabled(bool val) {
        if (val == _cdcEnabled) return;
        _cdcEnabled = val;
        tud_disconnect();
        sleep_ms(100);   // Windows needs ~100ms, Linux/Mac are fine with less
        tud_connect();
        display_printf("CDC_%d;", val);
    }

    // void update() {
    //     // static uint32_t last_print = 0;
    //     // if (millis() - last_print >= 1000) {
    //         // last_print = millis();
    //         // tud_cdc_write_str("tick\r\n");
    //         // tud_cdc_write_flush();
    //     // }
    // }
    // static void print(const char* what) {
    //     tud_cdc_write_str(what);
    //     tud_cdc_write_flush();
    // }

    // Called when data arrives on the CDC OUT endpoint
    void tud_cdc_rx_cb(uint8_t itf) {
        (void) itf;
        

        char buff[CFG_TUD_CDC_RX_BUFSIZE];
        uint32_t count = tud_cdc_read(buff, sizeof(buff));
        if (count) {
            for (uint32_t i = 0; i < count; ++i) {
                if (buff[i] == '\n' || buff[i] == '\r') {
                    push_n(buff, i);
                    evalBuffer();
                    //todo is this ok?
                    push_n(buff+i+1, count-i-1);
                    break;
                }
            }

            push_n(buff, count);
            // buf[count] = 0;
            tud_cdc_write(buff, count);
            tud_cdc_write_flush();
        }

    }
private:

    template <typename... Args>
    inline void printf(const char* fmt, Args&&... args) {
        char buffer[1024];
        int count = snprintf(buffer, sizeof(buffer), fmt, std::forward<Args>(args)...);
        display_printf(buffer);

        if (isCdcEnabled()) {
            tud_cdc_write(buffer, count);
            tud_cdc_write_flush();
        }
    }
    bool overflowed = false;
    void push_n(const char* ptr, size_t sz) {
        // printf("Push_%lu %x '%.*s'", sz, *ptr, sz, ptr);
        if (sz == 0) return;
        if (!commandBuffer.push_n(ptr, sz)) {
            onOverflow();
        }
    }
    void evalBuffer() {
        if (!commandBuffer.push_back(0)) {//won't be needed if we use std::string_view for parsing
            onOverflow();
        }
        if (overflowed) {
            commandBuffer.clear();
            overflowed = false;
            return;
        }

        printf("\r\nEval '%.*s'\r\n", commandBuffer.size(),  commandBuffer.begin());
        commandBuffer.clear();
    }
    void onOverflow() {
        //TODO fix
        printf("\r\nOverflowed\r\n");
        overflowed = true;
    }
};