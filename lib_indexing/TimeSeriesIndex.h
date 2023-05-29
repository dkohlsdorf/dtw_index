#pragma once
#include "IndexingTypes.h"
#include <shared_mutex>
#include <mutex>
#include <vector>
#include <map>

namespace tsidx {
  /**
   * Locking is hard so here are some hints:
   * Hints:
   *  [1] https://en.cppreference.com/w/cpp/thread/shared_mutex
   *  [2] https://en.cppreference.com/w/cpp/atomic/atomic
   */

  using Bucket = std::vector<int>;

  const int IDX_READY = 0;
  const int IDX_RUN = 1;

  template<typename T>
  class ThreadsafeCollection {
  public:
    ThreadsafeCollection() = default;
    void insert(T x);
    std::vector<T> get();
    int size();
  private:
    std::vector<T> collection;
    mutable std::shared_mutex mutex;
  };

  class TimeSeriesIndex {
  public:
    TimeSeriesIndex(int bucket_size, int ts_bucket_size,
		    float band_percentage);
    ~TimeSeriesIndex();
    int insert(const TimeSeries& ts);
    int search_idx(const TimeSeries& ts, std::vector<int> &nearest);
    int reindex(int n_samples);
  private:
    int bucket_size;
    int ts_bucket_size;
    float band_percentage;
    
    Node *root;
    TimeSeriesBatch indexing_batch;
    std::map<int, int> leaf_map;
    
    int status;
    mutable std::shared_mutex mutex;

    std::vector<ThreadsafeCollection<TimeSeries>*> timeseries;   
    std::vector<ThreadsafeCollection<int>*> buckets;  
  };
}
