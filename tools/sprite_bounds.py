"""Union bounds (source pixels) of a Flash sprite over every frame, to place FFDec sprite frame exports.
FFDec crops each exported frame to the sprite's union bounds, so frame image (0,0) sits at (xmin,ymin) of the sprite."""
import xml.etree.ElementTree as ET,sys
def load(path):
    r=ET.parse(path).getroot();tags=r.find('tags');return {t.get('spriteId') or t.get('shapeId') or t.get('characterID'):t for t in tags if t.get('type','').startswith('Define')}
def mat(t):
    m=t.find('matrix')
    if m is None:return (1,0,0,1,0,0)
    f=lambda k,d:float(m.get(k,d))
    sx=f('scaleX',1) if m.get('hasScale')=='true' else 1;sy=f('scaleY',1) if m.get('hasScale')=='true' else 1
    k0=f('rotateSkew0',0) if m.get('hasRotate')=='true' else 0;k1=f('rotateSkew1',0) if m.get('hasRotate')=='true' else 0
    return (sx,k0,k1,sy,f('translateX',0)/20,f('translateY',0)/20)
def apply(m,x,y):return (m[0]*x+m[2]*y+m[4],m[1]*x+m[3]*y+m[5])
cache={}
def bounds(defs,cid,depth=0):
    if cid in cache:return cache[cid]
    d=defs.get(cid);res=None
    if d is None:return None
    if d.get('type')=='DefineSpriteTag':
        box=None
        for t in d.find('subTags'):
            if t.get('type','').startswith('PlaceObject') and t.get('placeFlagHasCharacter')=='true':
                b=bounds(defs,t.get('characterId'),depth+1)
                if b is None:continue
                m=mat(t);pts=[apply(m,b[0],b[1]),apply(m,b[2],b[1]),apply(m,b[0],b[3]),apply(m,b[2],b[3])]
                bx=(min(p[0] for p in pts),min(p[1] for p in pts),max(p[0] for p in pts),max(p[1] for p in pts))
                box=bx if box is None else (min(box[0],bx[0]),min(box[1],bx[1]),max(box[2],bx[2]),max(box[3],bx[3]))
        res=box
    else:
        sb=d.find('shapeBounds')
        if sb is not None:res=(int(sb.get('Xmin'))/20,int(sb.get('Ymin'))/20,int(sb.get('Xmax'))/20,int(sb.get('Ymax'))/20)
    cache[cid]=res;return res
if __name__=='__main__':
    defs=load(sys.argv[1])
    for cid in sys.argv[2:]:print(cid,bounds(defs,cid))
