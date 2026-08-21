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
#include "RadamsaSwapLineMutator.hpp"
#include "RuntimeException.hpp"
#include <random>
#include <algorithm>

using namespace vmf;

#include "ModuleFactory.hpp"
REGISTER_MODULE(RadamsaSwapLineMutator);

Module* RadamsaSwapLineMutator::build(std::string name)
{
    return new RadamsaSwapLineMutator(name);
}

void RadamsaSwapLineMutator::init(ConfigInterface& config)
{
    rand = VmfRand::getInstance();
}

RadamsaSwapLineMutator::RadamsaSwapLineMutator(std::string name) : MutatorModule(name)
{
}

RadamsaSwapLineMutator::~RadamsaSwapLineMutator()
{
}





void RadamsaSwapLineMutator::registerStorageNeeds(StorageRegistry& registry)
{
    (void)registry;
}

void RadamsaSwapLineMutator::mutateTestCase(StorageModule& storage, StorageEntry* baseEntry, StorageEntry* newEntry, int testCaseKey)
{
    (void)storage;

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
    if (numLines < 2u)
    {
        CopyBufferAsIs(baseEntry, newEntry, testCaseKey);
        return;
    }

    std::vector<size_t> lineOrder(numLines);
    for (size_t i{0u}; i < numLines; ++i)
    {
        lineOrder[i] = i;
    }

    size_t firstRandomLineIndex{0u};
    
        firstRandomLineIndex = static_cast<size_t>(rand->randBetween(0ul, static_cast<unsigned long>(numLines - 2u)));
    

    const size_t secondRandomLineIndex{firstRandomLineIndex + 1u};
    std::swap(lineOrder[firstRandomLineIndex], lineOrder[secondRandomLineIndex]);

    const size_t newBufferSize{GetAllLineDataSize(lines)};
    if (newBufferSize > static_cast<size_t>(INT_MAX))
    {
        CopyBufferAsIs(baseEntry, newEntry, testCaseKey);
        return;
    }

    char* newBuffer{newEntry->allocateBuffer(testCaseKey, static_cast<int>(newBufferSize))};
    memset(newBuffer, 0u, newBufferSize);
    CopyAllLineDataToBuffer(originalBuffer, lines, lineOrder, newBuffer);
}
