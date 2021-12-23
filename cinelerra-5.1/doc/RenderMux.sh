#!/bin/bash
exec >& /tmp/render_mux.log
echo == $0 cin_render="$CIN_RENDER"
test -z "$CIN_RENDER" && exit 1
# Render output mux-ed into a single file
ffmpeg -f concat -safe 0 -i <(for f in "$CIN_RENDER"0*; do echo "file '$f'"; done) -c copy -y $CIN_RENDER 
