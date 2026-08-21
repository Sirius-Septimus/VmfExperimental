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
#include "RadamsaRandomizeByteMutator.hpp"
#include "RuntimeException.hpp"
#include <random>
#include <algorithm>
#include <limits>

using namespace vmf;

#include "ModuleFactory.hpp"
REGISTER_MODULE(RadamsaRandomizeByteMutator);

/**
 * @brief Builder method to support the ModuleFactory
 * Constructs an instance of this class
 * @return Module* - Pointer to the newly created instance
 */
Module* RadamsaRandomizeByteMutator::build(std::string name)
{
    return new RadamsaRandomizeByteMutator(name);
}

/**
 * @brief Initialization method
 *
 * @param config - Configuration object
 */
void RadamsaRandomizeByteMutator::init(ConfigInterface& config)
{
    rand = VmfRand::getInstance();
}

/**
 * @brief Construct a new RadamsaRandomizeByteMutator::RadamsaRandomizeByteMutator object
 *
 * @param name The of the name module
 */
RadamsaRandomizeByteMutator::RadamsaRandomizeByteMutator(std::string name) : MutatorModule(name)
{
    
}

/**
 * @brief Destroy the RadamsaRandomizeByteMutator::RadamsaRandomizeByteMutator object
 *
 */
RadamsaRandomizeByteMutator::~RadamsaRandomizeByteMutator()
{

}





/**
 * @brief Register the storage needs for this module
 *
 * @param registry - StorageRegistry object
 */
void RadamsaRandomizeByteMutator::registerStorageNeeds(StorageRegistry& registry)
{
    // This module does not register for a test case buffer key, because mutators are told which buffer to write in storage
    // by the input generator that calls them
}

void RadamsaRandomizeByteMutator::mutateTestCase(StorageModule& storage, StorageEntry* baseEntry, StorageEntry* newEntry, int testCaseKey)
{
    // Consume the original buffer by randomizing a random byte and appending a null-terminator to the end.

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

    // and replacement byte so both implementations can be compared under the
    size_t indexToRandomize{0u};
    unsigned char randomByte{0u};
    
        indexToRandomize = static_cast<size_t>(
            rand->randBetween(0ul, static_cast<unsigned long>(originalSize - 1u))
        );
        randomByte = static_cast<unsigned char>(
            rand->randBetween(0ul, static_cast<unsigned long>(std::numeric_limits<unsigned char>::max()))
        );
    

    newBuffer[indexToRandomize] = static_cast<char>(randomByte);
}
