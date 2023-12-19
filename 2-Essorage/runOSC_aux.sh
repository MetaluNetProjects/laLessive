#!/bin/sh
cd `dirname $0`
open-stage-control  -p 8181 -s localhost:18181 -l ./essorage.json
# --read-only
