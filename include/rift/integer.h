#pragma once

#include <cstdint>
#include <algorithm>
#include <array>
#include <charconv>
#include <stdexcept>
#include <string>

struct UInt128
{
    std::uint64_t high = 0;
    std::uint64_t low = 0;

    friend bool operator==(UInt128 a, UInt128 b) { return a.high == b.high && a.low == b.low; }
    friend bool operator<(UInt128 a, UInt128 b) { return a.high < b.high || (a.high == b.high && a.low < b.low); }

    static UInt128 add(UInt128 a, UInt128 b, bool& overflow)
    {
        UInt128 r{a.high + b.high, a.low + b.low};
        const bool carry = r.low < a.low;
        const auto oldHigh = r.high;
        r.high += static_cast<std::uint64_t>(carry);
        overflow = r.high < oldHigh || r.high < a.high;
        return r;
    }

    static UInt128 subtract(UInt128 a, UInt128 b)
    {
        const auto borrow = a.low < b.low;
        return {a.high - b.high - static_cast<std::uint64_t>(borrow), a.low - b.low};
    }

    static UInt128 multiply(UInt128 a, UInt128 b, bool& overflow)
    {
        std::uint32_t x[4] = {static_cast<std::uint32_t>(a.low), static_cast<std::uint32_t>(a.low >> 32), static_cast<std::uint32_t>(a.high), static_cast<std::uint32_t>(a.high >> 32)};
        std::uint32_t y[4] = {static_cast<std::uint32_t>(b.low), static_cast<std::uint32_t>(b.low >> 32), static_cast<std::uint32_t>(b.high), static_cast<std::uint32_t>(b.high >> 32)};
        std::uint32_t z[8]{};
        for (int i = 0; i < 4; ++i) {
            std::uint64_t carry = 0;
            for (int j = 0; j < 4; ++j) {
                const std::uint64_t cur = static_cast<std::uint64_t>(x[i]) * y[j] + z[i + j] + carry;
                z[i + j] = static_cast<std::uint32_t>(cur);
                carry = cur >> 32;
            }
            int k = i + 4;
            while (carry && k < 8) {
                const std::uint64_t cur = static_cast<std::uint64_t>(z[k]) + carry;
                z[k++] = static_cast<std::uint32_t>(cur);
                carry = cur >> 32;
            }
        }
        overflow = z[4] || z[5] || z[6] || z[7];
        return {(static_cast<std::uint64_t>(z[3]) << 32) | z[2], (static_cast<std::uint64_t>(z[1]) << 32) | z[0]};
    }

    static UInt128 divide(UInt128 value, UInt128 divisor, UInt128& remainder)
    {
        if (divisor == UInt128{}) throw std::runtime_error("Division by zero");
        UInt128 quotient{}, rem{};
        for (int bit = 127; bit >= 0; --bit) {
            const bool carry = (rem.high >> 63) != 0;
            rem.high = (rem.high << 1) | (rem.low >> 63);
            rem.low = (rem.low << 1) | (bit >= 64 ? ((value.high >> (bit - 64)) & 1) : ((value.low >> bit) & 1));
            if (carry || !(rem < divisor)) {
                rem = subtract(rem, divisor);
                if (bit >= 64) quotient.high |= std::uint64_t{1} << (bit - 64);
                else quotient.low |= std::uint64_t{1} << bit;
            }
        }
        remainder = rem;
        return quotient;
    }

    static UInt128 parse(const std::string& digits)
    {
        UInt128 value{};
        for (char ch : digits) {
            bool overflow = false;
            value = multiply(value, {0, 10}, overflow);
            if (overflow) throw std::runtime_error("Integer literal exceeds 128-bit range");
            UInt128 next = add(value, {0, static_cast<std::uint64_t>(ch - '0')}, overflow);
            if (overflow) throw std::runtime_error("Integer literal exceeds 128-bit range");
            value = next;
        }
        return value;
    }

    std::string toString() const
    {
        if (*this == UInt128{}) return "0";
        constexpr std::uint64_t decimalChunkBase = UINT64_C(10000000000000000000);
        std::array<std::uint64_t, 3> chunks{};
        std::size_t chunkCount = 0;
        UInt128 value = *this;
        while (!(value == UInt128{})) {
            UInt128 rem{};
            value = divide(value, {0, decimalChunkBase}, rem);
            chunks[chunkCount++] = rem.low;
        }
        std::string result;
        result.reserve(39);
        char digits[20];
        auto [first, error] = std::to_chars(digits, digits + sizeof(digits), chunks[chunkCount - 1]);
        if (error != std::errc{}) throw std::runtime_error("Integer formatting failed");
        result.append(digits, first);
        while (--chunkCount) {
            auto [chunkFirst, chunkError] = std::to_chars(digits, digits + sizeof(digits), chunks[chunkCount - 1]);
            if (chunkError != std::errc{}) throw std::runtime_error("Integer formatting failed");
            const auto digitCount = static_cast<std::size_t>(chunkFirst - digits);
            result.append(19 - digitCount, '0');
            result.append(digits, chunkFirst);
        }
        return result;
    }

    std::string toBase(unsigned base, bool uppercase = false) const
    {
        if (base < 2 || base > 36) throw std::runtime_error("Integer base must be from 2 to 36");
        if (*this == UInt128{}) return "0";
        const char* digits = uppercase ? "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ" : "0123456789abcdefghijklmnopqrstuvwxyz";
        std::string result;
        UInt128 value = *this;
        while (!(value == UInt128{})) {
            UInt128 rem{};
            value = divide(value, {0, base}, rem);
            result.push_back(digits[rem.low]);
        }
        std::reverse(result.begin(), result.end());
        return result;
    }
};

struct WideInteger
{
    UInt128 magnitude{};
    bool negative = false;
    bool isUnsigned = false;
    // Zero means an untyped literal; otherwise this records its language width.
    std::uint8_t bitWidth = 0;

    std::string toString() const { return (negative && !(magnitude == UInt128{}) ? "-" : "") + magnitude.toString(); }
    std::string toBase(unsigned base, bool uppercase = false) const { return (negative && !(magnitude == UInt128{}) ? "-" : "") + magnitude.toBase(base, uppercase); }
};

inline bool integerFitsType(const WideInteger& value, const std::string& type)
{
    if (type == "int") return integerFitsType(value, "int32");
    if (type == "bool") return false;
    const bool isUnsigned = type.starts_with("uint");
    const int bits = std::stoi(type.substr(isUnsigned ? 4 : 3));
    if (value.negative && isUnsigned) return false;
    UInt128 limit{};
    if (bits == 128) {
        limit.high = isUnsigned ? UINT64_MAX : (UINT64_C(1) << 63) - 1;
        limit.low = isUnsigned ? UINT64_MAX : UINT64_MAX;
    } else if (isUnsigned) {
        limit.low = (UINT64_C(1) << bits) - 1;
    } else {
        limit.low = (UINT64_C(1) << (bits - 1)) - 1;
    }
    if (!isUnsigned && value.negative) {
        if (bits == 128) limit = {UINT64_C(1) << 63, 0};
        else limit.low += 1;
    }
    return !(limit < value.magnitude);
}
