#!/bin/bash

# 开启错误退出模式
# set -e

clang-format -style=file -i *.cc *.h

sudo chown $USER:$USER *.cc *.h
sudo chmod 666 *.cc *.h

if [ ! -d `pwd`/build ]; then
    mkdir `pwd`/build
fi

rm -rf `pwd`/build/*

cd `pwd`/build &&
    cmake .. &&
    make

cd ..

if [ ! -d /usr/include/mymuduo ]; then
    mkdir /usr/include/mymuduo
fi

for header in `ls *.h`
do
    cp $header /usr/include/mymuduo
done

cp `pwd`/lib/libmymuduo.so /usr/lib

ldconfig

