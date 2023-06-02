#include "TimeSeriesIndex.h"
#include "IndexingUtil.h"
#include <glog/logging.h>
#include <cmath>
#include <set>

namespace tsidx {

template <typename T>
void ThreadsafeCollection<T>::insert(T x) {
  std::unique_lock lock(mutex);
  collection.push_back(x);
}
  
template <typename T>
std::vector<T> ThreadsafeCollection<T>::get() {
  std::shared_lock lock(mutex);
  return std::vector<T>(collection);
}

template <typename T>
int ThreadsafeCollection<T>::size() {
  std::shared_lock lock(mutex);
  return collection.size();
}

 
TimeSeriesIndex::TimeSeriesIndex(int bucket_size,
				 int ts_bucket_size,
				 float band_percentage) {
  this -> bucket_size = bucket_size;
  this -> band_percentage = band_percentage;
  this -> ts_bucket_size = ts_bucket_size;  
  status = IDX_READY;
  root = NULL;
}
    
TimeSeriesIndex::~TimeSeriesIndex() {
  std::unique_lock lock(mutex);
  LOG(INFO) << "Deconstruct";
  delete_tree(root);  
  for(int i = 0; i < timeseries.size(); i++) {
    delete timeseries[i];      
  }
  for(int i = 0; i < buckets.size(); i++) {
    delete buckets[i];
  }
}
  
int TimeSeriesIndex::insert(TimeSeries& ts) {
  if(status == IDX_RUN) {
    LOG(INFO) << "Currently Indexing, No insertion";
    return IDX_RUN;
  }
  if(root == NULL && timeseries.size() == 0) {
    std::unique_lock lock(mutex);
    buckets.push_back(new tsidx::ThreadsafeCollection<int>());
    timeseries.push_back(new tsidx::ThreadsafeCollection<TimeSeries>());
  }
  if(timeseries.back() -> size() >= ts_bucket_size) {
    std::unique_lock lock(mutex);
    timeseries.push_back(new tsidx::ThreadsafeCollection<TimeSeries>());
  }
  
  std::shared_lock lock(mutex);
  if(root == NULL) {
    int n = ts_bucket_size * (timeseries.size() - 1) + timeseries.back() -> size();
    buckets[0] -> insert(n);
    ts.id = n;
    timeseries.back() -> insert(ts);
  } else {
    int node = search(ts, indexing_batch, root, band_percentage);      
    int bucket = leaf_map[node];
    int n = ts_bucket_size * (timeseries.size() - 1) + timeseries.back() -> size();
    buckets[bucket] -> insert(n);
    ts.id = n;
    timeseries.back() -> insert(ts);
  }
  return IDX_READY;  
}

int TimeSeriesIndex::search_idx(const TimeSeries& ts, std::vector<int>& nearest) {
  if(status == IDX_RUN || root == NULL) {
    LOG(INFO) << "Currently Indexing, No insertion";
    return IDX_RUN;
  }
  std::shared_lock lock(mutex);
  int node = search(ts, indexing_batch, root, band_percentage);
  int bucket = leaf_map[node];
  for(const auto& i : buckets[bucket] -> get()) {
    nearest.push_back(i);
  }
  return status;
}
 
int TimeSeriesIndex::reindex(int n_samples) {
  std::unique_lock lock(mutex);                                                               
  LOG(WARNING) << "Reindex";
  status = IDX_RUN;  

  LOG(INFO) << "Delete all";
  if(root != NULL) {
    LOG(INFO) << "root not null";
    delete_tree(root);
    leaf_map.clear();
    indexing_batch.clear();   
  }
  LOG(INFO) << "Delete buckets";
  for(int i = 0; i < buckets.size(); i++) {
    delete buckets[i];
  }
  buckets.clear();

  LOG(INFO) << "#ts_buckets: " << timeseries.size();
  int n = ts_bucket_size * (timeseries.size() - 1) + timeseries.back() -> size();
  
  LOG(INFO) << "Find Samples: " << n_samples << " / " << n;
  std::set<int> closed;
  for(int i = 0; i < n_samples; i++) {
    int id = rand() % n;
    while(closed.find(id) != closed.end()) {
      id = rand() % n;
    }
    closed.insert(id);
    int bucket = i / ts_bucket_size;
    int bucket_i = i % ts_bucket_size;
    indexing_batch.push_back(timeseries[bucket] -> get()[bucket_i]);
  }

  LOG(INFO) << "Build Tree: bucket = " << bucket_size << " batch = " << indexing_batch.size();
  root = new Node;
  build_tree(indexing_batch, bucket_size, band_percentage, root);  
  
  std::vector<int> leafs;
  leaf_nodes(root, "", leafs);
  LOG(INFO) << "n_buckets: " << leafs.size();
  for(int i = 0; i < leafs.size(); i++) {
    buckets.push_back(new tsidx::ThreadsafeCollection<int>());
    leaf_map[leafs[i]] = i;
  }

  // insert all
  int i = 0;
  for(const auto& bucket : timeseries) {
    TimeSeriesBatch sequences = bucket -> get();
    for(const auto& ts : sequences) {
      int node = search(ts, indexing_batch, root, band_percentage);      
      int bucket = leaf_map[node];
      buckets[bucket] -> insert(i);
      i++;
    }
  }
  i = 0;
  for(const auto& bucket : buckets) {
    i++;
  }
  status = IDX_READY;
  return IDX_READY;
}

template class ThreadsafeCollection<int>;
template class ThreadsafeCollection<TimeSeries>;
  
}
