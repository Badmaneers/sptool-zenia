#pragma once
#include "types.h"
#include "usb_transport.h"
#include "mtk_config.h"
#include <vector>
#include <map>

namespace mtk {

// Default USB portconfig (matches Python usb_ids.py)
struct UsbIds {
    static std::map<uint16_t, std::vector<uint16_t>> default_portconfig() {
        return {
            {0x0E8D, {0x0003, 0x6000, 0x2000, 0x2001, 0x20FF, 0x3000}},
            {0x1004, {0x6000}},
            {0x22d9, {0x0006}},
            {0x0FCE, {0xF200, 0xD1E9, 0xD1E2, 0xD1EC, 0xD1DD}},
            {0x0403, {0x6001}},
            {0x1a86, {0x55d3, 0x7523, 0x5523, 0x5512}},
            {0x4348, {0x5523}},
            {0x10C4, {0xEA60}},
            {0x11CA, {0x0211}}
        };
    }
};

class Preloader {
public:
    Preloader(UsbTransport& transport, MtkConfig& config);

    bool init(bool display = true);
    void close(bool reset = false);

    // Handshake
    bool handshake(int max_tries = 1000);
    bool run_handshake(int retries = 5);

    // Register access
    bool read32(uint32_t addr, int dwords, std::vector<uint32_t>& result);
    bool write32(uint32_t addr, const std::vector<uint32_t>& values);
    bool write32(uint32_t addr, uint32_t value);
    bool read16(uint32_t addr, int dwords, std::vector<uint16_t>& result);
    bool write16(uint32_t addr, uint16_t value);

    // DA operations
    bool send_da(uint32_t address, uint32_t size, uint32_t sig_len, const uint8_t* data, size_t data_len);
    bool jump_da(uint32_t addr);
    bool jump_da64(uint32_t addr);
    bool jump_bl();

    // Partition operations
    bool jump_to_partition(const std::string& partitionname);
    bool send_partition_data(const std::string& partitionname, const uint8_t* data, size_t data_len);

    // Auth operations
    bool send_auth(const uint8_t* authdata, size_t auth_len);
    bool send_root_cert(const uint8_t* certdata, size_t cert_len);

    // Watchdog
    bool setreg_disablewatchdogtimer(uint32_t hwcode, uint16_t hwver);

    // Commands
    uint8_t get_hw_code();
    uint8_t get_bl_ver();
    uint8_t get_bromver();
    uint16_t get_hw_sw_ver(std::vector<uint16_t>& result);
    bool get_target_config(TargetConfig& tc, bool display = true);
    bool get_meid(std::vector<uint8_t>& meid);
    bool get_socid(std::vector<uint8_t>& socid);

    // Misc
    uint32_t read32_val(uint32_t addr);
    bool brom_register_access(uint32_t address, uint32_t length,
                              uint8_t* data = nullptr, int mode = -1,
                              bool check_status = true);
    void run_ext_cmd(uint8_t cmd);
    uint16_t read_a2(uint32_t addr, int dwords = 1);
    void writemem(uint32_t addr, const uint8_t* data, size_t len);

    // Preloader security patching
    std::vector<uint8_t> patch_preloader_security_da1(const uint8_t* data, size_t len);
    std::vector<uint8_t> patch_preloader_security_da2(const uint8_t* data, size_t len);

    // IoT detection and initialization
    bool init_iot();

    // Echo/send
    bool echo(uint8_t cmd);
    bool echo(uint32_t val);
    bool mtk_cmd(const uint8_t* cmd, size_t cmd_len, uint8_t* resp, size_t resp_len);
    bool usbwrite(const uint8_t* data, size_t len);
    std::vector<uint8_t> usbread(size_t len);
    uint32_t rdword();
    uint16_t rword();
    std::vector<uint8_t> rbyte(int count);

    UsbTransport& transport() { return transport_; }
    MtkConfig& config() { return config_; }

private:
    bool echo_data(const uint8_t* data, size_t len);
    bool upload_data(const uint8_t* data, size_t len, uint32_t checksum);
    std::pair<uint32_t, std::vector<uint8_t>> prepare_data(const uint8_t* data, size_t data_len,
                                                           const uint8_t* sigdata = nullptr,
                                                           size_t sig_len = 0, size_t maxsize = 0);
    bool handle_sla(bool isbrom = true);

    UsbTransport& transport_;
    MtkConfig& config_;
};

} // namespace mtk
