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
#include "RadamsaLineMutatorBase.hpp"
#include "VmfRand.hpp"

namespace vmf
{
/**
 * @brief This mutator selects a random line to repeat a random number of times to a set limit.
 */
class RadamsaRepeatLineMutator: public MutatorModule, public RadamsaLineMutatorBase
{
    public:

        static Module* build(std::string name);
        virtual void init(ConfigInterface& config);

        RadamsaRepeatLineMutator(std::string name);
        virtual ~RadamsaRepeatLineMutator();
        virtual void registerStorageNeeds(StorageRegistry& registry);
        virtual void mutateTestCase(StorageModule& storage, StorageEntry* baseEntry, StorageEntry* newEntry, int testCaseKey);



    private:
        VmfRand* rand;

        // Per-call upper bound on `lineData.Size * numberOfRandomLineRepetitions` bytes added to the output buffer. Default 16777216 (16 MiB). Set the `maxBufferGrowthBytes` config key to override; 0 disables the cap.
        size_t m_maxBufferGrowthBytes{16777216u};
};
}
