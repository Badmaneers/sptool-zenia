#include "mtk_config.h"
#include "logger.h"
#include <fstream>
#include <algorithm>

namespace mtk {

void MtkConfig::init_hwcode(uint32_t code) {
    hwcode = code;
    auto* cfg = ChipConfigDb::instance().get(code);
    if (cfg) {
        chipconfig = *cfg;
    } else {
        chipconfig = Chipconfig{};
    }
    ChipConfigDb::instance().init_defaults(chipconfig, code);
}

void MtkConfig::load_preloader(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        LOG_ERROR("Couldn't open preloader: %s", path.c_str());
        return;
    }

    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);
    preloader_data.resize(size);
    file.read(reinterpret_cast<char*>(preloader_data.data()), size);
    preloader_file = path;
    LOG_INFO("Loaded preloader: %s (%zu bytes)", path.c_str(), size);
}

std::pair<uint32_t, uint32_t> MtkConfig::get_watchdog_addr() const {
    auto info = get_watchdog_info(chipconfig.watchdog, hwcode);
    return {info.addr, info.value};
}

void MtkConfig::default_values(uint32_t hwcode_val) {
    ChipConfigDb::instance().init_defaults(chipconfig, hwcode_val);
}

} // namespace mtk
