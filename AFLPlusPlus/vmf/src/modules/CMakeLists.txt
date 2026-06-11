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

# Create AFLPlusPlus library
add_library(AFLPlusPlus SHARED
  common/mutator/AFLDWordAddSubMutator.cpp
  common/mutator/AFLInteresting8Mutator.cpp
  common/mutator/AFLInteresting16Mutator.cpp
  common/mutator/AFLInteresting32Mutator.cpp
  common/mutator/AFLOverwriteCopyMutator.cpp
  common/mutator/AFLOverwriteFixedMutator.cpp
  common/mutator/AFLWordAddSubMutator.cpp
)

#Set flag to export all symbols for windows builds
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

# windows needs an additional import to make the logger work
if(WIN32)
   target_compile_definitions(AFLPlusPlus PRIVATE PLOG_IMPORT)
endif()


# Build-time dependencies for AFLPlusPlus
link_directories(AFLPlusPlus PRIVATE
)

# Build-time dependencies for AFLPlusPlus
target_link_libraries(AFLPlusPlus PRIVATE
  # ${CMAKE_INSTALL_PREFIX}/bin/vader
  vmf_framework
  # vmf_core_modules
  # ${CMAKE_INSTALL_PREFIX}/../../submodules/LibAFL-legacy
)

# Build-time dependencies for AFLPlusPlus
target_include_directories(AFLPlusPlus PRIVATE
  ${CMAKE_INSTALL_PREFIX}/include
  ${CMAKE_INSTALL_PREFIX}/include/vmf
  ${CMAKE_INSTALL_PREFIX}/include/plog
  ${PROJECT_SOURCE_DIR}/src/module
)

# Install AFLPlusPlus library in VMF plugins directory
install(TARGETS AFLPlusPlus
  LIBRARY DESTINATION "${CMAKE_INSTALL_PREFIX}/plugins")

#On windows the /plugins directory above is ignored, and everything ends up in /lib and /bin
if(WIN32)
  cmake_path(SET WIN_LIB_FILE ${CMAKE_INSTALL_PREFIX}\\lib\\AFLPlusPlus.lib)
  cmake_path(SET WIN_DLL_FILE ${CMAKE_INSTALL_PREFIX}\\bin\\AFLPlusPlus.dll)
  install(FILES ${WIN_LIB_FILE} ${WIN_DLL_FILE}
    DESTINATION ${CMAKE_INSTALL_PREFIX}\\plugins)
endif()

