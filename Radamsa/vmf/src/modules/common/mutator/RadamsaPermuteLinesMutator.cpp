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
 /**
  *
  */
#include "RadamsaPermuteLinesMutator.hpp"
#include "RuntimeException.hpp"
#include <random>
#include <algorithm>

using namespace vmf;

#include "ModuleFactory.hpp"
REGISTER_MODULE(RadamsaPermuteLinesMutator);

/**
 * @brief Builder method to support the ModuleFactory
 * Constructs an instance of this class
 * @return Module* - Pointer to the newly created instance
 */
Module* RadamsaPermuteLinesMutator::build(std::string name)
{
    return new RadamsaPermuteLinesMutator(name);
}

/**
 * @brief Initialization method
 *
 * @param config - Configuration object
 */
void RadamsaPermuteLinesMutator::init(ConfigInterface& config)
{
    rand = VmfRand::getInstance();
}

/**
 * @brief Construct a new RadamsaPermuteLinesMutator::RadamsaPermuteLinesMutator object
 *
 * @param name The of the name module
 */
RadamsaPermuteLinesMutator::RadamsaPermuteLinesMutator(std::string name) : MutatorModule(name)
{
    
}

/**
 * @brief Destroy the RadamsaPermuteLinesMutator::RadamsaPermuteLinesMutator object
 *
 */
RadamsaPermuteLinesMutator::~RadamsaPermuteLinesMutator()
{

}

/**
 * @brief Register the storage needs for this module
 *
 * @param registry - StorageRegistry object
 */
void RadamsaPermuteLinesMutator::registerStorageNeeds(StorageRegistry& registry)
{
    // This module does not register for a test case buffer key, because mutators are told which buffer to write in storage
    // by the input generator that calls them
}

void RadamsaPermuteLinesMutator::mutateTestCase(StorageModule& storage, StorageEntry* baseEntry, StorageEntry* newEntry, int testCaseKey)
{
    // Randomize the order of given lines.

    const size_t minimumSize{3u};   // minimal case consists of three newlines
    const size_t minimumLines{3u};  // for two lines, just use SwapLine
    size_t originalSize;
    char* originalBuffer;

    try
    {
        originalBuffer = baseEntry->getBufferPointer(testCaseKey);
        originalSize = baseEntry->getBufferSize(testCaseKey);
    }
    catch(const RuntimeException e)
    {
        return;
    }

    if (originalBuffer == nullptr)
    {
        return;
    }

    if (originalSize < minimumSize)
    {
        CopyBufferAsIs(baseEntry, newEntry, testCaseKey);
        return;
    }

    std::vector<Line> lines;
    if (!TryGetAllLineData(originalBuffer, originalSize, lines))
    {
        CopyBufferAsIs(baseEntry, newEntry, testCaseKey);
        return;
    }
    const size_t numLines{lines.size()};
    if (numLines < minimumLines)
    {
        CopyBufferAsIs(baseEntry, newEntry, testCaseKey);
        return;
    }

    std::vector<size_t> lineOrder(numLines);
    for (size_t i{0u}; i < numLines; ++i)
    {
        lineOrder[i] = i;
    }

    const size_t maxStartIndex{numLines - 3u};
    const size_t startIndex{
        maxStartIndex == 0u
            ? 0u
            : static_cast<size_t>(this->rand->randBetween(0ul, static_cast<unsigned long>(maxStartIndex)))
    };
    const size_t maxShuffleLength{numLines - startIndex};
    const size_t shuffleLength{
        std::max(
            2u,
            std::min(
                static_cast<size_t>(this->rand->randBetween(2ul, static_cast<unsigned long>(std::min<size_t>(19u, maxShuffleLength)))),
                maxShuffleLength))
    };

    for (size_t i{startIndex + shuffleLength - 1u}; i > startIndex; --i)
    {
        const size_t randIndex = static_cast<size_t>(this->rand->randBetween(static_cast<unsigned long>(startIndex), static_cast<unsigned long>(i)));
        std::swap(lineOrder[i], lineOrder[randIndex]);
    }

    const size_t newBufferSize{GetAllLineDataSize(lines)};
    if (newBufferSize > static_cast<size_t>(INT_MAX))
    {
        CopyBufferAsIs(baseEntry, newEntry, testCaseKey);
        return;
    }

    char* newBuffer{newEntry->allocateBuffer(testCaseKey, static_cast<int>(newBufferSize))};
    memset(newBuffer, 0u, newBufferSize);
    CopyAllLineDataToBuffer(originalBuffer, lines, lineOrder, newBuffer);
}

