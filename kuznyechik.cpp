#include <array>
#include <iostream>
#include "kuznyechik.hpp"
#include "block128.hpp"


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
            ApplyLS(plaintext, enc_ls_table);
        }
    }
}

void kuznyechik::decrypt(block128 &ciphertext) {

    std::transform(ciphertext.a.begin(), ciphertext.a.end(), ciphertext.a.begin(),
                   [](uint8_t idx) { return PI_ARRAY[idx]; });
//    S(ciphertext);
//    L_inv(ciphertext);
//    ApplyLS(ciphertext, dec_l_table);
    for(std::size_t i = 9; i >= 1; i--) {
//        auto dec_key = iterative_keys[i + 1];
//        L_inv(dec_key);
        ApplyLS(ciphertext, dec_ls_table);
        X_k(ciphertext, iterative_keys[i + 1]);
    }
    std::transform(ciphertext.a.begin(), ciphertext.a.end(), ciphertext.a.begin(),
                   [](uint8_t idx) { return PI_INV_ARRAY[idx]; });
//    S_inv(ciphertext);
    X_k(ciphertext, iterative_keys[1]);
}

kuznyechik::kuznyechik(std::pair<block128, block128> key) {
    set_iterative_keys(key);
    GenerateMulTable();
    GenerateEncTable();
    GenerateDecTable();
    GenerateDecLTable();
}

void kuznyechik::GenerateDecTable()
{
    //initializing the martix
    Matrix l_matrix;
    for (size_t i = 0; i < 16; ++i)
        for (size_t j = 0; j < 16; ++j)
            if (i == 16 - 1)
                l_matrix[i][j] = block128::TRANSITION_ARRAY[(j + 15) % 16];
            else if (i + 1 == j)
                l_matrix[i][j] = 1;
            else
                l_matrix[i][j] = 0;
    // power l_matrix to degree of 2^4
    for (unsigned i = 0; i < 4; ++i)
        l_matrix = SqrMatrix(l_matrix);

    for (size_t i = 0; i < 16; ++i)
        for (size_t j = 0; j < 256; ++j)
            for (size_t k = 0; k < 16; ++k)
                dec_ls_table[i][j][k] = mul_table[PI_INV_ARRAY[j]][l_matrix[k][i]];
}


void kuznyechik::GenerateDecLTable()
{
    //initializing the martix
    Matrix l_matrix;
    for (size_t i = 0; i < 16; ++i)
        for (size_t j = 0; j < 16; ++j)
            if (i == 16 - 1)
                l_matrix[i][j] = block128::TRANSITION_ARRAY[15 - j];
            else if (i + 1 == j)
                l_matrix[i][j] = 1;
            else
                l_matrix[i][j] = 0;
    // power l_matrix to degree of 2^4
    for (unsigned i = 0; i < 4; ++i)
        l_matrix = SqrMatrix(l_matrix);

    for (size_t i = 0; i < 16; ++i)
        for (size_t j = 0; j < 256; ++j)
            for (size_t k = 0; k < 16; ++k)
                dec_l_table[i][j][k] = mul_table[j][l_matrix[k][i]];
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

static inline uint8x16_t combine_u64_to_u8x16(uint64_t a, uint64_t b) {
    uint64x1_t low = vcreate_u64(a);
    uint64x1_t high = vcreate_u64(b);
    // Combine low and high into a 128-bit vector: [a(64 bits), b(64 bits)]
    uint64x2_t combined = vcombine_u64(low, high);

    // Reinterpret as a uint8x16_t
    uint8x16_t val = vreinterpretq_u8_u64(combined);
    // val = [a0, a1, a2, a3, a4, a5, a6, a7, b0, b1, b2, b3, b4, b5, b6, b7]
    // (Here a0 is the lowest-order byte of 'a', a7 the highest, etc.)

    // Step 1: vrev64q_u8 reverses each 64-bit half independently:
    // After vrev64q_u8(val):
    // [a7, a6, a5, a4, a3, a2, a1, a0, b7, b6, b5, b4, b3, b2, b1, b0]
    val = vrev64q_u8(val);

    // Step 2: Now we need to swap the two 8-byte halves:
    // vextq_u8(val, val, 8) takes the last 8 bytes of val and rotates them to the front.
    // After vextq_u8(val, val, 8):
    // [b7, b6, b5, b4, b3, b2, b1, b0, a7, a6, a5, a4, a3, a2, a1, a0]
    val = vextq_u8(val, val, 8);

    return val;
}

static inline uint8x16_t CastBlock(const uint8_t* ptr) {
    return vld1q_u8(ptr);
}

void kuznyechik::ApplyLS(block128& a, LookupTable& lookup_table)
{
    // Step 1: Combine lower and upper 64-bit parts into a single 128-bit NEON vector
    uint8x16_t d = vld1q_u8(a.a.data());

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
    const LookupTable& lut = lookup_table;
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

// Reverse the steps used previously:
// 1. Reverse the halves again with vext (this moves the last 8 bytes to the front)
// 2. Reverse the bytes in each 64-bit half again with vrev64q_u8

// First swap the halves back
//    uint8x16_t reverted_vec = vextq_u8(final_vec, final_vec, 8);
//
//// Then reverse each 64-bit half back
//    reverted_vec = vrev64q_u8(reverted_vec);

// Now reverted_vec should be in the original byte order
//    uint64x2_t u64_vec = vreinterpretq_u64_u8(reverted_vec);
    vst1q_u8(a.a.data(), final_vec);
}

void kuznyechik::L(block128& a) {
    R(a); R(a); R(a); R(a);
    R(a); R(a); R(a); R(a);
    R(a); R(a); R(a); R(a);
    R(a); R(a); R(a); R(a);
}

void kuznyechik::R_inv(block128& a) {
    auto c = a.a[0];
    for (int j = 0; j < 15; j++) {
        a.a[j] = a.a[j + 1];
    }
    a.a[15] = c;

    a.a[15] = linear_transition(a);

//    uint8_t a15 = a.a[15];
//    std::memmove(a.a.data() + 1, a.a.data(), 15);
//    a.a[0] = a15;
//    a.a[0] = linear_transition(a);

//    uint8_t val = linear_transition(a);
//    std::memmove(a.a.data() + 1, a.a.data(), 15);
//    a.a[0] = val;
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
    S(a.first);
    L(a.first);
    X_k(a.first, c);
}

