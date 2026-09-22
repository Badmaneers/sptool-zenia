#include "chipconfig.h"
#include "types.h"

namespace mtk {

ChipConfigDb& ChipConfigDb::instance() {
    static ChipConfigDb inst;
    return inst;
}

ChipConfigDb::ChipConfigDb() {
    init_db();
}

const Chipconfig* ChipConfigDb::get(uint32_t hwcode) const {
    auto it = db_.find(hwcode);
    if (it != db_.end()) {
        return &it->second;
    }
    return nullptr;
}

void ChipConfigDb::init_defaults(Chipconfig& cfg, uint32_t hwcode) const {
    if (cfg.var1 == 0) cfg.var1 = DEFAULT_VAR1;
    if (cfg.watchdog == 0) cfg.watchdog = DEFAULT_WATCHDOG;
    if (cfg.uart == 0) cfg.uart = DEFAULT_UART;
    if (cfg.brom_payload_addr == 0) cfg.brom_payload_addr = DEFAULT_BROM_PAYLOAD_ADDR;
    if (cfg.da_payload_addr == 0) cfg.da_payload_addr = DEFAULT_DA_PAYLOAD_ADDR;
    if (cfg.dacode == 0) cfg.dacode = hwcode;
    if (cfg.damode == 0) cfg.damode = DA_LEGACY;
    if (cfg.ap_dma_mem == 0) cfg.ap_dma_mem = 0x110001A0;

    // Set IoT flag for known IoT chips
    if (hwcode == 0x6261 || hwcode == 0x2523 || hwcode == 0x2503 ||
        hwcode == 0x6225 || hwcode == 0x6226 || hwcode == 0x6236 ||
        hwcode == 0x6238 || hwcode == 0x6253 || hwcode == 0x6255 ||
        hwcode == 0x6256 || hwcode == 0x625A || hwcode == 0x6268 ||
        hwcode == 0x6270 || hwcode == 0x6276 || hwcode == 0x6280 ||
        hwcode == 0x6291 || hwcode == 0x8135) {
        cfg.iot = true;
    }

    // Set 64-bit flag for known 64-bit chips
    if (hwcode == 0x8168 || hwcode == 0x8172 || hwcode == 0x8176 ||
        hwcode == 0x8183 || hwcode == 0x8188 || hwcode == 0x8193 ||
        hwcode == 0x8195 || hwcode == 0x8695 || hwcode == 0x8690 ||
        hwcode == 0x8228 || hwcode == 0x8512 || hwcode == 0x8518 ||
        hwcode == 0x8590) {
        cfg.has64bit = true;
    }
}

WatchdogInfo get_watchdog_info(uint32_t wdt_addr, uint32_t hwcode) {
    switch (wdt_addr) {
        case 0x10007000: return {wdt_addr, 0x22000064};
        case 0x10212000: return {wdt_addr, 0x22000000};
        case 0x10211000: return {wdt_addr, 0x22000064};
        case 0x10007400: return {wdt_addr, 0x22000000};
        case 0xC0000000: return {wdt_addr, 0x2264};
        case 0xA0030000: return {wdt_addr, 0x2200};
        case 0x1C00A000: return {wdt_addr, 0x22000064};
        case 0x2200:
            if (hwcode == 0x6276 || hwcode == 0x8163) return {wdt_addr, 0x610C0000};
            if (hwcode == 0x6251 || hwcode == 0x6516) return {wdt_addr, 0x80030000};
            if (hwcode == 0x6255) return {wdt_addr, 0x701E0000};
            return {wdt_addr, 0x70025000};
        default: return {wdt_addr, 0x22000064};
    }
}

void ChipConfigDb::init_db() {
    // MT6797/MT6767
    db_[0x0279] = Chipconfig{
        .var1 = 10, .watchdog = 0x10007000, .uart = 0x11002000,
        .brom_payload_addr = 0x100A00, .da_payload_addr = 0x201000, .pl_payload_addr = 0x40200000,
        .cqdma_base = 0x10212C00, .ap_dma_mem = 0x110001A0, .sej_base = 0x1000A000,
        .gcpu_base = 0x10210000, .meid_addr = 0x1030AC, .misc_lock = 0x10002050,
        .efuse_addr = 0x10206000, .damode = 5, .dacode = 26519,
        .has64bit = false, .iot = false, .name = "MT6797/MT6767",
        .description = "Helio X23/X25/X27", .loader = "mt6797_payload.bin", .blacklist = {{0x10276C, 0x0}, {0x105704, 0x0}},
        .brom_register_access = {{0xA18C, 0xA354}}, .send_ptr = {{0x1027B0, 0x9EAC}},
    };
    // MT6735/T,MT8735A
    db_[0x0321] = Chipconfig{
        .var1 = 40, .watchdog = 0x10212000, .uart = 0x11002000,
        .brom_payload_addr = 0x100A00, .da_payload_addr = 0x201000, .pl_payload_addr = 0x40200000,
        .cqdma_base = 0x10217C00, .ap_dma_mem = 0x110001A0, .sej_base = 0x10008000,
        .gcpu_base = 0x10216000, .meid_addr = 0x1030B0, .misc_lock = 0x10001838,
        .efuse_addr = 0x11C50000, .damode = 3, .dacode = 26421,
        .has64bit = false, .iot = false, .name = "MT6735/T,MT8735A",
        .description = "", .loader = "mt6735_payload.bin", .blacklist = {{0x102760, 0x0}, {0x105704, 0x0}},
        .brom_register_access = {{0x98CC, 0x9A94}}, .send_ptr = {{0x1027A0, 0x95F8}},
    };
    // MT6755/MT6750/M/T/S
    db_[0x0326] = Chipconfig{
        .var1 = 10, .watchdog = 0x10007000, .uart = 0x11002000,
        .brom_payload_addr = 0x100A00, .da_payload_addr = 0x201000, .pl_payload_addr = 0x40200000,
        .cqdma_base = 0x10212C00, .ap_dma_mem = 0x110001A0, .sej_base = 0x1000A000,
        .gcpu_base = 0x10210000, .meid_addr = 0x1030AC, .misc_lock = 0x10001838,
        .efuse_addr = 0x10206000, .damode = 5, .dacode = 26453,
        .has64bit = false, .iot = false, .name = "MT6755/MT6750/M/T/S",
        .description = "Helio P10/P15/P18", .loader = "mt6755_payload.bin", .blacklist = {{0x10276C, 0x0}, {0x105704, 0x0}},
        .brom_register_access = {{0x9D4C, 0x9F14}}, .send_ptr = {{0x1027B0, 0x9A6C}},
    };
    // MT6737M/MT6735G
    db_[0x0335] = Chipconfig{
        .var1 = 40, .watchdog = 0x10212000, .uart = 0x11002000,
        .brom_payload_addr = 0x100A00, .da_payload_addr = 0x201000, .pl_payload_addr = 0x40200000,
        .cqdma_base = 0x10217C00, .ap_dma_mem = 0x110001A0, .sej_base = 0x10008000,
        .gcpu_base = 0x10216000, .meid_addr = 0x1030B0, .efuse_addr = 0x10206000,
        .damode = 3, .dacode = 26421, .has64bit = false,
        .iot = false, .name = "MT6737M/MT6735G", .description = "",
        .loader = "mt6737_payload.bin", .blacklist = {{0x102760, 0x0}, {0x105704, 0x0}}, .brom_register_access = {{0x98DC, 0x9AA4}},
        .send_ptr = {{0x1027A0, 0x9608}},
    };
    // MT6753
    db_[0x0337] = Chipconfig{
        .var1 = 40, .watchdog = 0x10212000, .uart = 0x11002000,
        .brom_payload_addr = 0x100A00, .da_payload_addr = 0x201000, .pl_payload_addr = 0x40200000,
        .cqdma_base = 0x10217C00, .ap_dma_mem = 0x110001A0, .sej_base = 0x10008000,
        .gcpu_base = 0x10216000, .meid_addr = 0x1030B0, .misc_lock = 0x10001838,
        .damode = 3, .dacode = 26421, .has64bit = false,
        .iot = false, .name = "MT6753", .description = "",
        .loader = "mt6753_payload.bin", .blacklist = {{0x102760, 0x0}, {0x105704, 0x0}}, .brom_register_access = {{0x993C, 0x9B04}},
        .send_ptr = {{0x1027A0, 0x9668}},
    };
    // MT6759
    db_[0x0507] = Chipconfig{
        .watchdog = 0x10210000, .uart = 0x11020000, .brom_payload_addr = 0x100A00,
        .da_payload_addr = 0x201000, .ap_dma_mem = 0x10301A0, .gcpu_base = 0x10210000,
        .damode = 3, .dacode = 26456, .has64bit = false,
        .iot = false, .name = "MT6759", .description = "Helio P30",
    };
    // MT6757/MT6757D
    db_[0x0551] = Chipconfig{
        .var1 = 10, .watchdog = 0x10007000, .uart = 0x11002000,
        .brom_payload_addr = 0x100A00, .da_payload_addr = 0x201000, .pl_payload_addr = 0x40200000,
        .cqdma_base = 0x10212C00, .ap_dma_mem = 0x110001A0, .sej_base = 0x1000A000,
        .gcpu_base = 0x10210000, .meid_addr = 0x1030B4, .misc_lock = 0x10001838,
        .efuse_addr = 0x10206000, .damode = 5, .dacode = 26455,
        .has64bit = false, .iot = false, .name = "MT6757/MT6757D",
        .description = "Helio P20", .loader = "mt6757_payload.bin", .blacklist = {{0x102774, 0x0}, {0x105704, 0x0}},
        .brom_register_access = {{0xA030, 0xA0E8}}, .send_ptr = {{0x1027B8, 0x9C2C}},
    };
    // MT6799
    db_[0x0562] = Chipconfig{
        .var1 = 10, .watchdog = 0x10211000, .uart = 0x11020000,
        .brom_payload_addr = 0x100A00, .da_payload_addr = 0x201000, .pl_payload_addr = 0x40200000,
        .cqdma_base = 0x11B30000, .ap_dma_mem = 0x110001A0, .sej_base = 0x1000A000,
        .dxcc_base = 0x11B20000, .gcpu_base = 0x10210000, .meid_addr = 0x1033B8,
        .socid_addr = 0x1033C8, .efuse_addr = 0x11F10000, .damode = 5,
        .dacode = 26521, .has64bit = false, .iot = false,
        .name = "MT6799", .description = "Helio X30/X35", .loader = "mt6799_payload.bin",
        .blacklist = {{0x102870, 0x0}, {0x107070, 0x0}}, .brom_register_access = {{0xF9C0, 0xFA78}}, .send_ptr = {{0x1028B4, 0xF5AC}},
    };
    // MT0571
    db_[0x0571] = Chipconfig{
        .watchdog = 0x10007000, .damode = 3, .dacode = 1393,
        .has64bit = false, .iot = false, .name = "MT0571",
        .description = "",
    };
    // ELBRUS/MT0598
    db_[0x0598] = Chipconfig{
        .watchdog = 0x10211000, .uart = 0x11020000, .brom_payload_addr = 0x100A00,
        .da_payload_addr = 0x201000, .cqdma_base = 0x10212C00, .ap_dma_mem = 0x110001A0,
        .sej_base = 0x1000A000, .gcpu_base = 0x10224000, .damode = 3,
        .dacode = 1432, .has64bit = false, .iot = false,
        .name = "ELBRUS/MT0598", .description = "",
    };
    // MT6750
    db_[0x0601] = Chipconfig{
        .var1 = 10, .watchdog = 0x10007000, .uart = 0x11002000,
        .brom_payload_addr = 0x100A00, .da_payload_addr = 0x201000, .pl_payload_addr = 0x40200000,
        .cqdma_base = 0x10212C00, .ap_dma_mem = 0x110001A0, .sej_base = 0x1000A000,
        .gcpu_base = 0x10210000, .misc_lock = 0x10001838, .efuse_addr = 0x10206000,
        .damode = 5, .dacode = 26453, .has64bit = false,
        .iot = false, .name = "MT6750", .description = "",
    };
    // MT6570/MT8321
    db_[0x0633] = Chipconfig{
        .watchdog = 0x10007000, .uart = 0x11002000, .brom_payload_addr = 0x100A00,
        .da_payload_addr = 0x201000, .pl_payload_addr = 0x80001000, .cqdma_base = 0x1020AC00,
        .ap_dma_mem = 0x110001A0, .sej_base = 0x1000A000, .gcpu_base = 0x1020D000,
        .efuse_addr = 0x10009000, .damode = 5, .dacode = 25968,
        .has64bit = false, .iot = false, .name = "MT6570/MT8321",
        .description = "",
    };
    // MT6758
    db_[0x0688] = Chipconfig{
        .var1 = 10, .watchdog = 0x10211000, .uart = 0x11020000,
        .brom_payload_addr = 0x100A00, .da_payload_addr = 0x201000, .pl_payload_addr = 0x40200000,
        .cqdma_base = 0x10200000, .ap_dma_mem = 0x110001A0, .sej_base = 0x10080000,
        .dxcc_base = 0x11240000, .gcpu_base = 0x10050000, .meid_addr = 0x102BF8,
        .socid_addr = 0x102C08, .efuse_addr = 0x10450000, .damode = 5,
        .dacode = 26456, .has64bit = false, .iot = false,
        .name = "MT6758", .description = "Helio P30", .loader = "mt6758_payload.bin",
        .blacklist = {{0x102830, 0x0}, {0x106A60, 0x0}}, .brom_register_access = {{0xDC74, 0xDD2C}}, .send_ptr = {{0x102874, 0xD860}},
    };
    // MT6763
    db_[0x0690] = Chipconfig{
        .var1 = 127, .watchdog = 0x10007000, .uart = 0x11002000,
        .brom_payload_addr = 0x100A00, .da_payload_addr = 0x201000, .pl_payload_addr = 0x40200000,
        .cqdma_base = 0x10212000, .ap_dma_mem = 0x11000C20, .sej_base = 0x1000A000,
        .dxcc_base = 0x10210000, .gcpu_base = 0x10050000, .meid_addr = 0x102B78,
        .socid_addr = 0x102B88, .prov_addr = 0x106804, .misc_lock = 0x1001A100,
        .efuse_addr = 0x11F10000, .damode = 5, .dacode = 26467,
        .has64bit = false, .iot = false, .name = "MT6763",
        .description = "Helio P23", .loader = "mt6763_payload.bin", .blacklist = {{0x102834, 0x0}, {0x106CA4, 0x0}},
        .brom_register_access = {{0xDA80, 0xDB38}}, .send_ptr = {{0x102878, 0xD66C}},
    };
    // MT6739/MT6731/MT8765
    db_[0x0699] = Chipconfig{
        .var1 = 180, .watchdog = 0x10007000, .uart = 0x11002000,
        .brom_payload_addr = 0x100A00, .da_payload_addr = 0x201000, .pl_payload_addr = 0x40200000,
        .cqdma_base = 0x10212000, .ap_dma_mem = 0x110001A0, .sej_base = 0x1000A000,
        .dxcc_base = 0x10210000, .gcpu_base = 0x10050000, .meid_addr = 0x102AF8,
        .socid_addr = 0x102B08, .prov_addr = 0x10720C, .misc_lock = 0x1001A100,
        .efuse_addr = 0x11C00000, .damode = 5, .dacode = 26425,
        .has64bit = false, .iot = false, .name = "MT6739/MT6731/MT8765",
        .description = "", .loader = "mt6739_payload.bin", .blacklist = {{0x10282C, 0x0}, {0x1076AC, 0x0}},
        .brom_register_access = {{0xE330, 0xE3E8}}, .send_ptr = {{0x102870, 0xDF1C}},
    };
    // MT6768/MT6769
    db_[0x0707] = Chipconfig{
        .var1 = 37, .watchdog = 0x10007000, .uart = 0x11002000,
        .brom_payload_addr = 0x100A00, .da_payload_addr = 0x201000, .pl_payload_addr = 0x40200000,
        .cqdma_base = 0x10212000, .ap_dma_mem = 0x110001A0, .sej_base = 0x1000A000,
        .dxcc_base = 0x10210000, .gcpu_base = 0x10050000, .meid_addr = 0x102AF8,
        .socid_addr = 0x102B08, .prov_addr = 0x1054F4, .misc_lock = 0x1001A100,
        .efuse_addr = 0x11CE0000, .damode = 5, .dacode = 26472,
        .has64bit = false, .iot = false, .name = "MT6768/MT6769",
        .description = "Helio P65/G85 k68v1", .loader = "mt6768_payload.bin", .blacklist = {{0x10282C, 0x0}, {0x105994, 0x0}},
        .brom_register_access = {{0xC598, 0xC650}}, .send_ptr = {{0x10286C, 0xC190}},
    };
    // MT6761/MT6762/MT3369/MT8766B/MT8761/AC8259/AC8257
    db_[0x0717] = Chipconfig{
        .var1 = 37, .watchdog = 0x10007000, .uart = 0x11002000,
        .brom_payload_addr = 0x100A00, .da_payload_addr = 0x201000, .pl_payload_addr = 0x40200000,
        .cqdma_base = 0x10212000, .ap_dma_mem = 0x11000C20, .sej_base = 0x1000A000,
        .dxcc_base = 0x10210000, .gcpu_base = 0x10050000, .meid_addr = 0x102AF8,
        .socid_addr = 0x102B08, .prov_addr = 0x1054F4, .misc_lock = 0x1001A100,
        .efuse_addr = 0x11C50000, .damode = 5, .dacode = 26465,
        .has64bit = false, .iot = false, .name = "MT6761/MT6762/MT3369/MT8766B/MT8761/AC8259/AC8257",
        .description = "Helio A20/P22/A22/A25/G25", .loader = "mt6761_payload.bin", .blacklist = {{0x102828, 0x0}, {0x105994, 0x0}},
        .brom_register_access = {{0xC0A0, 0xC158}}, .send_ptr = {{0x10286C, 0xBC8C}},
    };
    // MT6779
    db_[0x0725] = Chipconfig{
        .var1 = 10, .watchdog = 0x10007000, .uart = 0x11002000,
        .brom_payload_addr = 0x100A00, .da_payload_addr = 0x201000, .pl_payload_addr = 0x40200000,
        .cqdma_base = 0x10212000, .ap_dma_mem = 0x11000158, .sej_base = 0x1000A000,
        .dxcc_base = 0x10210000, .gcpu_base = 0x10050000, .meid_addr = 0x102B38,
        .socid_addr = 0x102B48, .prov_addr = 0x1065C0, .misc_lock = 0x1001A100,
        .efuse_addr = 0x11C10000, .damode = 5, .dacode = 26489,
        .has64bit = false, .iot = false, .name = "MT6779",
        .description = "Helio P90 k79v1", .loader = "mt6779_payload.bin", .blacklist = {{0x102838, 0x0}, {0x106A60, 0x0}},
        .brom_register_access = {{0xE454, 0xE50C}}, .send_ptr = {{0x102878, 0xE04C}},
    };
    // MT6765/MT8768t
    db_[0x0766] = Chipconfig{
        .var1 = 37, .watchdog = 0x10007000, .uart = 0x11002000,
        .brom_payload_addr = 0x100A00, .da_payload_addr = 0x201000, .pl_payload_addr = 0x40200000,
        .cqdma_base = 0x10212000, .ap_dma_mem = 0x110001A0, .sej_base = 0x1000A000,
        .dxcc_base = 0x10210000, .gcpu_base = 0x10050000, .meid_addr = 0x102AF8,
        .socid_addr = 0x102B08, .prov_addr = 0x1054F4, .misc_lock = 0x1001A100,
        .efuse_addr = 0x11C50000, .damode = 5, .dacode = 26469,
        .has64bit = false, .iot = false, .name = "MT6765/MT8768t",
        .description = "Helio P35/G35", .loader = "mt6765_payload.bin", .blacklist = {{0x102828, 0x0}, {0x105994, 0x0}},
        .brom_register_access = {{0xC1D4, 0xC28C}}, .send_ptr = {{0x10286C, 0xBDC0}},
    };
    // MT6771/MT8385/MT8183/MT8666
    db_[0x0788] = Chipconfig{
        .var1 = 10, .watchdog = 0x10007000, .uart = 0x11002000,
        .brom_payload_addr = 0x100A00, .da_payload_addr = 0x201000, .pl_payload_addr = 0x40200000,
        .cqdma_base = 0x10212000, .ap_dma_mem = 0x11000158, .sej_base = 0x1000A000,
        .dxcc_base = 0x10210000, .gcpu_base = 0x10050000, .meid_addr = 0x102B38,
        .socid_addr = 0x102B48, .prov_addr = 0x1065C0, .misc_lock = 0x1001A100,
        .efuse_addr = 0x11F10000, .damode = 5, .dacode = 26481,
        .has64bit = false, .iot = false, .name = "MT6771/MT8385/MT8183/MT8666",
        .description = "Helio P60/P70/G80", .loader = "mt6771_payload.bin", .blacklist = {{0x102834, 0x0}, {0x106A60, 0x0}},
        .brom_register_access = {{0xE2D0, 0xE388}}, .send_ptr = {{0x102878, 0xDEBC}},
    };
    // MT6785
    db_[0x0813] = Chipconfig{
        .var1 = 10, .watchdog = 0x10007000, .uart = 0x11002000,
        .brom_payload_addr = 0x100A00, .da_payload_addr = 0x201000, .pl_payload_addr = 0x40200000,
        .cqdma_base = 0x10212000, .ap_dma_mem = 0x11000158, .sej_base = 0x1000A000,
        .dxcc_base = 0x10210000, .gcpu_base = 0x10050000, .meid_addr = 0x102B38,
        .socid_addr = 0x102B48, .prov_addr = 0x1065C0, .misc_lock = 0x1001A100,
        .efuse_addr = 0x11C10000, .damode = 5, .dacode = 26501,
        .has64bit = false, .iot = false, .name = "MT6785",
        .description = "Helio G90", .loader = "mt6785_payload.bin", .blacklist = {{0x102838, 0x0}, {0x106A60, 0x0}},
        .brom_register_access = {{0xE6AC, 0xE764}}, .send_ptr = {{0x102878, 0xE2A4}},
    };
    // MT6885/MT6883/MT6889/MT6880/MT6890
    db_[0x0816] = Chipconfig{
        .var1 = 10, .watchdog = 0x10007000, .uart = 0x11002000,
        .brom_payload_addr = 0x100A00, .da_payload_addr = 0x201000, .pl_payload_addr = 0x40200000,
        .cqdma_base = 0x10212000, .ap_dma_mem = 0x11000C20, .sej_base = 0x1000A000,
        .dxcc_base = 0x10210000, .gcpu_base = 0x10050000, .meid_addr = 0x102B78,
        .socid_addr = 0x102B88, .prov_addr = 0x1066C0, .misc_lock = 0x1001A100,
        .efuse_addr = 0x11C10000, .damode = 5, .dacode = 26757,
        .has64bit = false, .iot = false, .name = "MT6885/MT6883/MT6889/MT6880/MT6890",
        .description = "Dimensity 1000L/1000", .loader = "mt6885_payload.bin", .blacklist = {{0x102848, 0x0}, {0x106B60, 0x0}},
        .brom_register_access = {{0xEB04, 0xEBBC}}, .send_ptr = {{0x102888, 0xE6FC}},
    };
    // MT6873
    db_[0x0886] = Chipconfig{
        .var1 = 10, .watchdog = 0x10007000, .uart = 0x11002000,
        .brom_payload_addr = 0x100A00, .da_payload_addr = 0x201000, .pl_payload_addr = 0x40200000,
        .cqdma_base = 0x10212000, .ap_dma_mem = 0x10217C20, .sej_base = 0x1000A000,
        .dxcc_base = 0x10210000, .gcpu_base = 0x10050000, .meid_addr = 0x102B78,
        .socid_addr = 0x102B88, .prov_addr = 0x1066C0, .misc_lock = 0x1001A100,
        .efuse_addr = 0x11C10000, .damode = 5, .dacode = 26739,
        .has64bit = false, .iot = false, .name = "MT6873",
        .description = "Dimensity 800/820 5G", .loader = "mt6873_payload.bin", .blacklist = {{0x10284C, 0x0}, {0x106B60, 0x0}},
        .brom_register_access = {{0xEE80, 0xEF38}}, .send_ptr = {{0x10288C, 0xEA78}},
    };
    // MT6983
    db_[0x0907] = Chipconfig{
        .var1 = 10, .watchdog = 0x1C007000, .uart = 0x11001000,
        .brom_payload_addr = 0x100A00, .da_payload_addr = 0x201000, .pl_payload_addr = 0x40200000,
        .cqdma_base = 0x10212000, .ap_dma_mem = 0x113009A0, .sej_base = 0x1000A000,
        .dxcc_base = 0x10210000, .gcpu_base = 0x10050000, .meid_addr = 0x1008EC,
        .socid_addr = 0x100934, .efuse_addr = 0x11EE0000, .damode = 6,
        .dacode = 2311, .has64bit = true, .iot = false,
        .name = "MT6983", .description = "Dimensity 9000/9000+",
    };
    // MT8696
    db_[0x0908] = Chipconfig{
        .watchdog = 0x10007000, .brom_payload_addr = 0x100A00, .da_payload_addr = 0x201000,
        .efuse_addr = 0x11C10000, .damode = 5, .dacode = 34454,
        .has64bit = false, .iot = false, .name = "MT8696",
        .description = "",
    };
    // MT8195 Chromebook
    db_[0x0930] = Chipconfig{
        .watchdog = 0x10007000, .uart = 0x11001200, .brom_payload_addr = 0x100A00,
        .da_payload_addr = 0x201000, .pl_payload_addr = 0x40200000, .misc_lock = 0x1001A100,
        .efuse_addr = 0x11C10000, .damode = 5, .dacode = 33173,
        .has64bit = false, .iot = false, .name = "MT8195 Chromebook",
        .description = "",
    };
    // MT6891/MT6893
    db_[0x0950] = Chipconfig{
        .var1 = 10, .watchdog = 0x10007000, .uart = 0x11002000,
        .brom_payload_addr = 0x100A00, .da_payload_addr = 0x201000, .pl_payload_addr = 0x40200000,
        .cqdma_base = 0x10212000, .ap_dma_mem = 0x11000C20, .sej_base = 0x1000A000,
        .dxcc_base = 0x10210000, .gcpu_base = 0x10050000, .meid_addr = 0x102B98,
        .socid_addr = 0x102BA8, .prov_addr = 0x1066C0, .efuse_addr = 0x11C10000,
        .damode = 5, .dacode = 26771, .has64bit = false,
        .iot = false, .name = "MT6891/MT6893", .description = "Dimensity 1200",
        .loader = "mt6893_payload.bin", .blacklist = {{0x102848, 0x0}, {0x106B60, 0x0}}, .brom_register_access = {{0xEBA4, 0xEC5C}},
        .send_ptr = {{0x102888, 0xE79C}},
    };
    // MT6877/MT6877V/MT8791N
    db_[0x0959] = Chipconfig{
        .var1 = 10, .watchdog = 0x10007000, .uart = 0x11002000,
        .brom_payload_addr = 0x100A00, .da_payload_addr = 0x201000, .pl_payload_addr = 0x40200000,
        .cqdma_base = 0x10212000, .ap_dma_mem = 0x10217C20, .sej_base = 0x1000A000,
        .dxcc_base = 0x10210000, .gcpu_base = 0x10050000, .meid_addr = 0x102B98,
        .socid_addr = 0x102BA8, .prov_addr = 0x1066C0, .efuse_addr = 0x11F10000,
        .damode = 5, .dacode = 26743, .has64bit = false,
        .iot = false, .name = "MT6877/MT6877V/MT8791N", .description = "Dimensity 900/1080/7050",
        .loader = "mt6877_payload.bin", .blacklist = {{0x102848, 0x0}, {0x106B60, 0x0}}, .brom_register_access = {{0xECD8, 0xED90}},
        .send_ptr = {{0x102888, 0xE8D0}},
    };
    // MT6833
    db_[0x0989] = Chipconfig{
        .var1 = 115, .watchdog = 0x10007000, .uart = 0x11002000,
        .brom_payload_addr = 0x100A00, .da_payload_addr = 0x201000, .pl_payload_addr = 0x40200000,
        .cqdma_base = 0x10212000, .ap_dma_mem = 0x10217C20, .sej_base = 0x1000A000,
        .dxcc_base = 0x10210000, .gcpu_base = 0x10050000, .meid_addr = 0x102B98,
        .socid_addr = 0x102BA8, .prov_addr = 0x1066B4, .efuse_addr = 0x11C10000,
        .damode = 5, .dacode = 26675, .has64bit = false,
        .iot = false, .name = "MT6833", .description = "Dimensity 700 5G k6833",
        .loader = "mt6833_payload.bin", .blacklist = {{0x102844, 0x0}, {0x106B54, 0x0}}, .brom_register_access = {{0xE3E8, 0xE4A0}},
        .send_ptr = {{0x102884, 0xDFE0}},
    };
    // MT6880/MT6890 Modem
    db_[0x0992] = Chipconfig{
        .watchdog = 0x10007000, .uart = 0x11003000, .brom_payload_addr = 0x100A00,
        .da_payload_addr = 0x201000, .sej_base = 0x1000A000, .gcpu_base = 0x10360000,
        .efuse_addr = 0x11EC0000, .damode = 5, .dacode = 2450,
        .has64bit = false, .iot = false, .name = "MT6880/MT6890 Modem",
        .description = "",
    };
    // MT6853
    db_[0x0996] = Chipconfig{
        .var1 = 10, .watchdog = 0x10007000, .uart = 0x11002000,
        .brom_payload_addr = 0x100A00, .da_payload_addr = 0x201000, .pl_payload_addr = 0x40200000,
        .cqdma_base = 0x10212000, .ap_dma_mem = 0x10217C20, .sej_base = 0x1000A000,
        .dxcc_base = 0x10210000, .gcpu_base = 0x10050000, .meid_addr = 0x102B78,
        .socid_addr = 0x102B88, .prov_addr = 0x1066C0, .misc_lock = 0x1001A100,
        .efuse_addr = 0x11C10000, .damode = 5, .dacode = 26707,
        .has64bit = false, .iot = false, .name = "MT6853",
        .description = "Dimensity 720 5G", .loader = "mt6853_payload.bin", .blacklist = {{0x10284C, 0x0}, {0x106B60, 0x0}},
        .brom_register_access = {{0xEE6C, 0xEF24}}, .send_ptr = {{0x10288C, 0xEA64}},
    };
    // MT6781
    db_[0x1066] = Chipconfig{
        .var1 = 115, .watchdog = 0x10007000, .uart = 0x11002000,
        .brom_payload_addr = 0x100A00, .da_payload_addr = 0x201000, .pl_payload_addr = 0x40200000,
        .sej_base = 0x1000A000, .dxcc_base = 0x10210000, .gcpu_base = 0x10050000,
        .meid_addr = 0x102B98, .socid_addr = 0x102BA8, .efuse_addr = 0x11CB0000,
        .damode = 5, .dacode = 26497, .has64bit = false,
        .iot = false, .name = "MT6781", .description = "Helio G96",
        .loader = "mt6781_payload.bin", .blacklist = {{0x10284C, 0x106B54}}, .brom_register_access = {{0xE9DC, 0xEA94}},
        .send_ptr = {{0x102890, 0xE5D8}},
    };
    // MT6855
    db_[0x1129] = Chipconfig{
        .var1 = 10, .watchdog = 0x1C007000, .uart = 0x11001000,
        .brom_payload_addr = 0x100A00, .da_payload_addr = 0x201000, .pl_payload_addr = 0x40200000,
        .cqdma_base = 0x10212000, .ap_dma_mem = 0x113009A0, .sej_base = 0x1000A000,
        .dxcc_base = 0x10210000, .gcpu_base = 0x10050000, .meid_addr = 0x1008EC,
        .socid_addr = 0x100934, .damode = 6, .dacode = 4393,
        .has64bit = false, .iot = false, .name = "MT6855",
        .description = "Dimensity 8100",
    };
    // MT6895
    db_[0x1172] = Chipconfig{
        .var1 = 10, .watchdog = 0x1C007000, .uart = 0x11001000,
        .brom_payload_addr = 0x100A00, .da_payload_addr = 0x201000, .pl_payload_addr = 0x40200000,
        .cqdma_base = 0x10212000, .ap_dma_mem = 0x113009A0, .sej_base = 0x1C009000,
        .dxcc_base = 0x1C807000, .gcpu_base = 0x10050000, .meid_addr = 0x1008EC,
        .socid_addr = 0x100934, .efuse_addr = 0x11F10000, .damode = 6,
        .dacode = 4466, .has64bit = false, .iot = false,
        .name = "MT6895", .description = "Dimensity 8200",
    };
    // MT6897
    db_[0x1203] = Chipconfig{
        .var1 = 10, .watchdog = 0x1C007000, .uart = 0x11002000,
        .brom_payload_addr = 0x100A00, .da_payload_addr = 0x201000, .pl_payload_addr = 0x40200000,
        .sej_base = 0x1040E000, .ssr_base = 0x10400000, .ssr_clk_base = 0x10400000,
        .gcpu_base = 0x1000D000, .socid_addr = 0x20E7090, .damode = 6,
        .dacode = 4611, .has64bit = false, .iot = false,
        .name = "MT6897", .description = "Dimensity 8300 Ultra",
    };
    // MT6789/MT8781V
    db_[0x1208] = Chipconfig{
        .var1 = 10, .watchdog = 0x10007000, .uart = 0x11002000,
        .brom_payload_addr = 0x100A00, .da_payload_addr = 0x201000, .pl_payload_addr = 0x40200000,
        .sej_base = 0x1000A000, .dxcc_base = 0x10210000, .meid_addr = 0x1008EC,
        .socid_addr = 0x100934, .efuse_addr = 0x11C10000, .damode = 6,
        .dacode = 4616, .has64bit = false, .iot = false,
        .name = "MT6789/MT8781V", .description = "MTK Helio G99", .blacklist = {{0x102D5C, 0x0}},
        .brom_register_access = {{0xF99A, 0xFA0C}},
    };
    // MT6835V/ZA
    db_[0x1209] = Chipconfig{
        .var1 = 10, .watchdog = 0x1C007000, .uart = 0x11002000,
        .da_payload_addr = 0x2001000, .pl_payload_addr = 0x40200000, .sej_base = 0x1000A000,
        .dxcc_base = 0x10210000, .meid_addr = 0x1008EC, .socid_addr = 0x100934,
        .efuse_addr = 0x11C10000, .damode = 6, .dacode = 4617,
        .has64bit = false, .iot = false, .name = "MT6835V/ZA",
        .description = "MTK Dimensity 6100+",
    };
    // MT6886
    db_[0x1229] = Chipconfig{
        .var1 = 10, .watchdog = 0x1C007000, .uart = 0x11002000,
        .brom_payload_addr = 0x100A00, .da_payload_addr = 0x2001000, .pl_payload_addr = 0x40200000,
        .sej_base = 0x1C009000, .dxcc_base = 0x1C807000, .gcpu_base = 0x1000D000,
        .meid_addr = 0x1008EC, .socid_addr = 0x100934, .efuse_addr = 0x11E30000,
        .damode = 6, .dacode = 4649, .has64bit = true,
        .iot = false, .name = "MT6886", .description = "Dimensity 7200 Ultra",
    };
    // MT6989W
    db_[0x1236] = Chipconfig{
        .watchdog = 0x1C00B000, .brom_payload_addr = 0x100A00, .da_payload_addr = 0x2001000,
        .pl_payload_addr = 0x40200000, .sej_base = 0x1040E000, .ssr_base = 0x10400000,
        .ssr_clk_base = 0x10400000, .meid_addr = 0x1008EC, .socid_addr = 0x100934,
        .efuse_addr = 0x11F10000, .damode = 6, .dacode = 4662,
        .has64bit = true, .iot = false, .name = "MT6989W",
        .description = "Dimensity 9300 Plus",
    };
    // MT6985
    db_[0x1296] = Chipconfig{
        .var1 = 10, .watchdog = 0x1C007000, .uart = 0x1C011000,
        .brom_payload_addr = 0x100A00, .da_payload_addr = 0x201000, .pl_payload_addr = 0x40200000,
        .sej_base = 0x1C009000, .dxcc_base = 0x1C807000, .meid_addr = 0x1008EC,
        .socid_addr = 0x100934, .efuse_addr = 0x11E80000, .damode = 6,
        .dacode = 4758, .has64bit = true, .iot = false,
        .name = "MT6985", .description = "Dimensity 9200/9200+",
    };
    // MT6991
    db_[0x1357] = Chipconfig{
        .var1 = 10, .watchdog = 0x1C010000, .uart = 0x16000000,
        .brom_payload_addr = 0x100A00, .da_payload_addr = 0x201000, .pl_payload_addr = 0x40200000,
        .sej_base = 0x1800E000, .ssr_base = 0x18003000, .ssr_clk_base = 0x18000000,
        .damode = 6, .dacode = 4951, .has64bit = false,
        .iot = false, .name = "MT6991", .description = "Dimensity 9400 Ultra",
    };
    // MT6878
    db_[0x1375] = Chipconfig{
        .var1 = 10, .watchdog = 0x1C00A000, .uart = 0x11001000,
        .brom_payload_addr = 0x100A00, .da_payload_addr = 0x2010000, .pl_payload_addr = 0x40200000,
        .sej_base = 0x1040E000, .dxcc_base = 0x1040C000, .ssr_base = 0x10400000,
        .ssr_clk_base = 0x10400000, .efuse_addr = 0x11F10000, .damode = 6,
        .dacode = 4981, .has64bit = true, .iot = false,
        .name = "MT6878", .description = "Dimensity 7300",
    };
    // MT6993
    db_[0x1471] = Chipconfig{
        .var1 = 10, .watchdog = 0x1C010000, .uart = 0x16010000,
        .brom_payload_addr = 0x100A00, .da_payload_addr = 0x201000, .pl_payload_addr = 0x40200000,
        .sej_base = 0x1800E000, .ssr_base = 0x18003000, .ssr_clk_base = 0x18000000,
        .meid_addr = 0x1008EC, .socid_addr = 0x100934, .efuse_addr = 0x10160000,
        .damode = 6, .dacode = 5233, .has64bit = true,
        .iot = false, .name = "MT6993", .description = "Dimensity 9500",
    };
    // MT2523
    db_[0x2523] = Chipconfig{
        .var1 = 10, .watchdog = 0x10007000, .uart = 0x11005000,
        .brom_payload_addr = 0x100A00, .da_payload_addr = 0x2008000, .pl_payload_addr = 0x81E00000,
        .sej_base = 0x1000A000, .meid_addr = 0x11142C34, .damode = 3,
        .dacode = 9507, .has64bit = false, .iot = true,
        .name = "MT2523", .description = "", .send_ptr = {{0x11141F4C, 0xBA68}},
    };
    // MT2601
    db_[0x2601] = Chipconfig{
        .var1 = 10, .watchdog = 0x10007000, .uart = 0x11005000,
        .brom_payload_addr = 0x100A00, .da_payload_addr = 0x2008000, .pl_payload_addr = 0x81E00000,
        .sej_base = 0x1000A000, .meid_addr = 0x11142C34, .damode = 3,
        .dacode = 9729, .has64bit = false, .iot = true,
        .name = "MT2601", .description = "", .loader = "mt2601_payload.bin",
        .blacklist = {{0x11141F0C, 0x0}, {0x11144BC4, 0x0}}, .brom_register_access = {{0x40BD48, 0x40BEFC}}, .send_ptr = {{0x11141F4C, 0xBA68}},
    };
    // MT2625
    db_[0x2625] = Chipconfig{
        .var1 = 10, .watchdog = 0x10007000, .uart = 0x11005000,
        .brom_payload_addr = 0x100A00, .da_payload_addr = 0x4001000, .sej_base = 0x1000A000,
        .damode = 3, .dacode = 9765, .has64bit = false,
        .iot = true, .name = "MT2625", .description = "",
    };
    // MT3967
    db_[0x3967] = Chipconfig{
        .brom_payload_addr = 0x100A00, .da_payload_addr = 0x201000, .pl_payload_addr = 0x40020000,
        .damode = 3, .dacode = 14695, .has64bit = false,
        .iot = false, .name = "MT3967", .description = "",
    };
    // MT5932
    db_[0x5932] = Chipconfig{
        .brom_payload_addr = 0x100A00, .da_payload_addr = 0x201000, .pl_payload_addr = 0x40020000,
        .damode = 3, .dacode = 22834, .has64bit = false,
        .iot = true, .name = "MT5932", .description = "",
    };
    // MT6225
    db_[0x6225] = Chipconfig{
        .sej_base = 0x80140000, .damode = 3, .dacode = 25125,
        .has64bit = false, .iot = true, .name = "MT6225",
        .description = "",
    };
    // MT6226
    db_[0x6226] = Chipconfig{
        .sej_base = 0x80140000, .damode = 3, .dacode = 25126,
        .has64bit = false, .iot = true, .name = "MT6226",
        .description = "",
    };
    // MT6236
    db_[0x6236] = Chipconfig{
        .damode = 3, .dacode = 25142, .has64bit = false,
        .iot = true, .name = "MT6236", .description = "",
    };
    // MT6238
    db_[0x6238] = Chipconfig{
        .damode = 3, .dacode = 25144, .has64bit = false,
        .iot = true, .name = "MT6238", .description = "",
    };
    // MT6253
    db_[0x6253] = Chipconfig{
        .damode = 3, .dacode = 25171, .has64bit = false,
        .iot = true, .name = "MT6253", .description = "",
    };
    // MT6255
    db_[0x6255] = Chipconfig{
        .sej_base = 0x80140000, .damode = 3, .dacode = 25173,
        .has64bit = false, .iot = true, .name = "MT6255",
        .description = "",
    };
    // MT6256
    db_[0x6256] = Chipconfig{
        .damode = 3, .dacode = 25174, .has64bit = false,
        .iot = true, .name = "MT6256", .description = "",
    };
    // MT625a
    db_[0x625A] = Chipconfig{
        .damode = 3, .dacode = 25178, .has64bit = false,
        .iot = true, .name = "MT625a", .description = "",
    };
    // MT6261/MT2503
    db_[0x6261] = Chipconfig{
        .var1 = 40, .watchdog = 0xA0030000, .uart = 0xA0080000,
        .brom_payload_addr = 0x100A00, .da_payload_addr = 0x201000, .sej_base = 0xA0110000,
        .damode = 3, .dacode = 25185, .has64bit = false,
        .iot = true, .name = "MT6261/MT2503", .description = "",
        .loader = "mt6261_payload.bin", .blacklist = {{0xE003FC83, 0x0}}, .send_ptr = {{0x700044B0, 0x700058EC}},
    };
    // MT6268
    db_[0x6268] = Chipconfig{
        .damode = 3, .dacode = 25192, .has64bit = false,
        .iot = true, .name = "MT6268", .description = "",
    };
    // MT6270
    db_[0x6270] = Chipconfig{
        .damode = 3, .dacode = 25200, .has64bit = false,
        .iot = true, .name = "MT6270", .description = "",
    };
    // MT6276
    db_[0x6276] = Chipconfig{
        .damode = 3, .dacode = 25206, .has64bit = false,
        .iot = true, .name = "MT6276", .description = "",
    };
    // MT6280
    db_[0x6280] = Chipconfig{
        .sej_base = 0x80080000, .damode = 3, .has64bit = false,
        .iot = true, .name = "MT6280", .description = "",
    };
    // MT6291
    db_[0x6291] = Chipconfig{
        .damode = 3, .dacode = 25233, .has64bit = false,
        .iot = true, .name = "MT6291", .description = "",
    };
    // MT6516
    db_[0x6516] = Chipconfig{
        .watchdog = 0x10003000, .uart = 0x10023000, .da_payload_addr = 0x201000,
        .sej_base = 0x1002D000, .damode = 3, .dacode = 25878,
        .has64bit = false, .iot = false, .name = "MT6516",
        .description = "",
    };
    // MT6571
    db_[0x6571] = Chipconfig{
        .watchdog = 0x10007400, .da_payload_addr = 0x2009000, .pl_payload_addr = 0x80001000,
        .misc_lock = 0x1000141C, .damode = 3, .dacode = 25969,
        .has64bit = false, .iot = false, .name = "MT6571",
        .description = "",
    };
    // MT6572
    db_[0x6572] = Chipconfig{
        .var1 = 10, .watchdog = 0x10007000, .uart = 0x11005000,
        .brom_payload_addr = 0x10036A0, .da_payload_addr = 0x2008000, .pl_payload_addr = 0x81E00000,
        .ap_dma_mem = 0x1100019C, .meid_addr = 0x11142C34, .misc_lock = 0x1000141C,
        .efuse_addr = 0x10009000, .damode = 3, .dacode = 25970,
        .has64bit = false, .iot = false, .name = "MT6572",
        .description = "", .loader = "mt6572_payload.bin", .blacklist = {{0x11141F0C, 0x0}, {0x11144BC4, 0x0}},
        .brom_register_access = {{0x40BD48, 0x40BEFC}}, .send_ptr = {{0x11141F4C, 0x40BA68}},
    };
    // MT6573/MT6260
    db_[0x6573] = Chipconfig{
        .watchdog = 0x70025000, .da_payload_addr = 0x90006000, .pl_payload_addr = 0xF1020000,
        .sej_base = 0x7002A000, .damode = 3, .dacode = 25971,
        .has64bit = false, .iot = false, .name = "MT6573/MT6260",
        .description = "",
    };
    // MT6575/MT8317
    db_[0x6575] = Chipconfig{
        .watchdog = 0xC0000000, .uart = 0xC1009000, .brom_payload_addr = 0xF0000A00,
        .da_payload_addr = 0xC2001000, .pl_payload_addr = 0xC2058000, .ap_dma_mem = 0xC100119C,
        .sej_base = 0xC101A000, .meid_addr = 0xF0002AF4, .efuse_addr = 0xC1019000,
        .damode = 3, .dacode = 25973, .has64bit = false,
        .iot = false, .name = "MT6575/MT8317", .description = "",
        .loader = "mt6575_payload.bin", .brom_register_access = {{0xFFFFA3AA, 0xFFFFA4C4}}, .send_ptr = {{0xF00025FC, 0xFFFFA0A0}},
    };
    // MT6577
    db_[0x6577] = Chipconfig{
        .watchdog = 0xC0000000, .uart = 0xC1009000, .da_payload_addr = 0xC2001000,
        .pl_payload_addr = 0xC2058000, .ap_dma_mem = 0xC100119C, .sej_base = 0xC101A000,
        .damode = 3, .dacode = 25975, .has64bit = false,
        .iot = false, .name = "MT6577", .description = "",
    };
    // MT6580
    db_[0x6580] = Chipconfig{
        .var1 = 172, .watchdog = 0x10007000, .uart = 0x11005000,
        .brom_payload_addr = 0x100A00, .da_payload_addr = 0x201000, .pl_payload_addr = 0x80001000,
        .cqdma_base = 0x1020AC00, .ap_dma_mem = 0x110001A0, .sej_base = 0x1000A000,
        .meid_addr = 0x1030B4, .misc_lock = 0x10001838, .efuse_addr = 0x10009000,
        .damode = 3, .dacode = 25984, .has64bit = false,
        .iot = false, .name = "MT6580", .description = "",
        .loader = "mt6580_payload.bin", .blacklist = {{0x102764, 0x0}, {0x1071D4, 0x0}}, .brom_register_access = {{0xB8E0, 0xBA94}},
        .send_ptr = {{0x1027A4, 0xB60C}},
    };
    // MT6582/MT6574/MT8382
    db_[0x6582] = Chipconfig{
        .var1 = 10, .watchdog = 0x10007000, .uart = 0x11002000,
        .brom_payload_addr = 0x100A00, .da_payload_addr = 0x201000, .pl_payload_addr = 0x80001000,
        .ap_dma_mem = 0x11000320, .sej_base = 0x1000A000, .gcpu_base = 0x1101B000,
        .meid_addr = 0x1030CC, .misc_lock = 0x10002050, .efuse_addr = 0x10206000,
        .damode = 3, .dacode = 25986, .has64bit = false,
        .iot = false, .name = "MT6582/MT6574/MT8382", .description = "",
        .loader = "mt6582_payload.bin", .blacklist = {{0x102788, 0x0}, {0x105BE4, 0x0}}, .brom_register_access = {{0xA8D0, 0xAA84}},
        .send_ptr = {{0x1027C8, 0xA5FC}},
    };
    // MT6583/6589
    db_[0x6583] = Chipconfig{
        .watchdog = 0x10000000, .uart = 0x11006000, .brom_payload_addr = 0x100A00,
        .da_payload_addr = 0x12001000, .pl_payload_addr = 0x80001000, .cqdma_base = 0x10212000,
        .ap_dma_mem = 0x11000320, .sej_base = 0x1000A000, .gcpu_base = 0x10210000,
        .misc_lock = 0x10002050, .damode = 3, .dacode = 25987,
        .has64bit = false, .iot = false, .name = "MT6583/6589",
        .description = "",
    };
    // MT6592/MT8392
    db_[0x6592] = Chipconfig{
        .var1 = 10, .watchdog = 0x10007000, .uart = 0x11002000,
        .brom_payload_addr = 0x100A00, .da_payload_addr = 0x111000, .pl_payload_addr = 0x80001000,
        .cqdma_base = 0x10212000, .ap_dma_mem = 0x11000320, .sej_base = 0x1000A000,
        .gcpu_base = 0x10210000, .meid_addr = 0x1030A8, .misc_lock = 0x10002050,
        .efuse_addr = 0x10206000, .damode = 3, .dacode = 26002,
        .has64bit = false, .iot = false, .name = "MT6592/MT8392",
        .description = "", .loader = "mt6592_payload.bin", .blacklist = {{0x102764, 0x0}, {0x105BF0, 0x0}},
        .brom_register_access = {{0xA838, 0xA9EC}}, .send_ptr = {{0x1027A4, 0xA564}},
    };
    // MT6595
    db_[0x6595] = Chipconfig{
        .var1 = 10, .watchdog = 0x10007000, .uart = 0x11002000,
        .brom_payload_addr = 0x100A00, .da_payload_addr = 0x111000, .ap_dma_mem = 0x110001A0,
        .sej_base = 0x1000A000, .meid_addr = 0x1030A4, .efuse_addr = 0x10206000,
        .damode = 3, .dacode = 26005, .has64bit = false,
        .iot = false, .name = "MT6595", .description = "",
        .loader = "mt6595_payload.bin", .blacklist = {{0x102768, 0x0}, {0x106C88, 0x0}}, .brom_register_access = {{0xB4EC, 0xB6A0}},
        .send_ptr = {{0x1027A8, 0xB218}},
    };
    // MT6752
    db_[0x6752] = Chipconfig{
        .var1 = 40, .watchdog = 0x10007000, .uart = 0x11002000,
        .brom_payload_addr = 0x100A00, .da_payload_addr = 0x201000, .pl_payload_addr = 0x40001000,
        .cqdma_base = 0x10212C00, .ap_dma_mem = 0x110001A0, .sej_base = 0x1000A000,
        .gcpu_base = 0x10210000, .meid_addr = 0x1030B4, .efuse_addr = 0x10206000,
        .damode = 3, .dacode = 26450, .has64bit = false,
        .iot = false, .name = "MT6752", .description = "",
        .loader = "mt6752_payload.bin", .blacklist = {{0x102764, 0x0}, {0x105704, 0x0}}, .brom_register_access = {{0x9BE0, 0x9DA8}},
        .send_ptr = {{0x1027A4, 0x990C}},
    };
    // MT6795
    db_[0x6795] = Chipconfig{
        .var1 = 10, .watchdog = 0x10007000, .uart = 0x11002000,
        .brom_payload_addr = 0x100A00, .da_payload_addr = 0x110000, .pl_payload_addr = 0x40200000,
        .cqdma_base = 0x10212C00, .ap_dma_mem = 0x110001A0, .sej_base = 0x1000A000,
        .gcpu_base = 0x10210000, .meid_addr = 0x1030A0, .efuse_addr = 0x10206000,
        .damode = 3, .dacode = 26517, .has64bit = false,
        .iot = false, .name = "MT6795", .description = "Helio X10",
        .loader = "mt6795_payload.bin", .blacklist = {{0x102764, 0x0}, {0x105704, 0x0}}, .brom_register_access = {{0x9A60, 0x9C28}},
        .send_ptr = {{0x1027A4, 0x978C}},
    };
    // MT6899
    db_[0x6899] = Chipconfig{
        .var1 = 10, .watchdog = 0x1C00B000, .uart = 0x11001000,
        .brom_payload_addr = 0x100A00, .da_payload_addr = 0x201000, .pl_payload_addr = 0x40200000,
        .sej_base = 0x1040E000, .ssr_base = 0x10400000, .ssr_clk_base = 0x10000000,
        .efuse_addr = 0x11F10000, .damode = 6, .dacode = 26777,
        .has64bit = true, .iot = false, .name = "MT6899",
        .description = "Dimensity 8400 Turbo/Ultra",
    };
    // MT7682
    db_[0x7682] = Chipconfig{
        .brom_payload_addr = 0x100A00, .da_payload_addr = 0x201000, .pl_payload_addr = 0x40020000,
        .damode = 3, .dacode = 30338, .has64bit = false,
        .iot = true, .name = "MT7682", .description = "",
    };
    // MT7686
    db_[0x7686] = Chipconfig{
        .brom_payload_addr = 0x100A00, .da_payload_addr = 0x201000, .pl_payload_addr = 0x40020000,
        .damode = 3, .dacode = 30342, .has64bit = false,
        .iot = true, .name = "MT7686", .description = "",
    };
    // MT8127/MT3367/AC8227L
    db_[0x8127] = Chipconfig{
        .var1 = 10, .watchdog = 0x10007000, .uart = 0x11002000,
        .brom_payload_addr = 0x100A00, .da_payload_addr = 0x201000, .pl_payload_addr = 0x80001000,
        .ap_dma_mem = 0x110001A0, .sej_base = 0x1000A000, .gcpu_base = 0x11010000,
        .meid_addr = 0x1031CC, .misc_lock = 0x10002050, .damode = 3,
        .dacode = 33063, .has64bit = false, .iot = false,
        .name = "MT8127/MT3367/AC8227L", .description = "", .loader = "mt8127_payload.bin",
        .blacklist = {{0x102870, 0x0}, {0x106C7C, 0x0}}, .brom_register_access = {{0xB58C, 0xB740}}, .send_ptr = {{0x1028B0, 0xB2B8}},
    };
    // MT8135
    db_[0x8135] = Chipconfig{
        .watchdog = 0x10000000, .uart = 0x11002000, .da_payload_addr = 0x12001000,
        .pl_payload_addr = 0x80001000, .gcpu_base = 0x11018000, .damode = 3,
        .dacode = 33077, .has64bit = false, .iot = false,
        .name = "MT8135", .description = "",
    };
    // MT8163
    db_[0x8163] = Chipconfig{
        .var1 = 177, .watchdog = 0x10007000, .uart = 0x11002000,
        .brom_payload_addr = 0x100A00, .da_payload_addr = 0x201000, .pl_payload_addr = 0x40001000,
        .cqdma_base = 0x10212C00, .ap_dma_mem = 0x110001A0, .sej_base = 0x1000A000,
        .gcpu_base = 0x10210000, .meid_addr = 0x1031C0, .misc_lock = 0x10002050,
        .efuse_addr = 0x10206000, .damode = 3, .dacode = 33123,
        .has64bit = false, .iot = false, .name = "MT8163",
        .description = "", .loader = "mt8163_payload.bin", .blacklist = {{0x102868, 0x0}, {0x1072DC, 0x0}},
        .brom_register_access = {{0xC400, 0xC5C8}}, .send_ptr = {{0x1028A8, 0xC12C}},
    };
    // MT8167/MT8516/MT8362
    db_[0x8167] = Chipconfig{
        .var1 = 204, .watchdog = 0x10007000, .uart = 0x11005000,
        .brom_payload_addr = 0x100A00, .da_payload_addr = 0x201000, .pl_payload_addr = 0x40001000,
        .cqdma_base = 0x10212C00, .ap_dma_mem = 0x110001A0, .sej_base = 0x1000A000,
        .gcpu_base = 0x1020D000, .meid_addr = 0x103478, .socid_addr = 0x103488,
        .efuse_addr = 0x10009000, .damode = 5, .dacode = 33127,
        .has64bit = false, .iot = false, .name = "MT8167/MT8516/MT8362",
        .description = "", .loader = "mt8167_payload.bin", .blacklist = {{0x102968, 0x0}, {0x107954, 0x0}},
        .brom_register_access = {{0xD6F2, 0xD7AC}}, .send_ptr = {{0x1029AC, 0xD2E4}},
    };
    // MT8168/MT6357
    db_[0x8168] = Chipconfig{
        .var1 = 10, .watchdog = 0x10007000, .uart = 0x11002000,
        .brom_payload_addr = 0x100A00, .da_payload_addr = 0x201000, .pl_payload_addr = 0x40001000,
        .ap_dma_mem = 0x11000420, .sej_base = 0x1000A000, .gcpu_base = 0x10241000,
        .meid_addr = 0x106438, .socid_addr = 0x106448, .efuse_addr = 0x10009000,
        .damode = 5, .dacode = 33128, .has64bit = false,
        .iot = false, .name = "MT8168/MT6357", .description = "",
        .loader = "mt8168_payload.bin", .blacklist = {{0x10303C, 0x0}, {0x10A540, 0x0}}, .brom_register_access = {{0x13C18, 0x13D78}},
        .send_ptr = {{0x103080, 0x13834}},
    };
    // MT8173
    db_[0x8172] = Chipconfig{
        .var1 = 10, .watchdog = 0x10007000, .uart = 0x11002000,
        .brom_payload_addr = 0x120A00, .da_payload_addr = 0xC0000, .pl_payload_addr = 0x40001000,
        .cqdma_base = 0x10212C00, .ap_dma_mem = 0x110001A0, .sej_base = 0x1000A000,
        .gcpu_base = 0x10210000, .meid_addr = 0x1230B0, .misc_lock = 0x1202050,
        .damode = 3, .dacode = 33139, .has64bit = false,
        .iot = false, .name = "MT8173", .description = "",
        .loader = "mt8173_payload.bin", .blacklist = {{0x122774, 0x0}, {0x125904, 0x0}}, .brom_register_access = {{0xA3B8, 0xA580}},
        .send_ptr = {{0x1227B4, 0xA0E4}},
    };
    // MT8176
    db_[0x8176] = Chipconfig{
        .var1 = 10, .watchdog = 0x10007000, .uart = 0x11002000,
        .brom_payload_addr = 0x120A00, .da_payload_addr = 0xC0000, .pl_payload_addr = 0x40200000,
        .cqdma_base = 0x10212C00, .ap_dma_mem = 0x110001A0, .sej_base = 0x1000A000,
        .gcpu_base = 0x10210000, .meid_addr = 0x1230B0, .misc_lock = 0x1202050,
        .efuse_addr = 0x10206000, .damode = 3, .dacode = 33139,
        .has64bit = false, .iot = false, .name = "MT8176",
        .description = "", .loader = "mt8176_payload.bin", .blacklist = {{0x122774, 0x0}, {0x125904, 0x0}},
        .brom_register_access = {{0xA3B8, 0xA580}}, .send_ptr = {{0x1227B4, 0xA0E4}},
    };
    // MT8512
    db_[0x8512] = Chipconfig{
        .var1 = 10, .watchdog = 0x10007000, .uart = 0x11002000,
        .brom_payload_addr = 0x100A00, .da_payload_addr = 0x111000, .pl_payload_addr = 0x40200000,
        .cqdma_base = 0x10214000, .ap_dma_mem = 0x110001A0, .sej_base = 0x1000A000,
        .gcpu_base = 0x1020F000, .meid_addr = 0x104638, .socid_addr = 0x104648,
        .efuse_addr = 0x11C50000, .damode = 5, .dacode = 34066,
        .has64bit = false, .iot = false, .name = "MT8512",
        .description = "", .loader = "mt8512_payload.bin", .blacklist = {{0x1041E4, 0x0}, {0x10AA84, 0x0}},
        .brom_register_access = {{0xD034, 0xD194}}, .send_ptr = {{0x104258, 0xCC44}},
    };
    // MT8518 VoiceAssistant
    db_[0x8518] = Chipconfig{
        .efuse_addr = 0x10009000, .damode = 5, .dacode = 34072,
        .has64bit = false, .iot = false, .name = "MT8518 VoiceAssistant",
        .description = "",
    };
    // MT8590/MT7683/MT8521/MT7623
    db_[0x8590] = Chipconfig{
        .var1 = 10, .watchdog = 0x10007000, .uart = 0x11002000,
        .brom_payload_addr = 0x100A00, .da_payload_addr = 0x201000, .pl_payload_addr = 0x80001000,
        .sej_base = 0x1000A000, .gcpu_base = 0x1101B000, .meid_addr = 0x1031D8,
        .damode = 3, .dacode = 34192, .has64bit = false,
        .iot = false, .name = "MT8590/MT7683/MT8521/MT7623", .description = "",
        .loader = "mt8590_payload.bin", .blacklist = {{0x102870, 0x0}, {0x106C7C, 0x0}}, .brom_register_access = {{0xBEB8, 0xC06C}},
        .send_ptr = {{0x1028B0, 0xBBE4}},
    };
    // MT8695
    db_[0x8695] = Chipconfig{
        .var1 = 10, .watchdog = 0x10007000, .uart = 0x11002000,
        .brom_payload_addr = 0x100A00, .da_payload_addr = 0x201000, .pl_payload_addr = 0x40001000,
        .ap_dma_mem = 0x11000420, .sej_base = 0x1000A000, .meid_addr = 0x1032B8,
        .efuse_addr = 0x10206000, .damode = 5, .dacode = 34453,
        .has64bit = false, .iot = false, .name = "MT8695",
        .description = "", .loader = "mt8695_payload.bin", .blacklist = {{0x103048, 0x0}, {0x106EC4, 0x0}},
        .brom_register_access = {{0xC298, 0xC3F8}}, .send_ptr = {{0x103088, 0xBEEC}},
    };
}

} // namespace mtk
