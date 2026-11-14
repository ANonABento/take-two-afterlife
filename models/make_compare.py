from PIL import Image, ImageDraw, ImageFont
import numpy as np
REF="ref/option-a-tpose-turnaround.png"; OUT="final/compare.png"; P=512; LH=36
im=Image.open(REF).convert("RGB"); a=np.asarray(im).astype(int)
mask=np.abs(a-a[5,5]).sum(2)>40; col=mask.sum(0)>3
runs=[];s=None
for x,v in enumerate(col):
    if v and s is None:s=x
    if not v and s is not None:runs.append((s,x));s=None
if s is not None:runs.append((s,len(col)))
runs=sorted(runs,key=lambda r:r[1]-r[0],reverse=True)[:3]; runs.sort()
def panel(img):
    img=img.convert("RGB"); img.thumbnail((P,P)); c=Image.new("RGB",(P,P),(208,208,208))
    c.paste(img,((P-img.width)//2,(P-img.height)//2)); return c
refs=[]
for x0,x1 in runs:
    rows=np.where(mask[:,x0:x1].any(1))[0]; pad=12
    refs.append(panel(im.crop((max(0,x0-pad),max(0,rows[0]-pad),min(im.width,x1+pad),min(im.height,rows[-1]+pad)))))
rends=[panel(Image.open(f"final/render_{v}.png")) for v in ("front","side","back")]
try: f=ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",22)
except: f=ImageFont.load_default()
W=3*P; H=2*(P+LH); out=Image.new("RGB",(W,H),"white"); d=ImageDraw.Draw(out)
for r,(name,row) in enumerate((("Reference (option A)",refs),("Model render",rends))):
    for i,(v,p) in enumerate(zip(("front","side","back"),row)):
        y=r*(P+LH); out.paste(p,(i*P,y+LH)); d.text((i*P+10,y+6),f"{name} - {v}",fill="black",font=f)
out.save(OUT); print("compare.png size", out.size, "panels", len(refs)+len(rends))
