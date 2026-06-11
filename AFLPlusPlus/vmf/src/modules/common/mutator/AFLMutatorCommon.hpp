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
#pragma once
#include <cstddef>
#include <algorithm>
#include "MutatorModule.hpp"
#include "StorageEntry.hpp"
#include "RuntimeException.hpp"
#include "VmfRand.hpp"

//TODO: C style coding here. Need to make this C++ worthy.
#define INTERESTING_8                                     \
    -128,    /* Overflow signed 8-bit when decremented  */ \
    -1,      /*                                         */ \
    0,       /*                                         */ \
    1,       /*                                         */ \
    16,      /* One-off with common buffer size         */ \
    32,      /* One-off with common buffer size         */ \
    64,      /* One-off with common buffer size         */ \
    100,     /* One-off with common buffer size         */ \
    127      /* Overflow signed 8-bit when incremented  */
 
#define INTERESTING_8_LEN 9
 
#define INTERESTING_16                                    \
   -32768,   /* Overflow signed 16-bit when decremented */ \
    -129,    /* Overflow signed 8-bit                   */ \
    128,     /* Overflow signed 8-bit                   */ \
    255,     /* Overflow unsig 8-bit when incremented   */ \
    256,     /* Overflow unsig 8-bit                    */ \
    512,     /* One-off with common buffer size         */ \
    1000,    /* One-off with common buffer size         */ \
    1024,    /* One-off with common buffer size         */ \
    4096,    /* One-off with common buffer size         */ \
    32767    /* Overflow signed 16-bit when incremented */
 
#define INTERESTING_16_LEN 10
 
#define INTERESTING_32                                           \
   -2147483648LL,   /* Overflow signed 32-bit when decremented */ \
    -100663046,     /* Large negative number (endian-agnostic) */ \
    -32769,         /* Overflow signed 16-bit                  */ \
    32768,          /* Overflow signed 16-bit                  */ \
    65535,          /* Overflow unsig 16-bit when incremented  */ \
    65536,          /* Overflow unsig 16 bit                   */ \
    100663045,      /* Large positive number (endian-agnostic) */ \
    2139095040,     /* float infinite                          */ \
    2147483647      /* Overflow signed 32-bit when incremented */
#define INTERESTING_32_LEN 9 

//TODO: Hack for core module dependancies. When merged with Vader Core. Please remove.
namespace vmf{

    constexpr int BLK_SMALL = 32;
    constexpr int BLK_MEDIUM = 128;
    constexpr int BLK_LARGE = 1500;
    constexpr int BLK_XL = 32768;

    inline int choose_block_len(vmf::VmfRand& rand, std::size_t limit) {
        
        int min_value, max_value;
        switch (rand.randBelow(3)) {

        case 0:
            min_value = 1;
            max_value = BLK_SMALL;
            break;
        case 1:
            min_value = BLK_SMALL;
            max_value = BLK_MEDIUM;
            break;
        default:
            if (rand.randBelow(10)) {
                min_value = BLK_MEDIUM;
                max_value = BLK_LARGE;
            } else {
                min_value = BLK_LARGE;
                max_value = BLK_XL;
            }
        }

        if (min_value >= static_cast<int>(limit)) { min_value = 1; }

        return rand.randBetween(min_value, std::min(max_value, static_cast<int>(limit)));

        }
}