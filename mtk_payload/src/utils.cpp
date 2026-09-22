#include "utils.h"
#include <cstring>
#include <cstdlib>
#include <algorithm>

namespace mtk {

void put_be16(uint8_t* buf, uint16_t val) {
    buf[0] = static_cast<uint8_t>((val >> 8) & 0xFF);
    buf[1] = static_cast<uint8_t>(val & 0xFF);
}

void put_be32(uint8_t* buf, uint32_t val) {
    buf[0] = static_cast<uint8_t>((val >> 24) & 0xFF);
    buf[1] = static_cast<uint8_t>((val >> 16) & 0xFF);
    buf[2] = static_cast<uint8_t>((val >> 8) & 0xFF);
    buf[3] = static_cast<uint8_t>(val & 0xFF);
}

uint16_t get_be16(const uint8_t* buf) {
    return static_cast<uint16_t>((buf[0] << 8) | buf[1]);
}

uint32_t get_be32(const uint8_t* buf) {
    return (static_cast<uint32_t>(buf[0]) << 24) |
           (static_cast<uint32_t>(buf[1]) << 16) |
           (static_cast<uint32_t>(buf[2]) << 8) |
           static_cast<uint32_t>(buf[3]);
}

void put_le16(uint8_t* buf, uint16_t val) {
    buf[0] = static_cast<uint8_t>(val & 0xFF);
    buf[1] = static_cast<uint8_t>((val >> 8) & 0xFF);
}

void put_le32(uint8_t* buf, uint32_t val) {
    buf[0] = static_cast<uint8_t>(val & 0xFF);
    buf[1] = static_cast<uint8_t>((val >> 8) & 0xFF);
    buf[2] = static_cast<uint8_t>((val >> 16) & 0xFF);
    buf[3] = static_cast<uint8_t>((val >> 24) & 0xFF);
}

uint16_t get_le16(const uint8_t* buf) {
    return static_cast<uint16_t>(buf[0] | (buf[1] << 8));
}

uint32_t get_le32(const uint8_t* buf) {
    return static_cast<uint32_t>(buf[0]) |
           (static_cast<uint32_t>(buf[1]) << 8) |
           (static_cast<uint32_t>(buf[2]) << 16) |
           (static_cast<uint32_t>(buf[3]) << 24);
}

uint32_t revdword(uint32_t val) {
    uint8_t buf[4];
    put_le32(buf, val);
    return get_be32(buf);
}

int getint(const std::string& str) {
    if (str.empty()) return 0;
    try {
        char* end = nullptr;
        long val = std::strtol(str.c_str(), &end, 0);
        if (end != str.c_str()) return static_cast<int>(val);
    } catch (...) {}
    return 0;
}

uint32_t calc_xflash_checksum(const uint8_t* data, size_t len) {
    uint32_t checksum = 0;
    size_t pos = 0;
    for (size_t i = 0; i < len / 4; ++i) {
        checksum += get_le32(data + pos);
        pos += 4;
    }
    if (len % 4 != 0) {
        for (size_t i = 0; i < (4 - (len % 4)); ++i) {
            checksum += data[pos];
            pos += 1;
        }
    }
    return checksum & 0xFFFFFFFF;
}

ssize_t find_binary(const uint8_t* data, size_t data_len,
                    const uint8_t* pattern, size_t pat_len) {
    if (pat_len > data_len) return -1;
    for (size_t i = 0; i <= data_len - pat_len; ++i) {
        if (memcmp(data + i, pattern, pat_len) == 0) {
            return static_cast<ssize_t>(i);
        }
    }
    return -1;
}

} // namespace mtk
