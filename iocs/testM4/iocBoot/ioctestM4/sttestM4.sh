#!/bin/sh
# This file was automatically generated on Thu 26 May 2016 16:08:02 BST from
# source: /home/hgv27681/R3.14.12.3/support/smaractEcm/etc/makeIocs/testM4.xml
# 
# *** Please do not edit this file: edit the source file instead. ***
# 
cd "$(dirname "$0")"
if [ -n "$1" ]; then
    export EPICS_CA_SERVER_PORT="$(($1))"
    export EPICS_CA_REPEATER_PORT="$(($1 + 1))"
    [ $EPICS_CA_SERVER_PORT -gt 0 ] || {
        echo "First argument '$1' should be a integer greater than 0"
        exit 1
    }
fi
exec ./testM4 sttestM4.boot