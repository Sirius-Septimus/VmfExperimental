/* =============================================================================
 * Vader Modular Fuzzer (VMF)
 * Copyright (c) 2021-2026 The Charles Stark Draper Laboratory, Inc.
 * <vmf@draper.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 (only) as 
 * published by the Free Software Foundation.
 *  
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *  
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *  
 * @license GPL-2.0-only <https://spdx.org/licenses/GPL-2.0-only.html>
 * ===========================================================================*/

#pragma once

#include "RadamsaMutatorBase.hpp"
#include <algorithm>
#include <cctype>
#include <gmpxx.h>
#include <optional>
#include <string>

using std::vector;
using std::isdigit;
using std::pair;
using std::optional;
using std::nullopt;
using std::move;

namespace vmf
{
/** 
 * @brief Base class for all Radamsa Byte Mutators.
 */
class RadamsaByteMutatorBase: public RadamsaMutatorBase
{
public:
    /** 
    * @struct NumInfo 
    *
    * @brief This structure contains the state of an array of numbers translated from an array of text.
    */
    using NumberValue = mpz_class;

    struct NumInfo {
        NumberValue value = 0; /**<The value of the data*/
        size_t offset = 0; /**<The index of the data */
        size_t length = 0; /**<the size of the data*/
    };

    static NumberValue numberValueMax() {
        return (NumberValue(1) << 255u) - 1;
    }

    /**
    * @brief Generates a random amount of repetitions to be performed. Upper limit is 20000;
    * 
    * @param rand Pointer to the random generator instance
    */
    static size_t GetRandomRepetitionLength(VmfRand* rand) noexcept
    {
        constexpr size_t MINIMUM_UPPER_LIMIT{0x2u};
        constexpr size_t MAXIMUM_UPPER_LIMIT{0x20000u};

        size_t randomUpperLimit{MINIMUM_UPPER_LIMIT};

        while(rand->randBetween(0ul, 1ul) != 0u &&
              randomUpperLimit != MAXIMUM_UPPER_LIMIT)
        {
            randomUpperLimit <<= 1u;
        }

        return rand->randBetween(
            0ul,
            static_cast<unsigned long>(randomUpperLimit - 1u)
        );
    }
    /**
     * @brief Encodes 21-bit character code points into UTF-8 values of 1 to 4 bytes
     * 
     * @param cp Character code to transform into UTF-8 values
     */
    vector<uint8_t> encodeUtf8(char32_t cp) {

        vector<uint8_t> result;
        if (cp <= 0x7F) {           // 1B case
            result.push_back(static_cast<uint8_t>(cp & 0xFF));       
        } 
        else if (cp <= 0x7FF) {     // 2B case
            result.push_back(static_cast<uint8_t>(0xC0 | (cp >> 6)));     // 110xxxxx, top 5b
            result.push_back(static_cast<uint8_t>(0x80 | (cp & 0x3F)));   // 10xxxxxx, bottom 6b
        } else if (cp <= 0xFFFF) {  // 3B case
            result.push_back(static_cast<uint8_t>(0xE0 | (cp >> 12)));            // 1110xxxx, top 4b
            result.push_back(static_cast<uint8_t>(0x80 | ((cp >> 6) & 0x3F)));    // 10xxxxxx, next 6b
            result.push_back(static_cast<uint8_t>(0x80 | (cp & 0x3F)));
        } else {                    // 4B case
            result.push_back(static_cast<uint8_t>(0xF0 | (cp >> 18)));            // 11110xxx, top 3b
            result.push_back(static_cast<uint8_t>(0x80 | ((cp >> 12) & 0x3F)));   // 10xxxxxx, next 6b
            result.push_back(static_cast<uint8_t>(0x80 | ((cp >> 6) & 0x3F)));
            result.push_back(static_cast<uint8_t>(0x80 | (cp & 0x3F)));
        }
        return result;
    }
    /**
     * @brief Converts and extracts ASCII numbers given a vector of bytes.
     * 
     * @param data Data vector to parse though
     */
    vector<NumInfo> extractTextualNumbers(const vector<uint8_t>& data) {

        vector<NumInfo> result;
        size_t i = 0;
        while (i < data.size()) {
            if (isdigit(static_cast<unsigned char>(data[i]))) {
                size_t start = i;
                NumberValue value = 0;
                bool overflow = false;
                while (i < data.size() && isdigit(static_cast<unsigned char>(data[i]))) {
                    const unsigned digit = static_cast<unsigned>(data[i] - static_cast<uint8_t>('0'));
                    if (!overflow && value <= (numberValueMax() - digit) / 10u) {
                        value = value * 10u + digit;
                    } else {
                        overflow = true;
                    }
                    ++i;
                }

                if (!overflow) {
                    result.push_back({value, start, i - start});
                }
            }
            else ++i;
        }

        return result;
    }
    /**
     * @brief Generates a vector of "interesting" numbers i.e. values that one off from maxes or minimums.
     */
    vector<NumberValue> generateInterestingNumbers() {
        vector<NumberValue> result;
        vector<unsigned int> shifts = {1, 7, 8, 15, 16, 31, 32, 63, 64, 127, 128};

        for (unsigned int s : shifts) {
            NumberValue val = NumberValue(1) << s;
            result.push_back(val);
            result.push_back(val - 1u);
            result.push_back(val + 1u);
        }

        return result;
    }

