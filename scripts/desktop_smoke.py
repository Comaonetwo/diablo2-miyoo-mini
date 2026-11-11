#!/usr/bin/env python3
"""Local integration test. Uses original images read-only and a new test hero."""
import json
import os
from pathlib import Path
import re
import socket
import subprocess
import time

root = Path(__file__).resolve().parents[1]
runtime = root / "work/runtime"
user = runtime / "smoke-user"
user.mkdir(exist_ok=True)
name = "Mini" + "".join(chr(65 + int(c)) for c in str(os.getpid()))
sock = f"/tmp/d2mm-{os.getpid()}.sock"
env = dict(os.environ, D2D_USER_DIR=str(user), D2_PATCH_INSTALLER=str(Path(os.environ["D2_PATCH_INSTALLER"])))
binary = Path(os.environ.get("D2_TEST_BINARY", root / "work/build/probe/d2-desktop"))
data_dir = Path(os.environ.get("D2_TEST_DATA", runtime))
log = open(root / "work/desktop-smoke.log", "w")
result = {"name": name, "tests": [], "human_playthrough": False, "mini_runtime_test": False}

def cmd(line):
    with socket.socket(socket.AF_UNIX) as client:
        client.settimeout(30)
        client.connect(sock)
        client.sendall((line + "\n").encode())
        out = ""
        while not re.search(r"(?:^|\n)(?:ok|err)(?: |\n)", out):
            chunk = client.recv(65536)
            if not chunk:
                raise RuntimeError(f"closed control connection: {line}")
            out += chunk.decode()
        if re.search(r"(?:^|\n)err(?: |\n)", out):
            raise RuntimeError(f"{line}: {out}")
        return out

def state():
    return dict(re.findall(r"(\w+)=(.*?)(?= \w+=|$)", cmd("state").splitlines()[0]))

def frames(n=3):
    goal = int(cmd("info").split("frame=")[1].split()[0]) + n
    until = time.monotonic() + 30
    while int(cmd("info").split("frame=")[1].split()[0]) < goal:
        if time.monotonic() > until:
            raise RuntimeError("frame loop stalled")
        time.sleep(.02)

proc = None
def start(screen):
    global proc
    proc = subprocess.Popen([str(binary), "--headless", "--no-video", "--data", str(data_dir),
                             "--devctl", sock, "--seed", "3", "--start-screen", screen,
                             "--start-name", name, "--start-class", "4"], env=env, stdout=log, stderr=log)
    until = time.monotonic() + 120
    while not Path(sock).exists():
        if proc.poll() is not None:
            raise RuntimeError(f"engine exited: {proc.returncode}")
        if time.monotonic() > until:
            raise RuntimeError("control channel did not start")
        time.sleep(.1)
    # The socket is bound before the heavy scene load; wait for first frame.
    cmd("ping")
    frames()

