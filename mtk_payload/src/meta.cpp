#include "meta.h"
#include "utils.h"
#include "logger.h"
#include <cstring>
#include <thread>
#include <chrono>
#include <openssl/evp.h>

namespace mtk {

MetaMode::MetaMode(Preloader& preloader, MtkConfig& config)
    : preloader_(preloader), config_(config) {}

bool MetaMode::init(const std::string& metamode, bool display) {
    if (display) {
        LOG_INFO("Status: Waiting for PreLoader VCOM, please connect mobile");
    }

    auto& transport = preloader_.transport();
    int counter = 0;
    int loop = 0;

    while (!transport.is_connected()) {
        if (counter == 100) break;

        if (transport.connect(MTK_VID, PRELOADER_PID)) {
            counter++;
            if (transport.pid() == 0x2000) {
                // Read READY
                auto ready = transport.usbread(5, 2000);
                if (ready.size() == 5 && std::equal(ready.begin(), ready.end(),
                    reinterpret_cast<const uint8_t*>("READY"))) {

                    // Send metamode
                    transport.usbwrite(reinterpret_cast<const uint8_t*>(metamode.data()),
                                      metamode.size());

                    auto resp = transport.usbread(7, 2000);
                    std::string resp_str(resp.begin(), resp.end());

                    if (resp_str == "METASLA") {
                        LOG_INFO("METASLA detected, handling auth...");
                        if (handle_vendor_auth()) {
                            LOG_INFO("Vendor auth successful");
                        } else {
                            LOG_WARN("Vendor auth failed");
                        }
                    }

                    if (resp_str == "ATEMATEM" || resp_str == "ATEMEVDX" ||
                        resp_str == "TOOBTSAF" || resp_str == "TCAFTCAF" ||
                        resp_str == "MYROTCAF") {

                        if (resp_str == "ATEMATEM") {
                            uint8_t atem_data1[] = {0x04, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0xC0};
                            transport.usbwrite(atem_data1, sizeof(atem_data1));
                            transport.usbwrite(atem_data1, sizeof(atem_data1));
                            uint8_t atem_data2[] = {0x06, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0xC0, 0x00, 0x80, 0x00, 0x00};
                            transport.usbwrite(atem_data2, sizeof(atem_data2));
                            transport.usbread(13); // !READYATEM
                        }

                        uint8_t disconnect[] = "DISCONNECT";
                        transport.usbwrite(disconnect, 10);
                        LOG_INFO("Successfully set meta mode: %s", metamode.c_str());
                        return true;
                    }

                    LOG_WARN("Unexpected response: %s", resp_str.c_str());
                }
            } else {
                transport.close(false);
            }
        }

        if (loop >= 10) LOG_INFO(".", false);
        if (loop >= 20) {
            LOG_INFO("\n", false);
            loop = 0;
        }
        loop++;
        counter++;
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }

    return false;
}

bool MetaMode::handle_vendor_auth() {
    auto& transport = preloader_.transport();

    // Send SLASTART
    uint8_t slastart[] = "SLASTART";
    transport.usbwrite(slastart, 8);

    auto resp = transport.usbread(100, 5000);
    if (resp.empty()) {
        LOG_WARN("No response after SLASTART");
        return false;
    }

    std::string resp_str(resp.begin(), resp.end());

    if (resp_str.find("RANDOM") == std::string::npos) {
        LOG_WARN("Unexpected response: %s", resp_str.c_str());
        return false;
    }

    // Determine vendor
    std::string vendor;
    if (resp_str.find("EXT") != std::string::npos) {
        vendor = "tecno";
    } else {
        vendor = "infinix";
    }

    if (resp_str.find("SHA") != std::string::npos) {
        // SHA-based auth (newer devices)
        // Extract time value and key ID
        if (resp.size() < 0x11) {
            LOG_WARN("Response too short for SHA auth");
            return false;
        }

        uint8_t timeval[4];
        memcpy(timeval, resp.data() + 6, 4);

        uint32_t keyid = get_le32(resp.data() + 0xD);

        // Infinix secret keys
        static const uint8_t infinix_secret[] = {
            0x7C, 0x34, 0xE1, 0x89, 0x12, 0xE1, 0xCD, 0x3D,
            0x56, 0x31, 0xAD, 0xB2, 0x24, 0x76, 0xD3, 0x12,
            0x34, 0xE2, 0xCA, 0xFD, 0x13, 0x12, 0x3D, 0x2B,
            0x3B, 0x13, 0xE1, 0x57, 0x22, 0xAD, 0xC1, 0x1D,
            0x3D, 0x34, 0xFD, 0x3D, 0x1A, 0x57, 0x46, 0x1A,
            0x35, 0x13, 0xC4, 0xAF, 0x5A, 0x86, 0x22, 0x45,
            0x9D, 0x3D, 0xD1, 0x46, 0x72, 0x41, 0x4F, 0xAD,
            0x46, 0xAD, 0x53, 0x11, 0xC2, 0x3B, 0x3D, 0x2D,
            0x1A, 0x2F, 0x3D, 0xFA, 0xDF, 0x35, 0x57, 0x24,
            0xA7, 0x4D, 0x5E, 0x4F, 0x34, 0xD3, 0x4F, 0x2D,
            0xDF, 0x1F, 0x13, 0xD3, 0xB2, 0x91, 0x41, 0x3D,
            0x4F, 0xD1, 0x5D, 0x91, 0xFD, 0x2E, 0x4D, 0x6F,
            0x3D, 0x41, 0x34, 0x7F, 0x45, 0xF3, 0x8A, 0x26,
            0x1A, 0x33, 0x4F, 0x3E, 0x5E, 0x64, 0x36, 0x8A,
            0xD1, 0xF6, 0x9F, 0x35, 0x6A, 0x96, 0x2A, 0x5D
        };

        if (vendor == "infinix") {
            // Extract key from secret based on keyid
            uint32_t offset = (0xC * keyid) % sizeof(infinix_secret);
            uint8_t key[12];
            memcpy(key, infinix_secret + offset, 12);

            // Compute SHA-256(timeval + key)
            uint8_t input[16];
            memcpy(input, timeval, 4);
            memcpy(input + 4, key, 12);

            uint8_t hash[32]; // SHA-256
            EVP_MD_CTX* ctx = EVP_MD_CTX_new();
            if (ctx) {
                EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr);
                EVP_DigestUpdate(ctx, input, 16);
                unsigned int hash_len = 0;
                EVP_DigestFinal_ex(ctx, hash, &hash_len);
                EVP_MD_CTX_free(ctx);
            }

            transport.usbwrite(hash, 32);
        }
    } else {
        // MD5-based auth (older devices)
        uint8_t timeval[4];
        memcpy(timeval, resp.data() + 6, 4);

        // Vendor-specific secrets
        static const uint8_t infinix_secret_md5[] = {
            0xC4, 0x92, 0xAD, 0x3A, 0x61, 0xF9, 0xCE, 0xC3,
            0x13, 0x7F, 0xA9, 0xCB
        };
        static const uint8_t tecno_secret_md5[] = {
            0x4C, 0xEE, 0xCB, 0x1C, 0xB4, 0xB1, 0x1D, 0x2B,
            0x43, 0x18, 0x84, 0x3F
        };

        const uint8_t* secret = nullptr;
        if (vendor == "infinix") {
            secret = infinix_secret_md5;
        } else if (vendor == "tecno" || vendor == "itel") {
            secret = tecno_secret_md5;
        }

        if (secret) {
            uint8_t input[16];
            memcpy(input, timeval, 4);
            memcpy(input + 4, secret, 12);

            uint8_t hash[16]; // MD5
            EVP_MD_CTX* ctx = EVP_MD_CTX_new();
            if (ctx) {
                EVP_DigestInit_ex(ctx, EVP_md5(), nullptr);
                EVP_DigestUpdate(ctx, input, 16);
                unsigned int hash_len = 0;
                EVP_DigestFinal_ex(ctx, hash, &hash_len);
                EVP_MD_CTX_free(ctx);
            }

            transport.usbwrite(hash, 16);
        }
    }

