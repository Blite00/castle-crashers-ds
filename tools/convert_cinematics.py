"""Export owned LEVEL13 choreography using authored camera guides and selectors.

--prepare writes each source actor SWF XML and its static scenery atlas XML.
Export the CLIPS frame list from each name-actors SWF and frame1 of each
name-bg SWF through FFDec with ignorebackground. Then run without arguments.
Backgrounds are grouped in four scanlines; moving RGBA pixels use translated
patches in independently compressed sixteen-frame groups. Runtime reuses the
98,304-byte destination framebuffer, preserving random-seek/radio snapshots.
"""
from pathlib import Path
import argparse
import json
import xml.etree.ElementTree as E
from PIL import Image
from convert_barbarian import array
from prepare_frames import freeze
R=Path(__file__).resolve().parents[1]
W,H=192,144
SOURCE_WIDTH=848

def camera_track(root,sid,count):
    sprites={int(t.get('spriteId')):t for t in root.find('tags') if t.get('spriteId')}
    guide_id=392 if sid==264 else 360
    parent=(228.5,-403.5) if sid==264 else (3516.85,-313.8)
    track=[];frame=1;depth=None;xy=(0,0)
    for node in sprites[guide_id].find('subTags'):
        if node.get('name')=='guide':depth=node.get('depth')
        if depth and node.get('depth')==depth and node.find('matrix') is not None:
            m=node.find('matrix');xy=(int(m.get('translateX'))/20+parent[0],int(m.get('translateY'))/20+parent[1])
        if node.get('type')=='ShowFrameTag':
            # MAIN f_GetCameraCoords follows the authored guide; camera bottom
            # is guide_y+100. Quantize only the DS translation, never the path.
            track.append((round((xy[0]-424)*W/SOURCE_WIDTH),round((xy[1]+100-636)*W/SOURCE_WIDTH)));frame+=1
    return (track+[track[-1]]*count)[:count]
# Scene407 places movies in original source coordinates. The crystal reaches
# above the playable world atlas, so export the full source scene at a fixed
# cinematic camera instead of moving isolated characters over gameplay terrain.
CLIPS=[('keep-intro',264,391,(-300,-1000),list(range(2,366,2))),
       ('keep-princess',360,265,(3000,-650),list(range(4,236,2)))]

