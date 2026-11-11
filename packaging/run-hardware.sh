#!/bin/sh
set -u
d2_here=$(CDPATH= cd -- "$(dirname "$0")" && pwd) || exit 2
cd "$d2_here" || exit 2
exec > "$d2_here/test-hardware.log" 2>&1
echo "Diablo II Miyoo hardware test $(cat VERSION)"
date
cat /proc/meminfo
ulimit -c 0
ulimit -v 49152 || exit 2
chmod a+x ./diablo2-hardware
./diablo2-hardware "$d2_here/data" "$d2_here/data/LODPatch_114d.exe" &
d2_pid=$!
trap 'kill -TERM "$d2_pid" 2>/dev/null; wait "$d2_pid"; exit 130' INT TERM
wait "$d2_pid"
d2_result=$?
trap - INT TERM
echo "Exit code: $d2_result"
if [ "$d2_result" -ne 0 ] && command -v infoPanel >/dev/null 2>&1; then
    infoPanel --title "Diablo II Screen Test" --message "Test stopped. Log: Roms/PORTS/Games/DiabloIIResearch/test-hardware.log"
fi
exit "$d2_result"
