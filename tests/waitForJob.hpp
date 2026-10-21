#pragma once

#include <thread>

// start() does nothing where there is no worker thread. Stepping is what finishes the job.
template <typename Job, typename Step>
void waitForJob(Job& job, Step step) {
    job.start();
    while (!job.finished()) {
        if (!job.runsOnWorker()) step();
        else std::this_thread::yield();
    }
}
