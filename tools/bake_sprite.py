"""Freeze a Flash sprite (and every nested sprite) at one frame so a static export shows that exact moment.

bake(definitions,identifier,frame,next_id) returns the identifier of a frozen clone appended to definitions."""
from copy import deepcopy
from prepare_frames import freeze

class Baker:
    def __init__(self,tags,first_id=2000):
        self.tags=tags;self.defs={int(t.get('spriteId')):t for t in tags if t.get('type')=='DefineSpriteTag'};self.next=first_id;self.cache={}
    def bake(self,ident,frame):
        if ident not in self.defs:return ident
        src=self.defs[ident];f=max(1,min(frame,int(src.get('frameCount'))))
        key=(ident,f)
        if key in self.cache:return self.cache[key]
        births={};now=1
        for t in src.find('subTags'):
            if t.get('type')=='ShowFrameTag':
                if now==f:break
                now+=1
            elif t.get('type','').startswith('Place') and t.get('placeFlagHasCharacter')=='true':births[t.get('depth')]=now
            elif t.get('type','').startswith('Remove'):births.pop(t.get('depth'),None)
        clone=deepcopy(src);freeze(clone,f);new=self.next;self.next+=1;clone.set('spriteId',str(new))
        self.cache[key]=new
        for t in clone.find('subTags'):
            if t.get('characterId'):
                child_frame=f-births.get(t.get('depth'),1)+1
                t.set('characterId',str(self.bake(int(t.get('characterId')),max(1,child_frame))))
        self.tags.append(clone);return new
