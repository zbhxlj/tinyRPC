export CC=/usr/bin/clang
export CXX=/usr/bin/clang++
GLOG_log_dir=../log

cd build || exit 1
cmake ..
make -j16
