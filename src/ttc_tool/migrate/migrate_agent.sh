#!/bin/sh

ulimit -c unlimited
MIGRATE_TOOL_BIN=migrate_agent

if [ "$1" = "stop" ] ; then
    killall -2 $MIGRATE_TOOL_BIN
elif [ "$1" = "restart" ]; then
    killall -2 $MIGRATE_TOOL_BIN
    ./$MIGRATE_TOOL_BIN
elif [ "$1" = "start" ]; then
    ./$MIGRATE_TOOL_BIN
else
    echo "usage: migrate_agent.sh start|stop|restart"
fi

