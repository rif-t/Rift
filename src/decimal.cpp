#include "decimal.h"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <stdexcept>

namespace {
void checkRange(const Decimal& value)
{
    const UInt128 positiveMax{(UINT64_C(1) << 63) - 1, UINT64_MAX};
    const UInt128 negativeMax{UINT64_C(1) << 63, 0};
    if ((value.negative && negativeMax < value.coefficient) || (!value.negative && positiveMax < value.coefficient))
        throw std::runtime_error("Decimal coefficient exceeds signed 128-bit range");
}

UInt128 multiply10(UInt128 value)
{
    bool overflow = false;
    value = UInt128::multiply(value, {0, 10}, overflow);
    if (overflow) throw std::runtime_error("Decimal coefficient overflow");
    return value;
}

Decimal normalized(Decimal value)
{
    while (value.scale) {
        UInt128 remainder{};
        const UInt128 quotient = UInt128::divide(value.coefficient, {0, 10}, remainder);
        if (remainder.low != 0) break;
        value.coefficient = quotient;
        --value.scale;
    }
    checkRange(value);
    if (value.coefficient == UInt128{}) { value.negative = false; value.scale = 0; }
    return value;
}

std::string plainMagnitude(const Decimal& value)
{
    std::string digits = value.coefficient.toString();
    if (value.scale == 0) return digits;
    if (digits.size() <= value.scale) digits.insert(0, value.scale + 1 - digits.size(), '0');
    digits.insert(digits.size() - value.scale, 1, '.');
    return digits;
}

int compareMagnitude(const Decimal& a, const Decimal& b)
{
    const std::string ad = a.coefficient.toString();
    const std::string bd = b.coefficient.toString();
    auto parts = [](const std::string& digits, unsigned scale) {
        std::string whole = digits.size() > scale ? digits.substr(0, digits.size() - scale) : "0";
        std::string fraction;
        if (scale) fraction = digits.size() >= scale ? digits.substr(digits.size() - scale) : std::string(scale - digits.size(), '0') + digits;
        return std::pair{whole, fraction};
    };
    auto [aw, af] = parts(ad, a.scale);
    auto [bw, bf] = parts(bd, b.scale);
    if (aw.size() != bw.size()) return aw.size() < bw.size() ? -1 : 1;
    if (aw != bw) return aw < bw ? -1 : 1;
    const std::size_t fractionSize = std::max(af.size(), bf.size());
    af.append(fractionSize - af.size(), '0');
    bf.append(fractionSize - bf.size(), '0');
    if (af != bf) return af < bf ? -1 : 1;
    return 0;
}
}

Decimal Decimal::parse(const std::string& text)
{
    if (text.empty()) throw std::runtime_error("Invalid decimal value");
    std::size_t pos = 0;
    bool neg = false;
    if (text[pos] == '-' || text[pos] == '+') { neg = text[pos] == '-'; ++pos; }
    std::string digits;
    unsigned scale = 0;
    bool dot = false, anyDigit = false;
    int exponent = 0;
    for (; pos < text.size() && text[pos] != 'e' && text[pos] != 'E'; ++pos) {
        const char ch = text[pos];
        if (ch == '.' && !dot) { dot = true; continue; }
        if (ch < '0' || ch > '9') throw std::runtime_error("Invalid decimal value: " + text);
        digits.push_back(ch); anyDigit = true;
        if (dot) ++scale;
    }
    if (!anyDigit) throw std::runtime_error("Invalid decimal value: " + text);
    if (pos < text.size()) {
        ++pos;
        try { exponent = std::stoi(text.substr(pos)); }
        catch (...) { throw std::runtime_error("Invalid decimal exponent: " + text); }
    }
    if (exponent < 0) scale += static_cast<unsigned>(-exponent);
    else if (static_cast<unsigned>(exponent) >= scale) {
        digits.append(static_cast<std::size_t>(exponent) - scale, '0'); scale = 0;
    } else scale -= static_cast<unsigned>(exponent);
    while (digits.size() > 1 && digits.front() == '0') digits.erase(digits.begin());
    while (scale && !digits.empty() && digits.back() == '0') { digits.pop_back(); --scale; }
    if (scale > 18) throw std::runtime_error("Decimal supports at most 18 fractional digits");
    if (digits.empty()) digits = "0";
    Decimal value{UInt128::parse(digits), neg, scale};
    return normalized(value);
}

Decimal Decimal::fromDouble(double value)
{
    if (!std::isfinite(value)) throw std::runtime_error("Cannot convert a non-finite value to decimal");
    char buffer[128];
    auto [end, ec] = std::to_chars(buffer, buffer + sizeof(buffer), value, std::chars_format::general);
    if (ec != std::errc{}) throw std::runtime_error("Cannot convert value to decimal");
    return parse(std::string(buffer, end));
}

