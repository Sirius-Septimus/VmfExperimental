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
#include "RadamsaInsertUnicodeMutator.hpp"
#include "RuntimeException.hpp"
#include <random>
#include <algorithm>

using namespace vmf;

#include "ModuleFactory.hpp"
REGISTER_MODULE(RadamsaInsertUnicodeMutator);

/**
 * @brief Builder method to support the ModuleFactory
 * Constructs an instance of this class
 * @return Module* - Pointer to the newly created instance
 */
Module* RadamsaInsertUnicodeMutator::build(std::string name)
{
    return new RadamsaInsertUnicodeMutator(name);
}

/**
 * @brief Initialization method
 *
 * @param config - Configuration object
 */
void RadamsaInsertUnicodeMutator::init(ConfigInterface& config)
{

}

/**
 * @brief Construct a new RadamsaInsertUnicodeMutator::RadamsaInsertUnicodeMutator object
 *
 * @param name The of the name module
 */
RadamsaInsertUnicodeMutator::RadamsaInsertUnicodeMutator(std::string name) : MutatorModule(name)
{
    // rand->randInit();

    // these values copied from github.com/microsoft/rusty-radamsa/blob/main/src/mutations.rs#L677
    this->funnyUnicode.push_back(this->encodeUtf8(U'\u202E'));     // Right to Left Override
    this->funnyUnicode.push_back(this->encodeUtf8(U'\u202D'));     // Left to Right Override
    this->funnyUnicode.push_back(this->encodeUtf8(U'\u180E'));     // Mongolian Vowel Separator
    this->funnyUnicode.push_back(this->encodeUtf8(U'\u2060'));     // Word Joiner
    this->funnyUnicode.push_back(this->encodeUtf8(U'\uFEFE'));     // reserved
    this->funnyUnicode.push_back(this->encodeUtf8(U'\uFFFF'));     // not a character
    this->funnyUnicode.push_back(this->encodeUtf8(U'\u0FED'));     // unassigned
    this->funnyUnicode.push_back({0xed, 0xba, 0xad});               // U+DEAD illegal low surrogate
    this->funnyUnicode.push_back({0xed, 0xaa, 0xad});               // U+DAAD illegal high surrogate
    this->funnyUnicode.push_back(this->encodeUtf8(U'\uF8FF'));     // private use char (Apple)
    this->funnyUnicode.push_back(this->encodeUtf8(U'\uFF0F'));     // full width solidus
    this->funnyUnicode.push_back(this->encodeUtf8(U'\U0001D7D6')); // MATHEMATICAL BOLD DIGIT EIGHT
    this->funnyUnicode.push_back(this->encodeUtf8(U'\u00DF'));     // IDNA deviant
    this->funnyUnicode.push_back(this->encodeUtf8(U'\uFDFD'));     // expands by 11x (UTF-8) and 18x (UTF-16) NFKC
    this->funnyUnicode.push_back(this->encodeUtf8(U'\u0390'));     // expands by 3x (UTF-8) NFD
    this->funnyUnicode.push_back(this->encodeUtf8(U'\u1F82'));     // expands by 4x (UTF-16) NFD
    this->funnyUnicode.push_back(this->encodeUtf8(U'\uFB2C'));     // expands by 3x (UTF-16) under NFC
    this->funnyUnicode.push_back(this->encodeUtf8(U'\U0001D160')); // expands by 3x (UTF-8) under NFC
    this->funnyUnicode.push_back({0xf4, 0x8f, 0xbf, 0xbe});         // illegal outside end of max range U+10FFFF
    this->funnyUnicode.push_back({239, 191, 191});                  // 65535
    this->funnyUnicode.push_back({240, 144, 128, 128});             // 65536
    this->funnyUnicode.push_back({0xef, 0xbb, 0xbf});               // the canonical utf8 bom
    this->funnyUnicode.push_back({0xfe, 0xff});                     // utf16 be bom
    this->funnyUnicode.push_back({0xff, 0xfe});                     // utf16 le bom
    this->funnyUnicode.push_back({0, 0, 0xff, 0xff});               // ascii null be
    this->funnyUnicode.push_back({0xff, 0xff, 0, 0});               // ascii null le
    this->funnyUnicode.push_back({43, 47, 118, 56});                // and some others from wikipedia
    this->funnyUnicode.push_back({43, 47, 118, 57});
    this->funnyUnicode.push_back({43, 47, 118, 43});
    this->funnyUnicode.push_back({43, 47, 118, 47});
    this->funnyUnicode.push_back({247, 100, 76});
    this->funnyUnicode.push_back({221, 115, 102, 115});
    this->funnyUnicode.push_back({14, 254, 255});
    this->funnyUnicode.push_back({251, 238, 40});
    this->funnyUnicode.push_back({251, 238, 40, 255});
    this->funnyUnicode.push_back({132, 49, 149, 51});
}

/**
 * @brief Destroy the RadamsaInsertUnicodeMutator::RadamsaInsertUnicodeMutator object
 *
 */
RadamsaInsertUnicodeMutator::~RadamsaInsertUnicodeMutator()
{

}

/**
 * @brief Register the storage needs for this module
 *
 * @param registry - StorageRegistry object
 */
void RadamsaInsertUnicodeMutator::registerStorageNeeds(StorageRegistry& registry)
{
    // This module does not register for a test case buffer key, because mutators are told which buffer to write in storage
    // by the input generator that calls them
}

void RadamsaInsertUnicodeMutator::mutateTestCase(StorageModule& storage, StorageEntry* baseEntry, StorageEntry* newEntry, int testCaseKey)
{
    // Insert a "funny" unicode sequence into a random index

    /*
     *	minimumSize is 1: a zero-byte input would underflow `data.size() - 1` to SIZE_MAX and produce undefined behaviour at the subsequent `data.insert(data.begin() + insert_index, ...)` call. Inputs below the floor return via CopyBufferAsIs.
     */

    // Minimum input size accepted by this mutator (in bytes).
    const size_t minimumSize{1u};
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
    if (originalSize > 0 && minimumSeedIndex > originalSize - 1u)
    {
        CopyBufferAsIs(baseEntry, newEntry, testCaseKey);
        return;
    }

    std::vector<uint8_t> data(originalBuffer, originalBuffer + originalSize);

    const unsigned long lower{0ul};
    unsigned long upper{static_cast<unsigned long>(data.size() - 1)};
    const size_t insert_index = static_cast<size_t>(this->rand->randBetween(lower, upper));

    upper = static_cast<unsigned long>(this->funnyUnicode.size() - 1);
    const std::vector<uint8_t> toInsert = this->funnyUnicode[static_cast<size_t>(this->rand->randBetween(lower, upper))];

    data.insert(
        data.begin() + insert_index,
        toInsert.rbegin(),
        toInsert.rend()
    );
    
    const size_t newBufferSize{data.size() + 1}; // +1 to implicitly append a null terminator
    char* newBuffer{newEntry->allocateBuffer(testCaseKey, newBufferSize)};
    memset(newBuffer, 0u, newBufferSize);
    memcpy(newBuffer, data.data(), data.size());

    return;
}
