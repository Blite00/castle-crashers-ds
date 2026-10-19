"""Create the DS menu icon from the original Blue Knight HUD portrait."""
from pathlib import Path
from PIL import Image
ROOT=Path(__file__).resolve().parents[1]
im=Image.open(ROOT/'assets/portraits/DefineSprite_167/3.png').convert('RGBA')
im=im.crop(im.getbbox());im.thumbnail((30,30),Image.Resampling.LANCZOS)
icon=Image.new('RGBA',(32,32));icon.paste(im,((32-im.width)//2,(32-im.height)//2))
q=icon.convert('RGB').quantize(colors=15,method=Image.Quantize.MEDIANCUT)
data=[0 if a<80 else v+1 for v,a in zip(q.getdata(),icon.getchannel('A').getdata())]
p=Image.new('P',(32,32));p.putpalette([0,0,0]+q.getpalette()[:45]+[0]*(768-48));p.putdata(data)
p.save(ROOT/'generated/game-icon.gif',transparency=0)