def prepare():
    for name,sid,frames,origin,_ in CLIPS:
        r=E.parse(R/'assets/extracted/level13.xml').getroot();tags=r.find('tags')
        by_id={int(t.get('spriteId')):t for t in tags if t.get('spriteId')}
        cameras=camera_track(r,sid,frames)
        animated=set()
        def descendants(child):
            if child in animated:return
            animated.add(child)
            if child in by_id:
                for node in by_id[child].find('subTags'):
                    if node.get('characterId'):descendants(int(node.get('characterId')))
        descendants(sid)
        for t in list(tags):
            if not t.get('type','').startswith('Define'):tags.remove(t)
            elif t.get('spriteId') and (int(t.get('spriteId')) in (4,5,263,268,286,389,391,392,275,278,284,360 if sid==264 else 264) or 394<=int(t.get('spriteId'))<=406):
                t.find('subTags').clear();E.SubElement(t.find('subTags'),'item',type='ShowFrameTag');t.set('frameCount','1')
            elif t.get('spriteId') in ('192','196','246'):
                freeze(t,13)
            elif t.get('spriteId') in ('292','295','313','316','338','347','351'):
                freeze(t,7)
            elif t.get('spriteId') in ('24','27','31','354','370'):
                # Original first-frame stop() selectors must not auto-cycle in
                # an offline renderer which does not execute ActionScript.
                freeze(t,1)
            elif t.get('spriteId') and int(t.get('spriteId')) not in animated:
                freeze(t,1)
        for i,(cx,cy) in enumerate(cameras):
            p=E.SubElement(tags,'item',type='PlaceObject2Tag',depth='1',placeFlagHasCharacter='true' if i==0 else 'false',placeFlagHasMatrix='true',placeFlagMove='false' if i==0 else 'true')
            if i==0:p.set('characterId','407')
            E.SubElement(p,'matrix',type='MATRIX',hasScale='true',scaleX=str(W*2/SOURCE_WIDTH),scaleY=str(W*2/SOURCE_WIDTH),hasRotate='false',translateX=str(-cx*40),translateY=str(-cy*40))
            E.SubElement(tags,'item',type='ShowFrameTag')
        r.set('frameCount',str(frames));rect=r.find('displayRect')
        for k,v in dict(Xmin=0,Ymin=0,Xmax=W*40,Ymax=H*40).items():rect.set(k,str(v))
        (R/f'assets/{name}-camera-track.json').write_text(json.dumps(cameras))
        E.ElementTree(r).write(R/f'assets/extracted/{name}.xml',encoding='utf-8',xml_declaration=True)
        from copy import deepcopy
        actor_root=deepcopy(r);at=actor_root.find('tags');px,py=(45,-698) if sid==264 else (3516.85,-313.8)
        for place in at:
            if place.get('type')=='PlaceObject2Tag':
                if place.get('characterId')=='407':place.set('characterId',str(sid))
                m=place.find('matrix')
                if m is not None:
                    m.set('translateX',str(round(int(m.get('translateX'))+px*W*40/SOURCE_WIDTH)))
                    m.set('translateY',str(round(int(m.get('translateY'))+py*W*40/SOURCE_WIDTH)))
        E.ElementTree(actor_root).write(R/f'assets/extracted/{name}-actors.xml',encoding='utf-8',xml_declaration=True)

        # Exactly the same scenery without the movie gives one reusable
        # backdrop. Moving-pixel deltas are derived AFTER compositing/resizing,
        # preventing detached limbs or silhouettes over a different camera.
        movie=next(t for t in tags if t.get('spriteId')==str(sid))
        movie.find('subTags').clear();E.SubElement(movie.find('subTags'),'item',type='ShowFrameTag');movie.set('frameCount','1')
        xs=[v[0] for v in cameras];ys=[v[1] for v in cameras];left=min(xs);top=min(ys);aw=max(xs)-left+W;ah=max(ys)-top+H
        for node in list(tags):
            if not node.get('type','').startswith('Define'):tags.remove(node)
        p=E.SubElement(tags,'item',type='PlaceObject2Tag',characterId='407',depth='1',placeFlagHasCharacter='true',placeFlagHasMatrix='true',placeFlagMove='false')
        E.SubElement(p,'matrix',type='MATRIX',hasScale='true',scaleX=str(W*2/SOURCE_WIDTH),scaleY=str(W*2/SOURCE_WIDTH),hasRotate='false',translateX=str(-left*40),translateY=str(-top*40))
        E.SubElement(tags,'item',type='ShowFrameTag');r.set('frameCount','1')
        for k,v in dict(Xmin=0,Ymin=0,Xmax=aw*40,Ymax=ah*40).items():rect.set(k,str(v))
        (R/f'assets/{name}-background-rect.json').write_text(json.dumps([left,top,aw,ah]))
        E.ElementTree(r).write(R/f'assets/extracted/{name}-bg.xml',encoding='utf-8',xml_declaration=True)

def princess_door_mask(cx,cy):
    """Original foreground102 is in front of neutral movie360 by source footY.

    The existing placement export uses source pixels plus origin(200,810).
    Apply the movie camera transform before quantizing its opaque silhouette.
    """
    if not hasattr(princess_door_mask,'alpha'):
        layers=json.loads((R/'assets/campaign-world-layers.json').read_text())
        index=next(i+1 for i,layer in enumerate(layers) if layer['shape']==102)
        princess_door_mask.alpha=Image.open(R/f'assets/campaign-world-fg/{index}.png').convert('RGBA').getchannel('A')
    return princess_door_mask.alpha.transform((W*2,H*2),Image.Transform.AFFINE,
        (SOURCE_WIDTH/(W*2),0,cx*SOURCE_WIDTH/W+200,
         0,SOURCE_WIDTH/(W*2),cy*SOURCE_WIDTH/W+810),Image.Resampling.BICUBIC).resize((W,H),Image.Resampling.LANCZOS)

