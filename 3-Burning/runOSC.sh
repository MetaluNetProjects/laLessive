#!/bin/sh
cd `dirname $0`

while true; do
	open-stage-control  -s localhost:18080 -l ./essorage.json
	sleep 5
done

