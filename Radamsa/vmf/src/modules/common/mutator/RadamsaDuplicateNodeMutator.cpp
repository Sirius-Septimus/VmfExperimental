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
#include "RadamsaDuplicateNodeMutator.hpp"
#include "RuntimeException.hpp"
#include <random>
#include <algorithm>
#include <climits>

using namespace vmf;

#include "ModuleFactory.hpp"
REGISTER_MODULE(RadamsaDuplicateNodeMutator);

/**
 * @brief Builder method to support the ModuleFactory
 * Constructs an instance of this class
 * @return Module* - Pointer to the newly created instance
 */
Module* RadamsaDuplicateNodeMutator::build(std::string name)
{
    return new RadamsaDuplicateNodeMutator(name);
}

/**
 * @brief Initialization method
 *
 * @param config - Configuration object
 */
void RadamsaDuplicateNodeMutator::init(ConfigInterface& config)
{
    rand = VmfRand::getInstance();

    // Clamp DuplicateNode growth by bounding the projected tree size before duplication.
    const int configured = config.getIntParam(getModuleName(), "maxDuplicateNodeNodes",
                                              static_cast<int>(m_maxDuplicateNodeNodes));
    m_maxDuplicateNodeNodes = (configured == 0)
        ? std::numeric_limits<size_t>::max()
        : static_cast<size_t>(configured);

    const int outputConfigured = config.getIntParam(getModuleName(), "maxDuplicateNodeOutputBytes",
                                                   static_cast<int>(m_maxDuplicateNodeOutputBytes));
    m_maxDuplicateNodeOutputBytes = (outputConfigured == 0)
        ? std::numeric_limits<size_t>::max()
        : static_cast<size_t>(outputConfigured);
}

/**
 * @brief Construct a new RadamsaDuplicateNodeMutator::RadamsaDuplicateNodeMutator object
 *
 * @param name The of the name module
 */
RadamsaDuplicateNodeMutator::RadamsaDuplicateNodeMutator(std::string name) : MutatorModule(name)
{
    
}

/**
 * @brief Destroy the RadamsaDuplicateNodeMutator::RadamsaDuplicateNodeMutator object
 *
 */
RadamsaDuplicateNodeMutator::~RadamsaDuplicateNodeMutator()
{

}





/**
 * @brief Register the storage needs for this module
 *
 * @param registry - StorageRegistry object
 */
void RadamsaDuplicateNodeMutator::registerStorageNeeds(StorageRegistry& registry)
{
    // This module does not register for a test case buffer key, because mutators are told which buffer to write in storage
    // by the input generator that calls them
}

void RadamsaDuplicateNodeMutator::mutateTestCase(StorageModule& storage, StorageEntry* baseEntry, StorageEntry* newEntry, int testCaseKey)
{
    // Duplicate a delimiter subtree beside the original.

    const size_t minimumSize{1u};
    size_t originalSize;
    char* originalBuffer;

    try
    {
        originalBuffer = baseEntry->getBufferPointer(testCaseKey);
        originalSize = baseEntry->getBufferSize(testCaseKey);
    }
    catch (const RuntimeException e)
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

    const std::string treeStr(originalBuffer, originalSize);
    auto maybeTree = RadamsaDelimiterTree::tryBuild(treeStr);
    if (!maybeTree)
    {
        CopyBufferAsIs(baseEntry, newEntry, testCaseKey);
        return;
    }
    RadamsaDelimiterTree& tree = *maybeTree;

    std::vector<RadamsaDelimiterTree::Node*> candidates;
    tree.collectCandidates(candidates);
    if (candidates.empty())
    {
        CopyBufferAsIs(baseEntry, newEntry, testCaseKey);
        return;
    }

    size_t selectedIndex{0u};
    
        selectedIndex = static_cast<size_t>(this->rand->randBetween(
            0ul, static_cast<unsigned long>(candidates.size() - 1u)));
    

    if (!tree.duplicateSelectedNode(
            candidates[selectedIndex],
            m_maxDuplicateNodeNodes,
            m_maxDuplicateNodeOutputBytes))
    {
        CopyBufferAsIs(baseEntry, newEntry, testCaseKey);
        return;
    }

    const size_t newBufferSize{tree.serializedSize()};
    if (newBufferSize > m_maxDuplicateNodeOutputBytes ||
        newBufferSize > static_cast<size_t>(INT_MAX))
    {
        CopyBufferAsIs(baseEntry, newEntry, testCaseKey);
        return;
    }

    const std::string modifiedTree = tree.toString();
    char* newBuffer{newEntry->allocateBuffer(testCaseKey, static_cast<int>(newBufferSize))};
    memset(newBuffer, 0u, newBufferSize);
    memcpy(newBuffer, modifiedTree.data(), newBufferSize);
}
