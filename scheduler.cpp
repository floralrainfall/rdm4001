#include "scheduler.hpp"
#include "logging.hpp"
#include <easy/profiler.h>
#include <chrono>

#ifdef __linux
#include <pthread.h>
#include <linux/prctl.h>  /* Definition of PR_* constants */
#include <sys/prctl.h>
#endif

namespace rdm {
Scheduler::~Scheduler() {
  waitToWrapUp();
}

void Scheduler::waitToWrapUp() {
  for(auto& job : jobs) {
    job->stopBlocking();
  }
}

void Scheduler::addJob(SchedulerJob* job) {
  jobs.push_back(std::unique_ptr<SchedulerJob>(job));
}

void Scheduler::startAllJobs() {
  for(int i = 0; i < jobs.size(); i++)
    jobs[i]->startTask();
}

SchedulerJob::SchedulerJob(const char* name, bool stopOnCancel) {
  this->stopOnCancel = stopOnCancel;
  stats.name = name;
  killMutex.lock();
}

SchedulerJob::~SchedulerJob() {
  stopBlocking();
}

void SchedulerJob::task(SchedulerJob* job) {
  EASY_THREAD_SCOPE(job->stats.name);
#ifndef NDEBUG
#ifdef __linux
  pthread_setname_np(pthread_self(), job->stats.name);
  prctl(PR_SET_NAME, job->stats.name);
#endif
#endif
  bool running = true;
  while(running) {
    EASY_BLOCK("Frame");
    std::chrono::time_point start = std::chrono::high_resolution_clock::now();
    
    Result r;
    try {
      EASY_BLOCK("Step");
      r = job->step();
    } catch(std::exception& e) {
      Log::printf(LOG_FATAL, "Fatal unhandled exception in %s, what() = '%s'", job->getStats().name, e.what());
      r = Cancel;

      try {
        job->error(e);
      } catch(std::exception& e) {
        Log::printf(LOG_FATAL, "Double error in SchedulerJob error handler, what() = '%s'", e.what());
      }
    }
    switch(r) {
    case Stepped:
      break;
    case Cancel:
      job->state = Stopped;
      break;
    }

    switch(job->state) {
    case Running:
    default:
      break;
    case StopPlease:
      if(job->stopOnCancel) {
        running = false;
        job->state = Stopped;
      }
      break;
    case Stopped:
      running = false;
      job->state = Stopped;
      break;
    }

    std::chrono::time_point end = std::chrono::high_resolution_clock::now();
    std::chrono::duration execution = end - start;
    job->stats.deltaTime = std::chrono::duration<double>(execution).count();
    double frameRate = job->getFrameRate();
    if(frameRate != 0.0) { // run as fast as we can if there is no frame rate
      std::chrono::duration sleep = std::chrono::duration<double>(frameRate) - execution;
      EASY_BLOCK("Sleep");
      if(job->killMutex.try_lock_until(std::chrono::system_clock::now() + sleep)) {
        Log::printf(LOG_DEBUG, "killMutex unlocked on %s", job->getStats().name);
        running = false;
      }
    }
    end = std::chrono::high_resolution_clock::now();
    execution = end - start;
    job->stats.totalDeltaTime = std::chrono::duration<double>(execution).count();
  }
  job->state = Stopped;
#ifndef NDEBUG
  Log::printf(LOG_DEBUG, "Task %s stopped", job->getStats().name);
#endif
}

void SchedulerJob::startTask() {
  thread = std::thread(&SchedulerJob::task, this);
}

SchedulerJob::Result SchedulerJob::step() {
  return Stepped;
}

void SchedulerJob::stopBlocking() {
  if(state == Running) {
    state = StopPlease;
    if(stopOnCancel)
      killMutex.unlock();
    while(state != Stopped) {
      std::this_thread::yield();
    }
    thread.join();
  }
}
};