def convert():
    import zlib,struct
    def compress(data):
        z=zlib.compressobj(9,zlib.DEFLATED,-15);return z.compress(data)+z.flush()
    def ds_palette(q):
        p=q.getpalette();return [0x8000|(p[i]>>3)|((p[i+1]>>3)<<5)|((p[i+2]>>3)<<10) for i in range(0,128*3,3)]
    alpha=[];palette=[];data=bytearray();offsets=[0];sizes=[];shifts=[];hashes=[];clip_offsets=[0]
    bgpalette=[];bgdata=bytearray();bgrows=[0];bgcliprows=[0];bgwidths=[];bgorigins=[];camerapos=[];debug=[]
    preview=Image.new('RGB',(256*8,192*6));max_patch=0;stats=[];door_occlusion=[]
    for kind,(name,sid,total,origin,frames) in enumerate(CLIPS):
        cams=json.loads((R/f'assets/{name}-camera-track.json').read_text())
        left,top,aw,ah=json.loads((R/f'assets/{name}-background-rect.json').read_text());bgorigins.extend([left,top])
        rawbg=Image.open(R/f'assets/{name}-atlas/1.png').convert('RGBA')
        sky=Image.open(R/'assets/campaign-sky-images/1.png').convert('RGBA').resize(rawbg.size,Image.Resampling.LANCZOS)
        composited=sky.copy();composited.alpha_composite(rawbg)
        bg=composited.convert('RGB').resize((aw,ah),Image.Resampling.LANCZOS)
        bgq=bg.quantize(colors=128,method=Image.Quantize.MEDIANCUT,dither=Image.Dither.NONE)
        bp=ds_palette(bgq);bgpalette+=bp;bgwidths.append(aw);debug.append({'bg':bgq.tobytes(),'aw':aw,'ah':ah,'frames':[]})
        for y in range(0,ah,4):bgdata.extend(compress(bgq.crop((0,y,aw,y+4)).tobytes()));bgrows.append(len(bgdata))
        bgcliprows.append(len(bgrows)-1)
        ims=[];masks=[];backgrounds=[]
        for f in frames:
            cx,cy=cams[f-1];x=cx-left;y=cy-top;camerapos.extend([x,y])
            raw=Image.open(R/f'assets/{name}-actors/{f}.png').convert('RGBA')
            fg=raw.resize((W,H),Image.Resampling.LANCZOS)
            mask=[v>8 for v in fg.getchannel('A').tobytes()]
            if kind==1:
                door=princess_door_mask(cx,cy).tobytes()
                hidden=sum(m and d>=128 for m,d in zip(mask,door))
                mask=[m and d<128 for m,d in zip(mask,door)]
                door_occlusion.append({'source_frame':f,'hidden_pixels':hidden})
            # Keep the actor palette unchanged; only source-occluded indices
            # become transparent, revealing the existing identical backdrop.
            ims.append(fg);masks.append(mask);backgrounds.append(bgq.crop((x,y,x+W,y+H)).tobytes())
        atlas=Image.new('RGBA',(W,H*len(ims)))
        for i,im in enumerate(ims):atlas.paste(im,(0,i*H))
        q=atlas.quantize(colors=128,method=Image.Quantize.FASTOCTREE,dither=Image.Dither.NONE)
        rgba=q.getpalette('RGBA');rawpal=[(0x8000|(rgba[j]>>3)|((rgba[j+1]>>3)<<5)|((rgba[j+2]>>3)<<10),rgba[j+3]) for j in range(0,128*4,4)]
        pal=list(dict.fromkeys(rawpal));remap=[pal.index(c) for c in rawpal];palette+=[0]+[c for c,a in pal]+[0x8000]*(255-len(pal));alpha+=[0]+[a for c,a in pal]+[255]*(255-len(pal))
        cams=json.loads((R/f'assets/{name}-camera-track.json').read_text());prev=None;before=len(data);movie_preview=[]
        for i,im in enumerate(ims):
            pixels=bytes(remap[v]+1 if m else 0 for v,m in zip(q.crop((0,i*H,W,(i+1)*H)).tobytes(),masks[i]))
            dx=dy=0
            if i%16==0:
                payload=pixels
            else:
                dx=cams[frames[i]-1][0]-cams[frames[i-1]-1][0];dy=cams[frames[i]-1][1]-cams[frames[i-1]-1][1]
                old=bytearray(W*H)
                for y in range(H):
                    if 0<=y+dy<H:
                        left=max(0,-dx);right=min(W,W-dx)
                        if right>left:old[y*W+left:y*W+right]=prev[(y+dy)*W+left+dx:(y+dy)*W+right+dx]
                payload=bytearray();pos=0
                while pos<W*H:
                    if pixels[pos]==old[pos]:pos+=1;continue
                    start=pos;last=pos;pos+=1
                    while pos<W*H:
                        if pixels[pos]!=old[pos]:last=pos
                        elif pos-last>4:break
                        pos+=1
                    end=last+1;payload+=struct.pack('<HH',start,end-start)+pixels[start:end];pos=end
                check=bytearray(old);j=0
                while j<len(payload):
                    at,n=struct.unpack_from('<HH',payload,j);j+=4;check[at:at+n]=payload[j:j+n];j+=n
                assert check==pixels
            debug[-1]['frames'].append((pixels,bytes(payload),dx,dy));encoded=compress(payload);assert zlib.decompress(encoded,-15)==payload
            data.extend(encoded);offsets.append(len(data));sizes.append(len(payload));shifts.extend([dx,dy]);max_patch=max(max_patch,len(payload));prev=pixels
            final=[]
            for p,b in zip(pixels,backgrounds[i]):
                color=bp[b]
                if p:
                    fc,a=pal[p-1];color=0x8000|sum((((fc>>shift)&31)*a+((color>>shift)&31)*(255-a)+127)//255<<shift for shift in (0,5,10))
                final.append(color)
            # MAINsprite42's black overlay starts atindex0. Camera392 frame15
            # callsf_FadeIn; each source tick eases toward255 and snaps when
            # abs(int((target-index)/7))<=10. Quantize only to15fps samples.
            brightness=255
            if kind==0:
                brightness=0
                for _ in range(max(0,frames[i]-15+1)):
                    diff=int((255-brightness)/7)
                    brightness=brightness+diff if abs(diff)>10 else 255
                if brightness!=255:
                    final=[0x8000|sum(((((c>>shift)&31)*brightness+127)//255)<<shift for shift in (0,5,10)) for c in final]
            digest=2166136261
            for y in range(192):
                for x in range(256):digest=((digest^final[((y*3+1)//4)*W+(x*3+1)//4])*16777619)&0xffffffff
            hashes.append(digest)
            proof=Image.new('RGB',(W,H));proof.putdata([((c&31)*255//31,((c>>5)&31)*255//31,((c>>10)&31)*255//31) for c in final]);movie_preview.append(proof.resize((256,192),Image.Resampling.NEAREST))
            if i in [round(j*(len(ims)-1)/23) for j in range(24)]:
                j=[round(j*(len(ims)-1)/23) for j in range(24)].index(i)
                baked=Image.new('RGB',(W,H));baked.putdata([((c&31)*255//31,((c>>5)&31)*255//31,((c>>10)&31)*255//31) for c in final])
                preview.paste(baked.resize((256,192),Image.Resampling.NEAREST),((j%8)*256,(kind*3+j//8)*192))
        movie_preview[0].save(R/f'generated/{name}-complete.gif',save_all=True,append_images=movie_preview[1:],duration=67,loop=0,disposal=2)
        stats.append(dict(name=name,frames=len(frames),compressed_bytes=len(data)-before,source_frames=frames));clip_offsets.append(len(sizes))
    import pickle
    (R/'build/cinema-prototype.pkl').write_bytes(pickle.dumps(debug))
    data=bytearray();offsets=[0];block_sizes=[];clip_blocks=[0]
    for kind,clip in enumerate(debug):
        before=len(data)
        for i in range(0,len(clip['frames']),16):
            payload=clip['frames'][i][0]+b''.join(f[1] for f in clip['frames'][i+1:i+16])
            assert len(payload)<=256*192*2
            encoded=compress(payload);assert zlib.decompress(encoded,-15)==payload
            data.extend(encoded);offsets.append(len(data));block_sizes.append(len(payload))
        stats[kind]['compressed_bytes']=len(data)-before;clip_blocks.append(len(block_sizes))
    hdr='/* Original complete movie choreography; lossless shifted temporal patches. */\n#include <stdint.h>\n'
    for n,t,v in [('cinema_bg_origins','int16_t',bgorigins),('cinema_bg_palette','uint16_t',bgpalette),('cinema_bg_rows','uint32_t',bgrows),('cinema_bg_clip_rows','uint16_t',bgcliprows),('cinema_bg_widths','uint16_t',bgwidths),('cinema_bg_pixels','uint8_t',bgdata),('cinema_camera','uint16_t',camerapos),('cinema_alpha','uint8_t',alpha),('cinema_palette','uint16_t',palette),('cinema_clip_offsets','uint16_t',clip_offsets),('cinema_clip_blocks','uint16_t',clip_blocks),('cinema_block_sizes','uint32_t',block_sizes),('cinema_offsets','uint32_t',offsets),('cinema_sizes','uint16_t',sizes),('cinema_shifts','int16_t',shifts),('cinema_pixels','uint8_t',data)]:hdr+=array(n,t,v)
    hdr+=f'#define CINEMA_KEY_INTERVAL 16u\n#define CINEMA_BG_ROWS 4u\n#define CINEMA_BG_MAX_WIDTH {max(bgwidths)}u\n'
    (R/'generated/cinematic_art.h').write_text(hdr)
    (R/'generated/cinematic_test_hashes.h').write_text('#include <stdint.h>\n'+array('cinema_expected_hashes','uint32_t',hashes)+array('cinema_test_clip_offsets','uint16_t',clip_offsets))
    preview.save(R/'generated/cinematic-preview.png')
    audit=dict(source='Owned LEVEL13 scene407 movies264/360, camera guides392/360',clips=stats,sample_size=[W,H],colors=128,fps=15,opening_king_hold_ticks=0,bytes=len(bgorigins)*2+len(clip_blocks)*2+len(block_sizes)*4+len(alpha)+len(bgdata)+len(bgpalette)*2+len(bgrows)*4+len(bgcliprows)*2+len(bgwidths)*2+len(camerapos)*2+len(data)+len(palette)*2+len(clip_offsets)*2+len(offsets)*4+len(sizes)*2+len(shifts)*2,background_bytes=len(bgdata),max_patch=max_patch,max_block=max(block_sizes),key_interval=16,codec='rawDEFLATE16-frame groups with translated RGBA-index patches; framebuffer-only scratch',camera_adaptation='Original animated guide path, source viewport848x636 mapped to192x144 indexed samples; camera bottom=guide_y+100. No inserted close-up.',source_events={'intro_visual_fadein':15,'crystal_force':172,'crystal_fly_music_fadeout':347,'crystal_music_stop':359,'intro_players_camera_release':366,'intro_field_music':374,'princess_scream':22,'princess_help':90,'princess_players_camera_release':236,'princess_goal_hud_complete':265},source_paths=['assets/extracted/level13.xml sprite392 camera timeline','assets/level13-scripts/scripts/DefineSprite_264','assets/level13-scripts/scripts/DefineSprite_360','assets/main-scripts/scripts/frame_2/DoAction.as f_GetCameraCoords/f_ResetCamera','assets/main-scripts/scripts/DefineSprite_42/frame_1/PlaceObject2_41_1/CLIPACTIONRECORD onClipEvent(enterFrame).as'],selectors={'192':13,'196':13,'246':13,'292':7,'295':7,'313':7,'316':7,'338':7,'347':7,'351':7})
    audit['princess_door_occlusion']={'shape':102,'movie':360,'source_foot_y':{'door':-251.55,'movie':-313.8},'mask':'Original placement alpha, camera transformed, coverage threshold128','frames':door_occlusion}
    (R/'generated/cinematic-audit.json').write_text(json.dumps(audit,indent=2));print(json.dumps({k:v for k,v in audit.items() if k!='clips'}));print([(c['name'],c['compressed_bytes']) for c in stats])

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--prepare',action='store_true');a=p.parse_args()
    prepare() if a.prepare else convert()


# Refresh chapter views/binaries after regenerating a source payload.
if __name__ == '__main__':
    from pack_chapter_assets import build as _refresh_chapter_banks
    _refresh_chapter_banks()
