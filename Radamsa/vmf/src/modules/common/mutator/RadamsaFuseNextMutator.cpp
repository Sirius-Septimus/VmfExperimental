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
#include "RadamsaFuseNextMutator.hpp"
#include "RuntimeException.hpp"
#include <random>
#include <algorithm>
#include <limits>

using namespace vmf;

#include "ModuleFactory.hpp"
REGISTER_MODULE(RadamsaFuseNextMutator);

/**
 * @brief Builder method to support the ModuleFactory
 * Constructs an instance of this class
 * @return Module* - Pointer to the newly created instance
 */
Module* RadamsaFuseNextMutator::build(std::string name)
{
    return new RadamsaFuseNextMutator(name);
}

/**
 * @brief Initialization method
 *
 * @param config - Configuration object
 */
void RadamsaFuseNextMutator::init(ConfigInterface& config)
{
    const int configured = config.getIntParam(getModuleName(), "maxFuseInputSize", static_cast<int>(m_maxFuseInputSize));
    m_maxFuseInputSize = (configured == 0)
        ? std::numeric_limits<size_t>::max()
        : static_cast<size_t>(configured);

    rand = VmfRand::getInstance();
}

/**
 * @brief Construct a new RadamsaFuseNextMutator::RadamsaFuseNextMutator object
 *
 * @param name The of the name module
 */
RadamsaFuseNextMutator::RadamsaFuseNextMutator(std::string name) : MutatorModule(name)
{
    
}

/**
 * @brief Destroy the RadamsaFuseNextMutator::RadamsaFuseNextMutator object
 *
 */
RadamsaFuseNextMutator::~RadamsaFuseNextMutator()
{

}

/**
 * @brief Register the storage needs for this module
 *
 * @param registry - StorageRegistry object
 */
void RadamsaFuseNextMutator::registerStorageNeeds(StorageRegistry& registry)
{
    // This module does not register for a test case buffer key, because mutators are told which buffer to write in storage
    // by the input generator that calls them
}

void RadamsaFuseNextMutator::mutateTestCase(StorageModule& storage, StorageEntry* baseEntry, StorageEntry* newEntry, int testCaseKey)
{
    // result = prefix(prefix(buffer_firstHalf) + suffix(buffer)) + suffix(buffer_secondHalf)

    const size_t minimumSize{2u};
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

    if (m_maxFuseInputSize != std::numeric_limits<size_t>::max() && originalSize > m_maxFuseInputSize)
    {
        CopyBufferAsIs(baseEntry, newEntry, testCaseKey);
        return;
    }

    vector<char> data(originalBuffer, originalBuffer + originalSize);

    const size_t midpoint = data.size() / 2;
    const vector<char> data_firstHalf(data.begin(), data.begin() + midpoint);
    const vector<char> data_secondHalf(data.begin() + midpoint, data.end());

    const vector<char> ab = fuse(data_firstHalf, data, this->rand);
    const vector<char> aba = fuse(ab, data_secondHalf, this->rand);

    const size_t newBufferSize{aba.size()};

    if (newBufferSize > INT_MAX) {
        //Check to see if the newBufferSize excedes the maximum size.
        CopyBufferAsIs(baseEntry, newEntry, testCaseKey);
        return;
    }
    char* newBuffer{newEntry->allocateBuffer(testCaseKey, static_cast<int>(newBufferSize))};
    memset(newBuffer, 0u, newBufferSize);
    memcpy(newBuffer, aba.data(), aba.size());
}
