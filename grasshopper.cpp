//
// Created by Ivan Artemev on 19.12.2024.
//
#include <algorithm>
#include <functional>
#include <cstdio>
#include <cassert>
#include "grasshopper.hpp"
#include "tables.hpp"
#include <arm_neon.h>


static Grasshopper::Block mask =
        {{
                 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF,
                 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF
         }};


Grasshopper::Grasshopper()
{
    GenerateMulTable();
    GenerateEncTable();
    GenerateDecTable();
}



void Grasshopper::GenerateMulTable()
{
    for (unsigned i = 0; i < 256; ++i)
        for (unsigned j = 0; j < 256; ++j)
            mul_table[i][j] = PolyMul(i, j);
}


void Grasshopper::GenerateEncTable()
{
    //initializing the martix
    Matrix l_matrix;
    for (size_t i = 0; i < block_size; ++i)
        for (size_t j = 0; j < block_size; ++j)
            if (i == 0)
                l_matrix[i][j] = Table::lin[j];
            else if (i == j + 1)
                l_matrix[i][j] = 1;
            else
                l_matrix[i][j] = 0;
    // power l_matrix to degree of 2^4
    for (unsigned i = 0; i < 4; ++i)
        l_matrix = SqrMatrix(l_matrix);

    for (size_t i = 0; i < block_size; ++i)
        for (size_t j = 0; j < 256; ++j)
            for (size_t k = 0; k < block_size; ++k)
                enc_ls_table[i][j][k] = mul_table[Table::S[j]][l_matrix[k][i]];
}

void Grasshopper::GenerateDecTable()
{
    //initializing the martix
    Matrix l_matrix;
    for (size_t i = 0; i < block_size; ++i)
        for (size_t j = 0; j < block_size; ++j)
            if (i == block_size - 1)
                l_matrix[i][j] = Table::lin[(j + 15) % block_size];
            else if (i + 1 == j)
                l_matrix[i][j] = 1;
            else
                l_matrix[i][j] = 0;
    // power l_matrix to degree of 2^4
    for (unsigned i = 0; i < 4; ++i)
        l_matrix = SqrMatrix(l_matrix);

    for (size_t i = 0; i < block_size; ++i)
        for (size_t j = 0; j < 256; ++j)
            for (size_t k = 0; k < block_size; ++k)
                dec_ls_table[i][j][k] = mul_table[Table::invS[j]][l_matrix[k][i]];
}

uint8_t Grasshopper::PolyMul(uint8_t left, uint8_t right)
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

Grasshopper::Matrix Grasshopper::SqrMatrix(const Matrix& mat)
{
    Matrix res{};
    for (size_t i = 0; i < block_size; ++i)
        for (size_t j = 0; j < block_size; ++j)
            for (size_t k = 0; k < block_size; ++k)
                res[i][j] ^= mul_table[mat[i][k]][mat[k][j]];
    return res;
}
