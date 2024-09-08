#pragma once

#include <vector>
#include <thread>
#include <atomic>
#include <mutex>

namespace rdm {  
  struct JobStatistics {
    const char* name;
    double deltaTime;
    double totalDeltaTime;
  };

  class SchedulerJob {
    enum State {
      Running,
      StopPlease,
      Stopped
    };

    std::atomic<State> state;
    std::thread thread;
    JobStatistics stats;
    std::timed_mutex killMutex;

    bool stopOnCancel;
  public:
    SchedulerJob(const char* name, bool stopOnCancel = true);
    virtual ~SchedulerJob();

    enum Result {
      Stepped,
      Cancel,
    };

    virtual double getFrameRate() { return 1.0 / 60.0; };
    virtual Result step();
    
    // called when you dont handle errors in step()
    virtual void error(std::exception& e) {};
    
    JobStatistics getStats() { return stats; }

    void stop();
    void stopBlocking();

    static void task(SchedulerJob* job);
    void startTask();
  };

  class Scheduler {
    std::vector<std::unique_ptr<SchedulerJob>> jobs;
  public:
    ~Scheduler();

    // Scheduler will manage lifetimes for the SchedulerJob. just allocate it and dont worry about it
    void addJob(SchedulerJob* job);

    void waitToWrapUp();

    void startAllJobs();
  };
};