#!/bin/bash
path=`pwd`
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$path
cd $path
./dtccrontab
