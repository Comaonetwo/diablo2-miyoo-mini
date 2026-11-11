"""Read RGB/RGBA PNG pixels for screenshot assertions (stdlib only)."""
import struct,zlib
from pathlib import Path
def read_png(path):
 b=Path(path).read_bytes();assert b[:8]==b'\x89PNG\r\n\x1a\n';at=8;parts=[]
 while at<len(b):
  n=struct.unpack_from('>I',b,at)[0];tag=b[at+4:at+8];data=b[at+8:at+8+n];at+=12+n
  if tag==b'IHDR':w,h,depth,colour,_,_,interlace=struct.unpack('>IIBBBBB',data)
  if tag==b'IDAT':parts.append(data)
 assert depth==8 and colour in (2,6) and interlace==0
 channels=3 if colour==2 else 4;stride=w*channels;raw=zlib.decompress(b''.join(parts));pixels=[];prev=bytearray(stride)
 for y in range(h):
  f=raw[y*(stride+1)];row=bytearray(raw[y*(stride+1)+1:(y+1)*(stride+1)])
  for x in range(stride):
   a=row[x-channels] if x>=channels else 0;b=prev[x];c=prev[x-channels] if x>=channels else 0
   if f==1:v=a
   elif f==2:v=b
   elif f==3:v=(a+b)//2
   elif f==4:
    p=a+b-c;pa,pb,pc=abs(p-a),abs(p-b),abs(p-c);v=a if pa<=pb and pa<=pc else b if pb<=pc else c
   else:assert f==0;v=0
   row[x]=(row[x]+v)&255
  pixels.append(row);prev=row
 return w,h,channels,pixels
