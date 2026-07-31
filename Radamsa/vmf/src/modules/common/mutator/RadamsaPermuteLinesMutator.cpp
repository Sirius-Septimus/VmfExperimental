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
    // Randomize the order of given lines

    const size_t minimumSize{3u};   // minimal case consists of three newlines
    const size_t minimumLines{3u};  // for two lines, just use SwapLine
    size_t originalSize;
    char* originalBuffer;

    // Try to get buffer size and pointer, return early if buffer is not allocated
    try
    {
        originalBuffer = baseEntry->getBufferPointer(testCaseKey);
        originalSize = baseEntry->getBufferSize(testCaseKey);
    }
    catch(const RuntimeException e)
    {
        // Buffer not allocated
        return;
    }

    // Check if buffer pointer is valid (not null)
    if (originalBuffer == nullptr)
    {
        return;
    }

    // Check if buffer size meets minimum requirement
    if (originalSize < minimumSize)
    {
        CopyBufferAsIs(baseEntry, newEntry, testCaseKey);
        return;
    }

    // Cache all line ranges in one pass so the shuffle works from a prebuilt line list.
    const std::vector<Line> lines{
        GetAllLineData(originalBuffer, originalSize)
    };
    const size_t numLines{lines.size()};

    // Check if buffer has minimum required number of lines
    if (numLines < minimumLines) {
        CopyBufferAsIs(baseEntry, newEntry, testCaseKey);
        return;
    }

    // create and initialize vector of indices
    std::vector<size_t> lineOrder(numLines);
    for(size_t i{0}; i < numLines; ++i) {
        lineOrder[i] = i;
    }

    // Randomize only the index order; the line data itself was already cached above.
    // homebrew Fisher-Yates shuffle because std::shuffle can't use VmfRand
    for(size_t i{numLines - 1}; i > 0; --i) {
        long unsigned int min = 0;
        long unsigned int max = static_cast<long unsigned int>(i);
        size_t randIndex = this->rand->randBetween(min, max);

        const size_t temp = lineOrder[i];
        lineOrder[i] = lineOrder[randIndex];
        lineOrder[randIndex] = temp;
    }    

    // create new buffer with modified order
    const size_t newBufferSize{originalSize + 1u};  // +1 to implicitly append null terminator
    char* newBuffer{newEntry->allocateBuffer(testCaseKey, static_cast<int>(newBufferSize))};
    memset(newBuffer, 0u, newBufferSize);
    for(
        size_t i{0}, nextBufferIndex{0}; 
        i < numLines; 
        nextBufferIndex += lines[lineOrder[i]].Size, ++i
    ) {
        memcpy(
            newBuffer + nextBufferIndex, 
            originalBuffer + lines[lineOrder[i]].StartIndex, 
            lines[lineOrder[i]].Size
        );
    }
}
