#include <array>
#include "kuznyechik.hpp"
#include "block128.hpp"


block128 kuznyechik::get_iterative_const(size_t i) {
    auto bl = block128((uint64_t)i);
    L(bl);
    return bl;
}

void kuznyechik::set_iterative_keys(std::pair<block128, block128> &key) {
    iterative_keys[1] = key.first;
    decryption_keys[1] = key.first;
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
    for(size_t i = 2; i < 11; i++) {
        decryption_keys[i] = iterative_keys[i];
        L_inv(decryption_keys[i]);
    }
}

void kuznyechik::encrypt(block128 &plaintext) {
    for(std::size_t i = 1; i <= 10; i++) {
        X_k(plaintext, iterative_keys[i]);
        if (i != 10) {
            ApplyLS(plaintext, enc_ls_table);
        }
    }
}

void kuznyechik::decrypt(block128 &ciphertext) {
    ApplyLS(ciphertext, dec_l_table);
    for(std::size_t i = 9; i >= 1; i--) {
        X_k(ciphertext, decryption_keys[i + 1]);
        if (i != 1) {
            ApplyLS(ciphertext, dec_ls_table);
        }
    }
    S_inv(ciphertext);
    X_k(ciphertext, iterative_keys[1]);
}

kuznyechik::kuznyechik(std::pair<block128, block128> key) {
    set_iterative_keys(key);
    GenerateMulTable();
    GenerateEncTable();
    GenerateDecTable();
    GenerateDecLTable();
}


void kuznyechik::update_key(std::pair<block128, block128> key) {
    set_iterative_keys(key);
}

block128 *kuznyechik::get_iterative_keys() {
    return iterative_keys;
}

uint8_t kuznyechik::PolyMul(uint8_t left, uint8_t right)
{
    uint8_t res = 0;
    while (left && right)
    {
        if (right & 1)
            res ^= left;
        left = (left << 1) ^ (left & 0x80 ? 0xC3 : 0x00);
        right >>= 1;
    }
    return res;
}


void kuznyechik::X_k(block128& a, block128 &k) {
    for(size_t i = 0; i < 16; i++) {
        a.a[i] ^= k.a[i];
    }
}

void kuznyechik::S(block128& a) {
    for (size_t i = 0; i < 16; i++) {
        a.a[i] = static_cast<uint64_t>(PI_ARRAY[a.a[i]]);
    }
}

void kuznyechik::S_inv(block128& a) {
    for (size_t i = 0; i < 16; i++) {
        a.a[i] = static_cast<uint64_t>(PI_INV_ARRAY[a.a[i]]);
    }
}

static inline uint8x16_t CastBlock(const uint8_t* ptr) {
    return vld1q_u8(ptr);
}

void kuznyechik::ApplyLS(block128& a, LookupTable& lookup_table)
{
    uint8x16_t d = vld1q_u8(a.a.data());

    uint8x16_t mask = vld1q_u8(mask_arr);

    uint8x16_t tmp1 = vandq_u8(mask, d);
    uint8x16_t tmp2 = vbicq_u8(d, mask);

    uint64x2_t t1_64 = vreinterpretq_u64_u8(tmp1);
    uint64x2_t t2_64 = vreinterpretq_u64_u8(tmp2);

    t1_64 = vshrq_n_u64(t1_64, 4);
    t2_64 = vshlq_n_u64(t2_64, 4);
    tmp1 = vreinterpretq_u8_u64(t1_64);
    tmp2 = vreinterpretq_u8_u64(t2_64);
    uint16x8_t tmp1_u16 = vreinterpretq_u16_u8(tmp1);
    uint16x8_t tmp2_u16 = vreinterpretq_u16_u8(tmp2);
    uint16_t tmp1_array[8];
    uint16_t tmp2_array[8];
    vst1q_u16(tmp1_array, tmp1_u16);
    vst1q_u16(tmp2_array, tmp2_u16);

    const LookupTable& lut = lookup_table;
    const uint8_t* table = &lut[0][0][0];

    uint8x16_t vec1 = vld1q_u8(table + tmp2_array[0] + 0x0000);
    uint8x16_t vec2 = vld1q_u8(table + tmp1_array[0] + 0x1000);

    for(int i=1;i<8;i++){
        uint8x16_t block1 = CastBlock(table + (i * 0x2000) + 0x1000 + tmp1_array[i]);
        uint8x16_t block2 = CastBlock(table + (i * 0x2000) + tmp2_array[i]);
        vec1 = veorq_u8(vec1, block2);
        vec2 = veorq_u8(vec2, block1);
    }
    uint8x16_t final_vec = veorq_u8(vec1, vec2);
    vst1q_u8(a.a.data(), final_vec);
}

