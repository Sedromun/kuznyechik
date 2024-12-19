#include <array>
#include <iostream>
#include "kuznyechik.hpp"
#include "block128.hpp"


kuznyechik::LookupTable& kuznyechik::get_lookup_table() {
    return enc_ls_table;
}

block128 kuznyechik::get_iterative_const(size_t i) {
    auto bl = block128((uint64_t)i);
    L(bl);
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
        X_k(plaintext, iterative_keys[i]);
        if (i != 10) {
            ApplyLS(plaintext);
//            S(plaintext);
//            L(plaintext);
        }
    }
}

void kuznyechik::decipher(block128 &ciphertext) {
    for(std::size_t i = 10; i >= 1; i--) {
        if (i != 10) {
            L_inv(ciphertext);
            S_inv(ciphertext);
        }
        X_k(ciphertext, iterative_keys[i]);
    }
}

kuznyechik::kuznyechik(std::pair<block128, block128> key) {
    set_iterative_keys(key);
    GenerateMulTable();
    GenerateEncTable();
}

void kuznyechik::update_key(std::pair<block128, block128> key) {
    set_iterative_keys(key);
}

block128 *kuznyechik::get_iterative_keys() {
    return iterative_keys;
}

uint8_t kuznyechik::PolyMul(uint8_t left, uint8_t right)
{
    // p(x) = x^8 + x^7 + x^6 + x + 1 => 0b111000011 => 0xC3 (without MSB)
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

void kuznyechik::GenerateMulTable()
{
    for (unsigned i = 0; i < 256; ++i)
        for (unsigned j = 0; j < 256; ++j)
            mul_table[i][j] = PolyMul(i, j);
}

void kuznyechik::GenerateEncTable()
{
    Matrix l_matrix;
    for (size_t i = 0; i < 16; ++i)
        for (size_t j = 0; j < 16; ++j)
            if (i == 0)
                l_matrix[i][j] = block128::TRANSITION_ARRAY[j];
            else if (i == j + 1)
                l_matrix[i][j] = 1;
            else
                l_matrix[i][j] = 0;
    // power l_matrix to degree of 2^4
    for (unsigned i = 0; i < 4; ++i)
        l_matrix = SqrMatrix(l_matrix);

    for (size_t i = 0; i < 16; ++i)
        for (size_t j = 0; j < 256; ++j)
            for (size_t k = 0; k < 16; ++k)
                enc_ls_table[i][j][k] = mul_table[PI_ARRAY[j]][l_matrix[k][i]];
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


void kuznyechik::X_k(block128& a, block128 &k) {
    a.upper ^= k.upper;
    a.lower ^= k.lower;
}

void kuznyechik::S(block128& a) {
    uint64_t res1 = 0, res2 = 0;
    for (uint8_t sft = 0; sft <= 56; sft += 8) {
        res1 |= static_cast<uint64_t>(PI_ARRAY[(a.upper >> sft) & 0xFF]) << sft;
        res2 |= static_cast<uint64_t>(PI_ARRAY[(a.lower >> sft) & 0xFF]) << sft;
    }
    a.upper = res1;
    a.lower = res2;
}

void kuznyechik::S_inv(block128& a) {

    // TODO
}

static inline uint8x16_t combine_u64_to_u8x16(uint64_t a, uint64_t b) {
    uint64x1_t low = vcreate_u64(a);
    uint64x1_t high = vcreate_u64(b);
    uint64x2_t combined = vcombine_u64(low, high);
    return vreinterpretq_u8_u64(combined);
}

static inline uint8x16_t CastBlock(const uint8_t* ptr) {
    return vld1q_u8(ptr);
}

void kuznyechik::ApplyLS(block128& a)
{
    // Step 1: Combine lower and upper 64-bit parts into a single 128-bit NEON vector
    uint8x16_t d = combine_u64_to_u8x16(a.lower, a.upper);

    // Step 2: Initialize the mask with 0x0F for each byte to isolate the lower 4 bits
    uint8_t arr[16] = {
                           0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF,
                           0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF
                   };
    uint8x16_t mask = vld1q_u8(arr);

    // Step 3: Apply mask operations
    uint8x16_t tmp1 = vandq_u8(mask, d);      // tmp1 = mask & data
    uint8x16_t tmp2 = vbicq_u8(d, mask);      // tmp2 = data & ~mask

    // Step 4: Reinterpret as 64-bit vectors for shifting
    uint64x2_t t1_64 = vreinterpretq_u64_u8(tmp1);
    uint64x2_t t2_64 = vreinterpretq_u64_u8(tmp2);

    // Step 5: Perform shift operations
    t1_64 = vshrq_n_u64(t1_64, 4);           // Shift right by 4 bits
    t2_64 = vshlq_n_u64(t2_64, 4);           // Shift left by 4 bits

    // Step 6: Reinterpret back to uint8x16_t
    tmp1 = vreinterpretq_u8_u64(t1_64);
    tmp2 = vreinterpretq_u8_u64(t2_64);

    // Step 7: Reinterpret as 16-bit vectors to extract indices
    uint16x8_t tmp1_u16 = vreinterpretq_u16_u8(tmp1);
    uint16x8_t tmp2_u16 = vreinterpretq_u16_u8(tmp2);

    // Step 8: Store the 16-bit vectors into temporary arrays for variable indexing
    uint16_t tmp1_array[8];
    uint16_t tmp2_array[8];
    vst1q_u16(tmp1_array, tmp1_u16);
    vst1q_u16(tmp2_array, tmp2_u16);

    // Step 9: Access the lookup table
    const LookupTable& lut = get_lookup_table();
    const uint8_t* table = &lut[0][0][0]; // Correctly cast to const uint8_t*

    // Step 10: Initialize vec1 and vec2 by loading from the lookup table using i=0 indices
    uint16_t idx2_0 = tmp2_array[0];
    uint16_t idx1_0 = tmp1_array[0];

    // Bounds checking for initial indices

    uint8x16_t vec1 = vld1q_u8(table + idx2_0 + 0x0000);
    uint8x16_t vec2 = vld1q_u8(table + idx1_0 + 0x1000); //HERE

    // Step 11: Iterate through indices 1 to 7 and perform XOR operations
    for(int i=1;i<8;i++){
        uint16_t idx2 = tmp2_array[i];
        uint16_t idx1 = tmp1_array[i];


        // Calculate byte offsets:
        // For vec1: i *0x2000 + idx2 *16
        // For vec2: i *0x2000 +0x1000 + idx1 *16
        uint32_t offset2 = (i * 0x2000) + idx2;
        uint32_t offset1 = (i * 0x2000) + 0x1000 + idx1;


        // Load the lookup table blocks and XOR them with vec1 and vec2
        uint8x16_t block1 = CastBlock(table + offset1);
        uint8x16_t block2 = CastBlock(table + offset2);
        vec1 = veorq_u8(vec1, block2);
        vec2 = veorq_u8(vec2, block1);
    }

    // Step 12: Final XOR to combine vec1 and vec2
    uint8x16_t final_vec = veorq_u8(vec1, vec2);

    // Step 13: Reinterpret the final vector as two 64-bit integers and store them back
    uint64x2_t u64_vec = vreinterpretq_u64_u8(final_vec);
    a.lower = vgetq_lane_u64(u64_vec,0);
    a.upper = vgetq_lane_u64(u64_vec,1);
}

void kuznyechik::L(block128& a) {
    R(a); R(a); R(a); R(a);
    R(a); R(a); R(a); R(a);
    R(a); R(a); R(a); R(a);
    R(a); R(a); R(a); R(a);
}

void kuznyechik::R_inv(block128& a) {
//    uint8_t a15 = a[15];
//    for (size_t i = 15; i > 0; i--) {
//        a[i] = a[i - 1];
//    }
//    a[0] = a15;
//    a[0] = linear_transition(a);
//TODO
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
//    S(a.first);
//    L(a.first);
    ApplyLS(a.first);
    X_k(a.first, c);
}

