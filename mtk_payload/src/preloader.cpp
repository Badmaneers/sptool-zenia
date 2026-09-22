#include "preloader.h"
#include "sla.h"
#include "utils.h"
#include "logger.h"
#include <cstring>
#include <algorithm>
#include <thread>
#include <chrono>

namespace mtk {

// Helper to convert hex string to bytes
static std::vector<uint8_t> hex_to_bytes(const char* hex) {
    std::vector<uint8_t> bytes;
    size_t len = strlen(hex);
    for (size_t i = 0; i + 1 < len; i += 2) {
        char byte_str[3] = {hex[i], hex[i + 1], 0};
        bytes.push_back(static_cast<uint8_t>(strtoul(byte_str, nullptr, 16)));
    }
    return bytes;
}

Preloader::Preloader(UsbTransport& transport, MtkConfig& config)
    : transport_(transport), config_(config) {}

bool Preloader::handshake(int max_tries) {
    int tries = 0;
    auto portconfig = UsbIds::default_portconfig();

    // If user specified VID/PID, use those instead
    if (config_.vid != -1 || config_.pid != -1) {
        uint16_t vid = config_.vid == -1 ? MTK_VID : static_cast<uint16_t>(config_.vid);
        uint16_t pid = config_.pid == -1 ? BROM_PID : static_cast<uint16_t>(config_.pid);
        portconfig = {{vid, {pid}}};
    }

    while (tries < max_tries) {
        if (transport_.connect(portconfig, 10)) {  // devclass=10 for CDC
            if (run_handshake()) {
                return true;
            }
            transport_.close(false);
        }
        tries++;
        if (tries % 5 == 0) {
            LOG_INFO("Hint: Power off the phone before connecting.");
            LOG_INFO("For brom mode, press and hold vol up, vol dwn, or all hw buttons and connect usb.");
        }
        if (tries % 10 == 0) {
            LOG_INFO(".", false);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }
    return false;
}

bool Preloader::run_handshake(int retries) {
    // Line coding and control line state must be set before handshake
    transport_.set_line_coding(921600, 0, 8, 1);
    transport_.set_control_line_state(true, false);

    // If not a BROM PID, send first byte separately (matches Python)
    static const std::vector<uint16_t> kBromPids = {
        0x0003, 0xF200, 0xD1E9, 0xD1E2, 0xD1EC, 0xD1DD
    };
    uint16_t pid = transport_.pid();
    bool is_brom = std::find(kBromPids.begin(), kBromPids.end(), pid) != kBromPids.end();
    if (!is_brom) {
        uint8_t first = HANDSHAKE_BYTES[0];
        transport_.usbwrite(&first, 1);
    }

    for (int attempt = 0; attempt < retries; ++attempt) {
        try {
            bool ok = true;
            for (size_t i = 0; i < HANDSHAKE_BYTES.size(); ++i) {
                uint8_t byte = HANDSHAKE_BYTES[i];
                transport_.usbwrite(&byte, 1);

                auto echo = transport_.usbread(1, 500);
                if (echo.size() != 1 || echo[0] != (~byte & 0xFF)) {
                    ok = false;
                    break;
                }
            }
            if (ok) {
                LOG_INFO("Device detected :)");
                return true;
            }
        } catch (...) {
        }

        try {
            transport_.usbread(transport_.max_packet_size(), 50);
        } catch (...) {}

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    LOG_INFO("Handshake failed after retries");
    return false;
}

bool Preloader::echo(uint8_t cmd) {
    return echo_data(&cmd, 1);
}

bool Preloader::echo(uint32_t val) {
    uint8_t buf[4];
    put_be32(buf, val);
    return echo_data(buf, 4);
}

bool Preloader::echo_data(const uint8_t* data, size_t len) {
    transport_.usbwrite(data, len);
    auto resp = transport_.usbread(len, 100);
    if (resp.size() != len) return false;
    return memcmp(resp.data(), data, len) == 0;
}

uint32_t Preloader::rdword() {
    auto data = transport_.usbread(4);
    if (data.size() < 4) return 0;
    return get_be32(data.data());
}

uint16_t Preloader::rword() {
    auto data = transport_.usbread(2);
    if (data.size() < 2) return 0;
    return get_be16(data.data());
}

std::vector<uint8_t> Preloader::rbyte(int count) {
    return transport_.usbread(count);
}

bool Preloader::usbwrite(const uint8_t* data, size_t len) {
    return transport_.usbwrite(data, len);
}

std::vector<uint8_t> Preloader::usbread(size_t len) {
    return transport_.usbread(len);
}

bool Preloader::init(bool display) {
    LOG_INFO("Status: Waiting for PreLoader VCOM, please reconnect mobile/iot device to brom mode");

    bool res = false;
    int tries = 0;
    while (!res && tries < 1000) {
        res = handshake(100);
        if (!res) {
            if (display) {
                LOG_ERROR("Status: Handshake failed, retrying...");
            }
            transport_.close(false);
            tries++;
        }
    }
    if (tries == 1000) return false;

    if (!echo(CMD_GET_HW_CODE)) {
        if (!echo(CMD_GET_HW_CODE)) {
            LOG_ERROR("Sync error. Please power off the device and retry.");
            return false;
        }
    }

    // Python: if not self.config.iot: val = self.rdword()
    // Python: if val is None or self.config.iot: read_a2 path
    // Note: val is never None in Python (rdword returns int), so IoT path
    // is only taken when --iot was explicitly set via config_.iot
    if (config_.iot) {
        config_.hwver = read_a2(0x80000000);
        config_.hwcode = read_a2(0x80000008);
        config_.hw_sub_code = read_a2(0x8000000C);
        config_.swver = (read32_val(0xA01C0108) & 0xFFFF0000) >> 16;
        LOG_INFO("Detected iot mode !");
    } else {
        uint32_t val = rdword();
        config_.hwcode = (val >> 16) & 0xFFFF;
        config_.hwver = val & 0xFFFF;
    }

    if (display) {
        LOG_INFO("HW code:\t\t\t0x%X", config_.hwcode);
    }

    config_.init_hwcode(config_.hwcode);

    if (display) {
        LOG_INFO("CPU:\t\t\t%s", config_.chipconfig.name.c_str());
        LOG_INFO("HW version:\t\t0x%X", config_.hwver);
        LOG_INFO("WDT:\t\t\t0x%X", config_.chipconfig.watchdog);
        LOG_INFO("Uart:\t\t\t0x%X", config_.chipconfig.uart);
        LOG_INFO("Brom payload addr:\t0x%X", config_.chipconfig.brom_payload_addr);
        LOG_INFO("DA payload addr:\t0x%X", config_.chipconfig.da_payload_addr);
        LOG_INFO("Var1:\t\t\t0x%X", config_.chipconfig.var1);
    }

    if (!config_.skipwdt && !config_.chipconfig.has64bit) {
        if (display) LOG_INFO("Disabling Watchdog...");
        setreg_disablewatchdogtimer(config_.hwcode, config_.hwver);
    }

    if (!get_target_config(config_.target_config, display)) {
        LOG_WARN("Get Target info failed");
    }

    get_bl_ver();
    get_bromver();

    std::vector<uint16_t> hwsw;
    uint16_t hwsw_res = get_hw_sw_ver(hwsw);
    if (hwsw_res != 0xFFFF) {
        config_.hw_sub_code = hwsw[0];
        config_.hwver = hwsw[1];
        config_.swver = hwsw[2];
    }

    if (display) {
        LOG_INFO("HW subcode:\t\t0x%X", config_.hw_sub_code);
        LOG_INFO("HW Ver:\t\t\t0x%X", config_.hwver);
        LOG_INFO("SW Ver:\t\t\t0x%X", config_.swver);
    }

    if (!config_.iot) {
        std::vector<uint8_t> meid;
        if (get_meid(meid)) {
            config_.meid = meid;
            if (display) {
                std::string hex;
                for (auto b : meid) {
                    char buf[4];
                    snprintf(buf, sizeof(buf), "%02X", b);
                    hex += buf;
                }
                LOG_INFO("ME_ID:\t\t\t%s", hex.c_str());
            }
        }
    }

    return true;
}

uint8_t Preloader::get_hw_code() {
    uint8_t cmd = CMD_GET_HW_CODE;
    transport_.usbwrite(&cmd, 1);
    auto resp = transport_.usbread(4);
    if (resp.size() < 4) return 0;
    return static_cast<uint8_t>((get_be32(resp.data()) >> 16) & 0xFFFF);
}

uint8_t Preloader::get_bl_ver() {
    uint8_t cmd = CMD_GET_BL_VER;
    transport_.usbwrite(&cmd, 1);
    auto resp = transport_.usbread(1);
    if (resp.empty()) return 0xFF;

    uint8_t ver = resp[0];
    if (ver == CMD_GET_BL_VER) {
        LOG_INFO("BROM mode detected.");
        config_.is_brom = true;
    }
    config_.blver = ver;
    return ver;
}

uint8_t Preloader::get_bromver() {
    uint8_t cmd = CMD_GET_VERSION;
    transport_.usbwrite(&cmd, 1);
    auto resp = transport_.usbread(1);
    if (resp.empty()) return 0;
    config_.bromver = resp[0];
    return resp[0];
}

uint16_t Preloader::get_hw_sw_ver(std::vector<uint16_t>& result) {
    uint8_t cmd = CMD_GET_HW_SW_VER;
    transport_.usbwrite(&cmd, 1);
    auto resp = transport_.usbread(8);
    if (resp.size() < 8) return 0xFFFF;

    result.resize(4);
    for (int i = 0; i < 4; ++i) {
        result[i] = get_be16(resp.data() + i * 2);
    }
    return 0;
}

bool Preloader::get_target_config(TargetConfig& tc, bool display) {
    uint8_t cmd = CMD_GET_TARGET_CONFIG;
    if (!echo_data(&cmd, 1)) {
        LOG_WARN("CMD Get_Target_Config not supported.");
        return false;
    }

    auto data = transport_.usbread(6);
    if (data.size() < 6) return false;

    uint32_t target_config = get_be32(data.data());
    uint16_t status = get_be16(data.data() + 4);

    tc.sbc = (target_config & 0x1) != 0;
    tc.sla = (target_config & 0x2) != 0;
    tc.daa = (target_config & 0x4) != 0;
    tc.epp = (target_config & 0x8) != 0;
    tc.cert = (target_config & 0x10) != 0;
    tc.memread = (target_config & 0x20) != 0;
    tc.memwrite = (target_config & 0x40) != 0;
    tc.cmdC8 = (target_config & 0x80) != 0;

    if (display) {
        LOG_INFO("Target config:\t\t0x%X", target_config);
        LOG_INFO("\tSBC enabled:\t\t%s", tc.sbc ? "true" : "false");
        LOG_INFO("\tSLA enabled:\t\t%s", tc.sla ? "true" : "false");
        LOG_INFO("\tDAA enabled:\t\t%s", tc.daa ? "true" : "false");
    }

    if (status > 0xFF) {
        LOG_ERROR("Get Target Config Error");
        return false;
    }
    return true;
}

bool Preloader::get_meid(std::vector<uint8_t>& meid) {
    uint8_t blcmd = CMD_GET_BL_VER;
    transport_.usbwrite(&blcmd, 1);
    auto resp = transport_.usbread(1);
    if (resp.empty()) return false;

    if (resp[0] == CMD_GET_BL_VER) {
        uint8_t mecmd = CMD_GET_ME_ID;
        transport_.usbwrite(&mecmd, 1);
        auto merep = transport_.usbread(1);
        if (merep.empty() || merep[0] != CMD_GET_ME_ID) return false;

        auto len_data = transport_.usbread(4);
        uint32_t length = get_be32(len_data.data());
        meid = transport_.usbread(length);

        auto status_data = transport_.usbread(2);
        uint16_t status = get_le16(status_data.data());
        if (status == 0) {
            config_.is_brom = true;
            return true;
        }
        LOG_ERROR("Error on get_meid, status: 0x%X", status);
    } else if (resp[0] > 2) {
        uint8_t mecmd = CMD_GET_ME_ID;
        transport_.usbwrite(&mecmd, 1);
        auto merep = transport_.usbread(1);
        if (merep.empty() || merep[0] != CMD_GET_ME_ID) return false;

        auto len_data = transport_.usbread(4);
        uint32_t length = get_be32(len_data.data());
        meid = transport_.usbread(length);

        auto status_data = transport_.usbread(2);
        uint16_t status = get_le16(status_data.data());
        if (status == 0) {
            config_.is_brom = true;
            return true;
        }
        LOG_ERROR("Error on get_meid, status: 0x%X", status);
    }
    return false;
}

std::pair<uint32_t, std::vector<uint8_t>> Preloader::prepare_data(
    const uint8_t* data, size_t data_len,
    const uint8_t* sigdata, size_t sig_len, size_t maxsize) {

    uint32_t gen_chksum = 0;
    std::vector<uint8_t> combined;

    if (maxsize > 0 && maxsize < data_len) {
        combined.assign(data, data + maxsize);
    } else {
        combined.assign(data, data + data_len);
    }
    if (sigdata && sig_len > 0) {
        combined.insert(combined.end(), sigdata, sigdata + sig_len);
    }

    if (combined.size() % 2 != 0) {
        combined.push_back(0);
    }

    for (size_t x = 0; x < combined.size(); x += 2) {
        gen_chksum ^= get_le16(combined.data() + x);
    }

    return {gen_chksum, combined};
}

bool Preloader::upload_data(const uint8_t* data, size_t len, uint32_t checksum) {
    size_t pos = 0;
    size_t max_packet = transport_.max_packet_size();

    while (pos < len) {
        size_t chunk = std::min(len - pos, max_packet);
        transport_.usbwrite(data + pos, chunk);
        pos += chunk;
        if (pos % 0x2000 == 0) {
            transport_.usbwrite(nullptr, 0);
        }
    }

    if (config_.hwcode != 0x2531) {
        transport_.usbwrite(nullptr, 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(120));
    }

    auto res = transport_.usbread(4);
    if (res.size() < 4) return false;

    uint16_t recv_checksum = get_be16(res.data());
    uint16_t status = get_be16(res.data() + 2);

    if (checksum != recv_checksum && recv_checksum != 0) {
        LOG_WARN("Upload checksum mismatch: expected 0x%X, got 0x%X", checksum, recv_checksum);
    }

    return status <= 0xFF;
}

bool Preloader::read32(uint32_t addr, int dwords, std::vector<uint32_t>& result) {
    if (!echo(CMD_READ32)) return false;
    if (!echo(addr)) return false;
    if (!echo(static_cast<uint32_t>(dwords))) return false;

    uint16_t status = rword();
    if (status > 0xFF) {
        LOG_ERROR("READ32 status error: 0x%X", status);
        return false;
    }

    result.resize(dwords);
    for (int i = 0; i < dwords; ++i) {
        result[i] = rdword();
    }

    uint16_t status2 = rword();
    return status2 <= 0xFF;
}

bool Preloader::write32(uint32_t addr, const std::vector<uint32_t>& values) {
    if (!echo(CMD_WRITE32)) return false;
    if (!echo(addr)) return false;
    if (!echo(static_cast<uint32_t>(values.size()))) return false;

    uint16_t status = rword();
    if (status > 0xFF) {
        LOG_ERROR("WRITE32 status error at 0x%X: 0x%X", addr, status);
        return false;
    }

    for (auto val : values) {
        uint8_t buf[4];
        put_be32(buf, val);
        if (!echo_data(buf, 4)) {
            LOG_ERROR("WRITE32 value echo error at 0x%X", addr);
            return false;
        }
    }

    uint16_t status2 = rword();
    if (status2 > 0xFF) {
        LOG_ERROR("WRITE32 status2 error at 0x%X: 0x%X", addr, status2);
        return false;
    }
    return true;
}

bool Preloader::write32(uint32_t addr, uint32_t value) {
    std::vector<uint32_t> vals = {value};
    return write32(addr, vals);
}

bool Preloader::read16(uint32_t addr, int dwords, std::vector<uint16_t>& result) {
    if (!echo(CMD_READ16)) return false;
    if (!echo(addr)) return false;
    if (!echo(static_cast<uint32_t>(dwords))) return false;

    uint16_t status = rword();
    if (status > 0xFF) return false;

    result.resize(dwords);
    for (int i = 0; i < dwords; ++i) {
        auto data = transport_.usbread(2);
        result[i] = get_be16(data.data());
    }

    auto status2_data = transport_.usbread(2);
    uint16_t status2 = get_be16(status2_data.data());
    return status2 <= 0xFF;
}

bool Preloader::write16(uint32_t addr, uint16_t value) {
    if (!echo(CMD_WRITE16)) return false;
    if (!echo(addr)) return false;
    if (!echo(static_cast<uint32_t>(1))) return false;

    uint16_t status = rword();
    if (status > 0xFF) return false;

    uint8_t buf[2];
    put_be16(buf, value);
    if (!echo_data(buf, 2)) return false;

    uint16_t status2 = rword();
    return status2 <= 0xFF;
}

bool Preloader::setreg_disablewatchdogtimer(uint32_t hwcode, uint16_t hwver) {
    auto [wdt_addr, wdt_value] = get_watchdog_info(config_.chipconfig.watchdog, hwcode);

    if (hwcode == 0x2625 || hwcode == 0x2523 || hwcode == 0x7682 || hwcode == 0x7686 || hwcode == 0x5932) {
        return write16(0xA2050000, 0x2200);
    } else if (hwcode == 0x6261) {
        if (hwver == 0xCA02 || hwver == 0xCB01) {
            write16(0xA0030000, 0x2200);
            write16(0x83070008, 0xABCD);
            write16(0x83070010, 0x0003);
            write32(0xA0510000, read32_val(0xA0510000) | 2);
        } else {
            write16(0xA0030000, 0x2200);
        }
        return true;
    } else if (hwcode == 0x6575 || hwcode == 0x6577) {
        return write16(wdt_addr, static_cast<uint16_t>(wdt_value));
    } else {
        bool res = write32(wdt_addr, wdt_value);
        if (res && hwcode == 0x6592) {
            res = write32(0x10000500, 0x22000000);
        }
        return res;
    }
}

uint32_t Preloader::read32_val(uint32_t addr) {
    std::vector<uint32_t> vals;
    if (read32(addr, 1, vals)) {
        return vals[0];
    }
    return 0;
}

bool Preloader::jump_da(uint32_t addr) {
    LOG_INFO("Jumping to 0x%X", addr);

    if (!echo(CMD_JUMP_DA)) return false;

    uint8_t buf[4];
    put_be32(buf, addr);
    transport_.usbwrite(buf, 4);

    uint32_t resaddr = rdword();
    if (resaddr != addr) {
        LOG_ERROR("Jump_DA address mismatch: expected 0x%X, got 0x%X", addr, resaddr);
        return false;
    }

    uint16_t status = rword();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    if (status == 0) {
        LOG_INFO("Jumping to 0x%X: ok.", addr);
        return true;
    }
    LOG_ERROR("Jump_DA status error: 0x%X", status);
    return false;
}

bool Preloader::jump_da64(uint32_t addr) {
    if (!echo(CMD_JUMP_DA64)) return false;

    uint8_t buf[4];
    put_be32(buf, addr);
    transport_.usbwrite(buf, 4);

    uint32_t resaddr = rdword();
    if (resaddr != addr) return false;

    uint8_t one = 0x01;
    echo_data(&one, 1);

    uint16_t status = rword();
    return status == 0;
}

bool Preloader::jump_bl() {
    if (!echo(CMD_JUMP_BL)) return false;

    uint16_t status = rword();
    if (status <= 0xFF) {
        uint16_t status2 = rword();
        return status2 <= 0xFF;
    }
    return false;
}

bool Preloader::jump_to_partition(const std::string& partitionname) {
    std::string name = partitionname;
    if (name.size() > 64) name.resize(64);
    name.resize(64, '\0');

    if (!echo(CMD_JUMP_TO_PARTITION)) return false;

    transport_.usbwrite(reinterpret_cast<const uint8_t*>(name.data()), name.size());

    uint16_t status2 = rword();
    return status2 <= 0xFF;
}

bool Preloader::send_partition_data(const std::string& partitionname,
                                     const uint8_t* data, size_t data_len) {
    uint32_t checksum = calc_xflash_checksum(data, data_len);

    std::string name = partitionname;
    if (name.size() > 64) name.resize(64);
    name.resize(64, '\0');

    if (!echo(CMD_SEND_PARTITION_DATA)) return false;

    transport_.usbwrite(reinterpret_cast<const uint8_t*>(name.data()), name.size());

    uint8_t len_buf[4];
    put_be32(len_buf, static_cast<uint32_t>(data_len));
    transport_.usbwrite(len_buf, 4);

    uint16_t status = rword();
    if (status > 0xFF) return false;

    // Send data in chunks
    size_t pos = 0;
    size_t remaining = data_len;
    while (remaining > 0) {
        size_t chunk = std::min(remaining, static_cast<size_t>(0x200));
        if (!transport_.usbwrite(data + pos, chunk)) break;
        pos += chunk;
        remaining -= chunk;
    }

    uint8_t csum_buf[4];
    put_be32(csum_buf, checksum);
    transport_.usbwrite(csum_buf, 4);

    return true;
}

bool Preloader::send_auth(const uint8_t* authdata, size_t auth_len) {
    if (!echo(CMD_SEND_AUTH)) return false;

    uint8_t len_buf[4];
    put_be32(len_buf, static_cast<uint32_t>(auth_len));
    transport_.usbwrite(len_buf, 4);

    uint16_t status = rword();
    if (status > 0xFF) return false;

    // Upload auth data
    size_t pos = 0;
    size_t max_packet = transport_.max_packet_size();
    while (pos < auth_len) {
        size_t chunk = std::min(auth_len - pos, max_packet);
        transport_.usbwrite(authdata + pos, chunk);
        pos += chunk;
    }

    uint16_t status2 = rword();
    return status2 <= 0xFF;
}

bool Preloader::send_root_cert(const uint8_t* certdata, size_t cert_len) {
    if (!echo(CMD_SEND_CERT)) return false;

    uint8_t len_buf[4];
    put_be32(len_buf, static_cast<uint32_t>(cert_len));
    transport_.usbwrite(len_buf, 4);

    uint16_t status = rword();
    if (status > 0xFF) return false;

    // Upload cert data
    size_t pos = 0;
    size_t max_packet = transport_.max_packet_size();
    while (pos < cert_len) {
        size_t chunk = std::min(cert_len - pos, max_packet);
        transport_.usbwrite(certdata + pos, chunk);
        pos += chunk;
    }

    uint16_t status2 = rword();
    return status2 <= 0xFF;
}

bool Preloader::send_da(uint32_t address, uint32_t size, uint32_t sig_len,
                         const uint8_t* data, size_t data_len) {
    LOG_INFO("Sending DA...");

    auto [gen_chksum, prepared] = prepare_data(data, data_len, nullptr, 0, size);

    if (!echo(CMD_SEND_DA)) return false;
    if (!echo(address)) return false;
    if (!echo(static_cast<uint32_t>(prepared.size()))) return false;
    if (!echo(sig_len)) return false;

    uint16_t status = rword();
    if (status == 0x1D0D) {
        LOG_INFO("SLA required...");
        if (!handle_sla(config_.is_brom)) {
            LOG_ERROR("Bad SLA challenge");
            return false;
        }
        status = 0;
    }

    if (status <= 0xFF) {
        return upload_data(prepared.data(), prepared.size(), gen_chksum);
    }

    LOG_ERROR("DA_Send status error: 0x%X", status);
    return false;
}

bool Preloader::brom_register_access(uint32_t address, uint32_t length,
                                      uint8_t* data, int mode,
                                      bool check_status) {
    if (mode == -1) {
        mode = (data != nullptr) ? 1 : 0;
    }

    if (!echo(CMD_BROM_REGISTER_ACCESS)) {
        LOG_DEBUG("brom_register_access: echo cmd failed");
        return false;
    }
    if (!echo(static_cast<uint32_t>(mode))) {
        LOG_DEBUG("brom_register_access: echo mode=%d failed", mode);
        return false;
    }
    if (!echo(address)) {
        LOG_DEBUG("brom_register_access: echo addr=0x%X failed", address);
        return false;
    }
    if (!echo(length)) {
        LOG_DEBUG("brom_register_access: echo len=%u failed", length);
        return false;
    }

    auto status_data = transport_.usbread(2);
    uint16_t status = get_le16(status_data.data());

    if (status != 0) {
        if (status == 0x1A1D) {
            LOG_ERROR("Kamakiri2 failed, cache issue :(");
            return false;
        }
        LOG_ERROR("BROM register access error: 0x%X", status);
        return false;
    }

    if (mode == 0 || mode == 2) {
        auto read_data = transport_.usbread(length);
        if (data) {
            memcpy(data, read_data.data(), std::min(read_data.size(), static_cast<size_t>(length)));
        }
    } else if (data) {
        transport_.usbwrite(data, length);
    }

    if (check_status) {
        auto status2_data = transport_.usbread(2);
        uint16_t status2 = get_le16(status2_data.data());
        if (status2 != 0) {
            LOG_DEBUG("brom_register_access: final status=0x%X addr=0x%X len=%u mode=%d",
                      status2, address, length, mode);
        }
        return status2 == 0;
    }
    // When check_status=false: Do NOT read the final 2 status bytes.
    // In the kamakiri2 exploit, the ptr_send write overwrites the BROM's USB
    // send function pointer BEFORE the BROM can send Phase 4 status bytes.
    // The payload executes and sends the ack (0xA1A2A3A4) directly.
    // Reading any bytes here would consume part of the ack.
    return true;
}

void Preloader::run_ext_cmd(uint8_t cmd) {
    uint8_t c8 = CMD_C8;
    transport_.usbwrite(&c8, 1);
    auto resp1 = transport_.usbread(1);

    transport_.usbwrite(&cmd, 1);
    auto resp2 = transport_.usbread(1);

    transport_.usbread(1);
    transport_.usbread(2);
}

uint16_t Preloader::read_a2(uint32_t addr, int dwords) {
    if (!echo(CMD_READ16_A2)) return 0;

    uint8_t addr_buf[4];
    put_be32(addr_buf, addr);
    if (!echo_data(addr_buf, 4)) return 0;

    uint8_t dwords_buf[4];
    put_be32(dwords_buf, static_cast<uint32_t>(dwords));
    if (!echo_data(dwords_buf, 4)) return 0;

    auto data = transport_.usbread(2);
    if (data.size() < 2) return 0;
    return get_be16(data.data());
}

void Preloader::writemem(uint32_t addr, const uint8_t* data, size_t len) {
    // Write data in 4-byte chunks via write32
    for (size_t i = 0; i < len; i += 4) {
        uint8_t buf[4] = {0};
        size_t chunk = std::min(len - i, static_cast<size_t>(4));
        memcpy(buf, data + i, chunk);
        write32(addr + i, get_le32(buf));
    }
}

void Preloader::close(bool reset) {
    transport_.close(reset);
}

bool Preloader::handle_sla(bool isbrom) {
    if (!isbrom) return true;

    const auto& keys = get_brom_sla_keys();
    for (const auto& key : keys) {
        if (!echo(CMD_SLA)) return false;

        uint16_t status = rword();
        if (status == 0x7017) {
            LOG_INFO("SLA already satisfied");
            return true;
        }
        if (status > 0xFF) {
            LOG_ERROR("SLA send auth error: 0x%X", status);
            return false;
        }

        uint32_t challenge_length = rdword();
        auto challenge = rbyte(challenge_length);

        auto response = generate_brom_sla_challenge(
            challenge.data(), challenge.size(),
            key.n_hex, key.d_hex);

        if (response.empty()) {
            LOG_WARN("SLA challenge generation failed for key: %s", key.name.c_str());
            continue;
        }

        uint32_t resplen = static_cast<uint32_t>(response.size());
        uint8_t len_buf[4];
        put_le32(len_buf, resplen);
        if (!echo_data(len_buf, 4)) return false;

        uint32_t rlen = rdword();
        if (resplen != rlen) {
            LOG_WARN("SLA response length mismatch: expected %u, got %u", resplen, rlen);
            continue;
        }

        status = rword();
        if (status > 0xFF) {
            LOG_WARN("SLA response len error: 0x%X (key: %s)", status, key.name.c_str());
            continue;
        }

        usbwrite(response.data(), resplen);
        uint32_t final_status = rdword();
        if (final_status < 0xFF) {
            LOG_INFO("SLA passed with key: %s", key.name.c_str());
            return true;
        }
        LOG_WARN("SLA auth error: 0x%X (key: %s)", final_status, key.name.c_str());
    }
    return false;
}

bool Preloader::get_socid(std::vector<uint8_t>& socid) {
    uint8_t cmd = CMD_GET_SOC_ID;
    if (!echo(cmd)) return false;

    auto len_data = transport_.usbread(4);
    if (len_data.size() < 4) return false;
    uint32_t length = get_be32(len_data.data());
    socid = transport_.usbread(length);

    auto status_data = transport_.usbread(2);
    uint16_t status = get_le16(status_data.data());
    return status == 0;
}

bool Preloader::mtk_cmd(const uint8_t* cmd, size_t cmd_len, uint8_t* resp, size_t resp_len) {
    if (!transport_.usbwrite(cmd, cmd_len)) return false;
    auto data = transport_.usbread(resp_len);
    if (data.size() < resp_len) return false;
    if (resp) memcpy(resp, data.data(), resp_len);
    return true;
}

std::vector<uint8_t> Preloader::patch_preloader_security_da1(const uint8_t* data, size_t len) {
    std::vector<uint8_t> result(data, data + len);
    bool patched = false;

    // Patches from Python mtk_class.py patch_preloader_security_da1
    struct PatchEntry {
        const char* search_hex;
        const char* replace_hex;
        const char* name;
    };

    PatchEntry patches[] = {
        {"A3687BB12846", "0123A3602846", "oppo security"},
        {"B3F5807F01D1", "B3F5807F01D14FF000004FF000007047", "mt6739 c30"},
        {"B3F5807F04BF4FF4807305F011B84FF0FF307047",
         "B3F5807F04BF4FF480734FF000004FF000007047", "regular"},
        {"10B50C680268", "10B5012010BD", "ram blacklist"},
        {"08B5104B7B441B681B68", "00207047000000000000", "seclib_sec_usbdl_enabled"},
        {"5072656C6F61646572205374617274", "50617463686564204C205374617274", "Patched loader msg"},
        {"F0B58BB002AE20250C460746", "002070470000000000205374617274", "sec_img_auth"},
        {"FFC0F3400008BD", "FF4FF0000008BD", "get_vfy_policy"},
        {"040007C0", "00000000", "hash_check"},
        {"CCF20709", "4FF00009", "hash_check2"}
    };

    for (auto& patch : patches) {
        std::vector<uint8_t> search = hex_to_bytes(patch.search_hex);
        std::vector<uint8_t> replace = hex_to_bytes(patch.replace_hex);

        ssize_t idx = find_binary(result.data(), result.size(),
                                  search.data(), search.size());
        if (idx >= 0) {
            memcpy(result.data() + idx, replace.data(), replace.size());
            LOG_INFO("Patched \"%s\" in preloader", patch.name);
            patched = true;
        }
    }

    // Binary pattern for hash_check3
    uint8_t hash_check3_search[] = {0x14, 0x2C, 0xF6, 0x2E, 0xFE, 0xE7};
    uint8_t hash_check3_replace[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    ssize_t idx = find_binary(result.data(), result.size(),
                              hash_check3_search, sizeof(hash_check3_search));
    if (idx >= 0) {
        memcpy(result.data() + idx, hash_check3_replace, sizeof(hash_check3_replace));
        LOG_INFO("Patched \"hash_check3\" in preloader");
        patched = true;
    }

    if (!patched) {
        LOG_WARN("Failed to patch preloader security");
    }

    return result;
}

std::vector<uint8_t> Preloader::patch_preloader_security_da2(const uint8_t* data, size_t len) {
    std::vector<uint8_t> result(data, data + len);
    bool patched = false;

    struct PatchEntry {
        const char* search_hex;
        const char* replace_hex;
        const char* name;
    };

    PatchEntry patches[] = {
        {"A3687BB12846", "0123A3602846", "oppo security"},
        {"B3F5807F01D1", "B3F5807F01D14FF000004FF000007047", "mt6739 c30"},
        {"B3F5807F04BF4FF4807305F011B84FF0FF307047",
         "B3F5807F04BF4FF480734FF000004FF000007047", "regular"},
        {"10B50C680268", "10B5012010BD", "ram blacklist"},
        {"08B5104B7B441B681B68", "00207047000000000000", "seclib_sec_usbdl_enabled"},
        {"5072656C6F61646572205374617274", "50617463686564204C205374617274", "Patched loader msg"},
        {"F0B58BB002AE20250C460746", "002070470000000000205374617274", "sec_img_auth"},
        {"FFC0F3400008BD", "FF4FF0000008BD", "get_vfy_policy"}
    };

    for (auto& patch : patches) {
        std::vector<uint8_t> search = hex_to_bytes(patch.search_hex);
        std::vector<uint8_t> replace = hex_to_bytes(patch.replace_hex);

        ssize_t idx = find_binary(result.data(), result.size(),
                                  search.data(), search.size());
        if (idx >= 0) {
            memcpy(result.data() + idx, replace.data(), replace.size());
            LOG_INFO("Patched \"%s\" in preloader", patch.name);
            patched = true;
        }
    }

    if (!patched) {
        LOG_WARN("Failed to patch preloader security");
    }

    return result;
}

bool Preloader::init_iot() {
    if (config_.hwcode == 0x6261) {
        uint32_t PMU_BASE = 0xA0700000;
        uint32_t EMI_REMAP = 0xA0510000;
        uint32_t RGU_BASE = 0xA0030000;
        uint32_t GPIO_BASE = 0xA0020000;

        if (config_.swver == 0x35C0 && config_.hw_sub_code == 0x8000) {
            if (config_.hwver == 0xcb01) {
                LOG_INFO("MTK 6261DA detected :)");
            } else {
                LOG_INFO("MTK 6261MA detected :)");
            }
            write16(RGU_BASE, 0x2200);           // disable system wdg
            write16(PMU_BASE + 0xa28, 0x8000);    // enter USB download
            write16(PMU_BASE + 0xa24, 2);          // disable battery wdg
        } else if (config_.hw_sub_code == 0x8000) {
            if (config_.swver == 0x3600) {
                LOG_INFO("MTK 2503 detected :)");
            } else if (config_.swver == 0x7640) {
                LOG_INFO("MTK 2503DV detected :)");
            }
            // SetReg_DisableChargeControl
            std::vector<uint16_t> val;
            if (read16(PMU_BASE + 0xA28, 1, val)) {
                write16(PMU_BASE + 0xA28, val[0] | 0x4000);
            }
            if (read16(PMU_BASE + 0xA00, 1, val)) {
                write16(PMU_BASE + 0xA00, val[0] | 0x10);
            }
            // disable system wdg
            write16(RGU_BASE, 0x2200);
        } else {
            LOG_INFO("Unknown MT6261 variant detected :)");
            // disable system wdg
            write16(RGU_BASE, 0x2200);
        }

        // SetReg_DownloadByUSB
        write16(GPIO_BASE + 0x220, 0x0001);
        write16(GPIO_BASE + 0x240, 0x0004);

        // MT6261 specific initialization
        write16(0xA0010000, 0x0010);
        write16(0xA0030000, 0x0220);
        write16(0xA0040000, 0x0000);
        write16(0xA0050000, 0x0000);

        // Configure EMI
        write16(EMI_REMAP, 0x0000);

        config_.iot = true;
        return true;
    }

    // MT2503/MT2523 initialization
    if (config_.hwcode == 0x2523 || config_.hwcode == 0x2503) {
        uint32_t RGU_BASE = 0xA0030000;
        uint32_t GPIO_BASE = 0xA0020000;

        // disable system wdg
        write16(RGU_BASE, 0x2200);

        // Configure GPIO for USB
        write16(GPIO_BASE + 0x220, 0x0001);
        write16(GPIO_BASE + 0x240, 0x0004);

        config_.iot = true;
        return true;
    }

    return false;
}

} // namespace mtk
