#!/usr/bin/env python3
"""Prepare only the owner's required MPQs for a private console test package."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil

parser=argparse.ArgumentParser()
parser.add_argument("discs",type=Path)
parser.add_argument("patch",type=Path)
parser.add_argument("output",type=Path)
parser.add_argument("--with-audio",action="store_true")
parser.add_argument("--music-only",action="store_true")
args=parser.parse_args()
root=Path(__file__).resolve().parents[1]
out=args.output.resolve()
if not (out.is_relative_to(root/"work") or out.is_relative_to(root/"outputs")):
    parser.error("Output must stay inside this project's work/ or outputs/; SD-card writes are not supported")
if out.exists() and any(out.iterdir()):
    parser.error("Output is not empty; choose a new version directory")
needed={"d2data.mpq","d2exp.mpq","d2char.mpq"}
if args.with_audio: needed.update(("d2sfx.mpq","d2speech.mpq","d2xtalk.mpq"))
if args.music_only: needed={"d2music.mpq","d2xmusic.mpq"}
entries={}
source_states={}

for image in sorted(args.discs.rglob("*")):
    if image.suffix.lower() not in (".img",".iso") or not image.is_file():continue
    source_states[image]=(image.stat().st_size,image.stat().st_mtime_ns)
    with image.open("rb") as src:
        layout=None
        for stride,payload in ((2048,0),(2352,16),(2352,24)):
            src.seek(16*stride+payload);pvd=src.read(2048)
            if len(pvd)==2048 and pvd[:7]==b"\x01CD001\x01":layout=(stride,payload,pvd);break
        if not layout:raise ValueError(f"Unsupported image: {image.name}")
        stride,payload,pvd=layout
        def chunks(lba,size):
            while size:
                count=min(size,128*2048);sectors=(count+2047)//2048
                src.seek(lba*stride+payload);raw=src.read((sectors-1)*stride+2048)
                if len(raw)!=(sectors-1)*stride+2048:raise ValueError("Truncated image extent")
                data=b"".join(raw[s*stride:s*stride+min(2048,count-s*2048)] for s in range(sectors))
                yield data;lba+=sectors;size-=count
        visited=set()
        def walk(lba,size,depth=0):
            if depth>8 or lba in visited:return
            if size>16*1024*1024:raise ValueError("Unexpected ISO directory size")
            visited.add(lba);data=b"".join(chunks(lba,size));pos=0
            while pos<len(data):
                n=data[pos]
                if not n:pos=(pos//2048+1)*2048;continue
                record=data[pos:pos+n];pos+=n
                if len(record)<34 or 33+record[32]>len(record):raise ValueError("Invalid ISO directory entry")
                name=record[33:33+record[32]]
                if name in (b"\0",b"\1"):continue
                name=name.decode("ascii").split(";")[0].lower()
                extent=int.from_bytes(record[2:6],"little");length=int.from_bytes(record[10:14],"little")
                if record[25]&2:walk(extent,length,depth+1)
                elif name in needed:
                    if name in entries:raise ValueError(f"Ambiguous duplicate {name}")
                    entries[name]=(image,stride,payload,extent,length)
        r=pvd[156:190];walk(int.from_bytes(r[2:6],"little"),int.from_bytes(r[10:14],"little"))
if needed!=entries.keys():raise ValueError(f"Missing MPQs: {needed-entries.keys()}")
out.mkdir(parents=True,exist_ok=True)
manifest={"private_owner_data":True,"files":[]}
for name,(image,stride,payload,lba,length) in entries.items():
    temp=out/(name+".part");sha=hashlib.sha256();left=length
    with image.open("rb") as src,temp.open("xb") as dst:
        while left:
            count=min(left,128*2048);sectors=(count+2047)//2048
            src.seek(lba*stride+payload);raw=src.read((sectors-1)*stride+2048)
            if len(raw)!=(sectors-1)*stride+2048:raise ValueError("Truncated archive source")
            data=b"".join(raw[s*stride:s*stride+min(2048,count-s*2048)] for s in range(sectors))
            dst.write(data);sha.update(data);left-=count;lba+=sectors
    temp.rename(out/name)
    manifest["files"].append({"name":name,"bytes":length,"sha256":sha.hexdigest(),"source_image":image.name})
    print(name,length,sha.hexdigest(),flush=True)
patch_name="LODPatch_114d.exe"
with args.patch.open("rb") as src,(out/patch_name).open("xb") as dst:
    sha=hashlib.sha256()
    while True:
        data=src.read(1024*1024)
        if not data:break
        dst.write(data);sha.update(data)
manifest["files"].append({"name":patch_name,"bytes":(out/patch_name).stat().st_size,"sha256":sha.hexdigest()})
for path,state in source_states.items():
    if (path.stat().st_size,path.stat().st_mtime_ns)!=state:raise RuntimeError("Source image changed during read")
(out/"OWNER-ASSETS.json").write_text(json.dumps(manifest,indent=2)+"\n")
print("Private assets prepared; source image sizes and modification times unchanged")