try:
    start("charcreate")
    assert state()["screen"] == "charcreate"
    cmd("click 690 555"); frames(10)
    created = state()
    assert created["screen"] == "ingame" and created["name"] == name and created["level"] == "1", created
    result["tests"].append("create_expansion_hero_and_enter_town")
    result["town_memory_categories"] = cmd("memory").strip()
    result["initial_state"] = created
    result["town_rss_kib"] = int(subprocess.check_output(["ps", "-o", "rss=", "-p", str(proc.pid)]).strip())
    level = cmd("debug level").split()
    assert level[1] == "1", level
    x, y = map(float, level[2:4])
    target = None
    for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
        if cmd(f"debug blocked {x+dx} {y+dy}").strip() == "ok 0":
            target = x + dx, y + dy
            break
    assert target, "No adjacent free test position"
    cmd(f"cmd move {target[0]} {target[1]}"); frames(40)
    moved = state()
    assert moved["cam"] != created["cam"], moved
    result["tests"].append("normal_world_move_command_changes_position")
    cmd("key I"); frames()
    assert state()["inv"] == "1"
    cmd("key I"); frames()
    result["tests"].append("inventory_panel_opens_and_closes")
    for key in ("C", "T", "Q"):
        cmd("key " + key); frames(5)
        cmd("key " + key); frames(5)
    result["tests"].append("deferred_character_skill_and_quest_panels_render_and_close")
    items = cmd("items")
    result["starting_item_rows"] = len(items.splitlines()) - 1
    cmd("save")
    save = user / "save" / (name + ".d2s")
    assert save.is_file() and save.stat().st_size > 0x2FD
    result["save_bytes"] = save.stat().st_size
    cmd("quit"); assert proc.wait(timeout=15) == 0
    start("charselect")
    cmd("click 690 555"); frames(10)
    loaded = state()
    assert loaded["screen"] == "ingame" and loaded["name"] == name, loaded
    assert loaded["gold"] == moved["gold"] and loaded["level"] == moved["level"]
    assert cmd("items") == items
    result["reload_rss_kib"] = int(subprocess.check_output(["ps", "-o", "rss=", "-p", str(proc.pid)]).strip())
    result["tests"].append("restart_load_preserves_starting_items_gold_and_level")
    result["loaded_state"] = loaded
    # Resource probe only: test-only level jump is explicitly not a playthrough.
    cmd("debug goto 2"); frames(20)
    assert cmd("debug level").split()[1] == "2"
    result["blood_moor_rss_kib"] = int(subprocess.check_output(["ps", "-o", "rss=", "-p", str(proc.pid)]).strip())
    result["blood_moor_monsters"] = len(cmd("monsters").splitlines()) - 1
    result["blood_moor_memory_categories"] = cmd("memory").strip()
    if os.environ.get("D2_TEST_COMBAT"):
        before_exp = int(state()["exp"])
        start_monsters = [r.split("\t") for r in cmd("monsters").splitlines()[:-1]]
        assert start_monsters, "No monsters for combat test"
        target_id = start_monsters[0][11].removeprefix("#")
        # The level-jump spawn and first monster can be separated by a wall.
        # Arrange a nearby free fight location; this is test setup, not campaign travel.
        first = start_monsters[0]
        for dx, dy in ((2, 0), (-2, 0), (0, 2), (0, -2)):
            fx, fy = float(first[1]) + dx, float(first[2]) + dy
            if cmd(f"debug blocked {fx} {fy}").strip() == "ok 0":
                cmd(f"debug warp {fx} {fy}"); frames(5)
                result["combat_setup"] = "debug level jump and nearby free position; ordinary starting stats and skills"
                break
        else:
            raise RuntimeError("No free nearby combat setup position")
        killed = False
        for attempt in range(50):
            rows = [r.split("\t") for r in cmd("monsters").splitlines()[:-1]]
            target = next((r for r in rows if r[11] == "#" + target_id and int(r[5].split("/")[0]) > 0), None)
            if target is None:
                killed = True; break
            # Starting Sorceress's Fire Bolt, ordinary game command, no debug damage/stat changes.
            cmd(f"cmd skill 36 {target[1]} {target[2]} {target_id} 0"); frames(25)
            if state()["life"].startswith("0/"):
                break
        result["combat_killed_target"] = killed
        result["combat_exp_delta"] = int(state()["exp"]) - before_exp
        result["combat_rss_kib"] = int(subprocess.check_output(["ps", "-o", "rss=", "-p", str(proc.pid)]).strip())
        result["combat_memory_categories"] = cmd("memory").strip()
        result["combat_final_state"] = state()
        result["combat_final_monsters"] = cmd("monsters")
        if os.environ.get("D2_TEST_SHOT"):
            cmd("screenshot " + os.environ["D2_TEST_SHOT"])
        assert killed and result["combat_exp_delta"] > 0, "Normal Fire Bolt combat did not kill target"
        result["tests"].append("normal_fire_bolt_kill_and_experience_no_debug_damage")
        drops = [r.split("\t") for r in cmd("ground").splitlines()[:-1]]
        result["combat_drop_count"] = len(drops)
        if drops:
            drop_id = drops[0][4].removeprefix("#")
            cmd("cmd pickup " + drop_id); frames(100)
            remaining = cmd("ground")
            assert not any(r.split("\t")[4] == "#" + drop_id for r in remaining.splitlines()[:-1]), "Pickup did not remove ground item"
            result["tests"].append("normal_pickup_of_combat_drop")
    if os.environ.get("D2_TEST_SHOT"):
        cmd("screenshot " + os.environ["D2_TEST_SHOT"])
    result["tests"].append("blood_moor_load_via_debug_jump_not_campaign_progress")
    cmd("key Escape"); frames()
    assert state()["gamemenu"].startswith("1:"), state()
    cmd("key Up"); frames()
    cmd("key Return"); frames(10)
    assert state()["screen"] == "charselect", state()
    if os.environ.get("D2_TEST_SHOT"):
        cmd("screenshot " + str(Path(os.environ["D2_TEST_SHOT"]).with_name("charselect-restored.png")))
    result["tests"].append("game_menu_exit_restores_lazy_character_selection_graphics")
    cmd("quit"); assert proc.wait(timeout=15) == 0
    result["result"] = "PASS"
finally:
    if proc is not None and proc.poll() is None:
        proc.terminate()
        try: proc.wait(timeout=10)
        except subprocess.TimeoutExpired: proc.kill(); proc.wait()
    log.close()
    Path(sock).unlink(missing_ok=True)
    (root / "work/desktop-smoke-result.json").write_text(json.dumps(result, indent=2) + "\n")
print(json.dumps(result, indent=2))
