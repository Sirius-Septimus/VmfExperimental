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
#include "RadamsaDeleteSequentialLinesMutator.hpp"
#include "RuntimeException.hpp"

using vmf::StorageModule;
using vmf::StorageRegistry;
using vmf::ModuleTestHelper;
using vmf::TestConfigInterface;
using vmf::SimpleStorage;
using vmf::StorageEntry;
using vmf::RadamsaDeleteSequentialLinesMutator;
using vmf::BaseException;
using vmf::RuntimeException;

class RadamsaDeleteSequentialLinesMutatorTest : public ::testing::Test {
  protected:
    RadamsaDeleteSequentialLinesMutatorTest() 
    {
      storage = new SimpleStorage("storage");
      registry = new StorageRegistry("TEST_INT", StorageRegistry::INT, StorageRegistry::ASCENDING);
      metadata = new StorageRegistry();
      testHelper = new ModuleTestHelper();
      theMutator = new RadamsaDeleteSequentialLinesMutator("RadamsaDeleteSequentialLinesMutator");
      config = testHelper -> getConfig();
    }

    ~RadamsaDeleteSequentialLinesMutatorTest() override = default;

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

    RadamsaDeleteSequentialLinesMutator* theMutator;
    StorageModule* storage;
    StorageRegistry* registry;
    StorageRegistry* metadata;
    ModuleTestHelper* testHelper;
    TestConfigInterface* config;
    int testCaseKey;
};

/*TEST_F(RadamsaDeleteSequentialLinesMutatorTest, BufferNotNull)
{
    // no way to test this without mocks
}*/

TEST_F(RadamsaDeleteSequentialLinesMutatorTest, BufferSizeGEOne)
{    
    StorageEntry* baseEntry = storage->createNewEntry();
    StorageEntry* modEntry = storage->createNewEntry();

    // By not allocating the buffer, we're forcing
    // StorageEntry::getBufferSize() to return '-1'.
    // The mutator should return early without throwing an exception
    // or modifying the entry.
    try{
        theMutator->mutateTestCase(*storage, baseEntry, modEntry, testCaseKey);
        // mutator should have returned early
        SUCCEED();
    }
    catch (BaseException e)
    {
        FAIL() << "Exception thrown: " << e.getReason();
    }
}

TEST_F(RadamsaDeleteSequentialLinesMutatorTest, OneLine)
{    
    StorageEntry* baseEntry = storage->createNewEntry();
    StorageEntry* modEntry = storage->createNewEntry();

    size_t buff_len = 2;
    size_t line_len = 2;
    char* modBuff;
    char* buff = baseEntry->allocateBuffer(testCaseKey, buff_len);
    buff[0] = '4';
    buff[1] = '\n';

    try{
        theMutator->mutateTestCase(*storage, baseEntry, modEntry, testCaseKey);
        modBuff = modEntry->getBufferPointer(testCaseKey);
    } 
    catch (BaseException e)
    {
        FAIL() << "Exception thrown: " << e.getReason();
    }

    // test buff ne
    ASSERT_FALSE(std::equal(buff,       buff + buff_len, 
                            modBuff,    modBuff + modEntry->getBufferSize(testCaseKey) - 1));
    // test number of lines in buff
    EXPECT_EQ(buff_len / line_len - 1, 
               std::count(modBuff, modBuff + modEntry->getBufferSize(testCaseKey), '\n'));
    // test buff len
    EXPECT_EQ(buff_len - line_len + 1, modEntry->getBufferSize(testCaseKey));
    // test buff contents
    std::string modString = std::string(modBuff);
    EXPECT_EQ(modString, "\0");
}

TEST_F(RadamsaDeleteSequentialLinesMutatorTest, TwoLines)
{    
    StorageEntry* baseEntry = storage->createNewEntry();
    StorageEntry* modEntry = storage->createNewEntry();

    size_t buff_len = 4;
    size_t line_len = 2;
    char* modBuff;
    char* buff = baseEntry->allocateBuffer(testCaseKey, buff_len);
    buff[0] = '4';
    buff[1] = '\n';
    buff[2] = '5';
    buff[3] = '\n';

    try{
        theMutator->mutateTestCase(*storage, baseEntry, modEntry, testCaseKey);
        modBuff = modEntry->getBufferPointer(testCaseKey);
    } 
    catch (BaseException e)
    {
        FAIL() << "Exception thrown: " << e.getReason();
    }

    // test buff ne
    ASSERT_FALSE(std::equal(buff,       buff + buff_len, 
                            modBuff,    modBuff + modEntry->getBufferSize(testCaseKey) - 1));
    // test number of lines in buff (at least one line removed)
    EXPECT_LE(std::count(modBuff, modBuff + modEntry->getBufferSize(testCaseKey), '\n'),
              buff_len / line_len - 1);
    // test buff len
    EXPECT_LE(modEntry->getBufferSize(testCaseKey), buff_len - line_len + 1);
    // test buff contents
    std::string modString = std::string(modBuff);
    EXPECT_TRUE(modString == "4\n\0" |  // deleted one line
                modString == "5\n\0" | 
                modString == "\0");     // deleted two lines
}

TEST_F(RadamsaDeleteSequentialLinesMutatorTest, ThreeLines)
{    
    StorageEntry* baseEntry = storage->createNewEntry();
    StorageEntry* modEntry = storage->createNewEntry();

    size_t buff_len = 6;
    size_t line_len = 2;
    char* modBuff;
    char* buff = baseEntry->allocateBuffer(testCaseKey, buff_len);
    buff[0] = '4';
    buff[1] = '\n';
    buff[2] = '5';
    buff[3] = '\n';
    buff[4] = '6';
    buff[5] = '\n';

    try{
        theMutator->mutateTestCase(*storage, baseEntry, modEntry, testCaseKey);
        modBuff = modEntry->getBufferPointer(testCaseKey);
    } 
    catch (BaseException e)
    {
        FAIL() << "Exception thrown: " << e.getReason();
    }

    // test buff ne
    ASSERT_FALSE(std::equal(buff,       buff + buff_len, 
                            modBuff,    modBuff + modEntry->getBufferSize(testCaseKey) - 1));
    // test number of lines in buff (at least one line removed)
    EXPECT_LE(std::count(modBuff, modBuff + modEntry->getBufferSize(testCaseKey), '\n'),
              buff_len / line_len - 1);
    // test buff len
    EXPECT_LE(modEntry->getBufferSize(testCaseKey), buff_len - line_len + 1);
    // test buff contents
    std::string modString = std::string(modBuff);
    EXPECT_TRUE(modString == "4\n5\n\0" |   // deleted one line
                modString == "5\n6\n\0" |
                modString == "4\n6\n\0" |
                modString == "4\n\0" |      // deleted two lines
                modString == "5\n\0" | 
                modString == "6\n\0" | 
                modString == "\0");         // deleted three lines
}