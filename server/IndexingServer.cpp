#include <grpc/grpc.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>

#include <glog/logging.h>

#include "IndexingTypes.h"
#include "TimeSeriesIndex.h"
#include "gen/indexing.grpc.pb.h"


using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerReaderWriter;
using grpc::ServerWriter;
using grpc::Status;


void copy(const TimeSeries *from, tsidx::TimeSeries &to) {
    int dim = from -> dim();
    int len = from -> length();
    to.id = -1;
    to.x = new float[dim * len];
    for(int i = 0; i < dim * len; i++) {
      to.x[i] = from -> ts(i);
    }
    to.dim = dim;
    to.length = len;    
}


class TimeSeriesServer final : public TimeSeriesService::Service {

public:
  TimeSeriesServer(int n_buckets, int bucket_size, float band_percentage) {
    idx = new tsidx::TimeSeriesIndex(n_buckets, bucket_size,
				     band_percentage);

  }

  ~TimeSeriesServer() {
    delete idx;
  }
  
  Status insert(ServerContext *context,
		const TimeSeries *ts,
		ReindexingResponse* response) override {
    tsidx::TimeSeries timeseries;
    LOG(INFO) << "INSERT SERVER sequence of length=" << ts->length()
	      << " and dim=" << ts->dim()
	      << " preview=" << ts->ts(0) << ","
      	      << " preview=" << ts->ts(1) << ","
      	      << " preview=" << ts->ts(2) << ","
      	      << " preview=" << ts->ts(3);

    copy(ts, timeseries);
    idx -> insert(timeseries);    
    return Status::OK;
  }
  
  Status query(ServerContext *context,
	       const TimeSeries *ts,
	       TimeSeriesIdx *idx) override {
    // TODO stub
    return Status::OK;
  }
  
  Status reindex(ServerContext *context,
		 const ReindexingRequest* request,
		 ReindexingResponse* response) override {
    // TODO stub
    return Status::OK;
  }

private:
  tsidx::TimeSeriesIndex *idx;
  
};


void run_server(int n_buckets, int bucket_size, float band_percentage) {
  std::string server_address("0.0.0.0:50051");
  TimeSeriesServer service(n_buckets, bucket_size, band_percentage);

  ServerBuilder builder;
  builder.AddListeningPort(server_address,
			   grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;
  server->Wait();
}


int main(int argc, char **argv) {
  run_server(100, 64, 0.75);
  return 0;
}
