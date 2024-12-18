#include "kuznyechik.hpp"
#include "block128.hpp"


block128 kuznyechik::get_iterative_const(size_t i) {
    auto bl = block128((uint64_t)i);
    bl.L();
    return bl;
}

void kuznyechik::set_iterative_keys(std::pair<block128, block128> &key) {
    iterative_keys[1] = key.first;
    iterative_keys[2] = key.second;
    std::pair<block128, block128> prev = key;
    for(std::size_t i = 1; i < 5; i++) {
        for (std::size_t j = 1; j < 9; j++) {
            block128 iter_const = get_iterative_const(8 * (i - 1) + j);
            F_k(iter_const, prev);
        }
        iterative_keys[2*i+1] = prev.first;
        iterative_keys[2*i+2] = prev.second;
    }
}

void kuznyechik::encrypt(block128 &plaintext) {
    for(std::size_t i = 1; i <= 10; i++) {
        plaintext.X_k(iterative_keys[i]);
        if (i != 10) {
            plaintext.S();
            plaintext.L();
        }
    }
}

void kuznyechik::decipher(block128 &ciphertext) {
    for(std::size_t i = 10; i >= 1; i--) {
        if (i != 10) {
            ciphertext.L_inv();
            ciphertext.S_inv();
        }
        ciphertext.X_k(iterative_keys[i]);
    }
}

kuznyechik::kuznyechik(std::pair<block128, block128> key) {
    set_iterative_keys(key);
}

void kuznyechik::update_key(std::pair<block128, block128> key) {
    set_iterative_keys(key);
}

block128 *kuznyechik::get_iterative_keys() {
    return iterative_keys;
}
