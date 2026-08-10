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
#pragma once

#include "MutatorModule.hpp"
#include "StorageEntry.hpp"
#include "RuntimeException.hpp"
#include "RadamsaTreeMutatorBase.hpp"
#include "VmfRand.hpp"

namespace vmf
{
/**
 * @brief This mutator selects a random node in a given tree and replaces the nodes random child with a random amount of recursive copies of itself.
 */
class RadamsaRepeatPathMutator: public MutatorModule, public RadamsaTreeMutatorBase
{
    public:

        static Module* build(std::string name);
        virtual void init(ConfigInterface& config);

        RadamsaRepeatPathMutator(std::string name);
        virtual ~RadamsaRepeatPathMutator();
        virtual void registerStorageNeeds(StorageRegistry& registry);
        virtual void mutateTestCase(StorageModule& storage, StorageEntry* baseEntry, StorageEntry* newEntry, int testCaseKey);

    private:
        VmfRand* rand;

        // Adaptive cap on the total tree-node count that a single `repeatPath` call may produce. Default 1048576 (~100 MB live-tree footprint at typical node sizes). Set the `maxRepeatPathNodes` config key to override; 0 disables the adaptive cap and restores the full uncapped distribution at the cost of unbounded per-call growth.
        size_t m_maxRepeatPathNodes{1048576u};

        // Optional per-call repetition cap retained for backward compatibility. Default 0 (unlimited). Set the `maxPathRepetitions` config key to override. When both this cap and `maxRepeatPathNodes` are configured they compose: the drawn repetition count is first clamped against this fixed cap, then the adaptive node-budget cap is applied inside `repeatPath`.
        size_t m_maxPathRepetitions{0u};

        // Output-size guard used before serializing the repeated tree back into text.
        size_t m_maxRepeatPathOutputBytes{1048576u};
};
}
