#pragma once
#include <algorithm>
#include <atomic>
#include <concepts>
#include <thread>
#include <vector>

template <std::invocable<int> Fn>
void parallelFor(int begin, int end, Fn&& fn) {
    const int count = end - begin;
#if defined(__EMSCRIPTEN__) && !defined(__EMSCRIPTEN_PTHREADS__)
    const int workers = 1;
#else
    const int workers = std::min<int>(count, std::max(1u, std::thread::hardware_concurrency()));
#endif
    if (workers <= 1) { for (int i = begin; i < end; ++i) fn(i); return; }

    std::atomic<int> next{begin};
    auto worker = [&] { for (int i = next++; i < end; i = next++) fn(i); };
    std::vector<std::jthread> threads;
    for (int t = 1; t < workers; ++t) threads.emplace_back(worker);
    worker();   // this thread takes a share too; jthreads join on scope exit
}
