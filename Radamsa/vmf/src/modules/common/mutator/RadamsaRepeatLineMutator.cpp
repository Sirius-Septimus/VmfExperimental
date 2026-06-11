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
#include "RadamsaRepeatLineMutator.hpp"
#include "RuntimeException.hpp"
#include <random>
#include <algorithm>
#include <limits>

using namespace vmf;

#include "ModuleFactory.hpp"
REGISTER_MODULE(RadamsaRepeatLineMutator);

/**
 * @brief Builder method to support the ModuleFactory
 * Constructs an instance of this class
 * @return Module* - Pointer to the newly created instance
 */
Module* RadamsaRepeatLineMutator::build(std::string name)
{
    return new RadamsaRepeatLineMutator(name);
}

/**
 * @brief Initialization method
 *
 * @param config - Configuration object
 */
void RadamsaRepeatLineMutator::init(ConfigInterface& config)
{
    /*
     * Cap comes from the configuration; 0 disables the cap.
     */

    const int configured = config.getIntParam(getModuleName(), "maxBufferGrowthBytes",
                                              static_cast<int>(m_maxBufferGrowthBytes));
    m_maxBufferGrowthBytes = (configured == 0)
        ? std::numeric_limits<size_t>::max()
        : static_cast<size_t>(configured);
}

/**
 * @brief Construct a new RadamsaRepeatLineMutator::RadamsaRepeatLineMutator object
 *
 * @param name The of the name module
 */
RadamsaRepeatLineMutator::RadamsaRepeatLineMutator(std::string name) : MutatorModule(name)
{
    // rand.randInit();
}

/**
 * @brief Destroy the RadamsaRepeatLineMutator::RadamsaRepeatLineMutator object
 *
 */
RadamsaRepeatLineMutator::~RadamsaRepeatLineMutator()
{

}

/**
 * @brief Register the storage needs for this module
 *
 * @param registry - StorageRegistry object
 */
void RadamsaRepeatLineMutator::registerStorageNeeds(StorageRegistry& registry)
{
    // This module does not register for a test case buffer key, because mutators are told which buffer to write in storage
    // by the input generator that calls them
}

void RadamsaRepeatLineMutator::mutateTestCase(StorageModule& storage, StorageEntry* baseEntry, StorageEntry* newEntry, int testCaseKey)
{
    // Consume the original buffer by repeating a random line multiple times and appending a null-terminator to the end.

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

    //Check if the buffer size hits the shared memory maximum limit (frida)
    if(originalSize >= (1024 * 1024))
    {
        CopyBufferAsIs(baseEntry, newEntry, testCaseKey);
        return;
    }

    const size_t numberOfLinesAfterIndex{
                                    GetNumberOfLinesAfterIndex(
                                                        originalBuffer,
                                                        originalSize,
                                                        characterIndex)};

    // Select a random line to duplicate.

    constexpr unsigned long minimumRandomLineIndex{0ul};
    const unsigned long maximumRandomLineIndex{static_cast<unsigned long>(numberOfLinesAfterIndex - 1u)};

    const size_t randomLineIndex{
                            static_cast<size_t>(rand->randBetween(
                                            minimumRandomLineIndex,
                                            maximumRandomLineIndex))};

    const Line lineData{
                    GetLineData(
                            originalBuffer,
                            originalSize,
                            randomLineIndex,
                            numberOfLinesAfterIndex)};

    /*
     *	Clamp numberOfRandomLineRepetitions against the configurable budget so the per-call allocation `lineData.Size * numberOfRandomLineRepetitions` stays bounded.
     */

    // Cap repetitions so the per-call growth `lineData.Size * numberOfRandomLineRepetitions` stays within `m_maxBufferGrowthBytes`.
    size_t numberOfRandomLineRepetitions{GetRandomRepetitionLength(this->rand)};
    if (lineData.Size > 0u)
    {
        const size_t maxRepetitions{m_maxBufferGrowthBytes / lineData.Size};
        if (numberOfRandomLineRepetitions > maxRepetitions)
        {
            numberOfRandomLineRepetitions = (maxRepetitions > 0u) ? maxRepetitions : 1u;
        }
    }

    // The new buffer will be multiple lines larger than the original buffer;
    // additionally, it will contain one additional byte since a null-terminator will be appended to the end.
    
    const size_t newBufferSize{originalSize + (lineData.Size * numberOfRandomLineRepetitions) + 1u};

    //For better performance do not produce testcases that are over the size limit for the executor.
    // if (newBufferSize >= (1024 * 1024)) {
    //     CopyBufferAsIs(baseEntry, newEntry, testCaseKey);
    //     return;
    // }

    // Allocate the new buffer and set it's elements to zero.

    char* newBuffer{newEntry->allocateBuffer(testCaseKey, newBufferSize)};
    memset(newBuffer, 0u, newBufferSize);

    // Copy data from the original buffer into the new buffer, but repeat the random line.
    // The last element in the new buffer is skipped since it was implicitly set to zero during allocation.

    const size_t lineStart{lineData.StartIndex};
    const size_t lineEnd{lineData.StartIndex + lineData.Size};
    size_t destinationIndex{0u};

    // Copy bytes before the selected line
    memcpy(&newBuffer[destinationIndex], &originalBuffer[0], lineStart);
    destinationIndex += lineStart;

    // Write the selected line (numberOfRandomLineRepetitions + 1) times
    for (size_t k{0u}; k < (numberOfRandomLineRepetitions + 1u); ++k)
    {
        memcpy(&newBuffer[destinationIndex], &originalBuffer[lineStart], lineData.Size);
        destinationIndex += lineData.Size;
    }

    // Copy bytes after the selected line
    memcpy(&newBuffer[destinationIndex], &originalBuffer[lineEnd], originalSize - lineEnd);
    destinationIndex += originalSize - lineEnd;

}
