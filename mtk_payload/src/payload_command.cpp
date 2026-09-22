#include "payload_command.h"
#include "logger.h"
#include "utils.h"
#include <filesystem>
#include <algorithm>
#include <fstream>

namespace mtk {

PayloadCommand::PayloadCommand() = default;

std::string PayloadCommand::resolve_payload_path(const std::string& file) {
    namespace fs = std::filesystem;

    // 1. If explicit path and it exists, use it
    if (!file.empty() && fs::exists(file)) {
        return file;
    }

    // Build candidate list for payloads directory
    std::vector<fs::path> search_dirs;

#ifdef PAYLOADS_DIR
    search_dirs.emplace_back(PAYLOADS_DIR);
#endif

    // Relative to executable
    try {
        fs::path exe_dir = fs::canonical("/proc/self/exe").parent_path();
        search_dirs.push_back(exe_dir);
        search_dirs.push_back(exe_dir / "payloads");
        search_dirs.push_back(exe_dir / ".." / "payloads");
    } catch (...) {}

    // CWD
    search_dirs.push_back(fs::current_path());
    search_dirs.push_back(fs::current_path() / "payloads");

    // 2. Try chipconfig loader name
    if (!config_.chipconfig.loader.empty()) {
        for (auto& dir : search_dirs) {
            fs::path candidate = dir / config_.chipconfig.loader;
            if (fs::exists(candidate)) {
                return candidate.string();
            }
        }
    }

    // 3. Try generic_patcher_payload.bin
    for (auto& dir : search_dirs) {
        fs::path candidate = dir / "generic_patcher_payload.bin";
        if (fs::exists(candidate)) {
            return candidate.string();
        }
    }

    // 4. Return original (will fail later with clear error)
    return file;
}

bool PayloadCommand::execute(const std::string& payload_file,
                              const std::string& ptype,
                              const std::string& metamode,
                              int vid, int pid,
                              const std::string& loader,
                              const std::string& preloader_file,
                              bool debugmode) {
    // Set log level
    if (debugmode) {
        Logger::instance().set_level(LogLevel::DEBUG);
        Logger::instance().set_log_file("logs/log.txt");
        config_.debugmode = true;
    }

    LOG_INFO("MTK Payload Tool (C++ port)");

    // Configure
    config_.vid = vid;
    config_.pid = pid;
    config_.payload_file = payload_file;
    config_.loader_file = loader;
    config_.metamode = metamode;

    if (!ptype.empty()) {
        if (ptype == "kamakiri") config_.ptype = PayloadType::KAMAKIRI;
        else if (ptype == "kamakiri2") config_.ptype = PayloadType::KAMAKIRI2;
        else if (ptype == "amonet") config_.ptype = PayloadType::AMONET;
        else if (ptype == "hashimoto") config_.ptype = PayloadType::HASHIMOTO;
        else if (ptype == "carbonara") config_.ptype = PayloadType::CARBONARA;
        else {
            LOG_ERROR("Unknown payload type: %s", ptype.c_str());
            return false;
        }
    }

    if (!loader.empty()) {
        config_.loader_file = loader;
    }

    if (!preloader_file.empty()) {
        config_.load_preloader(preloader_file);
    }

    // Initialize preloader
    Preloader preloader(transport_, config_);
    if (!preloader.init()) {
        LOG_ERROR("Failed to initialize preloader");
        return false;
    }

    // IoT initialization if needed
    if (config_.iot) {
        LOG_INFO("Initializing IoT mode...");
        if (!preloader.init_iot()) {
            LOG_WARN("IoT initialization failed, continuing anyway");
        }
    }

    // Write preloader to file if requested
    if (config_.write_preloader_to_file) {
        LOG_INFO("Dumping preloader to file...");
        // Read preloader data from device
        // This is a simplified version - in production, you'd read from specific addresses
        LOG_INFO("Preloader dump requested (not fully implemented in C++ port)");
    }

    // Generate keys if requested
    if (config_.generate_keys) {
        LOG_INFO("Key generation requested (requires DA mode - not available in payload-only mode)");
    }

    // Read SOC ID if requested
    if (config_.readsocid) {
        std::vector<uint8_t> socid;
        if (preloader.get_socid(socid)) {
            config_.socid = socid;
            std::string hex;
            for (auto b : socid) {
                char buf[4];
                snprintf(buf, sizeof(buf), "%02X", b);
                hex += buf;
            }
            LOG_INFO("SOC_ID: %s", hex.c_str());
        } else {
            LOG_WARN("Failed to read SOC ID");
        }
    }

    // Crasher - return to BROM if needed
    PLTools plt(preloader, config_);
    if (!plt.crasher(config_.enforcecrash)) {
        LOG_ERROR("Failed to crash device to BROM mode");
        return false;
    }

    // Resolve payload file
    std::string resolved = resolve_payload_path(config_.payload_file);
    if (resolved.empty() || !std::filesystem::exists(resolved)) {
        LOG_ERROR("Payload file not found");
        return false;
    }

    // Run payload
    LOG_INFO("Running payload: %s", resolved.c_str());
    if (!plt.runpayload(resolved)) {
        LOG_ERROR("Error on running payload");
        return false;
    }

    // Optional META mode
    if (!config_.metamode.empty()) {
        LOG_INFO("Entering META mode: %s", config_.metamode.c_str());

        // Re-handshake
        preloader.run_handshake();
        preloader.jump_bl();
        transport_.close(true);

        MetaMode meta(preloader, config_);
        if (meta.init(config_.metamode)) {
            LOG_INFO("Successfully set meta mode: %s", config_.metamode.c_str());
        }
    }

    transport_.close(true);
    return true;
}

BruteForceResult PayloadCommand::bruteforce_brom(Preloader& preloader) {
    BruteForceResult result;

    // BROM register access brute force addresses from Python
    // These are common addresses used in kamakiri2 exploits
    std::vector<uint32_t> brom_addrs = {
        0x00000000, 0x00100000, 0x00200000, 0x00300000,
        0x00400000, 0x00500000, 0x00600000, 0x00700000,
        0x00800000, 0x00900000, 0x00A00000, 0x00B00000,
        0x00C00000, 0x00D00000, 0x00E00000, 0x00F00000
    };

    // Try each address
    for (uint32_t addr : brom_addrs) {
        LOG_DEBUG("Trying brom_register_access at 0x%X", addr);

        // Try mode 0 (read)
        if (preloader.brom_register_access(addr, 4, nullptr, 0)) {
            LOG_INFO("Found valid brom_register_access at 0x%X (mode 0)", addr);
            result.success = true;
            result.brom_register_access_addr = addr;

            // Read SRAM dump
            std::vector<uint8_t> data(0x100);
            if (preloader.brom_register_access(addr, 0x100, data.data(), 0)) {
                result.sram_dump = data;
            }
            return result;
        }

        // Try mode 1 (write) with dummy data
        uint8_t dummy[4] = {0};
        if (preloader.brom_register_access(addr, 4, dummy, 1)) {
            LOG_INFO("Found valid brom_register_access at 0x%X (mode 1)", addr);
            result.success = true;
            result.brom_register_access_addr = addr;
            return result;
        }
    }

    LOG_WARN("Brute force failed, no valid address found");
    return result;
}

} // namespace mtk
