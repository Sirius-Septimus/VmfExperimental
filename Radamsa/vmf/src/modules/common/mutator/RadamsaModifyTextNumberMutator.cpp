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
#include "RadamsaModifyTextNumberMutator.hpp"
#include "RuntimeException.hpp"
#include <random>
#include <algorithm>

using namespace vmf;
using std::vector;

#include "ModuleFactory.hpp"
REGISTER_MODULE(RadamsaModifyTextNumberMutator);

/**
 * @brief Builder method to support the ModuleFactory
 * Constructs an instance of this class
 * @return Module* - Pointer to the newly created instance
 */
Module* RadamsaModifyTextNumberMutator::build(std::string name)
{
    return new RadamsaModifyTextNumberMutator(name);
}

/**
 * @brief Initialization method
 *
 * @param config - Configuration object
 */
void RadamsaModifyTextNumberMutator::init(ConfigInterface& config)
{
    rand = VmfRand::getInstance();
}

/**
 * @brief Construct a new RadamsaModifyTextNumberMutator::RadamsaModifyTextNumberMutator object
 *
 * @param name The of the name module
 */
RadamsaModifyTextNumberMutator::RadamsaModifyTextNumberMutator(std::string name) : MutatorModule(name)
{
    
}

/**
 * @brief Destroy the RadamsaModifyTextNumberMutator::RadamsaModifyTextNumberMutator object
 *
 */
RadamsaModifyTextNumberMutator::~RadamsaModifyTextNumberMutator()
{

}





/**
 * @brief Register the storage needs for this module
 *
 * @param registry - StorageRegistry object
 */
void RadamsaModifyTextNumberMutator::registerStorageNeeds(StorageRegistry& registry)
{
    // This module does not register for a test case buffer key, because mutators are told which buffer to write in storage
    // by the input generator that calls them
}

void RadamsaModifyTextNumberMutator::mutateTestCase(StorageModule& storage, StorageEntry* baseEntry, StorageEntry* newEntry, int testCaseKey)
{
    // Mutate a random ASCII number via a randomly selected numerical mutation

    const size_t minimumSize{1u};
    const size_t minimumNumbers{1u};
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

    vector<uint8_t> data(originalBuffer, originalBuffer + originalSize);
    vector<NumInfo> dataNums = this->extractTextualNumbers(data);

    // Check if buffer contains at least one ASCII number
    if (dataNums.size() < minimumNumbers)
    {
        CopyBufferAsIs(baseEntry, newEntry, testCaseKey);
        return;
    }

    NumInfo toMutate;
    size_t mutationCase{0u};
    
        toMutate = dataNums[this->rand->randBetween(0, int(dataNums.size() - 1))];
        mutationCase = static_cast<size_t>(this->rand->randBetween(0, 11));
    

    const vector<NumberValue> interestingNums = this->generateInterestingNumbers();
    auto selectInterestingNumber = [&]() -> NumberValue
    {
        
        return interestingNums[
            this->rand->randBetween(0, int(interestingNums.size() - 1))
        ];
    };

    auto selectSignedRandomValue = [&](const NumberValue& limit) -> NumberValue
    {
        

        // Narrow the bound to i128 before sampling,
        // then sample uniformly from [-abs(bound), abs(bound)).
        const NumberValue twoTo128 = NumberValue(1) << 128u;
        const NumberValue twoTo127 = NumberValue(1) << 127u;
        NumberValue narrowed = limit % twoTo128;
        if (narrowed >= twoTo127)
        {
            narrowed -= twoTo128;
        }
        if (narrowed == -twoTo127)
        {
            return 0;
        }
        const NumberValue magnitude = narrowed < 0 ? -narrowed : narrowed;
        if (magnitude == 0)
        {
            return 0;
        }

        const NumberValue span = magnitude * 2u;
        const size_t bitCount = mpz_sizeinbase(span.get_mpz_t(), 2);
        NumberValue candidate;
        do
        {
            candidate = 0;
            for (size_t bit = 0; bit < bitCount; bit += 64u)
            {
                const uint64_t low = static_cast<uint64_t>(
                    this->rand->randBetween(0ul, 0xfffffffful));
                const uint64_t high = static_cast<uint64_t>(
                    this->rand->randBetween(0ul, 0xfffffffful));
                const uint64_t word = low | (high << 32u);
                candidate += NumberValue(word) << bit;
            }
            candidate %= NumberValue(1) << bitCount;
        } while (candidate >= span);
        return candidate - magnitude;
    };

    NumberValue newValue;
    switch (mutationCase) {
        case 0:
            newValue = toMutate.value + static_cast<NumberValue>(1); break;
        case 1:
            newValue = toMutate.value - static_cast<NumberValue>(1); break;
        case 2:
            newValue = static_cast<NumberValue>(0); break;
        case 3:
            newValue = static_cast<NumberValue>(1); break;
        case 4:
        case 5:
        case 6:
            newValue = selectInterestingNumber(); break;
        case 7: {
            const NumberValue sampled = selectSignedRandomValue(selectInterestingNumber());
            newValue = sampled + toMutate.value;
            break;
        }
        case 8: {
            const NumberValue sampled = selectSignedRandomValue(selectInterestingNumber());
            newValue = sampled - toMutate.value;
            break;
        }
        case 9: {
            const NumberValue sampled = selectSignedRandomValue(toMutate.value * 2u);
            newValue = sampled - toMutate.value;
            break;
        }
        default: {
            NumberValue n;
            unsigned int signChoice;
            
                n = static_cast<NumberValue>(this->rand->randBetween(1, 128));
                signChoice = this->rand->randBetween(0, 2);
            
            if (signChoice == 0u)
            {
                newValue = toMutate.value - n;
            }
            else
            {
                newValue = toMutate.value + n;
            }
            break;
        }
    }

    // Match i256 overflowing arithmetic before formatting the result.
    const NumberValue i256Modulus = NumberValue(1) << 256u;
    const NumberValue i256SignBit = NumberValue(1) << 255u;
    newValue %= i256Modulus;
    if (newValue < 0) newValue += i256Modulus;
    if (newValue >= i256SignBit) newValue -= i256Modulus;

    std::string newValueStr = numberValueToString(newValue);
    vector<uint8_t> new_data;
    new_data.reserve(data.size() + newValueStr.size());
    new_data.insert(    // everything before the original value
        new_data.end(), 
        data.begin(), 
        data.begin() + toMutate.offset
    );
    new_data.insert(    // the new value
        new_data.end(),
        newValueStr.begin(),
        newValueStr.end()
    );
    new_data.insert(    // everything after the original value
        new_data.end(),
        data.begin() + toMutate.offset + toMutate.length,
        data.end()
    );

    const size_t newBufferSize{new_data.size()};
    if (newBufferSize > INT_MAX) {
        //Check to see if the newBufferSize excedes the maximum size.
        CopyBufferAsIs(baseEntry, newEntry, testCaseKey);
        return;
    }
    char* newBuffer{newEntry->allocateBuffer(testCaseKey, static_cast<int>(newBufferSize))};
    
    memset(newBuffer, 0u, newBufferSize);
    memcpy(newBuffer, new_data.data(), new_data.size());
}
