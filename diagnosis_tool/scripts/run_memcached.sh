#!/bin/sh

MEMSLAP_BIN_DIR=/home/mejbah/projects/lockperf/memcached-1.4.4/libmemcached/bin
MEMCACHED_BIN_DIR=/home/mejbah/projects/lockperf/memcached-1.4.4/build_dir/bin

MEMCACHED_ARGS="-t 16"

INPUT=./bins/memcached/inputs

echo /home/mejbah/pin/pin-2.14-71313-gcc.4.4.7-linux/intel64/bin/pinbin -t sync_track.so -threads 1024 -i $INPUT -- ${MEMCACHED_BIN_DIR}/memcached $MEMCACHED_ARGS 
time /home/mejbah/pin/pin-2.14-71313-gcc.4.4.7-linux/intel64/bin/pinbin -t sync_track.so -threads 1024 -i $INPUT -- ${MEMCACHED_BIN_DIR}/memcached $MEMCACHED_ARGS &
sleep 10

${MEMSLAP_BIN_DIR}/memslap --concurrency=16  --servers=127.0.0.1 

echo done

sleep 5

#need to kll memcached from terminal kill -2 memcached
#SIGINT 2
#kill -9 `ps -aef | grep memcached | grep -v grep | awk '{print $2}'`
#wait

