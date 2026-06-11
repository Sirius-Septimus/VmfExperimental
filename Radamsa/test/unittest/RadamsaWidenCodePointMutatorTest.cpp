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

#include "gtest/gtest.h"
#include "ModuleTestHelper.hpp"
#include "SimpleStorage.hpp"
#include "RadamsaWidenCodePointMutator.hpp"
#include "RuntimeException.hpp"

using vmf::StorageModule;
using vmf::StorageRegistry;
using vmf::ModuleTestHelper;
using vmf::TestConfigInterface;
using vmf::SimpleStorage;
using vmf::StorageEntry;
using vmf::RadamsaWidenCodePointMutator;
using vmf::BaseException;
using vmf::RuntimeException;

class RadamsaWidenCodePointMutatorTest : public ::testing::Test {
  protected:
    RadamsaWidenCodePointMutatorTest() 
    {
      storage = new SimpleStorage("storage");
      registry = new StorageRegistry("TEST_INT", StorageRegistry::INT, StorageRegistry::ASCENDING);
      metadata = new StorageRegistry();
      testHelper = new ModuleTestHelper();
      theMutator = new RadamsaWidenCodePointMutator("RadamsaWidenCodePointMutator");
      config = testHelper -> getConfig();
    }

    ~RadamsaWidenCodePointMutatorTest() override = default;

    void SetUp() override {
      testCaseKey = registry->registerKey(
          "TEST_CASE", 
          StorageRegistry::BUFFER, 
          StorageRegistry::READ_WRITE
      );
      // int_key = registry->registerKey(
      //     "TEST_INT",
      //     StorageRegistry::INT,
      //     StorageRegistry::READ_WRITE
      // );
      // normalTag = registry->registerTag(
      //     "RAN_SUCCESSFULLY",
      //     StorageRegistry::WRITE_ONLY
      // );
      // // registry->validateRegistration();
      storage->configure(registry, metadata);
      theMutator->init(*config);
      theMutator->registerStorageNeeds(*registry);
      theMutator->registerMetadataNeeds(*metadata);
    }

    void TearDown() override {
      delete theMutator;
      theMutator = nullptr;
      delete testHelper;
      testHelper = nullptr;
      delete registry;
      registry = nullptr;
      delete metadata;
      metadata = nullptr;
      delete storage;
      storage = nullptr;
    }

    RadamsaWidenCodePointMutator* theMutator;
    StorageModule* storage;
    StorageRegistry* registry;
    StorageRegistry* metadata;
    ModuleTestHelper* testHelper;
    TestConfigInterface* config;
    int testCaseKey;
};

/*TEST_F(RadamsaWidenCodePointMutatorTest, BufferNotNull)
{
    // no way to test this without mocks
}*/

TEST_F(RadamsaWidenCodePointMutatorTest, NotAscii)
{   
    StorageEntry* baseEntry = storage->createNewEntry();
    StorageEntry* modEntry = storage->createNewEntry();
    char* modBuff;

    const size_t buff_len = 1;
    char* buff = baseEntry->allocateBuffer(testCaseKey, buff_len);

    buff[0] = 127;

    try{
        theMutator->mutateTestCase(*storage, baseEntry, modEntry, testCaseKey);
        modBuff = modEntry->getBufferPointer(testCaseKey);
    }
    catch (BaseException e)
    {
        FAIL() << "Exception thrown: " << e.getReason();
    }

    // Mutator should have returned without modifying the buffer
    size_t modBuff_len = modEntry->getBufferSize(testCaseKey);
    EXPECT_EQ(modBuff[0], buff[0]);
    EXPECT_EQ(modBuff_len, buff_len);
}

TEST_F(RadamsaWidenCodePointMutatorTest, OneByte)
{   
    std::string buffString = "g";   // 0b01100111

    StorageEntry* baseEntry = storage->createNewEntry();
    StorageEntry* modEntry = storage->createNewEntry();
    char* modBuff;

    const size_t buff_len = buffString.length();
    char* buff = baseEntry->allocateBuffer(testCaseKey, buff_len);
    for(size_t i{0}; i < buff_len; ++i) {
        buff[i] = buffString[i];
    }
    try{
        theMutator->mutateTestCase(*storage, baseEntry, modEntry, testCaseKey);
        modBuff = modEntry->getBufferPointer(testCaseKey);
    } 
    catch (BaseException e)
    {
        FAIL() << "Exception thrown: " << e.getReason();
    }

    size_t modBuff_len = modEntry->getBufferSize(testCaseKey);

    ASSERT_FALSE(
        std::equal(
            buff,
            buff + buff_len, 
            modBuff
        )
    );
    EXPECT_EQ(modBuff_len, buff_len + 2);
    EXPECT_EQ(modBuff[0], '\xC0');  // 0b11000000
    EXPECT_EQ(modBuff[1], '\xE7');  // 0b11100111
}

TEST_F(RadamsaWidenCodePointMutatorTest, TwoBytes)
{   
    std::string buffString = "gh";   // 0b01100111, 0b01101000

    StorageEntry* baseEntry = storage->createNewEntry();
    StorageEntry* modEntry = storage->createNewEntry();
    char* modBuff;

    const size_t buff_len = buffString.length();
    char* buff = baseEntry->allocateBuffer(testCaseKey, buff_len);
    for(size_t i{0}; i < buff_len; ++i) {
        buff[i] = buffString[i];
    }
    try{
        theMutator->mutateTestCase(*storage, baseEntry, modEntry, testCaseKey);
        modBuff = modEntry->getBufferPointer(testCaseKey);
    } 
    catch (BaseException e)
    {
        FAIL() << "Exception thrown: " << e.getReason();
    }

    size_t modBuff_len = modEntry->getBufferSize(testCaseKey);

    ASSERT_FALSE(
        std::equal(
            buff,
            buff + buff_len, 
            modBuff
        )
    );
    EXPECT_EQ(modBuff_len, buff_len + 2);

    if (modBuff[0] == '\xC0') EXPECT_EQ(modBuff[1], '\xE7');

    else if (modBuff[1] == '\xC0') EXPECT_EQ(modBuff[2], '\xE8');

    else FAIL() << "modBuff[0] and modBuff[1] do not equal \'\\xC0\'";
}

TEST_F(RadamsaWidenCodePointMutatorTest, MixedValidInvalid)
{   
    StorageEntry* baseEntry = storage->createNewEntry();
    StorageEntry* modEntry = storage->createNewEntry();
    char* modBuff;

    const size_t buff_len = 3;
    char* buff = baseEntry->allocateBuffer(testCaseKey, buff_len);

    buff[0] = 127;
    buff[1] = 'g';
    buff[2] = 31;

    try{
        theMutator->mutateTestCase(*storage, baseEntry, modEntry, testCaseKey);
        modBuff = modEntry->getBufferPointer(testCaseKey);
    } 
    catch (BaseException e)
    {
        FAIL() << "Exception thrown: " << e.getReason();
    }

    size_t modBuff_len = modEntry->getBufferSize(testCaseKey);

    ASSERT_FALSE(
        std::equal(
            buff,
            buff + buff_len, 
            modBuff
        )
    );
    EXPECT_EQ(modBuff_len, buff_len + 2);
    EXPECT_EQ(modBuff[1], '\xC0');  // 0b11000000
    EXPECT_EQ(modBuff[2], '\xE7');  // 0b11100111
}