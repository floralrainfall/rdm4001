#pragma once

#include <atomic>
#include <mutex>
#include <thread>
#include <vector>

namespace rdm {
/**
 * @brief Statistics for a specific job.
 *
 */
struct JobStatistics {
  /**
   * @brief The job name.
   *
   */
  const char* name;
  /**
   * @brief The last delta time of a job.
   *
   */
  double deltaTime;
  /**
   * @brief The total delta time of a job (including sleep time.)
   *
   */
  double totalDeltaTime;
  /**
   * @brief The total time of a job (added using delta times)
   *
   */
  double time;
};

class SchedulerJob {
  enum State { Running, StopPlease, Stopped };

  std::atomic<State> state;
  std::thread thread;
  JobStatistics stats;
  std::timed_mutex killMutex;

  bool stopOnCancel;

 public:
  SchedulerJob(const char* name, bool stopOnCancel = true);
  virtual ~SchedulerJob();

  /**
   * @brief Step result.
   *
   */
  enum Result {
    /**
     * @brief Continue stepping the job.
     *
     */
    Stepped,
    /**
     * @brief Stop stepping the job, will set State to Stopped.
     *
     */
    Cancel,
  };

  /**
   * @brief The frame rate of a job.
   *
   * @return double The number of seconds a job should run for in a second. It
   * will sleep to account for unused time
   */
  virtual double getFrameRate() { return 1.0 / 60.0; };
  /**
   * @brief An individual step frame of a job.
   *
   * @return Result The result of the step. See... Result.
   */
  virtual Result step();

  /**
   * @brief Called when your Job throws an exception, and you don't handle it.
   *
   * @param e The thrown exception
   */
  virtual void error(std::exception& e) {};

  /**
   * @brief Returns the statistics of a job.
   *
   * @return JobStatistics The statistics.
   */
  JobStatistics getStats() { return stats; }

  /**
   * @brief Blocks and waits until a job is stopped.
   *
   * This will lock the killMutex, causing sleep to terminate the job if it
   * steps.
   */
  void stopBlocking();

  static void task(SchedulerJob* job);

  void startTask();

  std::thread& getThread() { return thread; }
};

class Scheduler {
  std::vector<std::unique_ptr<SchedulerJob>> jobs;

 public:
  ~Scheduler();

  /**
   * @brief Adds a job to the scheduler. It will start when startAllJobs is
   * called
   *
   * @param job The job to be added. The Scheduler will take exclusive ownership
   * over the lifetime of the job pointer.
   */
  SchedulerJob* addJob(SchedulerJob* job);

  void waitToWrapUp();

  void startAllJobs();

  SchedulerJob* getJob(std::string name);
};
};  // namespace rdm