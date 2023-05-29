#include "TimeSeriesIndex.h"
#include <vector>
#include <cmath>
#include <glog/logging.h>
#include <chrono>
#include <thread>
#include <fstream>

using std::chrono::system_clock;
using std::chrono::duration_cast;
using std::chrono::duration;
using std::chrono::milliseconds;

const std::string GUNPOINT_DATA = "../../data/gunpoint.tsv";
const int GUNPOINT_LEN = 150;

void gunpoint(tsidx::TimeSeriesBatch& data, std::vector<int>& labels) {
  std::fstream data_file;
  data_file.open(GUNPOINT_DATA, std::ios::in); 
  std::string line;
  std::string cell;
  while (std::getline(data_file, line)) { 
    std::stringstream token_stream(line);
    int i = 0;
    tsidx::TimeSeries ts;
    ts.length = GUNPOINT_LEN;
    ts.dim = 1;
    ts.x = new float[ts.length];
    while(std::getline(token_stream, cell, '\t')) {
      if(i > 0) {
	float val = atof(cell.c_str());
	ts.x[i - 1] = val; 
      } else {
	int label = atoi(cell.c_str());
	labels.push_back(label);
      }
      i++;
    }
    data.push_back(ts);
  }       
  data_file.close(); 
}

void random_inserts(int n, int n_buckets, std::vector<int> &op) {
  for(int i = 0; i < n; i++) {
    int operation = (rand() % n_buckets);
    if(rand() % 2 != 0) operation *= -1;
    op.push_back(operation);
  }  
}

void sequentialIndex(const std::vector<int> &op, int n_buckets) {
  std::vector<tsidx::Bucket> buckets;
  for(int i = 0; i < n_buckets; i++) {
    tsidx::Bucket b;
    buckets.push_back(b);
  }
  auto t1 = system_clock::now();
  for(const auto& operation : op) {
    if(operation > 0) {
      buckets[operation].push_back(operation);
    } else {
      std::vector<int> x(buckets[-operation]);
    }
  }  
  auto t2 = system_clock::now();
  duration<double, std::milli> ms_double = t2 - t1;
  LOG(INFO) << "Sequential time: " << ms_double.count() << " [ms]\n";
}

void parallelIndex(const std::vector<int> &op, int n_buckets, int n_threads) {
  LOG(INFO) << "Threading Experiment: ";
  std::vector<tsidx::ThreadsafeCollection<int>*> buckets;
  for(int i = 0; i < n_buckets; i++) {
    buckets.push_back(new tsidx::ThreadsafeCollection<int>());
  }
  
  std::vector<std::vector<int>> op_;
  for(int i = 0; i < n_threads; i++) {
    std::vector<int> v;        
    op_.push_back(v);
  }
  int round_robin = 0;
  for(int i = 0; i < op.size(); i++) {
    op_[round_robin].push_back(op[i]);
    round_robin++;
    if(round_robin > op_.size()) {
      round_robin = 0;
    }
  }
  
  auto write_insert = [&buckets, &op_](int thread) {    
    auto t1 = system_clock::now();
    for (int i = 0; i < op_[thread].size() ; i += 1) {
      if(op_[thread][i] > 0) {
	buckets[op_[thread][i]] -> insert(op_[thread][i]);
      } else {
	buckets[-op_[thread][i]] -> get();
      }
    }
    auto t2 = system_clock::now();
    duration<double, std::milli> ms_double = t2 - t1;
    LOG(INFO) << "\tThread time[" << thread << "]: " << ms_double.count() << " [ms]\n";    
  };

  auto t1 = system_clock::now();
  std::vector<std::thread*> threads;
  for(int i = 0; i < n_threads; i++) {
    threads.push_back(new std::thread(write_insert, i));
  }
  for(int i = 0; i < n_threads; i++) {
    threads[i] -> join();
  }
  auto t2 = system_clock::now();
  duration<double, std::milli> ms_double = t2 - t1;
  LOG(INFO) << "Parallel time: " << ms_double.count() << " [ms]\n";
}

void readFromIndexTest(tsidx::TimeSeriesBatch &data,
		       std::vector<int> &labels,
		       tsidx::TimeSeriesIndex& idx,
		       std::vector<int>& correctness,
		       std::vector<int>& attempted,
		       int start,
		       int stop,
		       int thread) {

    auto t1 = system_clock::now();
    int correct = 0;  
    for (int i = start; i < stop; i += 1) {
      std::vector<int> result;
      int status = idx.search_idx(data[i], result);
      int n_one = 0;
      for(const auto& label : result) {
	if(labels[label] == 1) n_one++;
      }
      int expected = labels[i];
      if(expected == 1 && n_one > (result.size() - n_one)) correct++;
      if(expected == 2 && n_one <= (result.size() - n_one)) correct++;
      attempted[thread]++;
    }
    auto t2 = system_clock::now();
    duration<double, std::milli> ms_double = t2 - t1;
    float acc = (float) correct / (stop - start);
    LOG(INFO) << "\tThread time[" << start << "]: " << ms_double.count() << " [ms]\n";    
    LOG(INFO) << "Accuracy: " << acc << " = " << correct << " / "  << stop - start;
    correctness[thread] = correct;
}

