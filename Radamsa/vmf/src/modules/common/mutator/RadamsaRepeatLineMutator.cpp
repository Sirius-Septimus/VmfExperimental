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
#include "RadamsaRepeatLineMutator.hpp"
#include "RuntimeException.hpp"
#include <random>
#include <algorithm>
#include <limits>

using namespace vmf;

#include "ModuleFactory.hpp"
REGISTER_MODULE(RadamsaRepeatLineMutator);

Module* RadamsaRepeatLineMutator::build(std::string name)
{
    return new RadamsaRepeatLineMutator(name);
}

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

    rand = VmfRand::getInstance();
}

RadamsaRepeatLineMutator::RadamsaRepeatLineMutator(std::string name) : MutatorModule(name)
{
}

RadamsaRepeatLineMutator::~RadamsaRepeatLineMutator()
{
}





void RadamsaRepeatLineMutator::registerStorageNeeds(StorageRegistry& registry)
{
    (void)registry;
}

void RadamsaRepeatLineMutator::mutateTestCase(StorageModule& storage, StorageEntry* baseEntry, StorageEntry* newEntry, int testCaseKey)
{
    (void)storage;

    // Consume the original buffer by repeating a random line multiple times and appending a null-terminator to the end.

    constexpr size_t minimumSize{1u};
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

    std::vector<Line> lines;
    if (!TryGetAllLineData(originalBuffer, originalSize, lines))
    {
        CopyBufferAsIs(baseEntry, newEntry, testCaseKey);
        return;
    }
    const size_t numLines{lines.size()};
    if (numLines == 0u)
    {
        CopyBufferAsIs(baseEntry, newEntry, testCaseKey);
        return;
    }

    size_t randomLineIndex{0u};
    size_t numberOfRandomLineRepetitions{0u};
    
        randomLineIndex = static_cast<size_t>(rand->randBetween(0ul, static_cast<unsigned long>(numLines - 1u)));
        numberOfRandomLineRepetitions = std::max<size_t>(
            2u,
            GetRustRandLog10(this->rand)
        );
    
    const Line lineData{lines.at(randomLineIndex)};

    if (lineData.Size > 0u)
    {
        const size_t maxRepetitions{m_maxBufferGrowthBytes / lineData.Size};
        if (numberOfRandomLineRepetitions > maxRepetitions)
        {
            numberOfRandomLineRepetitions = (maxRepetitions > 0u) ? maxRepetitions : 1u;
        }
    }

    std::vector<size_t> lineOrder;
    lineOrder.reserve(numLines + numberOfRandomLineRepetitions);
    for (size_t i{0u}; i < numLines; ++i)
    {
        lineOrder.push_back(i);
    }
    for (size_t i{0u}; i < numberOfRandomLineRepetitions; ++i)
    {
        lineOrder.insert(lineOrder.begin() + static_cast<std::vector<size_t>::difference_type>(randomLineIndex + 1u), randomLineIndex);
    }

    const size_t newBufferSize{GetAllLineDataSize(lines, lineOrder)};
    if (newBufferSize > static_cast<size_t>(INT_MAX))
    {
        CopyBufferAsIs(baseEntry, newEntry, testCaseKey);
        return;
    }

    char* newBuffer{newEntry->allocateBuffer(testCaseKey, static_cast<int>(newBufferSize))};
    memset(newBuffer, 0u, newBufferSize);
    CopyAllLineDataToBuffer(originalBuffer, lines, lineOrder, newBuffer);
}
