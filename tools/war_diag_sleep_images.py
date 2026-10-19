from PIL import Image,ImageDraw
from pathlib import Path
s=Image.new('RGB',(800,220),(50,50,50));d=ImageDraw.Draw(s)
for i,n in enumerate([1,16,31,46]):
 im=Image.open(f'assets/war-live-sleep/{n}.png').convert('RGBA');b=im.getbbox();im=im.crop(b);im.thumbnail((190,180));s.paste(im,(i*200,15),im);d.text((i*200,200),str(n),fill='white')
s.save('build/war_diag_sleep.png')
