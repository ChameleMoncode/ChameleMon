#ifndef _HYPERLOGLOG_H_
#define _HYPERLOGLOG_H_

#include <vector>
#include <cmath>
#include <sstream>
#include <stdexcept>
#include <algorithm>

#include "../Common/BOBHash32.h"
#include "../common_func.h"

#define HLL_HASH_SEED 313

#if defined(__has_builtin) && (defined(__GNUC__) || defined(__clang__))

#define _GET_CLZ(x, b) (uint8_t)std::min(b, ::__builtin_clz(x)) + 1

#else

inline uint8_t _get_leading_zero_count(uint32_t x, uint8_t b) {

#if defined (_MSC_VER)
    uint32_t leading_zero_len = 32;
    ::_BitScanReverse(&leading_zero_len, x);
    --leading_zero_len;
    return std::min(b, (uint8_t)leading_zero_len);
#else
    uint8_t v = 1;
    while (v <= b && !(x & 0x80000000)) {
        v++;
        x <<= 1;
    }
    return v;
#endif

}
#define _GET_CLZ(x, b) _get_leading_zero_count(x, b)
#endif /* defined(__GNUC__) */



namespace hll {

static const double pow_2_32 = 4294967296.0; ///< 2^32
static const double neg_pow_2_32 = -4294967296.0; ///< -(2^32)

class HyperLogLog
{
public:
	BOBHash32* bobhash = NULL;
	HyperLogLog(uint8_t b = 12, int reg_size = 8) throw (std::invalid_argument) : b_(b), reg_size_(reg_size), m_(1 << b), M_(m_, 0) {

        bobhash = new BOBHash32(750);

        if (b < 4 || 30 < b) {
            throw std::invalid_argument("bit width must be in the range [4,30]");
        }
        if (reg_size < 1 || 8 < reg_size){
            throw std::invalid_argument("register size must be in the range [1,8]");
        }

        printf("[HyperLogLog : Memory usage : %d Bytes\n", (int)(m_ * reg_size_ / 8.0) );

        double alpha;
        switch (m_) {
            case 16:
                alpha = 0.673;
                break;
            case 32:
                alpha = 0.697;
                break;
            case 64:
                alpha = 0.709;
                break;
            default:
                alpha = 0.7213 / (1.0 + 1.079 / m_);
                break;
        }
        alphaMM_ = alpha * m_ * m_;
    }

    ~HyperLogLog(){
        delete bobhash;
    }

    void add(const char* str, uint32_t len) {
        uint32_t hash = bobhash->run(str, len);
        uint32_t index = hash >> (32 - b_);
        uint8_t rank;

        // bound the zeros-length than the register's range
        // e.g., for 4-bit registers, max_zeros <= 15.
        uint8_t max_zeros = std::pow(2, reg_size_) - 1; // 15 for 4 bits
        if (32 - b_ > max_zeros){
            rank = _GET_CLZ((hash << (32 - max_zeros)), max_zeros);
        }
        else {
            rank = _GET_CLZ((hash << b_), 32 - b_);
        }

        if (rank > M_[index]) {
            M_[index] = rank;
        }
    }

    double estimate() const {
        double estimate;
        double sum = 0.0;
        for (uint32_t i = 0; i < m_; i++) {
            sum += 1.0 / (1 << M_[i]);
        }
        estimate = alphaMM_ / sum; // E in the original paper
        if (estimate <= 2.5 * m_) {
            uint32_t zeros = 0;
            for (uint32_t i = 0; i < m_; i++) {
                if (M_[i] == 0) {
                    zeros++;
                }
            }
            if (zeros != 0) {
                printf("[INFO] Activate Linear Counting!!\n");
                estimate = m_ * std::log(static_cast<double>(m_)/ zeros);
            }
        } else if (estimate > (1.0 / 30.0) * pow_2_32) {
            printf("[INFO] Correction is activated!!\n");
            estimate = neg_pow_2_32 * log(1.0 - (estimate / pow_2_32));
        }
        return estimate;
    }

    uint32_t registerSize() const {
        return m_;
    }

protected:
    uint8_t b_; ///< register bit width
    uint32_t m_; ///< register size (number)
    double alphaMM_; ///< alpha * m^2
    std::vector<uint8_t> M_; ///< registers
    int reg_size_ = 8;
};

} // namespace hll

#endif