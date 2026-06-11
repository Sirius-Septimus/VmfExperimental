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
#include "RadamsaDeleteNodeMutator.hpp"
#include "RuntimeException.hpp"
#include <random>
#include <algorithm>

using namespace vmf;

#include "ModuleFactory.hpp"
REGISTER_MODULE(RadamsaDeleteNodeMutator);

/**
 * @brief Builder method to support the ModuleFactory
 * Constructs an instance of this class
 * @return Module* - Pointer to the newly created instance
 */
Module* RadamsaDeleteNodeMutator::build(std::string name)
{
    return new RadamsaDeleteNodeMutator(name);
}

/**
 * @brief Initialization method
 *
 * @param config - Configuration object
 */
void RadamsaDeleteNodeMutator::init(ConfigInterface& config)
{

}

/**
 * @brief Construct a new RadamsaDeleteNodeMutator::RadamsaDeleteNodeMutator object
 *
 * @param name The of the name module
 */
RadamsaDeleteNodeMutator::RadamsaDeleteNodeMutator(std::string name) : MutatorModule(name)
{
    // rand->randInit();
}

/**
 * @brief Destroy the RadamsaDeleteNodeMutator::RadamsaDeleteNodeMutator object
 *
 */
RadamsaDeleteNodeMutator::~RadamsaDeleteNodeMutator()
{

}

/**
 * @brief Register the storage needs for this module
 *
 * @param registry - StorageRegistry object
 */
void RadamsaDeleteNodeMutator::registerStorageNeeds(StorageRegistry& registry)
{
    // This module does not register for a test case buffer key, because mutators are told which buffer to write in storage
    // by the input generator that calls them
}

void RadamsaDeleteNodeMutator::mutateTestCase(StorageModule& storage, StorageEntry* baseEntry, StorageEntry* newEntry, int testCaseKey)
{
    // Delete a random node from the tree without preserving its children

    const size_t minimumSize{1};   // minimal case consists of a single-character root node
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

    const unsigned long lower{0ul};
    const unsigned long upper{static_cast<unsigned long>(numNodes - 1)};
    size_t nodeIndexToDelete{static_cast<size_t>(this->rand->randBetween(lower, upper))};    // not const, because findNodeByIndex will modify it

    Node* nodeToDelete = tr.findNodeByIndex(tr.root, nodeIndexToDelete);
    tr.deleteNode(nodeToDelete);

    string modTreeStr = tr.toString(tr.root);

    const size_t newBufferSize{modTreeStr.length() + 1}; // +1 to implicitly append a null terminator

    char* newBuffer{newEntry->allocateBuffer(testCaseKey, newBufferSize)};
    memset(newBuffer, 0u, newBufferSize);

    std::strcpy(newBuffer, modTreeStr.c_str());
}