    static std::string numberValueToString(const NumberValue& value) {
        return value.get_str();
    }
    /**
     * @brief Suffix view used by the fuse helpers so we do not materialize every suffix as a separate vector.
     */
    template<typename T>
    struct SuffixView {
        const vector<T>* source{nullptr};
        size_t start{0u};

        size_t size() const noexcept {
            return (source == nullptr || start >= source->size()) ? 0u : source->size() - start;
        }

        typename vector<T>::const_iterator begin() const {
            return source->begin() + static_cast<typename vector<T>::difference_type>(start);
        }

        typename vector<T>::const_iterator end() const {
            return source->end();
        }

        bool empty() const noexcept {
            return size() == 0u;
        }
    };

    /**
     * @brief Picks one random suffix position from each vector and returns lightweight views into them.
     * This keeps the fuse helper O(1) on the search side and only pays for the final output copy.
     * 
     * @param a First vector to look though
     * @param b Second vector to look though
     * @param rand Pointer to the random generator instance
     */
    template<typename T>
    pair<SuffixView<T>, SuffixView<T>> findJumpPoints(
        const vector<T>& a,
        const vector<T>& b,
        VmfRand* rand
    ) {
        if (a.empty() || b.empty()) return {SuffixView<T>{}, SuffixView<T>{}};

        const size_t aStart = static_cast<size_t>(rand->randBetween(0ul, static_cast<unsigned long>(a.size() - 1)));
        const size_t bStart = static_cast<size_t>(rand->randBetween(0ul, static_cast<unsigned long>(b.size() - 1)));
        return {SuffixView<T>{&a, aStart}, SuffixView<T>{&b, bStart}};
    }

    /**
     * @brief Combines a prefix substring from "a" and a suffix substring from "b".
     * Very likely to return input unmodified for small buffer sizes if a==b.
     * 
     * @param a First vector to look though
     * @param b Second vector to look though
     * @param rand Pointer to the random generator instance
     */
    template<typename T>
    vector<T> fuse(const vector<T>& a, const vector<T>& b, VmfRand* rand) {

        if (a.empty() || b.empty()) return a;

        auto [from, to] = findJumpPoints(a, b, rand);
        if (from.source == nullptr || to.source == nullptr) return a;

        if (std::equal(from.begin(), from.end(), a.end() - from.size())) {
            vector<T> result;
            result.reserve(a.size() - from.size() + to.size());
            result.insert(result.end(), a.begin(), a.end() - from.size()); // keep prefix
            result.insert(result.end(), to.begin(), to.end());            // append suffix
            return result;
        }
        return a;
    }
};
}
