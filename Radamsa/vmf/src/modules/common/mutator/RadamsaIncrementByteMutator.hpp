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
#include "RadamsaByteMutatorBase.hpp"
#include "VmfRand.hpp"

namespace vmf
{
/**
 * @brief This mutator chooses a random byte in the buffer to increment by 1.
 */
class RadamsaIncrementByteMutator: public MutatorModule, public RadamsaByteMutatorBase
{
    public:

        static Module* build(std::string name);
        virtual void init(ConfigInterface& config);

        RadamsaIncrementByteMutator(std::string name);
        virtual ~RadamsaIncrementByteMutator();
        virtual void registerStorageNeeds(StorageRegistry& registry);
        virtual void mutateTestCase(StorageModule& storage, StorageEntry* baseEntry, StorageEntry* newEntry, int testCaseKey);

        // increments so VMF runs stay easy to compare under identical
        // decisions.

    private:
        VmfRand* rand;

        // When enabled, mutateTestCase() bypasses VmfRand and increments the
        // harness-provided byte index instead.
};
}