void kuznyechik::R(block128 &a) {
    uint8_t val = linear_transition(a);
    std::memmove(a.a.data() + 1, a.a.data(), 15);
    a.a[0] = val;
}

uint8_t kuznyechik::linear_transition(block128& a) {
    uint8_t res = 0;
    for (int i = 0; i < 16; ++i) {
        res ^= PolyMul(TRANSITION_ARRAY[i], a.a[i]);
    }
    return res;
}

void kuznyechik::L(block128& a) {
    R(a); R(a); R(a); R(a);
    R(a); R(a); R(a); R(a);
    R(a); R(a); R(a); R(a);
    R(a); R(a); R(a); R(a);
}

void kuznyechik::R_inv(block128& a) {
    auto c = a.a[0];
    memmove(a.a.data(), a.a.data()+1, 15);
    a.a[15] = c;
    a.a[15] = linear_transition(a);
}

void kuznyechik::L_inv(block128& a) {
    for (int i = 0; i < 16; i++) {
        R_inv(a);
    }
}

void kuznyechik::F_k(block128 &k, std::pair<block128, block128> &a) {
    auto c = a.second;
    a.second = a.first;
    X_k(a.first, k);
    S(a.first);
    L(a.first);
    X_k(a.first, c);
}

void kuznyechik::GenerateMulTable()
{
    for (unsigned i = 0; i < 256; ++i)
        for (unsigned j = 0; j < 256; ++j)
            mul_table[i][j] = PolyMul(i, j);
}

kuznyechik::Matrix kuznyechik::SqrMatrix(const Matrix& mat)
{
    Matrix res{};
    for (size_t i = 0; i < 16; ++i)
        for (size_t j = 0; j < 16; ++j)
            for (size_t k = 0; k < 16; ++k)
                res[i][j] ^= mul_table[mat[i][k]][mat[k][j]];
    return res;
}

void kuznyechik::GenerateEncTable()
{
    Matrix l_matrix;
    for (size_t i = 0; i < 16; ++i)
        for (size_t j = 0; j < 16; ++j)
            if (i == 0)
                l_matrix[i][j] = TRANSITION_ARRAY[j];
            else if (i == j + 1)
                l_matrix[i][j] = 1;
            else
                l_matrix[i][j] = 0;

    for (unsigned i = 0; i < 4; ++i)
        l_matrix = SqrMatrix(l_matrix);

    for (size_t i = 0; i < 16; ++i)
        for (size_t j = 0; j < 256; ++j)
            for (size_t k = 0; k < 16; ++k)
                enc_ls_table[i][j][k] = mul_table[PI_ARRAY[j]][l_matrix[k][i]];
}

void kuznyechik::GenerateDecTable()
{
    Matrix l_matrix;
    for (size_t i = 0; i < 16; ++i)
        for (size_t j = 0; j < 16; ++j)
            if (i == 16 - 1)
                l_matrix[i][j] = TRANSITION_ARRAY[(j + 15) % 16];
            else if (i + 1 == j)
                l_matrix[i][j] = 1;
            else
                l_matrix[i][j] = 0;

    for (unsigned i = 0; i < 4; ++i)
        l_matrix = SqrMatrix(l_matrix);

    for (size_t i = 0; i < 16; ++i)
        for (size_t j = 0; j < 256; ++j)
            for (size_t k = 0; k < 16; ++k)
                dec_ls_table[i][j][k] = mul_table[PI_INV_ARRAY[j]][l_matrix[k][i]];
}

void kuznyechik::GenerateDecLTable()
{
    Matrix l_matrix;
    for (size_t i = 0; i < 16; ++i)
        for (size_t j = 0; j < 16; ++j)
            if (i == 16 - 1)
                l_matrix[i][j] = TRANSITION_ARRAY[(j + 15) % 16];
            else if (i + 1 == j)
                l_matrix[i][j] = 1;
            else
                l_matrix[i][j] = 0;

    for (unsigned i = 0; i < 4; ++i)
        l_matrix = SqrMatrix(l_matrix);

    for (size_t i = 0; i < 16; ++i)
        for (size_t j = 0; j < 256; ++j)
            for (size_t k = 0; k < 16; ++k)
                dec_l_table[i][j][k] = mul_table[j][l_matrix[k][i]];
}
