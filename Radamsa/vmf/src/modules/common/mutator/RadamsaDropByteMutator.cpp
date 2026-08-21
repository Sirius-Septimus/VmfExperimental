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
#include "RadamsaDropByteMutator.hpp"
#include "RuntimeException.hpp"
#include <random>
#include <algorithm>

using namespace vmf;

#include "ModuleFactory.hpp"
REGISTER_MODULE(RadamsaDropByteMutator);

/**
 * @brief Builder method to support the ModuleFactory
 * Constructs an instance of this class
 * @return Module* - Pointer to the newly created instance
 */
Module* RadamsaDropByteMutator::build(std::string name)
{
    return new RadamsaDropByteMutator(name);
}

/**
 * @brief Initialization method
 *
 * @param config - Configuration object
 */
void RadamsaDropByteMutator::init(ConfigInterface& config)
{
    rand = VmfRand::getInstance();
}

/**
 * @brief Construct a new RadamsaDropByteMutator::RadamsaDropByteMutator object
 *
 * @param name The of the name module
 */
RadamsaDropByteMutator::RadamsaDropByteMutator(std::string name) : MutatorModule(name)
{
    
}

/**
 * @brief Destroy the RadamsaDropByteMutator::RadamsaDropByteMutator object
 *
 */
RadamsaDropByteMutator::~RadamsaDropByteMutator()
{

}





/**
 * @brief Register the storage needs for this module
 *
 * @param registry - StorageRegistry object
 */
void RadamsaDropByteMutator::registerStorageNeeds(StorageRegistry& registry)
{
    // This module does not register for a test case buffer key, because mutators are told which buffer to write in storage
    // by the input generator that calls them
}

void RadamsaDropByteMutator::mutateTestCase(StorageModule& storage, StorageEntry* baseEntry, StorageEntry* newEntry, int testCaseKey)
{
    // Consume the original buffer by dropping a byte.

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

    // The new buffer will contain one less byte.

    //Buffer will never grow past sizeof(int) because it is returned from getBufferSize()
    const int newBufferSize{static_cast<int>(originalSize - 1u)}; 

    // Allocate the new buffer and set it's elements to zero.

    char* newBuffer{newEntry->allocateBuffer(testCaseKey, newBufferSize)};
    memset(newBuffer, 0u, newBufferSize);

    size_t indexToDrop{0u};
    
        indexToDrop = static_cast<size_t>(
            rand->randBetween(0ul, static_cast<unsigned long>(originalSize - 1u))
        );
    

    // Copy data from the original buffer into the new buffer, but exclude the random byte.
    
    for (size_t sourceIndex{0u}, destinationIndex{0u}; sourceIndex < originalSize; ++sourceIndex)
    {
        if (sourceIndex != indexToDrop)
        {
            newBuffer[destinationIndex] = originalBuffer[sourceIndex];

            ++destinationIndex;
        }
    }
}
