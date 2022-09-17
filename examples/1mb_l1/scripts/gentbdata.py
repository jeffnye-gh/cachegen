#!/usr/bin/env python3

#basedir = 'data/'
# --------------------------------------------------------------------
# DARRAY - 
# --------------------------------------------------------------------
def dsramGen(base,ramName,cfg,ext,cmt):
  dsbase = base+'/'+ramName+'.'+cfg+'.'+ext;
  dscmt  = cmt
  if(cfg == 'cfg0'):
    dsword = "00w0W0II"
    for i in range(0,4):
      fn = dsbase.replace('N',str(i))
      cm = dscmt.replace('N',str(i))
      print('creating %s' % fn)
      with open(fn, 'w') as f:
        f.write(cm);
        for j in range(0,128):
          if(j == 0) : f.write('@0000 ')
          else:        f.write('      ')
    
          jhex = f'{j:02x}'
    
          for k in range(7,-1,-1):
            if(k == 7): un = ''
            else:       un = '_'
            ndsword =  dsword.replace('W',str(k))
            ndsword = ndsword.replace('w',str(i))
            ndsword = ndsword.replace('II',str(jhex))
            f.write(un)
            f.write(ndsword)
          f.write('\n')
        f.write('@0080 ')
        for m in range(0,8):
          if(m == 0): un = ''
          else:       un = '_'
          f.write(un)
          f.write('FFFFFFFF')
        f.write('\n')
  print('dsram configuration %s complete' % cfg)
# --------------------------------------------------------------------
# TAGS
# --------------------------------------------------------------------
def tagGen(base,ramName,cfg,ext,cmt):
  tagbase = base+'/'+ramName+'.'+cfg+'.'+ext;
  tagcmt  = cmt
  if(cfg == 'cfg0'):
    for i in range(0,4):
      fn = tagbase.replace('N',str(i))
      cm = tagcmt.replace('N',str(i))
      print('creating %s' % fn)
      with open(fn, 'w') as f:
        f.write(cm);
        k = i;
        for j in range(0,128):
          if(j == 0) :
            f.write('@0000 ')
          else:
            f.write('      ')
        
          f.write(f'{k:04x}')
          f.write('\n')
          k += 4
        f.write('      3FFF')
  
  print('tag configuration %s complete' % cfg)
## --------------------------------------------------------------------
## Valid bits
## --------------------------------------------------------------------
def valGen(base,arrayName,cfg,ext,cmt):
  valbase = base+'/'+arrayName+'.'+cfg+'.'+ext;
  valcmt  = cmt
  if(cfg == 'cfg0'):
    fn = valbase
    cm = valcmt
    print('creating %s' % fn)
    with open(fn, 'w') as f:
      f.write(cm);
      for j in range(0,8192):
        if(j == 0) :
          f.write('@0000 ')
        else:
          f.write('      ')
        if(j < 128) :
          f.write('F\n')
        else:
          f.write('0\n')
 
  print('val bit configuration %s complete' % cfg)
## --------------------------------------------------------------------
## Mod bits
## --------------------------------------------------------------------
def modGen(base,arrayName,cfg,ext,cmt):
  modbase = base+'/'+arrayName+'.'+cfg+'.'+ext;
  modcmt  = cmt
  if(cfg == 'cfg0'):
    fn = modbase
    cm = modcmt
    print('creating %s' % fn)
    with open(fn, 'w') as f:
      f.write(cm);
      for j in range(0,8192):
        if(j == 0) :
          f.write('@0000 ')
        else:
          f.write('      ')
        f.write('0\n')
 
  print('mod bit configuration %s complete' % cfg)
## --------------------------------------------------------------------
## LRU bits
## --------------------------------------------------------------------
def lruGen(base,arrayName,cfg,ext,cmt):
  lrubase = base+'/'+arrayName+'.'+cfg+'.'+ext;
  lrucmt  = cmt
#lrubase = basedir+'lru.cfg0.memh'
#lrucmt  = '''// vi:syntax=verilog
#// Config 0,  LRU array, 1 per index, 4 ways so 3 PLRU bits
#//
#// PLRU array - set to 000
#// 8192 (13b address) by 3 bit (3 PLRU bits)
#'''
  if(cfg == 'cfg0'):
    fn = lrubase
    cm = lrucmt
    print('creating %s' % fn)
    with open(fn, 'w') as f:
      f.write(cm);
      for j in range(0,8192):
        if(j == 0) :
          f.write('@0000 ')
        else:
          f.write('      ')
        f.write('0\n')
 
  print('lru bit configuration %s complete' % cfg)

# ===========================================================================
def main():
  basedir = './tmp'
  dsramCmnt = '''// vi:syntax=verilog
// Config -N-, data array way0
//
// Only first 129 locations are initialized , address 128 has all F's
// 8192(13b address) by 256 bits (32 byte line)
//    31:24 - => not allocated
//    23:20 w => way
//    19:16 p => pad(0)
//    15:12 W => word 0-7
//    11:8  p => pad(0)
//    7:0   i => increasing count 0 - 7f 
//    word 7   word 6   word 5   word 4   word 3   word 2   word 1   word 0
//    --wpWpii --wpWpii --wpWpii --wpWpii --wpWpii --wpWpii --wpWpii --wpWpii
'''

  tagCmnt  = '''// vi:syntax=verilog
// Config 0, tag array way N
//
// Only first 129 locations are initialized , address 128 has all F's
// 8192 (13b address) by 14 bit (tag)
'''

  valCmnt  = '''// vi:syntax=verilog
// Config 0, valid bit array, includes all 4 ways
//
// Valid array - 1st 128 entries in each way are valid
// 8192 (13b address) by 4 bit (valid bit per way)
'''

  modCmnt  = '''// vi:syntax=verilog
// Config 0, modified bit array, includes all 4 ways
//
// Mod array - all entries are clean (mod = 0)
// 8192 (13b address) by 4 bit (dirty bit per way)
'''

  lruCmnt  = '''// vi:syntax=verilog
// Config 0,  LRU array, 1 per index, 4 ways so 3 PLRU bits
//
// PLRU array - set to 000
// 8192 (13b address) by 3 bit (3 PLRU bits)
'''

  cmt = dsramCmnt.replace('-N-',str(0))
  dsramGen(basedir,'dsramN','cfg0','memh',cmt)

  cmt = tagCmnt.replace('-N-',str(0))
  tagGen(basedir,'tagN','cfg0','memh',cmt)

  cmt = valCmnt.replace('-N-',str(0))
  valGen(basedir,'val','cfg0','memh',cmt)

  cmt = modCmnt.replace('-N-',str(0))
  modGen(basedir,'mod','cfg0','memh',cmt)

  cmt = lruCmnt.replace('-N-',str(0))
  lruGen(basedir,'lru','cfg0','memh',cmt)

if __name__ == '__main__':
  main()

