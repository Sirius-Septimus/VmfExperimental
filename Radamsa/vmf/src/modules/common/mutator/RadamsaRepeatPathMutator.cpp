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
#include "RadamsaRepeatPathMutator.hpp"
#include "RadamsaDelimiterTree.hpp"
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
     *	Two independent caps govern repeatPath growth: - maxPathRepetitions: a fixed cap on the iteration count. 0 disables this cap. - maxRepeatPathNodes: an adaptive cap on the resulting live-tree node count, applied by RadamsaDelimiterTree before constructing repeated subtrees. 0 disables this cap. The default enables the adaptive node cap with no fixed iteration cap.
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

    const int outputConfigured = config.getIntParam(getModuleName(), "maxRepeatPathOutputBytes",
                                                    static_cast<int>(m_maxRepeatPathOutputBytes));
    m_maxRepeatPathOutputBytes = (outputConfigured == 0)
        ? std::numeric_limits<size_t>::max()
        : static_cast<size_t>(outputConfigured);

    rand = VmfRand::getInstance();
}

/**
 * @brief Construct a new RadamsaRepeatPathMutator::RadamsaRepeatPathMutator object
 *
 * @param name The of the name module
 */
RadamsaRepeatPathMutator::RadamsaRepeatPathMutator(std::string name) : MutatorModule(name)
{
    
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

    const size_t minimumSize{4u};   // minimal case consists of two single-character nodes
    size_t originalSize;
    char* originalBuffer;

    // Try to get buffer size and pointer, return early if buffer is not allocated
    originalBuffer = baseEntry->getBufferPointer(testCaseKey);
    originalSize = baseEntry->getBufferSize(testCaseKey);

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
    auto maybeTree = RadamsaDelimiterTree::tryBuild(treeStr);
    if (!maybeTree)
    {
        CopyBufferAsIs(baseEntry, newEntry, testCaseKey);
        return;
    }
    RadamsaDelimiterTree& tr = *maybeTree;

    std::vector<RadamsaDelimiterTree::Node*> candidates;
    tr.collectCandidates(candidates);
    if (candidates.empty())
    {
        CopyBufferAsIs(baseEntry, newEntry, testCaseKey);
        return;
    }

    size_t nodeIndex{0u};
    size_t numReps{0u};
    
        const unsigned long lower{0ul};
        const unsigned long upper{static_cast<unsigned long>(candidates.size() - 1u)};
        nodeIndex = static_cast<size_t>(this->rand->randBetween(lower, upper));
        numReps = this->GetRustRandLog10(this->rand);
        if (m_maxPathRepetitions != 0u &&
            m_maxPathRepetitions != std::numeric_limits<size_t>::max())
        {
            numReps = std::min(numReps, m_maxPathRepetitions);
        }
    

    // path already applies the configured limit above.
    

    tr.repeatSelectedPath(
        candidates[nodeIndex],
        numReps,
        m_maxRepeatPathNodes,
        m_maxRepeatPathOutputBytes);

    const size_t estimatedOutputSize{tr.serializedSize()};
    if (estimatedOutputSize > m_maxRepeatPathOutputBytes ||
        estimatedOutputSize > static_cast<size_t>(INT_MAX))
    {
        CopyBufferAsIs(baseEntry, newEntry, testCaseKey);
        return;
    }

    const string modTreeStr = tr.toString();
    const size_t newBufferSize{modTreeStr.length()};
    if (newBufferSize > INT_MAX) {
        //Check to see if the newBufferSize excedes the maximum size.
        CopyBufferAsIs(baseEntry, newEntry, testCaseKey);
        return;
    }
    char* newBuffer{newEntry->allocateBuffer(testCaseKey, static_cast<int>(newBufferSize))};
    memset(newBuffer, 0u, newBufferSize);

    memcpy(newBuffer, modTreeStr.data(), newBufferSize);
    return;
}
