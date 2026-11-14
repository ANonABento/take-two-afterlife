import json,struct,glob,os,sys
def info(p):
    b=open(p,'rb').read(); n=struct.unpack('<I',b[12:16])[0]; j=json.loads(b[20:20+n])
    tris=0;uvs=set()
    for m in j['meshes']:
        for pr in m['primitives']:
            a=pr['attributes']; uvs|={k for k in a if k.startswith('TEXCOORD')}
            tris+=(j['accessors'][pr['indices']]['count'] if 'indices' in pr else j['accessors'][a['POSITION']]['count'])//3
    return tris,sorted(uvs),[i.get('name','?') for i in j.get('images',[])],[m.get('name') for m in j.get('materials',[])]
ok=True
fs=['final/angel.glb']+sorted(f for f in glob.glob('hats/*/*.glb') if 'tripo' not in f)
for f in fs:
    t,u,i,m=info(f); lim=10000 if 'angel' in f else 3000
    good=t<=lim and len(u)==1 and len(i)>=1 and ('angel' not in f or len(m)==3)
    ok&=good; print(os.path.basename(f),'tris',t,'uv',u,'imgs',i,'mats',m,'OK' if good else 'FAIL')
print('ALL OK' if ok else 'SOME FAIL')
