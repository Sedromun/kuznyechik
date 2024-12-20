#include <iostream>
#include <sstream>
#include <iomanip>
#include <arm_neon.h>

#include "block128.hpp"

block128::block128() {}

block128::block128(std::array<uint8_t, 16>& a) : a(a) {}

block128::block128(uint64_t num) {
    a.fill(0);
    for (size_t i = 0; i < 8; ++i) {
        a[i + 8] = static_cast<uint8_t>((num >> (56 - i * 8)) & 0xFF); // Extract each byte
    }
}

block128::block128(std::string s) {
    if (s.length() != 32) {
        std::cerr << "Error: Hex string must have exactly 32 characters." << std::endl;
        block128();
        return;
    }

    for (size_t i = 0; i < 16; i++) {
        a[i] = static_cast<uint8_t>(std::stoi(s.substr(2 * i, 2), nullptr, 16));
    }
}

std::string block128::to_string() {
    std::ostringstream hexStream;

    for (size_t i = 0; i < 16; i++) {
        hexStream << std::hex << std::setw(2) << std::setfill('0') << static_cast<uint32_t>(a[i]);
    }

    return hexStream.str();
}

block128 &block128::operator=(block128 const &other) = default;

block128::block128(block128 const &other) = default;









