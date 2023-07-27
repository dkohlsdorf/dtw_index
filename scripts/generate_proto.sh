#!/usr/bin/env bash

rm -rf server/gen
mkdir server/gen


# OLD WAY
# ./build/_deps/grpc-build/third_party/protobuf/protoc -I=protos/ --cpp_out=server/gen/ protos/indexing.proto
# ./build/_deps/grpc-build/third_party/protobuf/protoc -I=protos/ --grpc_out=server/gen/ --plugin=protoc-gen-grpc=/usr/bin/grpc_cpp_plugin protos/indexing.proto

protoc -I=protos/ --cpp_out=server/gen/ protos/indexing.proto
protoc -I=protos/ --grpc_out=server/gen/ --plugin=protoc-gen-grpc=/usr/local/bin/grpc_cpp_plugin protos/indexing.proto

rm -rf scripts/gen
mkdir scripts/gen
touch scripts/gen/__init__.py

python -m grpc_tools.protoc -I=protos/ --grpc_python_out=scripts/gen --python_out=scripts/gen protos/indexing.proto

