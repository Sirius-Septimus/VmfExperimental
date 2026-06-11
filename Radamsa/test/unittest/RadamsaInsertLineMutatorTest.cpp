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
#include "RadamsaInsertLineMutator.hpp"
#include "RuntimeException.hpp"

using vmf::StorageModule;
using vmf::StorageRegistry;
using vmf::ModuleTestHelper;
using vmf::TestConfigInterface;
using vmf::SimpleStorage;
using vmf::StorageEntry;
using vmf::RadamsaInsertLineMutator;
using vmf::BaseException;
using vmf::RuntimeException;

class RadamsaInsertLineMutatorTest : public ::testing::Test {
  protected:
    RadamsaInsertLineMutatorTest() 
    {
      storage = new SimpleStorage("storage");
      registry = new StorageRegistry("TEST_INT", StorageRegistry::INT, StorageRegistry::ASCENDING);
      metadata = new StorageRegistry();
      testHelper = new ModuleTestHelper();
      theMutator = new RadamsaInsertLineMutator("RadamsaInsertLineMutator");
      config = testHelper -> getConfig();
    }

    ~RadamsaInsertLineMutatorTest() override = default;

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

    RadamsaInsertLineMutator* theMutator;
    StorageModule* storage;
    StorageRegistry* registry;
    StorageRegistry* metadata;
    ModuleTestHelper* testHelper;
    TestConfigInterface* config;
    int testCaseKey;
};

/*TEST_F(RadamsaInsertLineMutatorTest, BufferNotNull)
{
    // no way to test this without mocks
}*/

TEST_F(RadamsaInsertLineMutatorTest, OneByte)
{   
    std::string buffString = "\n";

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

    // Mutator should have returned without modifying the buffer
    size_t modBuff_len = modEntry->getBufferSize(testCaseKey);
    EXPECT_EQ(modBuff[0], buff[0]);
    EXPECT_EQ(modBuff_len, buff_len);
}

TEST_F(RadamsaInsertLineMutatorTest, OneLine)
{   
    std::string buffString = "G\n";

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

    // Mutator should have returned without modifying the buffer
    size_t modBuff_len = modEntry->getBufferSize(testCaseKey);
    EXPECT_EQ(modBuff[0], buff[0]);
    EXPECT_EQ(modBuff[1], buff[1]);
    EXPECT_EQ(modBuff_len, buff_len);
}

TEST_F(RadamsaInsertLineMutatorTest, TwoLines)
{   
    std::string buffString = "GHI\nJK\n";

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
    std::string modString = std::string(modBuff);

    // test buff ne
    // ASSERT_FALSE(std::equal(buff,       buff + buff_len, 
    //                         modBuff,    modBuff + modBuff_len - 1)
    //             );
    // test buff len
    EXPECT_GT(modBuff_len, buff_len + 1);
    // test buff contents
    EXPECT_TRUE(
        modString == "GHI\nGHI\nJK\n" ||    // 112
        modString == "GHI\nJK\nGHI\n" ||    // 121
        modString == "JK\nGHI\nJK\n" ||     // 212
        modString == "GHI\nJK\nJK\n"        // 122
    ) << "modString = \"" + modString + "\"";
}

TEST_F(RadamsaInsertLineMutatorTest, ThreeLines)
{   
    StorageEntry* baseEntry = storage->createNewEntry();
    StorageEntry* modEntry = storage->createNewEntry();
    
    std::string buffString = "GHI\nJK\nL\n";
    const size_t buff_len = buffString.length();
    char* modBuff;
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
        FAIL() << "Exception thrown: " << e.getReason() << std::endl << "buff_len = " + std::to_string(buff_len);
    }

    size_t modBuff_len = modEntry->getBufferSize(testCaseKey);
    std::string modString = std::string(modBuff);

    // test buff ne
    // ASSERT_FALSE(std::equal(buff,       buff + buff_len, 
    //                         modBuff,    modBuff + modBuff_len - 1)
    //             );
    // test buff len
    EXPECT_GT(modBuff_len, buff_len + 1);
    EXPECT_TRUE(
        modString == "GHI\nGHI\nJK\nL\n" ||  // 1123
        modString == "GHI\nJK\nGHI\nL\n" ||  // 1213
        modString == "GHI\nJK\nL\nGHI\n" ||  // 1231
        
        modString == "JK\nGHI\nJK\nL\n" ||  // 2123
        modString == "GHI\nJK\nJK\nL\n" ||  // 1223
        modString == "GHI\nJK\nL\nJK\n" ||  // 1232
        
        modString == "L\nGHI\nJK\nL\n" ||  // 3123
        modString == "GHI\nL\nJK\nL\n" ||  // 1323
        modString == "GHI\nJK\nL\nL\n"     // 1233
    ) << "modString = \"" + modString + "\"";
}