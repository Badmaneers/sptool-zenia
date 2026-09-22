#pragma once
#include <cstdint>
#include <vector>
#include <string>

namespace mtk {

// Big-endian encoding
void put_be16(uint8_t* buf, uint16_t val);
void put_be32(uint8_t* buf, uint32_t val);
uint16_t get_be16(const uint8_t* buf);
uint32_t get_be32(const uint8_t* buf);

// Little-endian encoding
void put_le16(uint8_t* buf, uint16_t val);
void put_le32(uint8_t* buf, uint32_t val);
uint16_t get_le16(const uint8_t* buf);
uint32_t get_le32(const uint8_t* buf);

// Reverse byte order of a 32-bit value
uint32_t revdword(uint32_t val);

// Parse integer from string (decimal or hex)
int getint(const std::string& str);

// Calculate XFLASH checksum
uint32_t calc_xflash_checksum(const uint8_t* data, size_t len);

// Find binary pattern in data
ssize_t find_binary(const uint8_t* data, size_t data_len,
                    const uint8_t* pattern, size_t pat_len);

} // namespace mtk
