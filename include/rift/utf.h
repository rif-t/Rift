#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>

inline std::u16string utf8ToUtf16(const std::string& input)
{
    std::u16string output;
    for (std::size_t i = 0; i < input.size();) {
        const auto first = static_cast<unsigned char>(input[i++]);
        std::uint32_t codepoint;
        unsigned continuation;
        if (first < 0x80) { codepoint = first; continuation = 0; }
        else if ((first & 0xe0) == 0xc0) { codepoint = first & 0x1f; continuation = 1; }
        else if ((first & 0xf0) == 0xe0) { codepoint = first & 0x0f; continuation = 2; }
        else if ((first & 0xf8) == 0xf0) { codepoint = first & 0x07; continuation = 3; }
        else throw std::runtime_error("Invalid UTF-8 in source string");
        for (unsigned j = 0; j < continuation; ++j) {
            if (i >= input.size()) throw std::runtime_error("Truncated UTF-8 in source string");
            const auto next = static_cast<unsigned char>(input[i++]);
            if ((next & 0xc0) != 0x80) throw std::runtime_error("Invalid UTF-8 continuation byte");
            codepoint = (codepoint << 6) | (next & 0x3f);
        }
        if ((continuation == 1 && codepoint < 0x80) || (continuation == 2 && codepoint < 0x800) ||
            (continuation == 3 && codepoint < 0x10000) || codepoint > 0x10ffff ||
            (codepoint >= 0xd800 && codepoint <= 0xdfff)) throw std::runtime_error("Invalid UTF-8 code point");
        if (codepoint <= 0xffff) output.push_back(static_cast<char16_t>(codepoint));
        else {
            codepoint -= 0x10000;
            output.push_back(static_cast<char16_t>(0xd800 + (codepoint >> 10)));
            output.push_back(static_cast<char16_t>(0xdc00 + (codepoint & 0x3ff)));
        }
    }
    return output;
}

inline std::string utf16ToUtf8(const std::u16string& input)
{
    std::string output;
    for (std::size_t i = 0; i < input.size(); ++i) {
        std::uint32_t codepoint = input[i];
        if (codepoint >= 0xd800 && codepoint <= 0xdbff) {
            if (i + 1 >= input.size() || input[i + 1] < 0xdc00 || input[i + 1] > 0xdfff)
                throw std::runtime_error("Invalid UTF-16 surrogate pair");
            codepoint = 0x10000 + ((codepoint - 0xd800) << 10) + (input[++i] - 0xdc00);
        } else if (codepoint >= 0xdc00 && codepoint <= 0xdfff) throw std::runtime_error("Unpaired UTF-16 low surrogate");
        if (codepoint < 0x80) output.push_back(static_cast<char>(codepoint));
        else if (codepoint < 0x800) {
            output.push_back(static_cast<char>(0xc0 | (codepoint >> 6)));
            output.push_back(static_cast<char>(0x80 | (codepoint & 0x3f)));
        } else if (codepoint < 0x10000) {
            output.push_back(static_cast<char>(0xe0 | (codepoint >> 12)));
            output.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3f)));
            output.push_back(static_cast<char>(0x80 | (codepoint & 0x3f)));
        } else {
            output.push_back(static_cast<char>(0xf0 | (codepoint >> 18)));
            output.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3f)));
            output.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3f)));
            output.push_back(static_cast<char>(0x80 | (codepoint & 0x3f)));
        }
    }
    return output;
}
