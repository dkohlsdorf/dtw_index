#pragma once

#include "IndexingTypes.h"

namespace tsidx {

  int n_bytes(const TimeSeries& ts);
  
  int serialize_ts(int start, const TimeSeries& ts, unsigned char* buffer);

  int deserialize_ts(int start, TimeSeries& ts, unsigned char* buffer);
  
}
