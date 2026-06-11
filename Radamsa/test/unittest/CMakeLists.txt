#===============================================================================
# Copyright (c) 2026 Vigilant Cyber Systems
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 (only) as 
# published by the Free Software Foundation.
#  
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.
#  
# @license GPL-2.0-only <https://spdx.org/licenses/GPL-2.0-only.html>
#===============================================================================

#set flag to export all symbols for windows builds
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

# required by gtest
find_package(Threads REQUIRED)
find_package(GTest REQUIRED)

include(vmf)

include(GoogleTest)

SET(TEST_SRCS
  RadamsaDecrementByteMutatorTest.cpp
  RadamsaDropByteMutatorTest.cpp
  RadamsaFlipByteMutatorTest.cpp
  RadamsaIncrementByteMutatorTest.cpp
  RadamsaInsertByteMutatorTest.cpp
  RadamsaPermuteByteMutatorTest.cpp
  RadamsaRandomizeByteMutatorTest.cpp
  RadamsaRepeatByteMutatorTest.cpp
  RadamsaDeleteLineMutatorTest.cpp
  RadamsaDeleteSequentialLinesMutatorTest.cpp
  RadamsaDuplicateLineMutatorTest.cpp
  RadamsaCopyLineCloseByMutatorTest.cpp
  RadamsaRepeatLineMutatorTest.cpp
  RadamsaSwapLineMutatorTest.cpp
  RadamsaRepeatByteSequenceMutatorTest.cpp
  RadamsaDeleteByteSequenceMutatorTest.cpp
  RadamsaPermuteLinesMutatorTest.cpp
  RadamsaInsertLineMutatorTest.cpp
  RadamsaReplaceLineMutatorTest.cpp
  RadamsaTreeMutatorBaseTest.cpp
  RadamsaDeleteNodeMutatorTest.cpp
  RadamsaDuplicateNodeMutatorTest.cpp
  RadamsaReplaceNodeMutatorTest.cpp
  RadamsaSwapNodesMutatorTest.cpp
  RadamsaRepeatPathMutatorTest.cpp
  RadamsaWidenCodePointMutatorTest.cpp
  RadamsaInsertUnicodeMutatorTest.cpp
  RadamsaModifyTextNumberMutatorTest.cpp
  RadamsaFuse_helpers.cpp
  RadamsaFuseThisMutatorTest.cpp
  RadamsaFuseNextMutatorTest.cpp
  RadamsaFuseOldMutatorTest.cpp
  RadamsaAsciiBadMutatorTest.cpp
)

add_executable(VmfTest ${TEST_SRCS})

set_target_properties(VmfTest PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(VmfTest PUBLIC
  ${CMAKE_INSTALL_PREFIX}/include
  ${CMAKE_INSTALL_PREFIX}/include/vmf
  ${CMAKE_INSTALL_PREFIX}/include/vmf/test
)

target_link_directories(VmfTest PUBLIC
  ${CMAKE_INSTALL_PREFIX}/lib
  ${CMAKE_INSTALL_PREFIX}/plugins
)

target_link_libraries(VmfTest
  PUBLIC
  gtest
  gtest_main
  VMFFramework
  CoreModules
  Threads::Threads
  Radamsa
  VmfTestLib
  yaml-cpp
)
gtest_discover_tests(VmfTest)
