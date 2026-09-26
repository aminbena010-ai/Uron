#!/bin/bash
set -e
cd "$(dirname "$0")"
for f in *.vert *.frag; do
    [ -f "$f" ] || continue
    glslc "$f" -o "$f.spv"
    echo "Compilado: $f -> $f.spv"
done