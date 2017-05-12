#!/bin/sh

MYSQL_BIN=/home/mejbah/projects/lockperf/mysql/build_dir/libexec/mysqld
MYSQL_DIR=/home/mejbah/projects/lockperf/mysql/build_dir
MYSQL_ARGS="--basedir=$MYSQL_DIR --datadir=$MYSQL_DIR/data --pid-file=$MYSQL_DIR/data/mysql.pid --log-error=$MYSQL_DIR/data/mysqld.err"

INPUT=./bins/mysql/inputs

#./build_dir/libexec/mysqld --basedir=./build_dir --datadir=./build_dir/data --pid-file=./build_dir/data/mysql.pid --log-error=./build_dir/data/mysqld.err

echo /home/mejbah/pin/pin-2.14-71313-gcc.4.4.7-linux/intel64/bin/pinbin -t sync_track.so -threads 1024 -i $INPUT -- ${MYSQL_BIN} $MYSQL_ARGS 
/home/mejbah/pin/pin-2.14-71313-gcc.4.4.7-linux/intel64/bin/pinbin -mt -t sync_track.so -threads 1024 -i $INPUT -- ${MYSQL_BIN} $MYSQL_ARGS 


#${MEMSLAP_BIN_DIR}/memslap --concurrency=16  --servers=127.0.0.1 



#need to kll memcached from terminal kill -2 memcached
#SIGINT 2
#kill -9 `ps -aef | grep memcached | grep -v grep | awk '{print $2}'`
#wait

