#!/bin/sh
cd `dirname $0`
open-stage-control  -s localhost:18080 -t ./theme-contrast.css -l ./teleco-OSC.json
# --read-only
