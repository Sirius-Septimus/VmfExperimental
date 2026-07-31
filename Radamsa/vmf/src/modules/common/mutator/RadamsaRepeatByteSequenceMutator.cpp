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
#include "RadamsaRepeatByteSequenceMutator.hpp"
#include "RuntimeException.hpp"
#include <random>
#include <algorithm>
#include <limits>

using namespace vmf;

#include "ModuleFactory.hpp"
REGISTER_MODULE(RadamsaRepeatByteSequenceMutator);

/**
 * @brief Builder method to support the ModuleFactory
 * Constructs an instance of this class
 * @return Module* - Pointer to the newly created instance
 */
Module* RadamsaRepeatByteSequenceMutator::build(std::string name)
{
    return new RadamsaRepeatByteSequenceMutator(name);
}

/**
 * @brief Initialization method
 *
 * @param config - Configuration object
 */
void RadamsaRepeatByteSequenceMutator::init(ConfigInterface& config)
{
    /*
     * Cap comes from the configuration; 0 disables the cap.
     */

    const int configured = config.getIntParam(getModuleName(), "maxBufferGrowthBytes",
                                              static_cast<int>(m_maxBufferGrowthBytes));
    m_maxBufferGrowthBytes = (configured == 0)
        ? std::numeric_limits<size_t>::max()
        : static_cast<size_t>(configured);

    rand = VmfRand::getInstance();
}

/**
 * @brief Construct a new RadamsaRepeatByteSequenceMutator::RadamsaRepeatByteSequenceMutator object
 *
 * @param name The of the name module
 */
RadamsaRepeatByteSequenceMutator::RadamsaRepeatByteSequenceMutator(std::string name) : MutatorModule(name)
{
    
}

/**
 * @brief Destroy the RadamsaRepeatByteSequenceMutator::RadamsaRepeatByteSequenceMutator object
 *
 */
RadamsaRepeatByteSequenceMutator::~RadamsaRepeatByteSequenceMutator()
{

}

/**
 * @brief Register the storage needs for this module
 *
 * @param registry - StorageRegistry object
 */
void RadamsaRepeatByteSequenceMutator::registerStorageNeeds(StorageRegistry& registry)
{
    // This module does not register for a test case buffer key, because mutators are told which buffer to write in storage
    // by the input generator that calls them
}

void RadamsaRepeatByteSequenceMutator::mutateTestCase(StorageModule& storage, StorageEntry* baseEntry, StorageEntry* newEntry, int testCaseKey)
{
    // select a random number of consecutive bytes and repeat them a random number of times

    constexpr size_t minimumSize{2u};
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
    
    // Select random indexes for the start and end of the sequence
    const unsigned long start_lower{0ul};
    const unsigned long start_upper{static_cast<unsigned long>(originalSize - 1u - 1u)}; // additional -1 to leave at least one byte at the end
    const size_t start_index{static_cast<size_t>(rand->randBetween(start_lower, start_upper))};

    const unsigned long end_lower{static_cast<unsigned long>(start_index + 1u)};
    const unsigned long end_upper{static_cast<unsigned long>(originalSize - 1u)};
    const size_t end_index{static_cast<size_t>(rand->randBetween(end_lower, end_upper))};

    const size_t seq_len{end_index - start_index + 1u};

    // Cap repetitions so the per-call growth `seq_len * numberOfRepetitions` stays within `m_maxBufferGrowthBytes`. The original numberOfRepetitions ceiling is 0x20000 from GetRandomRepetitionLength; without this cap a 64 KiB input would request ~8 GiB.
    size_t numberOfRepetitions{GetRandomRepetitionLength(rand)};
    const size_t maxRepetitions{m_maxBufferGrowthBytes / seq_len};
    if (numberOfRepetitions > maxRepetitions)
    {
        numberOfRepetitions = (maxRepetitions > 0u) ? maxRepetitions : 1u;
    }

    // Output layout: prefix [0, start_index) + numberOfRepetitions sequence copies + suffix [end_index + 1, originalSize) + null terminator.
    const size_t maxTotalSize{std::numeric_limits<size_t>::max() - 1u};
    if (originalSize > maxTotalSize)
    {
        CopyBufferAsIs(baseEntry, newEntry, testCaseKey);
        return;
    }
    const size_t remainingSizeBudget{maxTotalSize - originalSize};
    if (seq_len > 0u && numberOfRepetitions > (remainingSizeBudget / seq_len))
    {
        CopyBufferAsIs(baseEntry, newEntry, testCaseKey);
        return;
    }
    const size_t newBufferSize{originalSize + (seq_len * numberOfRepetitions) + 1u};

    if (newBufferSize > static_cast<size_t>(INT_MAX))
    {
        CopyBufferAsIs(baseEntry, newEntry, testCaseKey);
        return;
    }

    char* newBuffer{newEntry->allocateBuffer(testCaseKey, static_cast<int>(newBufferSize))};
    memset(newBuffer, 0u, newBufferSize);

    // Copy prefix [0, start_index).
    memcpy(newBuffer, originalBuffer, start_index);

    // Copy numberOfRepetitions copies of the sequence at successive offsets.
    for (size_t i = 0u; i < numberOfRepetitions; ++i)
    {
        memcpy(
            newBuffer + start_index + (i * seq_len),
            originalBuffer + start_index,
            seq_len
        );
    }

    // Copy suffix [end_index + 1, originalSize) immediately after the repetitions.
    memcpy(
        newBuffer + start_index + (numberOfRepetitions * seq_len),
        originalBuffer + end_index + 1u,
        originalSize - end_index - 1u
    );
}
