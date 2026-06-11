/* =============================================================================
 * Copyright (c) 2026 Vigilant Cyber Systems
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
#include "RadamsaDeleteByteSequenceMutator.hpp"
#include "RuntimeException.hpp"
#include <random>
#include <algorithm>

using namespace vmf;

#include "ModuleFactory.hpp"
REGISTER_MODULE(RadamsaDeleteByteSequenceMutator);

/**
 * @brief Builder method to support the ModuleFactory
 * Constructs an instance of this class
 * @return Module* - Pointer to the newly created instance
 */
Module* RadamsaDeleteByteSequenceMutator::build(std::string name)
{
    return new RadamsaDeleteByteSequenceMutator(name);
}

/**
 * @brief Initialization method
 *
 * @param config - Configuration object
 */
void RadamsaDeleteByteSequenceMutator::init(ConfigInterface& config)
{

}

/**
 * @brief Construct a new RadamsaDeleteByteSequenceMutator::RadamsaDeleteByteSequenceMutator object
 *
 * @param name The of the name module
 */
RadamsaDeleteByteSequenceMutator::RadamsaDeleteByteSequenceMutator(std::string name) : MutatorModule(name)
{
    // rand->randInit();
}

/**
 * @brief Destroy the RadamsaDeleteByteSequenceMutator::RadamsaDeleteByteSequenceMutator object
 *
 */
RadamsaDeleteByteSequenceMutator::~RadamsaDeleteByteSequenceMutator()
{

}

/**
 * @brief Register the storage needs for this module
 *
 * @param registry - StorageRegistry object
 */
void RadamsaDeleteByteSequenceMutator::registerStorageNeeds(StorageRegistry& registry)
{
    // This module does not register for a test case buffer key, because mutators are told which buffer to write in storage
    // by the input generator that calls them
}

void RadamsaDeleteByteSequenceMutator::mutateTestCase(StorageModule& storage, StorageEntry* baseEntry, StorageEntry* newEntry, int testCaseKey)
{
    // select a random number of consecutive bytes and remove them

    constexpr size_t minimumSize{2u};
    const size_t minimumSeedIndex{0u};
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

    // Check if minimum seed index is within valid range
    if (minimumSeedIndex > originalSize - 1u)
    {
        CopyBufferAsIs(baseEntry, newEntry, testCaseKey);
        return;
    }

    // Select random indexes for the start and end of the sequence
    const unsigned long start_lower{0ul};
    const unsigned long start_upper{static_cast<unsigned long>(originalSize - 1u - 1u)}; // additional -1 to leave at least one byte at the end
    const size_t start_index{static_cast<size_t>(rand->randBetween(start_lower, start_upper))};

    const unsigned long end_lower{static_cast<unsigned long>(start_index + 1u)};
    const unsigned long end_upper{static_cast<unsigned long>(originalSize - 1u)};
    const size_t end_index{static_cast<size_t>(rand->randBetween(end_lower, end_upper))};

    // Calculate the size of the modified buffer
    const size_t newBufferSize{originalSize - (end_index - start_index + 1u) + 1u};  // +1 because we're appending a null-terminator

    // Allocate the new buffer and set it's elements to zero.
    char* newBuffer{newEntry->allocateBuffer(testCaseKey, static_cast<int>(newBufferSize))};
    memset(newBuffer, 0u, newBufferSize);

    // Copy pre-sequence into modified buffer
    memcpy(newBuffer, originalBuffer, start_index);

    // Copy post-sequence into modified buffer
    /*
     *	Destination is `newBuffer + start_index`, immediately after the prefix copied above. Writing at offset 0 here would overwrite the prefix.
     */
    memcpy(
        newBuffer + start_index,
        originalBuffer + end_index + 1u,
        originalSize - (end_index + 1u)
    );
}
