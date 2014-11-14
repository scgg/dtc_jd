#!/bin/sh
ulimit -c unlimited

report()
{
                numThreadWorker=`cat ../conf/agent.conf | grep ThreadNum | awk -F"=" '{print $2}'`
                numThreadAll=`ps -eL | grep dtcagent | wc -l`
                targetThreadNum=$(($numThreadWorker+4))
                if [ $targetThreadNum -eq $numThreadAll ] ; then
                        echo "num of worker thread is $numThreadWorker"
                        echo "dtcagent start success"
                else
                        echo "num of worker thread is $numThreadWorker"
                        echo "dtcagent start fail"
                fi
}

if [ "$1" = "stop" ]; then
        killall dtcagent
elif [ "$1" = "restart" ]; then
        killall dtcagent
        sleep 1
        ./dtcagent
        report
elif [ "$1" = "start" ]; then
        ./dtcagent
        report
else
        echo "usage : agent.sh stop | start | restart"
fi
