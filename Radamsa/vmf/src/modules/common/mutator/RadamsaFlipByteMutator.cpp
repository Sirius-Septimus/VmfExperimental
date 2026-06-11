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
#include "RadamsaFlipByteMutator.hpp"
#include "RuntimeException.hpp"
#include <random>
#include <algorithm>

using namespace vmf;

#include "ModuleFactory.hpp"
REGISTER_MODULE(RadamsaFlipByteMutator);

/**
 * @brief Builder method to support the ModuleFactory
 * Constructs an instance of this class
 * @return Module* - Pointer to the newly created instance
 */
Module* RadamsaFlipByteMutator::build(std::string name)
{
    return new RadamsaFlipByteMutator(name);
}

/**
 * @brief Initialization method
 *
 * @param config - Configuration object
 */
void RadamsaFlipByteMutator::init(ConfigInterface& config)
{

}

/**
 * @brief Construct a new RadamsaFlipByteMutator::RadamsaFlipByteMutator object
 *
 * @param name The of the name module
 */
RadamsaFlipByteMutator::RadamsaFlipByteMutator(std::string name) : MutatorModule(name)
{
    // rand->randInit();
}

/**
 * @brief Destroy the RadamsaFlipByteMutator::RadamsaFlipByteMutator object
 *
 */
RadamsaFlipByteMutator::~RadamsaFlipByteMutator()
{

}

/**
 * @brief Register the storage needs for this module
 *
 * @param registry - StorageRegistry object
 */
void RadamsaFlipByteMutator::registerStorageNeeds(StorageRegistry& registry)
{
    // This module does not register for a test case buffer key, because mutators are told which buffer to write in storage
    // by the input generator that calls them
}

void RadamsaFlipByteMutator::mutateTestCase(StorageModule& storage, StorageEntry* baseEntry, StorageEntry* newEntry, int testCaseKey)
{
    // Consume the original buffer by flipping a byte and appending a null-terminator to the end.

    constexpr size_t minimumSize{1u};
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

    // The new buffer size will contain one additional element since we are appending a null-terminator to the end.

    const size_t newBufferSize{originalSize + 1u};

    // Allocate the new buffer, set it's elements to those of the original buffer, and append a null-terminator to the end.

    char* newBuffer{newEntry->allocateBuffer(testCaseKey, static_cast<int>(newBufferSize))};
    memset(newBuffer, 0u, newBufferSize);
    memcpy(newBuffer, originalBuffer, originalSize);

    // Select a random byte to drop from the original buffer.

    const unsigned long lower{0ul};
    const size_t upper{originalSize - 1u};
    const unsigned long maximumRandomIndexValue{static_cast<unsigned long>(originalSize - minimumSeedIndex)};
    const size_t randomIndexToFlip{
                            std::clamp(
                                    static_cast<size_t>(rand->randBetween(
                                                lower,
                                                maximumRandomIndexValue
                                    )) + minimumSeedIndex,
                                    static_cast<size_t>(lower),
                                    upper)};

    // Select a random bit to flip from the random byte.
    // When computing the random bit shift,
    // we add 1 so that a maximum number of 8 bit shift operations can be performed against a char containing the value 0x01.

    const size_t randomBitShift{
        static_cast<size_t>(rand->randBetween(
            0ul, 
            static_cast<unsigned long>(std::numeric_limits<char>::digits + 1u)
        ))
    };
    const char randomMaskedBit{static_cast<char>(0x01u << randomBitShift)};

    // Flip the random byte by performing an XOR operation with a random masked bit.
    newBuffer[randomIndexToFlip] ^= randomMaskedBit;
}