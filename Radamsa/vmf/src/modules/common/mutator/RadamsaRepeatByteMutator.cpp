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
#include "RadamsaRepeatByteMutator.hpp"
#include "RuntimeException.hpp"
#include <random>
#include <algorithm>
#include <limits>

using namespace vmf;

#include "ModuleFactory.hpp"
REGISTER_MODULE(RadamsaRepeatByteMutator);

/**
 * @brief Builder method to support the ModuleFactory
 * Constructs an instance of this class
 * @return Module* - Pointer to the newly created instance
 */
Module* RadamsaRepeatByteMutator::build(std::string name)
{
    return new RadamsaRepeatByteMutator(name);
}

/**
 * @brief Initialization method
 *
 * @param config - Configuration object
 */
void RadamsaRepeatByteMutator::init(ConfigInterface& config)
{
    /*
     * Cap comes from the configuration; 0 disables the cap.
     */

    const int configured = config.getIntParam(getModuleName(), "maxByteRepetitions",
                                              static_cast<int>(m_maxByteRepetitions));
    m_maxByteRepetitions = (configured == 0)
        ? std::numeric_limits<size_t>::max()
        : static_cast<size_t>(configured);
}

/**
 * @brief Construct a new RadamsaRepeatByteMutator::RadamsaRepeatByteMutator object
 *
 * @param name The of the name module
 */
RadamsaRepeatByteMutator::RadamsaRepeatByteMutator(std::string name) : MutatorModule(name)
{
    // rand->randInit();
}

/**
 * @brief Destroy the RadamsaRepeatByteMutator::RadamsaRepeatByteMutator object
 *
 */
RadamsaRepeatByteMutator::~RadamsaRepeatByteMutator()
{

}

/**
 * @brief Register the storage needs for this module
 *
 * @param registry - StorageRegistry object
 */
void RadamsaRepeatByteMutator::registerStorageNeeds(StorageRegistry& registry)
{
    // This module does not register for a test case buffer key, because mutators are told which buffer to write in storage
    // by the input generator that calls them
}

void RadamsaRepeatByteMutator::mutateTestCase(StorageModule& storage, StorageEntry* baseEntry, StorageEntry* newEntry, int testCaseKey)
{
    // Consume the original buffer by repeating a byte a random number of times and appending a null-terminator to the end.

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

    // The new buffer size will contain a random number of additional elements since we are repeating a random byte.
    // Furthermore, it will contain one more element since we are appending a null-terminator to the end.

    /*
     *	Clamp numberOfRandomByteRepetitions against the configurable budget so the per-call growth (up to ~128 KiB without the cap) stays bounded under GA-feedback iteration.
     */

    // Cap repetitions so per-call growth stays within `m_maxByteRepetitions`.
    size_t numberOfRandomByteRepetitions{GetRandomRepetitionLength(rand)};
    if (numberOfRandomByteRepetitions > m_maxByteRepetitions)
    {
        numberOfRandomByteRepetitions = m_maxByteRepetitions;
    }
    const size_t newBufferSize{originalSize + numberOfRandomByteRepetitions + 1u};

    // Allocate the new buffer and set it's elements to zero.

    char* newBuffer{newEntry->allocateBuffer(testCaseKey, static_cast<int>(newBufferSize))};
    memset(newBuffer, 0u, newBufferSize);

    // Select a random index from which the new bytes will be repeated.

    const unsigned long lower{0ul};
    const size_t upper{originalSize - 1u};
    const unsigned long maximumRandomIndexValue{static_cast<unsigned long>(originalSize - minimumSeedIndex)};
    const size_t randomByteRepetitionIndex{
                                    std::clamp(
                                        static_cast<size_t>(rand->randBetween(
                                            lower,
                                            maximumRandomIndexValue)) + minimumSeedIndex,
                                        static_cast<size_t>(lower),
                                        upper
                                    )
    };

    // Copy data from the original buffer into the new buffer, but repeat the target byte.
    // The last element in the new buffer is skipped since it was implicitly set to zero during allocation.

    for (size_t sourceIndex{0u}, destinationIndex{0u}; sourceIndex < originalSize; ++sourceIndex)
    {
        if (sourceIndex == randomByteRepetitionIndex)
        {
            memset(&newBuffer[destinationIndex], originalBuffer[sourceIndex], numberOfRandomByteRepetitions + 1u);

            ++destinationIndex += numberOfRandomByteRepetitions;
        }
        else
        {
            newBuffer[destinationIndex] = originalBuffer[sourceIndex];

            ++destinationIndex;
        }
    }
}
