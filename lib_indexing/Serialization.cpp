#include "Serialization.h"

namespace tsidx {

  template <typename T>
  int write(int start, T value, unsigned char *buf) {
    unsigned char *ptr = (unsigned char *) &value;
    unsigned int i;
    for (i = 0; i < sizeof(T); i++) {
      buf[start + i] = ptr[i];
    }
    return start + i;
  }

  template <typename T>
  int read(int start, unsigned char *buf, T &val) {
    unsigned char *ptr = (unsigned char *)&val;
    unsigned int i;
    for (i = 0; i < sizeof(T); i++) {
      ptr[i] = buf[start + i];
    }
    return start + i;
  }
   
  // Header: [id|len|dim]
  const int HEADER = sizeof(int) * 3;
  
  int n_bytes(const TimeSeries& ts) {    
    int data_sze = ts.length * ts.dim * sizeof(float);
    return HEADER + data_sze;
  }
  
  int serialize_ts(int start,
		   const TimeSeries& ts, unsigned char* buffer) {
    start = write(start, ts.id, buffer);
    start = write(start, ts.length, buffer);
    start = write(start, ts.dim, buffer);
    for(int i = 0; i < ts.length * ts.dim; i++) {
      start = write(start, ts.x[i], buffer);
    }
    return start;
  }
  
  int deserialize_ts(int start,
		      TimeSeries& ts, unsigned char* buffer) {      

    start = read(start, buffer, ts.id);
    start = read(start, buffer, ts.length);
    start = read(start, buffer, ts.dim);
    ts.x = new float[ts.length * ts.dim];
    for(int i = 0; i < ts.length * ts.dim; i++) {
      start = read(start, buffer, ts.x[i]);
    }
    return start;
  }
  
}
