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
    // Duplicates existing node, including its children, and adds it to the same parent as the original

    const size_t minimumSize{4u};   // minimal case consists of two single-character nodes
    const size_t minimumNodes{2u};
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

    const std::string treeStr(originalBuffer, originalSize);
    /*
     *	Build the tree via the noexcept tryBuild factory; fall back to CopyBufferAsIs when the input does not parse as a tree.
     */
    auto maybeTree = Tree::tryBuild(treeStr);
    if (!maybeTree)
    {
        CopyBufferAsIs(baseEntry, newEntry, testCaseKey);
        return;
    }
    Tree& tr = *maybeTree;

    size_t numNodes = tr.countNodes(tr.root);
    // Check if tree has minimum required number of nodes
    if (numNodes < minimumNodes)
    {
        CopyBufferAsIs(baseEntry, newEntry, testCaseKey);
        return;
    }

    const unsigned long lower{1ul};
    const unsigned long upper{static_cast<unsigned long>(numNodes - 2)};
    size_t nodeIndexToDuplicate{static_cast<size_t>(this->rand->randBetween(lower, upper))}; // not const, because findNodeByIndex will modify it
    Node* nodeToDuplicate = tr.findNodeByIndex(tr.root, nodeIndexToDuplicate); 

    // Estimate the growth of the duplicated subtree before attaching it.
    const size_t duplicatedSubtreeNodes{tr.countNodes(nodeToDuplicate)};
    if (m_maxDuplicateNodeNodes != std::numeric_limits<size_t>::max())
    {
        const size_t projectedTotalNodes = numNodes + duplicatedSubtreeNodes;
        if (projectedTotalNodes > m_maxDuplicateNodeNodes)
        {
            CopyBufferAsIs(baseEntry, newEntry, testCaseKey);
            return;
        }
    }

    tr.duplicateNode(nodeToDuplicate, nodeToDuplicate->parent);

    const string modTreeStr = tr.toString(tr.root);
    const size_t newBufferSize{modTreeStr.length() + 1}; // +1 to implicitly append a null terminator
    if (newBufferSize > INT_MAX) {
        //Check to see if the newBufferSize excedes the maximum size.
        CopyBufferAsIs(baseEntry, newEntry, testCaseKey);
        return;
    }
    char* newBuffer{newEntry->allocateBuffer(testCaseKey, static_cast<int>(newBufferSize))};
    memset(newBuffer, 0u, newBufferSize);

    std::strcpy(newBuffer, modTreeStr.c_str());
}
