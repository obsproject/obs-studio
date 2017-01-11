#!/usr/bin/env bash

if [  $# -lt 2 ]
then
    echo "Need at least 2 arguments."
    echo "$0 InputVideo InputSRT [OutputFilename]"
    exit 1
fi

VIDEO=$1
SRT=$2

if [ -z "$3" ]
then
    OUTFILE="out.flv"
else
    OUTFILE="$3"
fi

echo "Video=$VIDEO"
echo "Captions=$SRT"
echo "Outfile=$OUTFILE"

# ffmpeg -i $VIDEO -acodec copy -vcodec copy -f flv - | ./flv+srt - $SRT - | ffmpeg -i - -acodec copy -vcodec copy $OUTFILE
ffmpeg -i $VIDEO -threads 0 -vcodec libx264 -profile:v main -preset:v medium \
-r 30 -g 60 -keyint_min 60 -sc_threshold 0 -b:v 4000k -maxrate 4000k \
-bufsize 4000k -filter:v scale="trunc(oh*a/2)*2:720" \
-sws_flags lanczos+accurate_rnd -strict -2 -acodec aac -b:a 96k -ar 48000 -ac 2 \
-f flv - | ./flv+srt - $SRT - | ffmpeg -i - -acodec copy -vcodec copy -y $OUTFILE
