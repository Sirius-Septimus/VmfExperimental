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
#include "RadamsaSwapLineMutator.hpp"
#include "RuntimeException.hpp"
#include <random>
#include <algorithm>

using namespace vmf;

#include "ModuleFactory.hpp"
REGISTER_MODULE(RadamsaSwapLineMutator);

/**
 * @brief Builder method to support the ModuleFactory
 * Constructs an instance of this class
 * @return Module* - Pointer to the newly created instance
 */
Module* RadamsaSwapLineMutator::build(std::string name)
{
    return new RadamsaSwapLineMutator(name);
}

/**
 * @brief Initialization method
 *
 * @param config - Configuration object
 */
void RadamsaSwapLineMutator::init(ConfigInterface& config)
{

}

/**
 * @brief Construct a new RadamsaSwapLineMutator::RadamsaSwapLineMutator object
 *
 * @param name The of the name module
 */
RadamsaSwapLineMutator::RadamsaSwapLineMutator(std::string name) : MutatorModule(name)
{
    // rand.randInit();
}

/**
 * @brief Destroy the RadamsaSwapLineMutator::RadamsaSwapLineMutator object
 *
 */
RadamsaSwapLineMutator::~RadamsaSwapLineMutator()
{

}

/**
 * @brief Register the storage needs for this module
 *
 * @param registry - StorageRegistry object
 */
void RadamsaSwapLineMutator::registerStorageNeeds(StorageRegistry& registry)
{
    // This module does not register for a test case buffer key, because mutators are told which buffer to write in storage
    // by the input generator that calls them
}

void RadamsaSwapLineMutator::mutateTestCase(StorageModule& storage, StorageEntry* baseEntry, StorageEntry* newEntry, int testCaseKey)
{
    // Consume the original buffer by copying a line to a random location and appending a null-terminator to the end.

    constexpr size_t minimumSize{1u};
    const size_t minimumSeedIndex{0u};
    const size_t characterIndex{0u};
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

    // Check if character index is within valid range
    if (characterIndex > originalSize - 1u)
    {
        CopyBufferAsIs(baseEntry, newEntry, testCaseKey);
        return;
    }

    const size_t numberOfLinesAfterIndex{
                                    GetNumberOfLinesAfterIndex(
                                                        originalBuffer,
                                                        originalSize,
                                                        characterIndex)};

    // Check if buffer has minimum required number of lines
    if (numberOfLinesAfterIndex < 2) {
        CopyBufferAsIs(baseEntry, newEntry, testCaseKey);
        return;
    }

    // Select random line to copy from/to.

    const unsigned long minimumRandomLineOffset{0ul};

    const size_t firstRandomLineIndex{
                                static_cast<size_t>(rand->randBetween(
                                                    minimumRandomLineOffset,
                                                    static_cast<unsigned long>(numberOfLinesAfterIndex - 1u)))};

    const size_t totalNumberOfLines{
                                GetNumberOfLinesAfterIndex(
                                                        originalBuffer,
                                                        originalSize,
                                                        0u)};

    constexpr size_t lower{0u};
    const size_t upper{totalNumberOfLines - 1u};
    const size_t secondRandomLineIndex{
                                    std::clamp(
                                            firstRandomLineIndex + 1u,
                                            lower,
                                            upper)};

    const Line firstRandomLineData{
                            GetLineData(
                                    originalBuffer,
                                    originalSize,
                                    firstRandomLineIndex,
                                    numberOfLinesAfterIndex)};

    const Line secondRandomLineData{
                            GetLineData(
                                    originalBuffer,
                                    originalSize,
                                    secondRandomLineIndex,
                                    numberOfLinesAfterIndex)};

    // The new buffer will be one line larger than the original buffer;
    // additionally, it will contain one additional byte since a null-terminator will be appended to the end.

    const size_t newBufferSize{originalSize + 1u};

    // Allocate the new buffer and set it's elements to zero.

    char* newBuffer{newEntry->allocateBuffer(testCaseKey, newBufferSize)};
    memset(newBuffer, 0u, newBufferSize);

    // Copy data from the original buffer into the new buffer, but duplicate the random line.
    // The last element in the new buffer is skipped since it was implicitly set to zero during allocation.

    {
        // Copy all of the elements including the line that is to be duplicated.

        const size_t numberOfBytes{originalSize};

        char* destination{newBuffer};
        const char* source{originalBuffer};

        memcpy(destination, source, numberOfBytes);
    }

    {
        // Duplicate the line.

        const size_t numberOfBytes{secondRandomLineData.Size};

        char* destination{newBuffer + firstRandomLineData.StartIndex};
        const char* source{originalBuffer + secondRandomLineData.StartIndex};

        memcpy(destination, source, numberOfBytes);
    }

    {
        // Copy all of the elements after the line that was duplicated.

        const size_t numberOfBytes{firstRandomLineData.Size};

        char* destination{
                    (firstRandomLineData == secondRandomLineData) ?
                    (newBuffer + firstRandomLineData.StartIndex) :
                    (newBuffer + firstRandomLineData.StartIndex + secondRandomLineData.Size)};

        const char* source{originalBuffer + firstRandomLineData.StartIndex};

        memcpy(destination, source, numberOfBytes);
    }
}

