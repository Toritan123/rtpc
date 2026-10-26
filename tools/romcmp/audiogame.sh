#!/bin/zsh
# Record one game's sound on PC (RTPC_AUTO=4) and in mGBA with the same
# input, then compare them with audiocmp.py.
#   tools/romcmp/audiogame.sh WORKDIR GAME[=0xADDR] SECONDS
set -e
here=${0:A:h}; root=${here:h:h}
work=$1; spec=$2; secs=$3
game=${spec%%=*}
if [[ $spec == *=* ]]; then addr=${spec#*=}; else addr=$(python3 $here/scene_addrs.py | awk -v g=$game '$1==g{print $2}'); fi
mkdir -p $work/$game; cd $root
[[ -x $work/mgba_ref ]] || cc -arch x86_64 -I/usr/local/include tools/mgba_ref.c -L/usr/local/lib -lmgba -o $work/mgba_ref
RTPC_SAVE=/dev/null RTPC_HEADLESS=1 RTPC_SCENE=$game RTPC_AUTO=4 RTPC_KEYLOG=$work/$game/akeys.txt \
  RTPC_AUDIO_DUMP=$work/$game/pc.wav ./build/macos/rhythmtengoku_pc > $work/$game/apc.log 2>&1 &
pid=$!; sleep $secs; kill $pid
frames=$(( secs * 60 + 200 ))
MGBA_REF_AUDIO=$work/$game/rom.wav $work/mgba_ref ~/Downloads/'2462 - Rhythm Tengoku (J)(WRG).gba' $addr $frames 100000 $work/$game $work/$game/akeys.txt > /dev/null 2>&1
python3 $here/audiocmp.py $work/$game/pc.wav $work/$game/rom.wav
