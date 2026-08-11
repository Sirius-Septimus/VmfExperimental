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
#include "RadamsaDeleteSequentialLinesMutator.hpp"
#include "RuntimeException.hpp"
#include <random>
#include <algorithm>

using namespace vmf;

#include "ModuleFactory.hpp"
REGISTER_MODULE(RadamsaDeleteSequentialLinesMutator);

/**
 * @brief Builder method to support the ModuleFactory
 * Constructs an instance of this class
 * @return Module* - Pointer to the newly created instance
 */
Module* RadamsaDeleteSequentialLinesMutator::build(std::string name)
{
    return new RadamsaDeleteSequentialLinesMutator(name);
}

/**
 * @brief Initialization method
 *
 * @param config - Configuration object
 */
void RadamsaDeleteSequentialLinesMutator::init(ConfigInterface& config)
{
    rand = VmfRand::getInstance();
}

/**
 * @brief Construct a new RadamsaDeleteSequentialLinesMutator::RadamsaDeleteSequentialLinesMutator object
 *
 * @param name The of the name module
 */
RadamsaDeleteSequentialLinesMutator::RadamsaDeleteSequentialLinesMutator(std::string name) : MutatorModule(name)
{
    
}

/**
 * @brief Destroy the RadamsaDeleteSequentialLinesMutator::RadamsaDeleteSequentialLinesMutator object
 *
 */
RadamsaDeleteSequentialLinesMutator::~RadamsaDeleteSequentialLinesMutator()
{

}

/**
 * @brief Register the storage needs for this module
 *
 * @param registry - StorageRegistry object
 */
void RadamsaDeleteSequentialLinesMutator::registerStorageNeeds(StorageRegistry& registry)
{
    // This module does not register for a test case buffer key, because mutators are told which buffer to write in storage
    // by the input generator that calls them
}

void RadamsaDeleteSequentialLinesMutator::mutateTestCase(StorageModule& storage, StorageEntry* baseEntry, StorageEntry* newEntry, int testCaseKey)
{
    // Consume the original buffer by deleting sequential lines from it and appending a null-terminator to the end.

    constexpr size_t minimumSize{1u};
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
    if (numLines < 2u)
    {
        CopyBufferAsIs(baseEntry, newEntry, testCaseKey);
        return;
    }

    const size_t randomLineIndexStart{static_cast<size_t>(rand->randBetween(0ul, static_cast<unsigned long>(numLines - 1u)))};
    const size_t randomLineIndexEnd{static_cast<size_t>(rand->randBetween(0ul, static_cast<unsigned long>((numLines - 1u) - randomLineIndexStart))) + randomLineIndexStart};

    std::vector<size_t> lineOrder(numLines);
    for (size_t i{0u}; i < numLines; ++i)
    {
        lineOrder[i] = i;
    }
    lineOrder.erase(
        lineOrder.begin() + static_cast<std::vector<size_t>::difference_type>(randomLineIndexStart),
        lineOrder.begin() + static_cast<std::vector<size_t>::difference_type>(randomLineIndexEnd + 1u)
    );

    const size_t newBufferSize{GetAllLineDataSize(lines, lineOrder)};
    if (newBufferSize > static_cast<size_t>(INT_MAX))
    {
        CopyBufferAsIs(baseEntry, newEntry, testCaseKey);
        return;
    }

    char* newBuffer{newEntry->allocateBuffer(testCaseKey, static_cast<int>(newBufferSize))};
    memset(newBuffer, 0u, newBufferSize);
    CopyAllLineDataToBuffer(originalBuffer, lines, lineOrder, newBuffer);
}

