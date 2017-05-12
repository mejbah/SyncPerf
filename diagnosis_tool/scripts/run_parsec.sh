#!/bin/sh

APP_NAME=$1
NCORES=16
DATASET_HOME=/home/mejbah/projects/lockperf/multithreadingtests/datasets
APP_HOME=/home/mejbah/projects/lockperf/multithreadingtests/tests

case $APP_NAME in
	"canneal" )
		THREADS=`expr $NCORES - 1`
		APP_ARGS="$THREADS 15000 2000 $DATASET_HOME/canneal/2500000.nets 6000"
		APP_BIN=./bins/$APP_NAME/$APP_NAME-lockperf
		INPUT=./bins/$APP_NAME/inputs
		;;
	"fluidanimate" )
		#THREADS=$(NCORES)
		APP_ARGS=" $NCORES 500 $DATASET_HOME/fluidanimate/in_500K.fluid out.fluid"
		APP_BIN=./bins/$APP_NAME/$APP_NAME-lockperf
		INPUT=./bins/$APP_NAME/inputs
		;;
	"dedup" )
		THREADS=4
		APP_ARGS=" -c -p -f -t $THREADS -i $DATASET_HOME/dedup/FC-6-x86_64-disc1.iso -o output.dat.ddp"
		APP_BIN=./bins/$APP_NAME/$APP_NAME-lockperf
		INPUT=./bins/$APP_NAME/inputs
		;;
	"facesim" )
		APP_ARGS="-timing -threads $NCORES -lastframe 100"
		APP_BIN=./bins/$APP_NAME/$APP_NAME-lockperf
		INPUT=./bins/$APP_NAME/inputs
		;;

	

esac


#/home/mejbah/pin/pin-2.14-71313-gcc.4.4.7-linux/pin.sh -t sync_track.so -threads 4 -i ./bins/input  -- ./bins/test
#/home/mejbah/pin/pin-2.14-71313-gcc.4.4.7-linux/intel64/bin/pinbin -t sync_track.so -threads 4 -i ./bins/input  -- ./bins/test
echo /home/mejbah/pin/pin-2.14-71313-gcc.4.4.7-linux/intel64/bin/pinbin -t sync_track.so -threads $NCORES -i ./bins/input --  $APP_BIN $APP_ARGS
time /home/mejbah/pin/pin-2.14-71313-gcc.4.4.7-linux/intel64/bin/pinbin -t sync_track.so -threads 1024 -i $INPUT --  $APP_BIN $APP_ARGS
