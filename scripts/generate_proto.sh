#!/usr/bin/env bash

rm -rf server/gen
mkdir server/gen
./build/_deps/grpc-build/third_party/protobuf/protoc -I=protos/ --cpp_out=server/gen/ protos/indexing.proto
./build/_deps/grpc-build/third_party/protobuf/protoc -I=protos/ --grpc_out=server/gen/ --plugin=protoc-gen-grpc=/usr/bin/grpc_cpp_plugin protos/indexing.proto
