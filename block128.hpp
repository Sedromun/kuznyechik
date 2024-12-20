#pragma once

#include <cstdint>
#include <string>
#include <cstring>
#include <array>
#include <arm_neon.h>


struct block128 {
    std::array<uint8_t, 16> a;

    block128();

    block128(std::array<uint8_t, 16>& a);

    explicit block128(uint64_t num);
    explicit block128(std::string s);

    block128(block128 const& other);
    block128& operator=(block128 const& other);

    std::string to_string();
};
