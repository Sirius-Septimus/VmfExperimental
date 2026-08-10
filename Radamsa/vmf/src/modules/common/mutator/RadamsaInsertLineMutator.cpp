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
#include "RadamsaInsertLineMutator.hpp"
#include "RuntimeException.hpp"
#include <random>
#include <algorithm>

using namespace vmf;

#include "ModuleFactory.hpp"
REGISTER_MODULE(RadamsaInsertLineMutator);

/**
 * @brief Builder method to support the ModuleFactory
 * Constructs an instance of this class
 * @return Module* - Pointer to the newly created instance
 */
Module* RadamsaInsertLineMutator::build(std::string name)
{
    return new RadamsaInsertLineMutator(name);
}

/**
 * @brief Initialization method
 *
 * @param config - Configuration object
 */
void RadamsaInsertLineMutator::init(ConfigInterface& config)
{
    rand = VmfRand::getInstance();
}

/**
 * @brief Construct a new RadamsaInsertLineFromElsewhereMutator::RadamsaInsertLineFromElsewhereMutator object
 *
 * @param name The of the name module
 */
RadamsaInsertLineMutator::RadamsaInsertLineMutator(std::string name) : MutatorModule(name)
{
    
}

/**
 * @brief Destroy the RadamsaInsertLineFromElsewhereMutator::RadamsaInsertLineFromElsewhereMutator object
 *
 */
RadamsaInsertLineMutator::~RadamsaInsertLineMutator()
{

}

/**
 * @brief Register the storage needs for this module
 *
 * @param registry - StorageRegistry object
 */
void RadamsaInsertLineMutator::registerStorageNeeds(StorageRegistry& registry)
{
    // This module does not register for a test case buffer key, because mutators are told which buffer to write in storage
    // by the input generator that calls them
}

void RadamsaInsertLineMutator::mutateTestCase(StorageModule& storage, StorageEntry* baseEntry, StorageEntry* newEntry, int testCaseKey)
{
    // Insert a random existing line into a random place.

    constexpr size_t minimumSize{2u};   // minimal case consists of two newlines
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

    const std::vector<Line> lines{GetAllLineData(originalBuffer, originalSize)};
    const size_t numLines{lines.size()};
    if (numLines == 0u)
    {
        CopyBufferAsIs(baseEntry, newEntry, testCaseKey);
        return;
    }

    std::vector<size_t> lineOrder(numLines);
    for (size_t i{0u}; i < numLines; ++i)
    {
        lineOrder[i] = i;
    }

    const size_t originalLineIndex = static_cast<size_t>(this->rand->randBetween(0ul, static_cast<unsigned long>(numLines - 1u)));
    const size_t newLineIndex = static_cast<size_t>(this->rand->randBetween(0ul, static_cast<unsigned long>(numLines)));
    lineOrder.insert(lineOrder.begin() + static_cast<std::vector<size_t>::difference_type>(newLineIndex), originalLineIndex);

    const size_t newBufferSize{GetAllLineDataSize(lines, lineOrder) + 1u};
    if (newBufferSize > static_cast<size_t>(INT_MAX))
    {
        CopyBufferAsIs(baseEntry, newEntry, testCaseKey);
        return;
    }

    char* newBuffer{newEntry->allocateBuffer(testCaseKey, static_cast<int>(newBufferSize))};
    memset(newBuffer, 0u, newBufferSize);
    CopyAllLineDataToBuffer(originalBuffer, lines, lineOrder, newBuffer);
}

