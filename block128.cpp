#include <cstring>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <arm_neon.h>

#include "block128.hpp"
#include "kuznyechik.hpp"


block128::block128() {}

block128::block128(uint64_t upper, uint64_t lower) : upper(upper), lower(lower) {}

block128::block128(uint64_t num) : lower(num), upper(0) {}



block128::block128(std::string s) {
    if (s.length() != 32) {
        std::cerr << "Error: Hex string must have exactly 32 characters." << std::endl;
        block128();
        return;
    }

    upper = static_cast<uint64_t>(std::stoull(s.substr(0, 16), nullptr, 16));
    lower = static_cast<uint64_t>(std::stoull(s.substr(16, 16), nullptr, 16));
}

std::string block128::to_string() {
    std::ostringstream hexStream;

    hexStream << std::hex << std::setw(16) << std::setfill('0') << static_cast<uint64_t>(upper);
    hexStream << std::hex << std::setw(16) << std::setfill('0') << static_cast<uint64_t>(lower);

    return hexStream.str();
}

block128 &block128::operator=(block128 const &other) = default;

block128::block128(block128 const &other) = default;









