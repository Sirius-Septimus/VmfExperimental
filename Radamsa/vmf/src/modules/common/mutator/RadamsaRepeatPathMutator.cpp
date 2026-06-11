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
#include "RadamsaRepeatPathMutator.hpp"
#include "RuntimeException.hpp"
#include <random>
#include <algorithm>
#include <limits>

using namespace vmf;

#include "ModuleFactory.hpp"
REGISTER_MODULE(RadamsaRepeatPathMutator);


/**
 * @brief Builder method to support the ModuleFactory
 * Constructs an instance of this class
 * @return Module* - Pointer to the newly created instance
 */
Module* RadamsaRepeatPathMutator::build(std::string name)
{
    return new RadamsaRepeatPathMutator(name);
}

/**
 * @brief Initialization method
 *
 * @param config - Configuration object
 */
void RadamsaRepeatPathMutator::init(ConfigInterface& config)
{
    /*
     *	Two independent caps govern repeatPath growth: - maxPathRepetitions: a fixed cap on the iteration count. 0 disables this cap. - maxRepeatPathNodes: an adaptive cap on the resulting live-tree node count, applied inside Tree::repeatPath via its maxTotalNodes parameter. 0 disables this cap. The default enables the adaptive node cap with no fixed iteration cap.
     */

    const int repsConfigured = config.getIntParam(getModuleName(), "maxPathRepetitions",
                                                  static_cast<int>(m_maxPathRepetitions));
    m_maxPathRepetitions = (repsConfigured == 0)
        ? std::numeric_limits<size_t>::max()
        : static_cast<size_t>(repsConfigured);

    const int nodesConfigured = config.getIntParam(getModuleName(), "maxRepeatPathNodes",
                                                   static_cast<int>(m_maxRepeatPathNodes));
    m_maxRepeatPathNodes = (nodesConfigured == 0)
        ? std::numeric_limits<size_t>::max()
        : static_cast<size_t>(nodesConfigured);
}

/**
 * @brief Construct a new RadamsaRepeatPathMutator::RadamsaRepeatPathMutator object
 *
 * @param name The of the name module
 */
RadamsaRepeatPathMutator::RadamsaRepeatPathMutator(std::string name) : MutatorModule(name)
{
    // rand->randInit();
}

/**
 * @brief Destroy the RadamsaRepeatPathMutator::RadamsaRepeatPathMutator object
 *
 */
RadamsaRepeatPathMutator::~RadamsaRepeatPathMutator()
{

}

/**
 * @brief Register the storage needs for this module
 *
 * @param registry - StorageRegistry object
 */
void RadamsaRepeatPathMutator::registerStorageNeeds(StorageRegistry& registry)
{
    // This module does not register for a test case buffer key, because mutators are told which buffer to write in storage
    // by the input generator that calls them
}

void RadamsaRepeatPathMutator::mutateTestCase(StorageModule& storage, StorageEntry* baseEntry, StorageEntry* newEntry, int testCaseKey)
{
    // Replace a random node's random child with a random amount of recursive copies of itself

    const size_t minimumSize{4u};   // minimal case consists of two single-character nodes
    const size_t minimumSeedIndex{0u};
    const size_t minimumNodes{2u};
    size_t originalSize;
    char* originalBuffer;

    // Try to get buffer size and pointer, return early if buffer is not allocated
    try
    {
        originalBuffer = baseEntry->getBufferPointer(testCaseKey);
        originalSize = baseEntry->getBufferSize(testCaseKey);
    }
    catch (const RuntimeException& e)
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
    // Check if tree has minimum required number of nodes
    if (numNodes < minimumNodes)
    {
        CopyBufferAsIs(baseEntry, newEntry, testCaseKey);
        return;
    }

    const unsigned long lower{0ul};
    unsigned long upper{static_cast<unsigned long>(numNodes - 1)};
    size_t parentIndex;
    Node* parent;
    do {
        parentIndex = static_cast<size_t>(this->rand->randBetween(lower, upper));
        parent =  tr.findNodeByIndex(tr.root, parentIndex);
    } while (parent->children.size() <= 0);   // find a parent that actually has children

    upper = static_cast<unsigned long>(parent->children.size() - 1);
    size_t childIndex{static_cast<size_t>(this->rand->randBetween(lower, upper))};
    /*
     * Repetition count comes from GetRandomRepetitionLength. Two composable caps then bound the work: the fixed `m_maxPathRepetitions` cap clamps the iteration count up front, and the adaptive `m_maxRepeatPathNodes` cap is forwarded to `repeatPath`, which converts it into an effective iteration count from the per-iteration node delta.
     */
    size_t numReps = this->GetRandomRepetitionLength(this->rand);
    if (m_maxPathRepetitions != std::numeric_limits<size_t>::max())
    {
        numReps = std::min(numReps, m_maxPathRepetitions);
    }

    tr.repeatPath(parent, childIndex, numReps, m_maxRepeatPathNodes);

    const string modTreeStr = tr.toString(tr.root);
    const size_t newBufferSize{modTreeStr.length() + 1}; // +1 to implicitly append a null terminator

    char* newBuffer{newEntry->allocateBuffer(testCaseKey, newBufferSize)};
    memset(newBuffer, 0u, newBufferSize);

    std::strcpy(newBuffer, modTreeStr.c_str());
    return;
}
