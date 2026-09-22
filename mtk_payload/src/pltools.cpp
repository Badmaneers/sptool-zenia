#include "pltools.h"
#include "exploit_kamakiri.h"
#include "exploit_kamakiri2.h"
#include "exploit_amonet.h"
#include "exploit_hashimoto.h"
#include "exploit_carbonara.h"
#include "utils.h"
#include "logger.h"
#include <fstream>
#include <algorithm>

namespace mtk {

PLTools::PLTools(Preloader& preloader, MtkConfig& config)
    : preloader_(preloader), config_(config) {
    exploit_ = create_exploit();
}

std::unique_ptr<Exploit> PLTools::create_exploit() {
    switch (config_.ptype) {
        case PayloadType::KAMAKIRI:
            return std::make_unique<ExploitKamakiri>(preloader_, config_);
        case PayloadType::KAMAKIRI2:
            return std::make_unique<ExploitKamakiri2>(preloader_, config_);
        case PayloadType::AMONET:
            return std::make_unique<ExploitAmonet>(preloader_, config_);
        case PayloadType::HASHIMOTO:
            return std::make_unique<ExploitHashimoto>(preloader_, config_);
        case PayloadType::CARBONARA:
            return std::make_unique<ExploitCarbonara>(preloader_, config_);
        default:
            return std::make_unique<ExploitKamakiri2>(preloader_, config_);
    }
}

bool PLTools::runpayload(const std::string& filename, int offset,
                          uint32_t ack, uint32_t addr, bool dontack) {
    // Read payload file
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        LOG_ERROR("Couldn't open %s for reading.", filename.c_str());
        return false;
    }

    size_t size = file.tellg();
    file.seekg(offset, std::ios::beg);
    std::vector<uint8_t> payload(size);
    file.read(reinterpret_cast<char*>(payload.data()), size);
    LOG_INFO("Loading payload from %s, 0x%zX bytes", filename.c_str(), size);

    bool success = exploit_->runpayload(
        payload.data(), payload.size(), ack, addr, dontack);

    if (success) {
        LOG_INFO("Successfully sent payload: %s", filename.c_str());
        return true;
    } else {
        LOG_ERROR("Error on sending payload: %s", filename.c_str());
    }
    return false;
}

bool PLTools::crash(int mode) {
    return exploit_->crash(mode);
}

bool PLTools::crasher(bool enforcecrash) {
    if (enforcecrash || config_.meid.empty() || !config_.is_brom) {
        LOG_INFO("We're not in bootrom, trying to crash da...");
        for (int crashmode = 0; crashmode < 3; ++crashmode) {
            try {
                crash(crashmode);
            } catch (...) {}

            // Re-init preloader after crash
            if (preloader_.init(false)) {
                if (config_.is_brom) break;
            }
        }
    }
    return true;
}

bool PLTools::run_dump_brom(const std::string& filename, uint32_t length) {
    LOG_INFO("Dumping BROM to %s, 0x%X bytes", filename.c_str(), length);

    std::vector<uint8_t> data(length, 0);
    uint32_t addr = 0;

    // Read BROM in chunks
    size_t pos = 0;
    while (pos < length) {
        std::vector<uint32_t> values;
        uint32_t chunk_size = std::min(length - pos, static_cast<size_t>(0x1000));

        if (preloader_.read32(addr + pos, chunk_size / 4, values)) {
            for (uint32_t val : values) {
                if (pos + 4 <= length) {
                    put_le32(data.data() + pos, val);
                    pos += 4;
                }
            }
        } else {
            LOG_WARN("Read failed at offset 0x%X", addr + pos);
            break;
        }
    }

    // Write to file
    std::ofstream out(filename, std::ios::binary);
    if (!out.is_open()) {
        LOG_ERROR("Failed to open %s for writing", filename.c_str());
        return false;
    }
    out.write(reinterpret_cast<const char*>(data.data()), pos);
    LOG_INFO("BROM dump saved: %zu bytes", pos);
    return true;
}

bool PLTools::run_dump_preloader(const std::string& filename, uint32_t length) {
    LOG_INFO("Dumping preloader to %s, 0x%X bytes", filename.c_str(), length);

    std::vector<uint8_t> data(length, 0);
    uint32_t addr = 0;

    // Read preloader in chunks
    size_t pos = 0;
    while (pos < length) {
        std::vector<uint32_t> values;
        uint32_t chunk_size = std::min(length - pos, static_cast<size_t>(0x1000));

        if (preloader_.read32(addr + pos, chunk_size / 4, values)) {
            for (uint32_t val : values) {
                if (pos + 4 <= length) {
                    put_le32(data.data() + pos, val);
                    pos += 4;
                }
            }
        } else {
            LOG_WARN("Read failed at offset 0x%X", addr + pos);
            break;
        }
    }

    // Write to file
    std::ofstream out(filename, std::ios::binary);
    if (!out.is_open()) {
        LOG_ERROR("Failed to open %s for writing", filename.c_str());
        return false;
    }
    out.write(reinterpret_cast<const char*>(data.data()), pos);
    LOG_INFO("Preloader dump saved: %zu bytes", pos);
    return true;
}

} // namespace mtk
