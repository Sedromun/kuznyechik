#pragma once

#include <cstdint>
#include <utility>
#include "block128.hpp"

class kuznyechik {
    block128 iterative_keys[11] = {block128()};

    void set_iterative_keys(std::pair<block128, block128> &key);

public:
    static block128 get_iterative_const(size_t i);

    explicit kuznyechik(std::pair<block128, block128> key);
    block128* get_iterative_keys();

    void update_key(std::pair<block128, block128> key);

    void encrypt(block128 &plaintext);
    void decipher(block128 &ciphertext);
};
