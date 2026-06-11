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
#include "RadamsaFuseThisMutator.hpp"
#include "RuntimeException.hpp"
#include <random>
#include <algorithm>
#include <limits>

using namespace vmf;

#include "ModuleFactory.hpp"
REGISTER_MODULE(RadamsaFuseThisMutator);

/**
 * @brief Builder method to support the ModuleFactory
 * Constructs an instance of this class
 * @return Module* - Pointer to the newly created instance
 */
Module* RadamsaFuseThisMutator::build(std::string name)
{
    return new RadamsaFuseThisMutator(name);
}

/**
 * @brief Initialization method
 *
 * @param config - Configuration object
 */
void RadamsaFuseThisMutator::init(ConfigInterface& config)
{
    const int configured = config.getIntParam(getModuleName(), "maxFuseInputSize", static_cast<int>(m_maxFuseInputSize));
    m_maxFuseInputSize = (configured == 0)
        ? std::numeric_limits<size_t>::max()
        : static_cast<size_t>(configured);
}

/**
 * @brief Construct a new RadamsaFuseThisMutator::RadamsaFuseThisMutator object
 *
 * @param name The of the name module
 */
RadamsaFuseThisMutator::RadamsaFuseThisMutator(std::string name) : MutatorModule(name)
{
    // rand->randInit();
}

/**
 * @brief Destroy the RadamsaFuseThisMutator::RadamsaFuseThisMutator object
 *
 */
RadamsaFuseThisMutator::~RadamsaFuseThisMutator()
{

}

/**
 * @brief Register the storage needs for this module
 *
 * @param registry - StorageRegistry object
 */
void RadamsaFuseThisMutator::registerStorageNeeds(StorageRegistry& registry)
{
    // This module does not register for a test case buffer key, because mutators are told which buffer to write in storage
    // by the input generator that calls them
}

void RadamsaFuseThisMutator::mutateTestCase(StorageModule& storage, StorageEntry* baseEntry, StorageEntry* newEntry, int testCaseKey)
{
    // Combines a prefix substring and a suffix substring from random indexes of the buffer
    // NOTE: Very likely to return buffer unmodified for small buffer sizes

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

    if (m_maxFuseInputSize != std::numeric_limits<size_t>::max() && originalSize > m_maxFuseInputSize)
    {
        CopyBufferAsIs(baseEntry, newEntry, testCaseKey);
        return;
    }

    vector<char> data(originalBuffer, originalBuffer + originalSize);
    vector<char> new_data = fuse(data, data, this->rand);

    const size_t newBufferSize{new_data.size() + 1}; // +1 to implicitly append a null terminator
    char* newBuffer{newEntry->allocateBuffer(testCaseKey, newBufferSize)};
    memset(newBuffer, 0u, newBufferSize);
    memcpy(newBuffer, new_data.data(), new_data.size());
}
