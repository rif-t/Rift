#pragma once

#include "integer.h"

#include <string>

// Fixed-scale decimal: signed 128-bit coefficient with at most 18 fractional
// digits. Arithmetic is exact unless division requires discarded digits.
struct Decimal
{
    UInt128 coefficient{};
    bool negative = false;
    unsigned scale = 0;

    static Decimal parse(const std::string& text);
    static Decimal fromFloat(float value);
    static Decimal fromDouble(double value);
    std::string toString() const;
    long double toLongDouble() const;
    int compare(const Decimal& other) const;
};

Decimal decimalArithmetic(const std::string& operation, Decimal left, Decimal right);
