#!/bin/sh
cd `dirname $0`
open-stage-control  -s localhost:18080 -l ./essorage.json
# --read-only
