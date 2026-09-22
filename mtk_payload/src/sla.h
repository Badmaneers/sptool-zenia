#pragma once
#include <vector>
#include <cstdint>
#include <string>

namespace mtk {

struct SlaKey {
    std::string vendor;
    std::string name;
    std::vector<int> da_codes;
    // n, d, e stored as hex strings for GMP/OpenSSL BIGNUM
    std::string n_hex;
    std::string d_hex;
    std::string e_hex;
};

// Returns all BROM SLA keys (try these when device requests SLA)
const std::vector<SlaKey>& get_brom_sla_keys();

// Generate BROM SLA challenge response
// Implements MTK custom RSA sign: byte-swap pairs, PKCS#1 v1.5 pad, raw RSA, byte-swap result
// Returns the signed response, or empty vector on failure
std::vector<uint8_t> generate_brom_sla_challenge(
    const uint8_t* challenge, size_t challenge_len,
    const std::string& n_hex, const std::string& d_hex);

} // namespace mtk
