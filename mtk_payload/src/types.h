#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <map>
#include <functional>

namespace mtk {

// USB IDs
constexpr uint16_t MTK_VID = 0x0E8D;
constexpr uint16_t BROM_PID = 0x0003;
constexpr uint16_t PRELOADER_PID = 0x6000;

// Handshake bytes
constexpr std::array<uint8_t, 4> HANDSHAKE_BYTES = {0xA0, 0x0A, 0x50, 0x05};
constexpr std::array<uint8_t, 4> HANDSHAKE_ECHO = {0x5F, 0xF5, 0xAF, 0xFA};

// Response codes
constexpr uint8_t RSP_CONF = 0x69;
constexpr uint8_t RSP_STOP = 0x96;
constexpr uint8_t RSP_ACK = 0x5A;
constexpr uint8_t RSP_NACK = 0xA5;

// Ack values (big-endian uint32)
constexpr uint32_t ACK_PAYLOAD = 0xA1A2A3A4;
constexpr uint32_t ACK_DUMP = 0xC1C2C3C4;
constexpr uint32_t ACK_STAGE2_OK = 0xD0D0D0D0;
constexpr uint32_t ACK_JUMP_OK = 0xB1B2B3B4;

// BROM Commands
constexpr uint8_t CMD_SEND_PARTITION_DATA = 0x70;
constexpr uint8_t CMD_JUMP_TO_PARTITION = 0x71;
constexpr uint8_t CMD_CHECK_USB_CMD = 0x72;
constexpr uint8_t CMD_STAY_STILL = 0x80;
constexpr uint8_t CMD_READ16_A2 = 0xA2;
constexpr uint8_t CMD_C8 = 0xC8;
constexpr uint8_t CMD_READ16 = 0xD0;
constexpr uint8_t CMD_READ32 = 0xD1;
constexpr uint8_t CMD_WRITE16 = 0xD2;
constexpr uint8_t CMD_WRITE32 = 0xD4;
constexpr uint8_t CMD_JUMP_DA = 0xD5;
constexpr uint8_t CMD_JUMP_BL = 0xD6;
constexpr uint8_t CMD_SEND_DA = 0xD7;
constexpr uint8_t CMD_GET_TARGET_CONFIG = 0xD8;
constexpr uint8_t CMD_SEND_ENV_PREPARE = 0xD9;
constexpr uint8_t CMD_BROM_REGISTER_ACCESS = 0xDA;
constexpr uint8_t CMD_UART1_LOG_EN = 0xDB;
constexpr uint8_t CMD_UART1_SET_BAUDRATE = 0xDC;
constexpr uint8_t CMD_BROM_DEBUGLOG = 0xDD;
constexpr uint8_t CMD_JUMP_DA64 = 0xDE;
constexpr uint8_t CMD_GET_BROM_LOG_NEW = 0xDF;
constexpr uint8_t CMD_SEND_CERT = 0xE0;
constexpr uint8_t CMD_GET_ME_ID = 0xE1;
constexpr uint8_t CMD_SEND_AUTH = 0xE2;
constexpr uint8_t CMD_SLA = 0xE3;
constexpr uint8_t CMD_GET_SOC_ID = 0xE7;
constexpr uint8_t CMD_GET_HW_SW_VER = 0xFC;
constexpr uint8_t CMD_GET_PL_CAP = 0xFB;
constexpr uint8_t CMD_GET_HW_CODE = 0xFD;
constexpr uint8_t CMD_GET_BL_VER = 0xFE;
constexpr uint8_t CMD_GET_VERSION = 0xFF;

// Default values
constexpr uint32_t DEFAULT_WATCHDOG = 0x10007000;
constexpr uint32_t DEFAULT_UART = 0x11002000;
constexpr uint32_t DEFAULT_BROM_PAYLOAD_ADDR = 0x100A00;
constexpr uint32_t DEFAULT_DA_PAYLOAD_ADDR = 0x200000;
constexpr int DEFAULT_VAR1 = 0xA;

// DA modes
constexpr int DA_LEGACY = 3;
constexpr int DA_XFLASH = 5;
constexpr int DA_XML = 6;

// USBDL constants
constexpr uint32_t USBDL_BIT_EN = 0x00000001;
constexpr uint32_t USBDL_BROM = 0x00000002;
constexpr uint32_t USBDL_TIMEOUT_MASK = 0x0000FFFC;
constexpr uint32_t USBDL_TIMEOUT_MAX = 0x3FFF; // max before <<2 and mask
constexpr uint32_t USBDL_MAGIC = 0x444C0000;
constexpr uint32_t MISC_LOCK_KEY_MAGIC = 0xAD98;

// Preloader magic
constexpr uint32_t PRELOADER_MAGIC = 0x014D4D4D;

// Payload types
enum class PayloadType {
    KAMAKIRI,
    KAMAKIRI2,
    AMONET,
    HASHIMOTO,
    CARBONARA
};

// Target config flags
struct TargetConfig {
    bool sbc = false;    // Secure Boot Chain
    bool sla = false;    // Software License Agreement
    bool daa = false;    // DA Authentication
    bool epp = false;    // EPP_PARAM
    bool cert = false;   // Root cert required
    bool memread = false;  // Memory read auth
    bool memwrite = false; // Memory write auth
    bool cmdC8 = false;    // CMD_C8 blocked
};

} // namespace mtk
