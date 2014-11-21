#!/bin/bash
if test $( pgrep -f dtcagent | wc -l ) -lt 2
then
	killall dtcagent > /dev/null 2>&1
	/export/servers/agent/agent_bin/dtcagent > /dev/null 2>&1
fi