    // Wait for auth response
    resp = transport.usbread(10, 5000);
    if (resp.empty()) {
        LOG_WARN("No auth response");
        return false;
    }

    resp_str = std::string(resp.begin(), resp.end());
    if (resp_str == "ATEM0001") {
        // Send confirmation
        uint8_t confirm1[] = {0x04, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00};
        transport.usbwrite(confirm1, sizeof(confirm1));

        resp = transport.usbread(10, 5000);
        resp_str = std::string(resp.begin(), resp.end());

        if (resp_str == "ATEM0002") {
            uint8_t confirm2[] = {0x06, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00};
            transport.usbwrite(confirm2, sizeof(confirm2));

            resp = transport.usbread(10, 5000);
            resp_str = std::string(resp.begin(), resp.end());

            if (resp_str == "ATEMATEX") {
                uint8_t disconnect[] = "DISCONNECT";
                transport.usbwrite(disconnect, 10);
                return true;
            }
        }
    }

    LOG_WARN("Auth sequence failed: %s", resp_str.c_str());
    return false;
}

bool MetaMode::init_wdg(bool display) {
    if (display) {
        LOG_INFO("Status: Waiting for PreLoader VCOM, please reconnect mobile/iot device to brom mode");
    }

    // Handshake
    int tries = 0;
    while (tries < 1000) {
        if (preloader_.handshake(100)) {
            break;
        }
        if (display) {
            LOG_ERROR("Status: Handshake failed, retrying...");
        }
        preloader_.transport().close(false);
        tries++;
    }
    if (tries == 1000) return false;

    // Get HW code
    if (!preloader_.echo(CMD_GET_HW_CODE)) {
        if (!preloader_.echo(CMD_GET_HW_CODE)) {
            LOG_ERROR("Sync error. Please power off the device and retry.");
            return false;
        }
    }

    uint32_t val = preloader_.rdword();
    config_.hwcode = (val >> 16) & 0xFFFF;
    config_.hwver = val & 0xFFFF;
    config_.init_hwcode(config_.hwcode);

    // Disable watchdog
    auto [wdt_addr, wdt_value] = get_watchdog_info(config_.chipconfig.watchdog, config_.hwcode);
    preloader_.setreg_disablewatchdogtimer(config_.hwcode, config_.hwver);

    // Set meta mode via brom_register_access
    uint8_t one = 0x01;
    preloader_.brom_register_access(0, 1, &one, 3);
    preloader_.brom_register_access(0, 1, nullptr, 2);

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Enable watchdog, reset phone
    preloader_.write32(wdt_addr + 0x14, 0x00001209);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    return true;
}

} // namespace mtk
