import sys
sys.path.append('gen')


import numpy as np
import grpc
import indexing_pb2
import indexing_pb2_grpc

DATA = "../data/gunpoint.tsv"

def keogh():
    x = np.genfromtxt(DATA)    
    return x[:, 1:]

def timeseries(ts):
    if len(ts.shape) == 1:
        length = ts.shape[0]
        dim = 1
    else:
        length, dim = ts.shape
    repeated = list(ts.flatten())
    return indexing_pb2.TimeSeries(ts=repeated, dim=dim, length=length)

def run():
    X = keogh()
    with grpc.insecure_channel('localhost:50051') as channel:
        stub = indexing_pb2_grpc.TimeSeriesServiceStub(channel)
        for i in range(0, len(X)):
            ts = timeseries(X[i])
            response = stub.insert(ts)
            print(f"Insert Response: status = {response.status} id = {response.ts_id}")
        request = indexing_pb2.ReindexingRequest(n_samples = 150)
        response = stub.reindex(request)
        print(f"Reindexing Response: status = {response.status} id = {response.ts_id}")
        query = timeseries(X[0])
        response = stub.query(query)
        print(f"Query Response: status = {response.status}")
        for neighbor in response.ids:
            print(f"\t ... neighbor: {neighbor}")

        
if __name__ == "__main__":
    run()
