#Generates base color palette image for the game
#Use Python 2.7.x to run this script
#Requires the pillow (PIL) library. install it via: pip install pillow
#
#Palette format: [BSRRGGBB] where VV: 00 = darkest, 11 = lightest
#Output pixels: RGB: [V*R,V*G,V*B]
#

from PIL import Image

img = Image.new("RGB",(256,1))

def repeat2(chanval):
    t = chanval & 0xC0
    return (t|(t>>2)|(t>>4)|(t>>6))&0b00111111
def mixer(chval,brval):
    v = brval&0b11000000
    v = (v>>2)
    return v|(chval&0b11000000)

for i in range(256):
    '''
    v = ((i & 0b11000000) >> 6) + 1
    r = repeat2(i<<2) * v
    g = repeat2(i<<4) * v
    b = repeat2(i<<6) * v
    '''
    v = i
    r = mixer(i<<2,v)
    g = mixer(i<<4,v)
    b = mixer(i<<6,v)
    
    
    
    
    
    print [r,g,b,v]
    img.putpixel((i,0),(r,g,b))
    
img.save("mainpalette.png")



