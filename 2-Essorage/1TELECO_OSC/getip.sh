#!/bin/sh
# usage: getip.sh RTN_PORT IFACE

ipadd=`ifconfig $2 | grep -Eo 'inet (ad*r:)?([0-9]*\.){3}[0-9]*' | grep -Eo '([0-9]*\.){3}[0-9]*' | grep -v '127.0.0.1'`
echo $ipadd ";" | pdsend $1
#ifconfig $2 | grep -Eo 'inet (ad*r:)?([0-9]*\.){3}[0-9]*' | grep -Eo '([0-9]*\.){3}[0-9]*' | grep -v '127.0.0.1'
