#pragma once
#include "types.h"
#include "preloader.h"
#include "exploit.h"
#include <memory>

namespace mtk {

class PLTools {
public:
    PLTools(Preloader& preloader, MtkConfig& config);
    ~PLTools() = default;

    bool runpayload(const std::string& filename, int offset = 0,
                    uint32_t ack = ACK_PAYLOAD, uint32_t addr = 0, bool dontack = false);
    bool crash(int mode = 0);
    bool crasher(bool enforcecrash = false);

    // Dump methods
    bool run_dump_brom(const std::string& filename = "brom.bin", uint32_t length = 0x100000);
    bool run_dump_preloader(const std::string& filename = "preloader.bin", uint32_t length = 0x100000);

    Exploit* exploit() { return exploit_.get(); }
    PayloadType payload_type() const { return config_.ptype; }

private:
    std::unique_ptr<Exploit> create_exploit();

    Preloader& preloader_;
    MtkConfig& config_;
    std::unique_ptr<Exploit> exploit_;
};

} // namespace mtk
