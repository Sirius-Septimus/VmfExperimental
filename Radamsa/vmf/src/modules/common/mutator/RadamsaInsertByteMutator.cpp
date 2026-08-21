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
#include "RadamsaInsertByteMutator.hpp"
#include "RuntimeException.hpp"
#include <random>
#include <algorithm>
#include <limits>

using namespace vmf;

#include "ModuleFactory.hpp"
REGISTER_MODULE(RadamsaInsertByteMutator);

/**
 * @brief Builder method to support the ModuleFactory
 * Constructs an instance of this class
 * @return Module* - Pointer to the newly created instance
 */
Module* RadamsaInsertByteMutator::build(std::string name)
{
    return new RadamsaInsertByteMutator(name);
}

/**
 * @brief Initialization method
 *
 * @param config - Configuration object
 */
void RadamsaInsertByteMutator::init(ConfigInterface& config)
{
    rand = VmfRand::getInstance();
}

/**
 * @brief Construct a new RadamsaInsertByteMutator::RadamsaInsertByteMutator object
 *
 * @param name The of the name module
 */
RadamsaInsertByteMutator::RadamsaInsertByteMutator(std::string name) : MutatorModule(name)
{
    
}

/**
 * @brief Destroy the RadamsaInsertByteMutator::RadamsaInsertByteMutator object
 *
 */
RadamsaInsertByteMutator::~RadamsaInsertByteMutator()
{

}





/**
 * @brief Register the storage needs for this module
 *
 * @param registry - StorageRegistry object
 */
void RadamsaInsertByteMutator::registerStorageNeeds(StorageRegistry& registry)
{
    // This module does not register for a test case buffer key, because mutators are told which buffer to write in storage
    // by the input generator that calls them
}

void RadamsaInsertByteMutator::mutateTestCase(StorageModule& storage, StorageEntry* baseEntry, StorageEntry* newEntry, int testCaseKey)
{
    // Consume the original buffer by inserting a byte at a randomly chosen position.

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

    // The new buffer size will contain one additional element since we are inserting a random byte.
    const size_t newBufferSize{originalSize + 1u};

    // Allocate the new buffer and set it's elements to zero.
    char* newBuffer{newEntry->allocateBuffer(testCaseKey, static_cast<int>(newBufferSize))};
    memset(newBuffer, 0u, newBufferSize);

    // semantic decisions this mutator should make. That keeps the focus on
    // the focus on mutation behavior rather than RNG implementation details.
    size_t insertionIndex{0u};
    char insertedByte{0};

    
        insertionIndex = static_cast<size_t>(
            rand->randBetween(0ul, static_cast<unsigned long>(originalSize))
        );
        insertedByte = static_cast<char>(
            rand->randBetween(
                0ul,
                static_cast<unsigned long>(std::numeric_limits<unsigned char>::max())
            )
        );
    

    // Copy data from the original buffer into the new buffer while inserting one extra byte.
    for (size_t sourceIndex{0u}, destinationIndex{0u}; sourceIndex < originalSize; ++sourceIndex, ++destinationIndex)
    {
        if (destinationIndex == insertionIndex)
        {
            newBuffer[destinationIndex++] = insertedByte;
        }

        newBuffer[destinationIndex] = originalBuffer[sourceIndex];
    }

    if (insertionIndex == originalSize)
    {
        newBuffer[originalSize] = insertedByte;
    }
}
