#!/bin/sh
# Diablo II for Miyoo / Onion. Does not change OS libraries or clocks.
set -u
d2_here=$(CDPATH= cd -- "$(dirname "$0")" && pwd) || exit 2
cd "$d2_here" || exit 2
d2_log="$d2_here/diablo2.log"
if [ -f "$d2_here/diablo2.previous.log" ]; then mv -f "$d2_here/diablo2.previous.log" "$d2_here/diablo2.previous-2.log"; fi
if [ -f "$d2_log" ]; then mv -f "$d2_log" "$d2_here/diablo2.previous.log"; fi
exec > "$d2_log" 2>&1
echo "Diablo II Miyoo $(cat VERSION)"
date
uname -a
cat /proc/meminfo
d2_missing=""
for d2_file in d2data.mpq d2exp.mpq d2char.mpq LODPatch_114d.exe; do
    if [ ! -s "data/$d2_file" ]; then d2_missing="$d2_missing $d2_file"; fi
done
if [ -n "$d2_missing" ]; then
    echo "Missing game files:$d2_missing"
    echo "Copy your Diablo II LoD files into Roms/PORTS/Games/DiabloII/data. See INSTALL.txt."
    if command -v infoPanel >/dev/null 2>&1; then
        infoPanel --title "DIABLO II" --message "Missing:$d2_missing. Copy game files into Roms/PORTS/Games/DiabloII/data. See INSTALL.txt."
    fi
    exit 2
fi
d2_available=$(awk '/^MemFree:/ {f=$2} /^Buffers:/ {b=$2} /^Cached:/ {c=$2} /^Shmem:/ {s=$2} /^MemAvailable:/ {a=$2} END {if(a>0) print a;else print f+b+c-s}' /proc/meminfo)
case "$d2_available" in ''|*[!0-9]*) echo "Cannot determine memory budget"; exit 2;; esac
d2_limit=$((d2_available - 8192))
[ "$d2_limit" -gt 98304 ] && d2_limit=98304
if [ "$d2_limit" -lt 49152 ]; then
    echo "Not enough free memory for Diablo II: ${d2_available} KiB"
    exit 2
fi
# Linux 4.9 also applies RLIMIT_DATA to private allocation mappings.
# Keep physical allocation budget separate from device mappings and stack reserve.
d2_data_limit=$((d2_limit - 8192))
d2_address_limit=$((d2_limit + 32768))
[ "$d2_address_limit" -gt 131072 ] && d2_address_limit=131072
echo "Launcher memory policy: 0.2.1-memory-fix"
echo "Available estimate=${d2_available} KiB; working budget=${d2_limit} KiB"
echo "Heap/data cap=${d2_data_limit} KiB; address-space cap=${d2_address_limit} KiB; 8192 KiB allowance for code/stacks"
ulimit -c 0
ulimit -d "$d2_data_limit" || exit 2
ulimit -v "$d2_address_limit" || exit 2
ulimit -a
mkdir -p state
chmod a+x ./diablo2-miyoo
export D2D_USER_DIR="$d2_here/state"
# Set D2D_AUDIO=0 before launch to keep the previous silent mode.
if [ "${D2D_AUDIO:-1}" = 0 ]; then
    export D2D_SILENT=1
else
    unset D2D_SILENT
    export D2D_LOOPBACK=1
    export D2D_AUDIO_HELPER="$d2_here/diablo2-audio"
fi
export D2D_SERIAL_LOADING=1
export D2_PATCH_INSTALLER="$d2_here/data/LODPatch_114d.exe"
# Original title and character selection. Saves stay inside this port state.
./diablo2-miyoo --miyoo --data "$d2_here/data" --no-video &
d2_pid=$!
trap 'kill -TERM "$d2_pid" 2>/dev/null; wait "$d2_pid"; exit 130' INT TERM
wait "$d2_pid"
d2_result=$?
trap - INT TERM
echo "Exit code: $d2_result"
if [ "$d2_result" -ne 0 ] && command -v infoPanel >/dev/null 2>&1; then
    infoPanel --title "DIABLO II" --message "Game stopped. Log: Roms/PORTS/Games/DiabloII/diablo2.log"
fi
exit "$d2_result"
