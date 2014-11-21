#!/bin/sh
#create by foryzhou
#2010-12-02 11:25:18

make tar -C ../src/ttc_tool/migrate
mkdir ../dist
cp ../src/ttc_tool/migrate/*.tgz ../dist
