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
#include "RadamsaRepeatPathMutator.hpp"
#include "RuntimeException.hpp"
#include "RadamsaTreeMutatorBase.hpp"

using vmf::StorageModule;
using vmf::StorageRegistry;
using vmf::ModuleTestHelper;
using vmf::TestConfigInterface;
using vmf::SimpleStorage;
using vmf::StorageEntry;
using vmf::RadamsaRepeatPathMutator;
using vmf::BaseException;
using vmf::RuntimeException;
using vmf::RadamsaTreeMutatorBase;

class RadamsaRepeatPathMutatorTest : public ::testing::Test {
  protected:
    RadamsaRepeatPathMutatorTest() 
    {
      storage = new SimpleStorage("storage");
      registry = new StorageRegistry("TEST_INT", StorageRegistry::INT, StorageRegistry::ASCENDING);
      metadata = new StorageRegistry();
      testHelper = new ModuleTestHelper();
      theMutator = new RadamsaRepeatPathMutator("RadamsaRepeatPathMutator");
      config = testHelper -> getConfig();
    }

    ~RadamsaRepeatPathMutatorTest() override = default;

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

    RadamsaRepeatPathMutator* theMutator;
    StorageModule* storage;
    StorageRegistry* registry;
    StorageRegistry* metadata;
    ModuleTestHelper* testHelper;
    TestConfigInterface* config;
    int testCaseKey;
};

/*TEST_F(RadamsaRepeatPathMutatorTest, BufferNotNull)
{
    // no way to test this without mocks
}*/

TEST_F(RadamsaRepeatPathMutatorTest, ThreeBytes)
{   
    std::string buffString = "GHI";

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
    EXPECT_EQ(modBuff[2], buff[2]);
    EXPECT_EQ(modBuff_len, buff_len);
}

TEST_F(RadamsaRepeatPathMutatorTest, JustRoot)
{   
    std::string buffString = "GHIJ";

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
    EXPECT_EQ(modBuff[2], buff[2]);
    EXPECT_EQ(modBuff[3], buff[3]);
    EXPECT_EQ(modBuff_len, buff_len);
}

TEST_F(RadamsaRepeatPathMutatorTest, OneChild)
{   
    std::string buffString = "GH(IJ)";

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

    char lastNodeChar = ')';
    for (size_t i = 1; lastNodeChar == ')'; ++i) {
        lastNodeChar = modString[modString.length() - i];
    }

    ASSERT_FALSE(
        std::equal(
            buff,
            buff + buff_len, 
            modBuff
        )
    );
    EXPECT_EQ((modBuff_len - 1 - buff_len) % 4, 0); // 4 is the number of additional characters per repitition
    EXPECT_EQ(lastNodeChar, 'J');
}

TEST_F(RadamsaRepeatPathMutatorTest, TwoChildren)
{   
    std::string buffString = "GH(IJ)(KL)";

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

    char lastNodeChar = ')';
    for (size_t i = 1; lastNodeChar == ')'; ++i) {
        lastNodeChar = modString[modString.length() - i];
    }

    ASSERT_FALSE(
        std::equal(
            buff,
            buff + buff_len, 
            modBuff
        )
    );
    EXPECT_EQ((modBuff_len - 1 - buff_len) % 4, 0); // 4 is the number of additional characters per repitition
    EXPECT_TRUE(lastNodeChar == 'J' || lastNodeChar == 'L');
}

TEST_F(RadamsaRepeatPathMutatorTest, TwoChildren_OneGrandchild)
{   
    std::string buffString = "g(h(i))(j)";

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

    std::map<char, size_t> charFreqMap;
    for (char c : modString) charFreqMap[c]++;

    ASSERT_FALSE(
        std::equal(
            buff,
            buff + buff_len, 
            modBuff
        )
    );
    EXPECT_EQ((modBuff_len - 1 - buff_len) % 3, 0); // 3 is the number of additional characters per repitition
    EXPECT_TRUE(
        (       // g replaced h
            charFreqMap['g'] > 1 &&  
            charFreqMap['h'] == 1 &&
            charFreqMap['i'] == 1 &&
            charFreqMap['j'] == 1
        ) || (  // h replaced i
            charFreqMap['g'] == 1 &&  
            charFreqMap['h'] > 1 &&
            charFreqMap['i'] == 1 &&
            charFreqMap['j'] == 1
        ) || (  // g replaced j
            charFreqMap['g'] > 1 &&  
            charFreqMap['h'] > 1 &&
            charFreqMap['i'] > 1 &&
            charFreqMap['j'] == 1
        )
    ) << "modString: " + modString;
}