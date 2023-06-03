#include "Serialization.h"
#include "gtest/gtest.h"

void timeseries(tsidx::TimeSeries &ts) {
  ts.id = 0;
  ts.length = 5;
  ts.dim = 2;
  ts.x = new float[ts.dim * ts.length];  
  for(int i = 0; i < ts.length; i++) {
    for(int j = 0; j < ts.dim; j++) {
      ts.x[i * ts.dim + j] = i * ts.dim + j;
    }
  }
}

TEST(SerializationTest, n_bytes) {
  tsidx::TimeSeries ts;
  timeseries(ts);
  // header: [4 byte id, 4 byte length, 4 byte size]
  int header = 12;
  // dim * len * float = 10 * 4
  int size = 40;
  int expected = header + size;
  int actual = n_bytes(ts);
  ASSERT_EQ(actual, expected);
  delete[] ts.x;
}

TEST(SerializationTest, de_serialize) {
  tsidx::TimeSeries ts;
  tsidx::TimeSeries ts2;
  timeseries(ts);

  int n = n_bytes(ts);
  unsigned char* ts_ser = new unsigned char[n];
  int n_ser = serialize_ts(0, ts, ts_ser);
  int n_des = deserialize_ts(0, ts2, ts_ser);
  ASSERT_EQ(n_ser, n);
  ASSERT_EQ(n_des, n);

  ASSERT_EQ(ts.id, ts2.id);
  ASSERT_EQ(ts.length, ts2.length);
  ASSERT_EQ(ts.dim, ts2.dim);
  for(int i = 0; i < ts.dim * ts.length; i++) {
    ASSERT_EQ(ts.x[i], ts2.x[i]);
  }    
  delete[] ts2.x;
  delete[] ts.x;
  delete[] ts_ser;
}

