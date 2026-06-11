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
#include "RadamsaSwapNodesMutator.hpp"
#include "RuntimeException.hpp"

using vmf::StorageModule;
using vmf::StorageRegistry;
using vmf::ModuleTestHelper;
using vmf::TestConfigInterface;
using vmf::SimpleStorage;
using vmf::StorageEntry;
using vmf::RadamsaSwapNodesMutator;
using vmf::BaseException;
using vmf::RuntimeException;

class RadamsaSwapNodesMutatorTest : public ::testing::Test {
  protected:
    RadamsaSwapNodesMutatorTest() 
    {
      storage = new SimpleStorage("storage");
      registry = new StorageRegistry("TEST_INT", StorageRegistry::INT, StorageRegistry::ASCENDING);
      metadata = new StorageRegistry();
      testHelper = new ModuleTestHelper();
      theMutator = new RadamsaSwapNodesMutator("RadamsaSwapNodesMutator");
      config = testHelper -> getConfig();
    }

    ~RadamsaSwapNodesMutatorTest() override = default;

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

    RadamsaSwapNodesMutator* theMutator;
    StorageModule* storage;
    StorageRegistry* registry;
    StorageRegistry* metadata;
    ModuleTestHelper* testHelper;
    TestConfigInterface* config;
    int testCaseKey;
};

/*TEST_F(RadamsaSwapNodesMutatorTest, BufferNotNull)
{
    // no way to test this without mocks
}*/

TEST_F(RadamsaSwapNodesMutatorTest, ThreeBytes)
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

TEST_F(RadamsaSwapNodesMutatorTest, JustRoot)
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

TEST_F(RadamsaSwapNodesMutatorTest, OneChild)
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

    // test buff len
    EXPECT_EQ(modBuff_len, buff_len + 1);
    // test buff contents
    EXPECT_TRUE(
        modString == "GH(IJ)\0" ||  // node1 == node2
        modString == "IJ(GH)\0"
    );
}

TEST_F(RadamsaSwapNodesMutatorTest, TwoChildren)
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

    // test buff len
    EXPECT_EQ(modBuff_len, buff_len + 1);
    // test buff contents
    EXPECT_TRUE(
        modString == "GH(IJ)(KL)" ||    // node1 == node2
        modString == "IJ(GH)(KL)" ||    // root <-> left
        modString == "KL(IJ)(GH)" ||    // root <-> right
        modString == "GH(KL)(IJ)"       // left <-> right
    );
}

TEST_F(RadamsaSwapNodesMutatorTest, TwoChildren_OneGrandchild)
{   
    std::string buffString = "GH(IJ(KL))(MN)";

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

    // test buff len
    EXPECT_EQ(modBuff_len, buff_len + 1);
    // test buff contents
    EXPECT_TRUE(
        modString == "GH(IJ(KL))(MN)\0" ||  // node1 == node2
        modString == "IJ(GH(KL))(MN)\0" ||  // root <-> ...
        modString == "KL(IJ(GH))(MN)\0" ||
        modString == "MN(IJ(KL))(GH)\0" || 
        modString == "GH(KL(IJ))(MN)\0" ||  // left <-> ...
        modString == "GH(MN(KL))(IJ)\0" ||
        modString == "GH(IJ(MN))(KL)\0"     // grandchild <-> ...
    );
}