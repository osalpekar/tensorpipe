/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <sys/eventfd.h>

#include <deque>

#include <tensorpipe/common/defs.h>
#include <tensorpipe/transport/shm/loop.h>

#include <gtest/gtest.h>

using namespace tensorpipe::transport::shm;

namespace {

class Handler : public EventHandler {
 public:
  void handleEventsFromReactor(int events) override {
    std::unique_lock<std::mutex> lock(m_);
    events_.push_back(events);
    cv_.notify_all();
  }

  int nextEvents() {
    std::unique_lock<std::mutex> lock(m_);
    while (events_.empty()) {
      cv_.wait(lock);
    }
    int events = events_.front();
    events_.pop_front();
    return events;
  }

 private:
  std::mutex m_;
  std::condition_variable cv_;
  std::deque<int> events_;
};

} // namespace

TEST(Loop, Create) {
  auto loop = Loop::create();
  ASSERT_TRUE(loop);
  loop->join();
}

TEST(Loop, RegisterUnregister) {
  auto loop = Loop::create();
  auto handler = std::make_shared<Handler>();
  auto efd = Fd(eventfd(0, EFD_NONBLOCK));

  {
    // Test if writable (always).
    loop->registerDescriptor(efd.fd(), EPOLLOUT | EPOLLONESHOT, handler);
    ASSERT_EQ(handler->nextEvents(), EPOLLOUT);
    efd.writeOrThrow<uint64_t>(1337);

    // Test if readable (only if previously written to).
    loop->registerDescriptor(efd.fd(), EPOLLIN | EPOLLONESHOT, handler);
    ASSERT_EQ(handler->nextEvents(), EPOLLIN);
    ASSERT_EQ(efd.readOrThrow<uint64_t>(), 1337);

    // Test if we can unregister the descriptor.
    loop->unregisterDescriptor(efd.fd());
  }

  loop->join();
}

TEST(Loop, Monitor) {
  auto loop = Loop::create();
  auto efd = Fd(eventfd(0, EFD_NONBLOCK));
  constexpr uint64_t kValue = 1337;

  {
    std::mutex mutex;
    std::condition_variable cv;
    bool done = false;

    // Test if writable (always).
    auto shared = std::make_shared<int>(1338);
    auto monitor = loop->monitor<int>(
        shared, efd.fd(), EPOLLOUT, [&](int& i, FunctionEventHandler& handler) {
          ASSERT_EQ(i, 1338);
          {
            std::unique_lock<std::mutex> lock(mutex);
            done = true;
            efd.writeOrThrow<uint64_t>(kValue);
          }
          handler.cancel();
          cv.notify_all();
        });

    // Wait for monitor to trigger and perform a write.
    std::unique_lock<std::mutex> lock(mutex);
    while (!done) {
      cv.wait(lock);
    }
  }

  {
    std::mutex mutex;
    std::condition_variable cv;
    bool done = false;
    uint64_t value = 0;

    // Test if readable (only if previously written to).
    auto shared = std::make_shared<int>(1338);
    auto monitor = loop->monitor<int>(
        shared, efd.fd(), EPOLLIN, [&](int& i, FunctionEventHandler& handler) {
          ASSERT_EQ(i, 1338);
          {
            std::unique_lock<std::mutex> lock(mutex);
            done = true;
            value = efd.readOrThrow<uint64_t>();
          }
          handler.cancel();
          cv.notify_all();
        });

    // Wait for monitor to trigger and perform a read.
    std::unique_lock<std::mutex> lock(mutex);
    while (!done) {
      cv.wait(lock);
    }

    // Verify we read the correct value.
    ASSERT_EQ(value, kValue);
  }

  loop->join();
}

TEST(Loop, Defer) {
  auto loop = Loop::create();
  auto promise = std::make_shared<std::promise<void>>();
  auto future = promise->get_future();
  loop->deferToReactor([promise]() { promise->set_value(); });
  future.wait();
  ASSERT_TRUE(future.valid());
  loop->join();
}
