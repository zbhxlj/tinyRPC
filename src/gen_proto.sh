#!/usr/bin/env bash

cd /home/zbh/project//tinyRPC/src/rpc/protocol/protobuf
protoc rpc_meta.proto --cpp_out=.
cd -

cd /home/zbh/project//tinyRPC/src/testing
protoc echo_service.proto --cpp_out=.
cd -
