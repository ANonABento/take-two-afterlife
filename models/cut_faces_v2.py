# Cut 8 faces from option-c-faces.png into 512x512 RGBA decals, eye centres aligned.
import numpy as np, os, sys
from PIL import Image
from scipy import ndimage as nd
src = sys.argv[1] if len(sys.argv) > 1 else 'option-c-faces.png'
out = sys.argv[2] if len(sys.argv) > 2 else 'models/angel-v2/faces'
os.makedirs(out, exist_ok=True)
NAMES = ['Smug','Shocked','KO','Angry','Grimace','Happy','Dizzy','Wink']  # row-major
R = [(71,423),(526,883)]; C = [(29,380),(406,754),(778,1128),(1154,1502)]
TARGET = (256, 224)  # eye midpoint in output
SEP = 144  # eye-centre spacing in output px
a = np.array(Image.open(src).convert('RGB')).astype(np.float32)
res = {}
for i, name in enumerate(NAMES):
    y0,y1 = R[i//4]; x0,x1 = C[i%4]
    cell = a[y0:y1, x0:x1]; g = cell.mean(2)
    d = g < 110
    lab, n = nd.label(d, np.ones((3,3)))
    objs = nd.find_objects(lab); sz = nd.sum(d, lab, range(1,n+1))
    H, W = g.shape
    keep = np.zeros_like(d)
    eyes = {0: [], 1: []}
    for k,(o,s) in enumerate(zip(objs, sz), 1):
        h = o[0].stop-o[0].start; w = o[1].stop-o[1].start
        if h > 0.8*H or w > 0.8*W: continue  # panel frame
        if s < 30: continue
        keep |= lab == k
        cy = (o[0].start+o[0].stop)/2; cx = (o[1].start+o[1].stop)/2
        if s > 300 and 100 < cy < 215:
            eyes[int(cx > W/2)].append((cy, cx))
    # eye = lowest candidate per side (brows sit above eyes)
    L = max(eyes[0]); Rr = max(eyes[1])
    # Wink: closed lid sits low; use the open (left) eye's height as the eye line
    my = L[0] if name == 'Wink' else (L[0]+Rr[0])/2
    mx = (L[1]+Rr[1])/2
    sc = SEP / (Rr[1]-L[1])  # uniform scale so eye spacing matches across faces
    # alpha from darkness, limited to kept strokes (dilated for antialias)
    mask = nd.binary_dilation(keep, iterations=2)
    alpha = np.clip((235 - g) / (235 - 60), 0, 1) * mask
    rgba = np.zeros((H, W, 4), np.uint8)
    rgba[...,:3] = 20; rgba[...,3] = (alpha*255).astype(np.uint8)
    canvas = Image.new('RGBA', (512,512), (0,0,0,0))
    img = Image.fromarray(rgba).resize((round(W*sc), round(H*sc)), Image.LANCZOS)
    ox = int(round(TARGET[0]-mx*sc)); oy = int(round(TARGET[1]-my*sc))
    canvas.alpha_composite(img, (max(ox,0), max(oy,0))) if ox>=0 and oy>=0 else canvas.paste(img,(ox,oy))
    p = f'{out}/T_Face_{name}.png'; canvas.save(p)
    ey = my*sc+oy
    res[name] = (L[1]*sc+ox, ey if name=='Wink' else L[0]*sc+oy, Rr[1]*sc+ox, ey if name=='Wink' else Rr[0]*sc+oy)
# verify
pts = []
for name in NAMES:
    im = Image.open(f'{out}/T_Face_{name}.png'); A = np.array(im)[...,3]
    lx,ly,rx,ry = res[name]; pts.append(res[name])
    print(f'T_Face_{name}.png {im.size} {im.mode} alpha_min={A.min()} alpha_max={A.max()} opaque%={100*(A>0).mean():.1f} eyesL=({lx:.0f},{ly:.0f}) R=({rx:.0f},{ry:.0f})')
P = np.array(pts); spread = (P.max(0)-P.min(0))
print('files', len(NAMES), 'eye spread px (Lx,Ly,Rx,Ry):', spread.round(1), 'max', spread.max().round(1), 'PASS' if spread.max() <= 8 else 'FAIL')
