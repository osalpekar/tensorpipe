# Copyright (c) Facebook, Inc. and its affiliates.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree.

# We need a separate CMakeLists.txt file for this subdirectory because of an
# issue in CMake: https://gitlab.kitware.com/cmake/cmake/issues/18828.

set(TENSORPIPE_PROTOBUFS
  ${CMAKE_CURRENT_SOURCE_DIR}/basic.proto
)

if(TP_ENABLE_CMA)
  list(APPEND TENSORPIPE_PROTOBUFS
    ${CMAKE_CURRENT_SOURCE_DIR}/cma.proto
  )
endif()

find_package(Protobuf 3 REQUIRED)
protobuf_generate_cpp(
  PROTOBUF_SOURCES
  PROTOBUF_HEADERS
  ${TENSORPIPE_PROTOBUFS}
  )
add_library(tensorpipe_proto_channel ${PROTOBUF_SOURCES})
target_include_directories(tensorpipe_proto_channel PUBLIC ${CMAKE_BINARY_DIR})
target_include_directories(
  tensorpipe_proto_channel
  PUBLIC
  ${Protobuf_INCLUDE_DIRS}
  )
target_link_libraries(tensorpipe_proto_channel PRIVATE ${Protobuf_LIBRARIES})
