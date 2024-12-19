#ifndef GRASSHOPPER_H
#define GRASSHOPPER_H

#include <array>
#include <vector>
#include <utility>
#include <cstdlib>
#include <cstdint>
#include <stdexcept>
#include <arm_neon.h>

template<typename T>
class AlignedAllocator
{
public:
    typedef T value_type;
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;

    T* allocate(std::size_t n)
    {
        if (!n)
            return nullptr;
        if (n > max_size)
            throw std::length_error("AlignedAllocator::allocate() - integer overflow");
        void *p = std::aligned_alloc(sizeof(T) * 2, n * sizeof(T));
        if (!p)
            throw std::bad_alloc();
        return static_cast<T*>(p);
    }

    void deallocate(T* p, std::size_t n __attribute__((unused)))
    {
        std::free(static_cast<void*>(p));
    }

    bool operator ==(const AlignedAllocator& rhs __attribute__((unused))) const
    {
        return true;
    }

    bool operator !=(const AlignedAllocator& rhs) const
    {
        return !(*this == rhs);
    }

private:
    static constexpr std::size_t max_size =
            (static_cast<std::size_t>(0) - static_cast<std::size_t>(1)) / sizeof(T);
};

enum class Mode
{
    ECB,
    CBC,
    CFB,
    OFB
};

class Grasshopper
{
public:
    static constexpr std::size_t block_size = 16;
    static constexpr unsigned num_rounds = 10;

    struct alignas(block_size) Block : std::array<uint8_t, block_size>
    {
        Block() = default;

        Block(const int8x16_t& val)
        {
            *reinterpret_cast<int8x16_t*>(this) = val;
        }

        Block(const std::array<uint8_t, block_size>& arr)
                : std::array<uint8_t, block_size>(arr)
        {
        }

        operator int8x16_t() const
        {
            return *reinterpret_cast<const int8x16_t*>(this);
        }

        void Dump()
        {
            for (auto x: *this)
                printf("%02x", x);
            printf("\n");
        }
    };

    using Key = std::array<uint8_t, block_size * 2>;
    using Data = std::vector<Block, AlignedAllocator<Block>>;

    Grasshopper();

    using Matrix = std::array<Block, block_size>;
    using Keys = std::array<Block, num_rounds>;
    using KeyPair = std::pair<Block, Block>;
    using LookupTable = Block[block_size][256];
    Block coef_table[num_rounds / 2 - 1][8];
    uint8_t mul_table[256][256];
    LookupTable enc_ls_table;
    LookupTable dec_ls_table;

    void GenerateMulTable();
    void GenerateEncTable();
    void GenerateDecTable();

    uint8_t PolyMul(uint8_t left, uint8_t right);
    Matrix SqrMatrix(const Matrix& mat);
};

#endif