#pragma once
#include "types.h"
#include <map>
#include <memory>

namespace mtk {

struct Chipconfig {
    int var1 = DEFAULT_VAR1;
    uint32_t watchdog = DEFAULT_WATCHDOG;
    uint32_t uart = DEFAULT_UART;
    uint32_t brom_payload_addr = DEFAULT_BROM_PAYLOAD_ADDR;
    uint32_t da_payload_addr = DEFAULT_DA_PAYLOAD_ADDR;
    uint32_t pl_payload_addr = 0;
    uint32_t cqdma_base = 0;
    uint32_t ap_dma_mem = 0;
    uint32_t sej_base = 0;
    uint32_t dxcc_base = 0;
    uint32_t ssr_base = 0;
    uint32_t ssr_clk_base = 0;
    uint32_t gcpu_base = 0;
    uint32_t meid_addr = 0;
    uint32_t socid_addr = 0;
    uint32_t prov_addr = 0;
    uint32_t misc_lock = 0;
    uint32_t efuse_addr = 0;
    int damode = DA_LEGACY;
    int dacode = 0;
    bool has64bit = false;
    bool iot = false;
    std::string name;
    std::string description;
    std::string loader;
    std::vector<std::pair<uint32_t, uint32_t>> blacklist;
    std::vector<std::pair<uint32_t, uint32_t>> brom_register_access;
    std::vector<std::pair<uint32_t, uint32_t>> send_ptr;
};

class ChipConfigDb {
public:
    static ChipConfigDb& instance();
    const Chipconfig* get(uint32_t hwcode) const;
    void init_defaults(Chipconfig& cfg, uint32_t hwcode) const;

private:
    ChipConfigDb();
    void init_db();

    std::map<uint32_t, Chipconfig> db_;
};

// Watchdog address/value lookup
struct WatchdogInfo {
    uint32_t addr;
    uint32_t value;
};

WatchdogInfo get_watchdog_info(uint32_t wdt_addr, uint32_t hwcode);

} // namespace mtk
