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

// C++ Includes
#include <random>
#include <algorithm>
#include <iostream>
#include <climits>
#include "VmfRand.hpp"

namespace vmf
{
/**
 * @brief Base class for all Radamsa Mutators.
 */
class RadamsaMutatorBase
{
public:

/**
 * @brief Generates a random amount of repetitions to be performed. Upper limit is 20000;
 * 
 * @param rand Pointer the random generator instance
 */
size_t GetRandomRepetitionLength(VmfRand* rand) noexcept
{
    constexpr size_t MINIMUM_UPPER_LIMIT{0x2u};
    constexpr size_t MAXIMUM_UPPER_LIMIT{0x20000u};

    size_t randomStop{rand->randBetween(0ul, static_cast<unsigned long>(MINIMUM_UPPER_LIMIT))};
    size_t randomUpperLimit{MINIMUM_UPPER_LIMIT};

    while(randomStop != 0u)
    {
        if(randomUpperLimit == MAXIMUM_UPPER_LIMIT)
            break;

        randomUpperLimit <<= 1u;
        randomStop = rand->randBetween(0ul, static_cast<unsigned long>(MINIMUM_UPPER_LIMIT));
    }

    return rand->randBetween(0ul, static_cast<unsigned long>(randomUpperLimit)) + 1u; // We add one to the return value in order to account for the case where the random upper value is zero.
}

/**
 * @brief Copies an entry to a new entry using the same testCaseKey. The entry remains unchanged.
 * 
 * @param baseEntry Entry to copy from
 * @param newEntry Entry to copy to
 * @param testCaseKey Testcase key to be used for the new entry
 */
void CopyBufferAsIs(StorageEntry* baseEntry, StorageEntry* newEntry, int testCaseKey)
{
    char* originalBuffer = baseEntry->getBufferPointer(testCaseKey);
    int originalSize = baseEntry->getBufferSize(testCaseKey);
    char* newBuffer{newEntry->allocateBuffer(testCaseKey, originalSize)};
    memcpy(newBuffer, originalBuffer, originalSize);
    return;
}
};
}