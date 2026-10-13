#!/bin/sh
# .specs/2026-10-13_ping-pong-imitation-learning.md
# Symbolic links into ArNI. Nothing is copied.
set -e
HERE=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
ARNI=$(CDPATH= cd -- "$HERE/../../../ArNI/Workplace" && pwd)
cd "$HERE"
for name in ArNIGPU ArNI_GA ArNICPU SuperArNI; do
  if [ -e "$ARNI/$name" ]; then
    ln -sfn "$ARNI/$name" "$name"
  fi
done
for f in "$ARNI"/*.so "$ARNI"/*.dll; do
  if [ -e "$f" ]; then
    ln -sfn "$f" "$(basename "$f")"
  fi
done
if [ -e "$HERE/../Experiments/ping-pong-environment.so" ]; then
  ln -sfn "$HERE/../Experiments/ping-pong-environment.so" ping-pong-environment.so
fi
if [ -e "$HERE/../Experiments/ping-pong-environment.dll" ]; then
  ln -sfn "$HERE/../Experiments/ping-pong-environment.dll" ping-pong-environment.dll
fi
ln -sfn "$HERE/../Experiments/ImitationPingPong.inc" ArNI_OPT.inc
echo "Workplace links are in $HERE"
echo "Point ArNI/ArNI_OPT.inc at Experiments/ImitationPingPong.inc and rebuild SuperArNI before the full GA."