void experimentGunpointSpeedParallelSearch(int n_threads, int n_samples) {
  tsidx::TimeSeriesIndex idx(1, 10, 0.1);
  tsidx::TimeSeriesBatch data;
  std::vector<int> labels;  
  gunpoint(data, labels);
  
  LOG(INFO) << "Inserting sequences";
  for(const auto& ts : data) {
    idx.insert(ts);
  }
  LOG(INFO) << "Reindexing start";
  int status = idx.reindex(n_samples);
  LOG(INFO) << "Reindexing status: " << status;
  std::vector<int> correctness(n_threads + 1);
  std::vector<int> attempted(n_threads + 1);
  auto read = [&data, &labels,
	       &idx, &correctness, &attempted](int start, int stop, int thread) {
    readFromIndexTest(data, labels, idx, correctness, attempted, start, stop, thread);
  };
    
  auto t1 = system_clock::now();
  std::vector<std::thread*> threads;
  int size = data.size() / n_threads;
  int start = 0;
  for(int i = 0; i < n_threads; i++) {
    int stop = start + size;
    threads.push_back(new std::thread(read, start, stop, i));
    start = stop;
  }
  if(data.size() - start > 0) {
    threads.push_back(new std::thread(read, start, data.size(), n_threads));
  }
  int total_correct = 0;
  int total = 0;
  for(int i = 0; i < n_threads + 1; i++) {
    if(i < threads.size()) {
      threads[i] -> join();
      total_correct += correctness[i];
      total += attempted[i];
    }
  }
  auto t2 = system_clock::now();
  
  float acc = (float) total_correct / total;
  duration<double, std::milli> ms_double = t2 - t1;
  LOG(INFO) << "Parallel time: "
	    << ms_double.count() << " [ms] for "
	    << n_threads << " thread";
  LOG(INFO) << "Accuracy: " << acc << " = " << total_correct << " / "  << total;
}

void experimentThreadSafetyBuild(int n_samples) {
  tsidx::TimeSeriesIndex idx(1, 10, 0.1);
  tsidx::TimeSeriesBatch data;
  std::vector<int> labels;  
  gunpoint(data, labels);
  
  LOG(INFO) << "Inserting sequences";
  for(const auto& ts : data) {
    idx.insert(ts);
  }
  
  auto build = [&data, &idx, &n_samples]() {
    idx.reindex(n_samples);    
  };

  auto read = [&data, &idx]() {
    for(const auto& ts : data) {
      std::vector<int> result;
      int status = idx.search_idx(ts, result);
      if(status == 1) LOG(INFO) << "Status RUN";
    }    
  };
  std::thread ri(build);
  std::thread r(read);

  ri.join();
  r.join();
}

void experimentThreadSafetyReadWrite(int n_threads, int n_samples) {
  tsidx::TimeSeriesIndex idx(1, 10, 0.1);
  tsidx::TimeSeriesBatch data;
  std::vector<int> labels;  
  gunpoint(data, labels);
  for(const auto& ts : data) {
    idx.insert(ts);
  }

  idx.reindex(n_samples);
  std::vector<int> correctness(n_threads + 1);
  std::vector<int> attempted(n_threads + 1);
  auto read = [&data, &labels,
	       &idx, &correctness, &attempted](int start, int stop, int thread) {
    readFromIndexTest(data, labels, idx, correctness, attempted, start, stop, thread);
  };
  auto write = [&data, &idx](int start, int stop) {
    for(int i = start; i < stop; i++) {
      std::vector<int> result;
      int status = idx.insert(data[i]);
      if(status == 1) LOG(INFO) << "Status RUN";
    }    
  };

  std::vector<std::thread*> threads;
  std::vector<std::thread*> threads_write;
  int size = data.size() / n_threads;
  int start = 0;

  auto t1 = system_clock::now();
  for(int i = 0; i < n_threads; i++) {    
    int stop = start + size;
    threads.push_back(new std::thread(read, start, stop, i));
    threads_write.push_back(new std::thread(write, start, stop));
    start = stop;
  }
  if(data.size() - start > 0) {
    threads.push_back(new std::thread(read, start, data.size(), n_threads));
  }
  int total_correct = 0;
  int total = 0;
  for(int i = 0; i < n_threads + 1; i++) {
    if(i < threads_write.size()) {      
      threads_write[i] -> join();
    }
    if(i < threads.size()) {
      threads[i] -> join();
      total_correct += correctness[i];
      total += attempted[i];
    }
  }
  auto t2 = system_clock::now();
  
  float acc = (float) total_correct / total;
  duration<double, std::milli> ms_double = t2 - t1;
  LOG(INFO) << "Parallel time: "
	    << ms_double.count() << " [ms] for "
	    << n_threads << " thread";
  LOG(INFO) << "Accuracy: " << acc << " = " << total_correct << " / "  << total;
}

int main(int argc, char **argv) { 
  // Test threads and such
  experimentThreadSafetyBuild(100); 
  experimentThreadSafetyReadWrite(100, 75); 
  experimentThreadSafetyReadWrite(10, 75); 
  experimentThreadSafetyReadWrite(1, 75);
  // Index constructed and Parallel Search Test
  experimentGunpointSpeedParallelSearch(10, 75);
  experimentGunpointSpeedParallelSearch(1, 75);
  // Basic Threadsafe Collection Tests
  std::vector<int> ops;
  random_inserts(150000, 50, ops);
  sequentialIndex(ops, 50);
  parallelIndex(ops, 50, 10);
  return 0;  
}
