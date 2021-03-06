/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <tensorpipe/test/channel/cma/cma_test.h>

CmaChannelTestHelper helper;

INSTANTIATE_TEST_CASE_P(Cma, ChannelTest, ::testing::Values(&helper));
