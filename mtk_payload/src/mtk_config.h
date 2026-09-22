#pragma once
#include "types.h"
#include "chipconfig.h"
#include <string>
#include <vector>

namespace mtk {

class MtkConfig {
public:
    MtkConfig() = default;

    // USB connection
    int vid = -1;
    int pid = -1;
    int interface_num = -1;
    std::string serial_port;

    // Payload settings
    PayloadType ptype = PayloadType::KAMAKIRI2;
    std::string payload_file;
    std::string loader_file;
    std::string preloader_file;
    std::vector<uint8_t> preloader_data;

    // Device info
    uint32_t hwcode = 0;
    uint16_t hwver = 0xCA00;
    uint16_t swver = 0;
    uint16_t hw_sub_code = 0;
    uint8_t bromver = 0;
    int blver = -2;
    bool is_brom = false;

    // Flags
    bool skipwdt = false;
    bool enforcecrash = false;
    bool iot = false;
    bool debugmode = false;
    bool stock = false;
    bool reconnect = true;
    bool readsocid = false;
    bool write_preloader_to_file = false;
    bool generate_keys = false;

    // Crypto
    std::vector<uint8_t> meid;
    std::vector<uint8_t> socid;
    std::string auth_file;
    std::string cert_file;
    std::string appid;

    // META mode
    std::string metamode;

    // Chip config
    Chipconfig chipconfig;

    // Target security
    TargetConfig target_config;

    // Path config
    std::string hwparam_path = ".";
    std::string payloads_path;
    std::string loader_path;

    // Methods
    void init_hwcode(uint32_t code);
    void load_preloader(const std::string& path);
    std::pair<uint32_t, uint32_t> get_watchdog_addr() const;
    void default_values(uint32_t hwcode);
};

} // namespace mtk
