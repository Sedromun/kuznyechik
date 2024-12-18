#include <cstring>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <arm_neon.h>

#include "block128.hpp"


block128::block128() {}

block128::block128(uint64_t upper, uint64_t lower) : upper(upper), lower(lower) {}

block128::block128(uint64_t num) : lower(num), upper(0) {}

void block128::X_k(block128 &k) {
    upper ^= k.upper;
    lower ^= k.lower;
}

void block128::S() {
    uint64_t res1 = 0, res2 = 0;
    for (uint8_t sft = 0; sft <= 56; sft += 8) {
        res1 |= static_cast<uint64_t>(PI_ARRAY[(upper >> sft) & 0xFF]) << sft;
        res2 |= static_cast<uint64_t>(PI_ARRAY[(lower >> sft) & 0xFF]) << sft;
    }
    upper = res1;
    lower = res2;

}

void block128::S_inv() {

    // TODO
}

void block128::L() {
    R(); R(); R(); R();
    R(); R(); R(); R();
    R(); R(); R(); R();
    R(); R(); R(); R();
}

void block128::R_inv() {
//    uint8_t a15 = a[15];
//    for (size_t i = 15; i > 0; i--) {
//        a[i] = a[i - 1];
//    }
//    a[0] = a15;
//    a[0] = linear_transition(a);
//TODO
}

void block128::L_inv() {
    for (int i = 0; i < 16; i++) {
        R_inv();
    }
}

void F_k(block128 &k, std::pair<block128, block128> &a) {
    auto c = a.second;
    a.second = a.first;
    a.first.X_k(k);
    a.first.S();
    a.first.L();
    a.first.X_k(c);
}

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









