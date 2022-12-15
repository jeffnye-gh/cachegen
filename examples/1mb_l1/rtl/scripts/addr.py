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
lines = [
'// ======================================================================',
'// #x',
'// Access is to BINARY -> BYTE hex',
'// Line addr is       BINARY ->  BYTE hex', #27,7
'// Index addr                          BINARY ->     BYTE hex',
'// Word  addr                                       BIN ->        X hex',
'// Byte  enbs                                      BYTE ->        ETYB bin',
'// LRU bits are xxx -> lru way is x',
'//',
'// Tag at index U/way x is: 14\'hxxxx (14\'bxx xxxx xxxx xxxx)',
'// The data at index x/way x  is:',
'// xxxxxxxx_xxxxxxxx_xxxxxxxx_xxxxxxxx_xxxxxxxx_xxxxxxxx_xxxxxxxx_xxxxxxxx ',
'//',
'// The main memory (line) address for the write back is {tag,index,5\'b0}',
'// <--- tag ----> <-- index --> <-0->',
'// xxxxxxxxxxxxxx xxxxxxxxxxxxx xxxxx',
'// xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx -> xxxxxxxx byte address',
'// xxx xxxx xxxx xxxx xxxx xxxx xxxx       ->  xxxxxxx line address',
'//',
'// At the end of the access:',
'//',
'// main memory at 26\'hxxxxx should contain:',
'// xxxxxxxx_xxxxxxxx_xxxxxxxx_xxxxxxxx_xxxxxxxx_xxxxxxxx_xxxxxxxx_xxxxxxxx ',
'//',
'// ----------------------------------------------------------------------',
'// After allocation',
'// ----------------------------------------------------------------------',
'// tag at index x way x should contain:',
'//   xx xxxx xxxx xxxx  -> xxxx (from upper 14b of the access that missed)',
'//',
'// data at index x way x should contain (the contents of mm @0xxxxx):',
'//   xxxxxxxx_xxxxxxxx_xxxxxxxx_xxxxxxxx_xxxxxxxx_xxxxxxxx_xxxxxxxx_xxxxxxxx ',
'//',
'// control bits at index x should be ',
'//   val = 1111 (no change) ',
'//   mod = xxxx (only wx is modified)',
'//   lru =  xxx (was xxx, after read allocate to wayx becomes [x x  x])',
'//',
'//a:XXXXXXXX #N',
'// ======================================================================',
]

def run(num,tag,idx,wrd,byt,bes):
  # byte address is formed and then unpacks, for some cross checking 
  # not strictly necessary
  byte_address  = (tag << 18) + (idx << 5) + (wrd << 2)
  line_address  = byte_address >> 5
  index_address = (byte_address >> 5)  & 0x1FFF
  word_address  = (byte_address >> 2)  & 0x7
  tag_address   = (byte_address >> 18) &  0x3FFF

  as_binary = get_binary(get_nibbles(byte_address,8),32)
  byte_address = '{0:#0{1}x}'.format(byte_address,8+2)

  print(lines[0])
  print(lines[1].replace('x',str(num)))
  line2 = lines[2].replace('BINARY',str(as_binary))
  line2 = line2.replace('BYTE',str(byte_address))
  pe_wr_addr = byte_address
  print(line2)

  # 7 hex digits 27 bits
  as_binary = get_binary(get_nibbles(line_address,7),27)
  line_address = '{0:#0{1}x}'.format(line_address,7+2)

  as_binary = as_binary[1:]
  line3 = lines[3].replace('BINARY',str(as_binary))
  line3 = line3.replace('BYTE',str(line_address))
  print(line3)

  #4 hex digits, 13 bits
  as_binary = get_binary(get_nibbles(index_address,4),13)
  index_address = '{0:#0{1}x}'.format(index_address,4+2) 

  as_binary = as_binary[3:]
  line4 = lines[4].replace('BINARY',str(as_binary))
  line4 = line4.replace('BYTE',str(index_address))
  print(line4)

  #1 hex digits, 3 bits
  as_binary = get_binary(get_nibbles(word_address,1),3)
  word_address = '{0:#0{1}x}'.format(word_address,1+2)

  as_binary = as_binary[1:]
  line5 = lines[5].replace('BIN',str(as_binary))
  line5 = line5.replace('X',str(word_address))
  print(line5)

  line6 = lines[6].replace('BYTE',str(bes))
  line6 = line6.replace('ETYB',str(bes))
  print(line6)

  for i in range(7,37):
    print(lines[i])

  line38 = lines[38].replace('XXXXXXXX',pe_wr_addr)
  line38 = line38.replace('N',str(num))
  print(line38)
  print(lines[39])
#
# ---------------------------------------------------------------------
# ---------------------------------------------------------------------
if __name__ == "__main__":
  num = 0
  tag = 0x002
  idx = 0x002
  wrd = 0x6
  byt = 0x0
  bes = '1100'
  run(num,tag,idx,wrd,byt,bes)
#  print('write back')
#  tag = 0x003
#  idx = 0x001
#  wrd = 0x7
#  byt = 0x0
#  run(tag,idx,wrd,byt,bes)

