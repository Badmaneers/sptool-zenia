#pragma once
#include "types.h"
#include "mtk_config.h"
#include "usb_transport.h"
#include "preloader.h"
#include "pltools.h"
#include "meta.h"
#include <string>
#include <vector>

namespace mtk {

struct BruteForceResult {
    bool success = false;
    uint32_t brom_register_access_addr = 0;
    std::vector<uint8_t> sram_dump;
};

class PayloadCommand {
public:
    PayloadCommand();
    ~PayloadCommand() = default;

    bool execute(const std::string& payload_file = "",
                 const std::string& ptype = "",
                 const std::string& metamode = "",
                 int vid = -1, int pid = -1,
                 const std::string& loader = "",
                 const std::string& preloader = "",
                 bool debugmode = false);

    MtkConfig& config() { return config_; }

private:
    std::string resolve_payload_path(const std::string& file);
    BruteForceResult bruteforce_brom(Preloader& preloader);

    MtkConfig config_;
    UsbTransport transport_;
};

} // namespace mtk
