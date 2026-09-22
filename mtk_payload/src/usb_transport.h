#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <functional>
#include <map>
#include <libusb-1.0/libusb.h>

namespace mtk {

class UsbTransport {
public:
    UsbTransport();
    ~UsbTransport();

    UsbTransport(const UsbTransport&) = delete;
    UsbTransport& operator=(const UsbTransport&) = delete;

    bool connect(uint16_t vid, uint16_t pid, int interface_num = -1);
    bool connect(std::map<uint16_t, std::vector<uint16_t>> portconfig, int devclass = 10);
    void close(bool reset = false);
    bool is_connected() const;

    bool usbwrite(const uint8_t* data, size_t len);
    std::vector<uint8_t> usbread(size_t len, int timeout_ms = 1000);

    bool write(const uint8_t* data, size_t len);
    std::vector<uint8_t> read(size_t len, int timeout_ms = 1000);

    // Control transfer (matches Python ctrl_transfer return style)
    int ctrl_transfer(uint8_t bm_request_type, uint8_t b_request,
                      uint16_t w_value, uint16_t w_index,
                      uint8_t* data, uint16_t w_length);
    std::vector<uint8_t> ctrl_transfer_read(uint8_t bm_request_type, uint8_t b_request,
                                            uint16_t w_value, uint16_t w_index,
                                            uint16_t w_length);

    // High-level helpers
    uint32_t rdword(int count = 1);
    uint16_t rword(int count = 1);
    std::vector<uint8_t> rbyte(int count);

    // CDC line coding
    void set_line_coding(uint32_t baudrate = 921600, uint8_t parity = 0,
                         uint8_t databits = 8, uint8_t stopbits = 1);
    void set_control_line_state(bool rts, bool dtr);

    uint16_t vid() const { return vid_; }
    uint16_t pid() const { return pid_; }
    int interface_num() const { return interface_; }
    int endpoint_in() const { return ep_in_; }
    int endpoint_out() const { return ep_out_; }
    size_t max_packet_size() const { return max_packet_size_; }
    libusb_device_handle* device_handle() { return handle_; }
    libusb_context* ctx() { return ctx_; }

private:
    bool claim_interface(libusb_device_handle* handle, int interface_num);
    void release_resources();

    libusb_context* ctx_ = nullptr;
    libusb_device_handle* handle_ = nullptr;
    uint16_t vid_ = 0;
    uint16_t pid_ = 0;
    int interface_ = 0;
    int ep_in_ = 0;
    int ep_out_ = 0;
    size_t max_packet_size_ = 512;
    bool connected_ = false;
};

} // namespace mtk
