#include "utils.h"
#include "logger.h"
#include "types.h"
#include <cassert>
#include <cstring>
#include <iostream>

using namespace mtk;

void test_be16_encode() {
    uint8_t buf[2];
    put_be16(buf, 0x1234);
    assert(buf[0] == 0x12);
    assert(buf[1] == 0x34);
    assert(get_be16(buf) == 0x1234);
    LOG_INFO("PASS: be16 encode/decode");
}

void test_be32_encode() {
    uint8_t buf[4];
    put_be32(buf, 0x12345678);
    assert(buf[0] == 0x12);
    assert(buf[1] == 0x34);
    assert(buf[2] == 0x56);
    assert(buf[3] == 0x78);
    assert(get_be32(buf) == 0x12345678);
    LOG_INFO("PASS: be32 encode/decode");
}

void test_le16_encode() {
    uint8_t buf[2];
    put_le16(buf, 0x1234);
    assert(buf[0] == 0x34);
    assert(buf[1] == 0x12);
    assert(get_le16(buf) == 0x1234);
    LOG_INFO("PASS: le16 encode/decode");
}

void test_le32_encode() {
    uint8_t buf[4];
    put_le32(buf, 0x12345678);
    assert(buf[0] == 0x78);
    assert(buf[1] == 0x56);
    assert(buf[2] == 0x34);
    assert(buf[3] == 0x12);
    assert(get_le32(buf) == 0x12345678);
    LOG_INFO("PASS: le32 encode/decode");
}

void test_revdword() {
    assert(revdword(0x12345678) == 0x78563412);
    assert(revdword(0x0000000A) == 0x0A000000);
    LOG_INFO("PASS: revdword");
}

void test_getint() {
    assert(getint("42") == 42);
    assert(getint("0x2A") == 0x2A);
    assert(getint("0xFF") == 255);
    assert(getint("invalid") == 0);
    LOG_INFO("PASS: getint");
}

void test_calc_checksum() {
    uint8_t data[] = {0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00};
    uint32_t cs = calc_xflash_checksum(data, sizeof(data));
    assert(cs == 3); // 1 + 2 = 3
    LOG_INFO("PASS: calc_xflash_checksum");
}

void test_find_binary() {
    uint8_t haystack[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05};
    uint8_t needle[] = {0x02, 0x03};
    assert(find_binary(haystack, sizeof(haystack), needle, sizeof(needle)) == 2);
    uint8_t missing[] = {0x06, 0x07};
    assert(find_binary(haystack, sizeof(haystack), missing, sizeof(missing)) == -1);
    LOG_INFO("PASS: find_binary");
}

void test_handshake_constants() {
    assert(HANDSHAKE_BYTES[0] == 0xA0);
    assert(HANDSHAKE_ECHO[0] == 0x5F);
    LOG_INFO("PASS: handshake constants");
}

void test_ack_values() {
    assert(ACK_PAYLOAD == 0xA1A2A3A4);
    assert(ACK_DUMP == 0xC1C2C3C4);
    assert(ACK_STAGE2_OK == 0xD0D0D0D0);
    assert(ACK_JUMP_OK == 0xB1B2B3B4);
    LOG_INFO("PASS: ack values");
}

void test_command_constants() {
    assert(CMD_GET_HW_CODE == 0xFD);
    assert(CMD_WRITE32 == 0xD4);
    assert(CMD_READ32 == 0xD1);
    assert(CMD_JUMP_DA == 0xD5);
    assert(CMD_SEND_DA == 0xD7);
    LOG_INFO("PASS: command constants");
}

int main() {
    Logger::instance().set_level(LogLevel::DEBUG);

    std::cout << "Running MTK Payload unit tests...\n";

    test_be16_encode();
    test_be32_encode();
    test_le16_encode();
    test_le32_encode();
    test_revdword();
    test_getint();
    test_calc_checksum();
    test_find_binary();
    test_handshake_constants();
    test_ack_values();
    test_command_constants();

    std::cout << "All tests passed!\n";
    return 0;
}
