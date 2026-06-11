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

}

/**
 * @brief Construct a new RadamsaDeleteSequentialLinesMutator::RadamsaDeleteSequentialLinesMutator object
 *
 * @param name The of the name module
 */
RadamsaDeleteSequentialLinesMutator::RadamsaDeleteSequentialLinesMutator(std::string name) : MutatorModule(name)
{
    // rand.randInit();
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

    // Select a random line to delete.

    const unsigned long minimumRandomLineOffset{0ul};

    const size_t randomLineIndexStart{
                                static_cast<size_t>(rand->randBetween(
                                                minimumRandomLineOffset,
                                                static_cast<unsigned long>(numberOfLinesAfterIndex - 1u)))};

    const size_t randomLineIndexEnd{
                                static_cast<size_t>(rand->randBetween(
                                                minimumRandomLineOffset,
                                                static_cast<unsigned long>((numberOfLinesAfterIndex - 1u) - randomLineIndexStart))) + randomLineIndexStart};

    const Line startLineData{
                        GetLineData(
                                originalBuffer,
                                originalSize,
                                randomLineIndexStart,
                                numberOfLinesAfterIndex)};

    const Line endLineData{
                        GetLineData(
                                originalBuffer,
                                originalSize,
                                randomLineIndexEnd,
                                numberOfLinesAfterIndex)};

    // The new buffer will be multiple lines smaller than the original buffer;
    // additionally, it will contain one additional byte since a null-terminator will be appended to the end.

    const size_t newBufferSize{(originalSize - ((endLineData.StartIndex + endLineData.Size) - startLineData.StartIndex)) + 1u};

    // Allocate the new buffer and set it's elements to zero.

    char* newBuffer{newEntry->allocateBuffer(testCaseKey, newBufferSize)};
    memset(newBuffer, 0u, newBufferSize);

    // Copy data from the original buffer into the new buffer, but skip the elements in the random lines that are to be deleted.
    // The last element in the new buffer is skipped since it was implicitly set to zero during allocation.

    for(size_t sourceIndex{0u}, destinationIndex{0u}; sourceIndex < originalSize; ++sourceIndex)
    {
        const size_t lineStartIndex{startLineData.StartIndex};
        const size_t lineEndIndex{endLineData.StartIndex + endLineData.Size};

        if(sourceIndex < lineStartIndex || sourceIndex >= lineEndIndex)
        {
            newBuffer[destinationIndex] = originalBuffer[sourceIndex];
            ++destinationIndex;
        }
    }
}