Decimal Decimal::fromFloat(float value)
{
    if (!std::isfinite(value)) throw std::runtime_error("Cannot convert a non-finite value to decimal");
    char buffer[64];
    auto [end, ec] = std::to_chars(buffer, buffer + sizeof(buffer), value, std::chars_format::general);
    if (ec != std::errc{}) throw std::runtime_error("Cannot convert value to decimal");
    return parse(std::string(buffer, end));
}

std::string Decimal::toString() const
{
    return (negative && !(coefficient == UInt128{}) ? "-" : "") + plainMagnitude(*this);
}

long double Decimal::toLongDouble() const
{
    long double value = std::stold(coefficient.toString());
    for (unsigned i = 0; i < scale; ++i) value /= 10.0L;
    return negative ? -value : value;
}

int Decimal::compare(const Decimal& other) const
{
    if (negative != other.negative) return negative ? -1 : 1;
    const int mag = compareMagnitude(*this, other);
    return negative ? -mag : mag;
}

Decimal decimalArithmetic(const std::string& op, Decimal a, Decimal b)
{
    if (op == "+" || op == "-") {
        if (op == "-") b.negative = !(b.coefficient == UInt128{}) && !b.negative;
        const unsigned scale = std::max(a.scale, b.scale);
        for (unsigned i = a.scale; i < scale; ++i) a.coefficient = multiply10(a.coefficient);
        for (unsigned i = b.scale; i < scale; ++i) b.coefficient = multiply10(b.coefficient);
        WideInteger wa{a.coefficient, a.negative, false}, wb{b.coefficient, b.negative, false};
        WideInteger sum;
        if (wa.negative == wb.negative) {
            bool overflow = false; sum.magnitude = UInt128::add(wa.magnitude, wb.magnitude, overflow);
            if (overflow) throw std::runtime_error("Decimal overflow");
            sum.negative = wa.negative;
        } else if (wa.magnitude < wb.magnitude) { sum.magnitude = UInt128::subtract(wb.magnitude, wa.magnitude); sum.negative = wb.negative; }
        else { sum.magnitude = UInt128::subtract(wa.magnitude, wb.magnitude); sum.negative = wa.negative; }
        return normalized({sum.magnitude, sum.negative, scale});
    }
    if (op == "*") {
        bool overflow = false;
        UInt128 product = UInt128::multiply(a.coefficient, b.coefficient, overflow);
        if (overflow) throw std::runtime_error("Decimal overflow");
        unsigned scale = a.scale + b.scale;
        while (scale > 18) { UInt128 rem{}; const UInt128 quotient = UInt128::divide(product, {0, 10}, rem); if (rem.low) break; product = quotient; --scale; }
        if (scale > 18) throw std::runtime_error("Decimal multiplication exceeds 18 fractional digits");
        return normalized({product, (a.negative != b.negative), scale});
    }
    if (op == "%") {
        const unsigned scale = std::max(a.scale, b.scale);
        for (unsigned i = a.scale; i < scale; ++i) a.coefficient = multiply10(a.coefficient);
        for (unsigned i = b.scale; i < scale; ++i) b.coefficient = multiply10(b.coefficient);
        if (b.coefficient == UInt128{}) throw std::runtime_error("Remainder by zero");
        UInt128 remainder{}; UInt128::divide(a.coefficient, b.coefficient, remainder);
        return normalized({remainder, a.negative, scale});
    }
    if (op == "/") {
        if (b.coefficient == UInt128{}) throw std::runtime_error("Division by zero");
        for (int outScale = 18; outScale >= 0; --outScale) {
            const int power = outScale + static_cast<int>(a.scale) - static_cast<int>(b.scale);
            UInt128 numerator = a.coefficient, denominator = b.coefficient;
            try {
                if (power >= 0) for (int i = 0; i < power; ++i) numerator = multiply10(numerator);
                else for (int i = 0; i < -power; ++i) denominator = multiply10(denominator);
            } catch (...) { continue; }
            UInt128 rem{}; UInt128 quotient = UInt128::divide(numerator, denominator, rem);
            return normalized({quotient, a.negative != b.negative, static_cast<unsigned>(outScale)});
        }
        throw std::runtime_error("Decimal division overflow");
    }
    if (op == "^") {
        if (b.scale != 0) throw std::runtime_error("Decimal exponent must be an integer");
        if (b.coefficient.high || b.coefficient.low > 10000) throw std::runtime_error("Decimal exponent is too large");
        if (b.negative) return decimalArithmetic("/", Decimal{{0, 1}, false, 0}, decimalArithmetic("^", a, Decimal{b.coefficient, false, 0}));
        Decimal result{{0, 1}, false, 0};
        while (!(b.coefficient == UInt128{})) {
            if (b.coefficient.low & 1) result = decimalArithmetic("*", result, a);
            UInt128 rem{}; b.coefficient = UInt128::divide(b.coefficient, {0, 2}, rem);
            if (!(b.coefficient == UInt128{})) a = decimalArithmetic("*", a, a);
        }
        return result;
    }
    throw std::runtime_error("Unknown decimal operator");
}
