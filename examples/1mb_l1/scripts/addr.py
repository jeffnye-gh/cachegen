#! /usr/bin/env python3

def get_nibbles(byte_address,num):
  nibbles = []
  for j in range(num,0,-1):
    i = (j-1) * 4
    nibbles.append((byte_address >> i) & 0xF)
  return nibbles

def show_nibbles(ary):
  for i in ary:
    print('{0:#0{1}x}'.format(i,3))

def get_binary(ary,bits):
  ret = ''
  for i in ary:
    p = '{0:0{1}b}'.format(i,4)
    ret += p + ' '
  return ret
# ---------------------------------------------------------------------
# {0:#0{1}x}
#
# { Format identifier
#   0:    first parameter
#   #     use "0x" prefix
#   0     fill with zeroes
#   {1}   to a length of n characters (including 0x), 
#         defined by the second parameter
# x       as a hexadecimal number, using lowercase letters for a-f
# } End of format identifier
# ---------------------------------------------------------------------
#
#              tttt tttt tttt ttii iiii iiii iiiw wwbb
#
# ---------------------------------------------------------------------
def run(tag,idx,wrd,byt):

  # byte address is formed and then unpacks, for some cross checking 
  # not strictly necessary
  byte_address  = (tag << 18) + (idx << 5) + (wrd << 2)
  line_address  = byte_address >> 5
  index_address = (byte_address >> 5)  & 0x1FFF
  word_address  = (byte_address >> 2)  & 0x7
  tag_address   = (byte_address >> 18) &  0x3FFF

  # FIXME: digits/bits can be figured out with code

  # 8 hex digits 32 bits
  as_binary = get_binary(get_nibbles(byte_address,8),32)
  print('byte address  {0:#0{1}x}  ::  '.format(byte_address,8+2),end=' ')
  print(as_binary)

  # 7 hex digits 27 bits
  as_binary = get_binary(get_nibbles(line_address,7),27)
  print('line address  {0:#0{1}x}  ::  '.format(line_address,7+2),end=' ')
  print(as_binary)

  #4 hex digits, 14 bits
  as_binary = get_binary(get_nibbles(tag_address,4),14)
  print('tag  address  {0:#0{1}x}  ::  '.format(tag_address,4+2),end=' ')
  print(as_binary)

  #4 hex digits, 13 bits
  as_binary = get_binary(get_nibbles(index_address,4),13)
  print('index address  {0:#0{1}x}  ::  '.format(index_address,4+2),end=' ')
  print(as_binary)

  #1 hex digits, 3 bits
  as_binary = get_binary(get_nibbles(word_address,1),3)
  print('word address  {0:#0{1}x}  ::  '.format(word_address,1+2),end=' ')
  print(as_binary)

# ---------------------------------------------------------------------
# ---------------------------------------------------------------------
if __name__ == "__main__":
  tag = 0x001
  idx = 0x00a
  wrd = 0x1
  byt = 0x0
  run(tag,idx,wrd,byt)
  print('write back')
  tag = 0x00e
  idx = 0x00a
  wrd = 0x0
  byt = 0x0
  run(tag,idx,wrd,byt)

