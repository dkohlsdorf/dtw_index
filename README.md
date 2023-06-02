# Time Series Indexing

Builds a time series index by recursively partitioning the dataset
by picking two far away candidates and grouping all sequences around those.
The distance is measured using the dynamic time warping.

## Testing
### Unitttests
I tested the base functions in IndexingUtil with unit tests
this includes mainly the tree construction and dtw code. Some
of the tests rely on randomness so those are a little shaky.
Actually most of them are not real unit tests but more a small test run.

### Performance testing
Mainly the actual index. I am testing the speedup of parallel computation
as well as the thread safty. The performance tests can be executed
using the ./benchmark tool. I suggest to disable logging for actual measurments.
I am also using this to test for memory leaks or parallelisation issues. Great bianry to run valgrind

```
valgrind -v --leak-check=full --show-leak-kinds=all --log-file=vg_benchmark_20230601.log ./benchmark
``` 

```
valgrind -tool=helgrind  --log-file=hg_benchmark_20230601_threads.log ./benchmark
```

```
cmake .. -D CMAKE_BUILD_TYPE=Debug
```

```
GLOG_minlevel=1 GLOG_log_dir=. ./benchmark
```

## Compile

```
$ mkdir build; cd build
$ cmake ..
$ make
$ make test
```

## Requirements
+ [1] gtest
+ [2] glog
+ [3] gflags
