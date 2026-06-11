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
/*****
 * The following includes code copied from the LibAFL_Legacy repository.
 * 
 *       american fuzzy lop++ - fuzzer header
 *  ------------------------------------
 *  Originally written by Michal Zalewski
 *  Now maintained by Marc Heuse <mh@mh-sec.de>,
 *                    Heiko Eißfeldt <heiko.eissfeldt@hexco.de>,
 *                    Andrea Fioraldi <andreafioraldi@gmail.com>,
 *                    Dominik Maier <mail@dmnk.co>
 *  Copyright 2016, 2017 Google Inc. All rights reserved.
 *  Copyright 2019-2020 AFLplusplus Project. All rights reserved.
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *    http://www.apache.org/licenses/LICENSE-2.0
 *  This is the Library based on AFL++ which can be used to build
 *  customized fuzzers for a specific target while taking advantage of
 *  a lot of features that AFL++ already provides.
 */

#include "AFLOverwriteFixedMutator.hpp"
/*When added to main repo this can be added back*/
// #include "AFLDeleteMutator.hpp" //For static choose_block_len method
/* And this can be deleted.*/
#include "AFLMutatorCommon.hpp"
#include "RuntimeException.hpp"
#include <random>
#include <algorithm>

using namespace vmf;
using u32 = uint32_t;

#include "ModuleFactory.hpp"
REGISTER_MODULE(AFLOverwriteFixedMutator);

/**
 * @brief Builder method to support the ModuleFactory
 * Constructs an instance of this class
 * @return Module* 
 */
Module* AFLOverwriteFixedMutator::build(std::string name)
{
    return new AFLOverwriteFixedMutator(name);
}

/**
 * @brief Initialization method
 * 
 * @param config 
 */
void AFLOverwriteFixedMutator::init(ConfigInterface& config)
{

}

/**
 * @brief Construct a new AFLOverwriteFixedMutator::AFLOverwriteFixedMutator object
 * 
 * @param name the name of the module
 */
AFLOverwriteFixedMutator::AFLOverwriteFixedMutator(std::string name) :
    MutatorModule(name)
{
    // rand->randInit();
}

/**
 * @brief Destroy the AFLOverwriteFixedMutator::AFLOverwriteFixedMutator object
 * 
 */
AFLOverwriteFixedMutator::~AFLOverwriteFixedMutator()
{

}

/**
 * @brief Registers storage needs
 * This class uses only the "TEST_CASE" key
 * 
 * @param registry 
 */
void AFLOverwriteFixedMutator::registerStorageNeeds(StorageRegistry& registry)
{
    testCaseKey = registry.registerKey("TEST_CASE", StorageRegistry::BUFFER, StorageRegistry::READ_WRITE);
}
 
void AFLOverwriteFixedMutator::mutateTestCase(StorageModule& storage, StorageEntry* baseEntry, StorageEntry* newEntry, int testCaseKey)
{

    int size = baseEntry->getBufferSize(testCaseKey);
    char* buffer = baseEntry->getBufferPointer(testCaseKey);

    if(size <= 0)
    {
        throw RuntimeException("AFLOverwriteFixedMutator mutate called with zero sized buffer", RuntimeException::USAGE_ERROR);
    }
    char* newBuff = newEntry->allocateBuffer(testCaseKey, size);
    // Copy base entry to new entry
    memcpy((void*)newBuff, (void*)buffer, size);

    if (size < 2) {
        return;
    }

    // Choose a random block length
    // u32 copy_len = AFLDeleteMutator::choose_block_len(*rand, size - 1);
    //TODO: Make sure to add this ^ back in after merge to main VMF
    u32 copy_len = vmf::choose_block_len(*rand, size - 1);
    // Choose a random location to copy the block to
    u32 copy_to = rand->randBelow((unsigned long)(size - copy_len + 1));
    // Choose a random strategy with 50% chance of each
    u32 strat = rand->randBelow(2);
    // Copy from the location before the copy_to location or 0 if copy_to is 0
    u32 copy_from = copy_to ? copy_to - 1 : 0;
    // Either repeat the byte at the copy_from location or choose a random byte
    u32 item = strat ? rand->randBelow(256) : buffer[copy_from];

    memset(newBuff + copy_to, item, copy_len);

    return;
}