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
#include "RadamsaWidenCodePointMutator.hpp"
#include "RuntimeException.hpp"
#include <random>
#include <algorithm>

using namespace vmf;

#include "ModuleFactory.hpp"
REGISTER_MODULE(RadamsaWidenCodePointMutator);

/**
 * @brief Builder method to support the ModuleFactory
 * Constructs an instance of this class
 * @return Module* - Pointer to the newly created instance
 */
Module* RadamsaWidenCodePointMutator::build(std::string name)
{
    return new RadamsaWidenCodePointMutator(name);
}

/**
 * @brief Initialization method
 *
 * @param config - Configuration object
 */
void RadamsaWidenCodePointMutator::init(ConfigInterface& config)
{

}

/**
 * @brief Construct a new RadamsaWidenCodePointMutator::RadamsaWidenCodePointMutator object
 *
 * @param name The of the name module
 */
RadamsaWidenCodePointMutator::RadamsaWidenCodePointMutator(std::string name) : MutatorModule(name)
{
    // rand->randInit();
}

/**
 * @brief Destroy the RadamsaWidenCodePointMutator::RadamsaWidenCodePointMutator object
 *
 */
RadamsaWidenCodePointMutator::~RadamsaWidenCodePointMutator()
{

}

/**
 * @brief Register the storage needs for this module
 *
 * @param registry - StorageRegistry object
 */
void RadamsaWidenCodePointMutator::registerStorageNeeds(StorageRegistry& registry)
{
    // This module does not register for a test case buffer key, because mutators are told which buffer to write in storage
    // by the input generator that calls them
}

void RadamsaWidenCodePointMutator::mutateTestCase(StorageModule& storage, StorageEntry* baseEntry, StorageEntry* newEntry, int testCaseKey)
{
    // Replace a random 6-bit ASCII character with an equivalent 2-byte UTF-8-like sequence

    const size_t minimumSize{1u};
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

    std::vector<uint8_t> data(originalBuffer, originalBuffer + originalSize);

    // Collect indices of all valid printable ASCII bytes
    std::vector<size_t> validIndices;
    for (size_t i = 0; i < data.size(); ++i)
    {
        if (data[i] >= 32 && data[i] <= 126)
        {
            validIndices.push_back(i);
        }
    }

    // If no valid ASCII byte exists, copy buffer as-is
    if (validIndices.empty())
    {
        CopyBufferAsIs(baseEntry, newEntry, testCaseKey);
        return;
    }

    // Pick a random valid index
    const unsigned long lower{0ul};
    const unsigned long upper{static_cast<unsigned long>(validIndices.size() - 1)};
    size_t index = validIndices[static_cast<size_t>(this->rand->randBetween(lower, upper))];
    uint8_t codePoint = data[index];

    data[index] = 0b11000000;   // set 2-byte utf prefix (110xxxxx)
    data.insert(data.begin() + index + 1, codePoint | 0b10000000); // set continuation byte prefix (10xxxxxx)

    const size_t newBufferSize{data.size() + 1}; // +1 to implicitly append a null terminator
    char* newBuffer{newEntry->allocateBuffer(testCaseKey, newBufferSize)};
    memset(newBuffer, 0u, newBufferSize);
    memcpy(newBuffer, data.data(), data.size());

    return;
}