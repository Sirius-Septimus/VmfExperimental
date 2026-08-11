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
#include "RadamsaPermuteByteMutator.hpp"
#include "RuntimeException.hpp"
#include <random>
#include <algorithm>

using namespace vmf;

#include "ModuleFactory.hpp"
REGISTER_MODULE(RadamsaPermuteByteMutator);

/**
 * @brief Builder method to support the ModuleFactory
 * Constructs an instance of this class
 * @return Module* - Pointer to the newly created instance
 */
Module* RadamsaPermuteByteMutator::build(std::string name)
{
    return new RadamsaPermuteByteMutator(name);
}

/**
 * @brief Initialization method
 *
 * @param config - Configuration object
 */
void RadamsaPermuteByteMutator::init(ConfigInterface& config)
{
    rand = VmfRand::getInstance();
}

/**
 * @brief Construct a new RadamsaPermuteByteMutator::RadamsaPermuteByteMutator object
 *
 * @param name The of the name module
 */
RadamsaPermuteByteMutator::RadamsaPermuteByteMutator(std::string name) : MutatorModule(name)
{

}

/**
 * @brief Destroy the RadamsaPermuteByteMutator::RadamsaPermuteByteMutator object
 *
 */
RadamsaPermuteByteMutator::~RadamsaPermuteByteMutator()
{

}

/**
 * @brief Register the storage needs for this module
 *
 * @param registry - StorageRegistry object
 */
void RadamsaPermuteByteMutator::registerStorageNeeds(StorageRegistry& registry)
{
    // This module does not register for a test case buffer key, because mutators are told which buffer to write in storage
    // by the input generator that calls them
}

void RadamsaPermuteByteMutator::mutateTestCase(StorageModule& storage, StorageEntry* baseEntry, StorageEntry* newEntry, int testCaseKey)
{
    constexpr size_t minimumSize{1u};
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

    // The new buffer size matches the original buffer length.

    const size_t newBufferSize{originalSize};

    // Allocate the new buffer, set it's elements to those of the original buffer, and append a null-terminator to the end.

    char* newBuffer{newEntry->allocateBuffer(testCaseKey, static_cast<int>(newBufferSize))};
    memset(newBuffer, 0u, newBufferSize);
    memcpy(newBuffer, originalBuffer, originalSize);

    // Shuffle a local slice, matching rusty-radamsa's p..n window.
    const size_t p{static_cast<size_t>(rand->randBetween(0ul, static_cast<unsigned long>(originalSize - 1u)))};
    const size_t sliceLength{static_cast<size_t>(rand->randBetween(2ul, 19ul))};
    const size_t n{std::min(p + sliceLength, originalSize)};

    for (size_t i{n - 1u}; i > p; --i)
    {
        const size_t randomIndexToSwap{static_cast<size_t>(rand->randBetween(static_cast<unsigned long>(p), static_cast<unsigned long>(i)))};
        std::swap(newBuffer[i], newBuffer[randomIndexToSwap]);
    }
}
