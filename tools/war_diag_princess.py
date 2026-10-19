from PIL import Image,ImageDraw
s=Image.new('RGB',(1000,300),(40,40,40));d=ImageDraw.Draw(s)
for i,n in enumerate([1,5,9,13,16]):
 im=Image.open(f'assets/boss-prop-carried/{n}.png').convert('RGBA');print(n,im.size,im.getbbox());im.thumbnail((200,270));s.paste(im,(i*200,0),im);d.text((i*200,280),str(n),fill='white')
s.save('build/war_diag_princess.png')
