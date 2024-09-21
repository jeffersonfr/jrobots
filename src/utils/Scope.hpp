#pragma once

#include <functional>

#include <boost/asio/io_service.hpp>
#include <boost/asio/strand.hpp>
#include <boost/thread/thread.hpp>

struct Scope {
  Scope(std::size_t threads = 1)
  : mThreads{}, mIoService{}, mStrand{mIoService} {
    for (auto i = 0; i < threads; ++i) {
      mThreads.create_thread([&]() { mIoService.run(); });
    }
  }

  void post(std::function<void()> task) {
    mIoService.post(task);
  }

  void post_ordered(std::function<void()> task) {
    mStrand.post(task);
  }

  void stop() {
    mIoService.stop();
    mThreads.interrupt_all();
    mThreads.join_all();
  }

private:
  boost::thread_group mThreads;
  boost::asio::io_service mIoService;
  boost::asio::io_service::strand mStrand;
};
