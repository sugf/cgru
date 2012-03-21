import os, sys

pythons = os.environ['PYTHONS']
print('Pythons folder = "%s"' % pythons)

for python in os.listdir( pythons):
   if python[0] == '.' or python[0] == '-': continue
   pythondir = os.path.join( pythons, python )
   if not os.path.isdir( pythondir): continue
   print('Python = "%s"' % python)

   pythonlib = 'python' + python[:3].replace('.','').replace('_','')
   arch = 'amd64'
   if python.find('-32') > 0 or python.find('_32') > 0: arch = 'x86'

   os.environ['AF_PYAFVER'] = python
   os.environ['PYTHONDIR'] = pythondir
   os.environ['PYTHONLIB'] = pythonlib

   print('PYTHONDIR=%s' % pythondir)
   print('PYTHONLIB=%s' % pythonlib)
   
   cmd = 'make_pyaf.cmd ' + arch
   os.system( cmd)