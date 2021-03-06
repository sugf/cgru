#!/usr/bin/env python

import os, re, shutil, signal, sys

from optparse import OptionParser

Parser = OptionParser(usage="%prog [options]\nType \"%prog -h\" for help", version="%prog 1.0")

Parser.add_option('-s', '--sources',  dest='sources',  type  ='string', default='', help='Sources')
Parser.add_option('-d', '--dest',     dest='dest',     type  ='string', default='', help='Destination')
Parser.add_option('-r', '--refs',     dest='refs',     type  ='string', default='', help='References')
Parser.add_option('-t', '--template', dest='template', type  ='string', default='', help='Shot template')
Parser.add_option('-p', '--padding',  dest='padding',  type  ='string', default='', help='Shot renaming padding')
Parser.add_option('--shot_src',       dest='shot_src', type  ='string', default='SRC', help='Shot sources folder')
Parser.add_option('--shot_ref',       dest='shot_ref', type  ='string', default='REF', help='Shot references folder')
Parser.add_option('--test',           dest='test',     action='store_true', default=False, help='Test inputs only')

def errExit( i_msg):
	print('{"error":"%s"},' % i_msg)
	print('{"status":"error"}]}')
	sys.exit(1)

def interrupt( signum, frame):
	errExit('Interrupt received')
signal.signal(signal.SIGTERM, interrupt)
signal.signal(signal.SIGABRT, interrupt)
signal.signal(signal.SIGINT,  interrupt)

print('{"deploy":[')

(Options, args) = Parser.parse_args()

if Options.sources == '': errExit('Sources are not specified')
if not os.path.isdir( Options.sources): errExit('Sources folder does not exist')
if Options.dest == '': errExit('Destination is not specified')
if not os.path.isdir( Options.dest): errExit('Destination folder does not exist')
if Options.template == '': errExit('Shot template is not specified')
if not os.path.isdir( Options.template): errExit('Shot template folder does not exist')

References = []
if Options.refs != '' and os.path.isdir( Options.refs):
	References = os.listdir( Options.refs)
	#References.sort()

Sources = os.listdir( Options.sources)
Sources.sort()
Sources_skip = []
for shot in Sources:
	if shot in Sources_skip: continue
	src = os.path.join( Options.sources, shot)
	if not os.path.isdir( src):
		print('{"warning":"\"%s\" is not a folder"},' % shot)
		continue

	src_sources = [src]

	for folder in Sources:
		if folder == shot: continue
		if folder.find( shot + '_') != 0: continue
		Sources_skip.append( folder)
		src_sources.append( os.path.join( Options.sources, folder))

	src_refs = []
	for ref in References:
		if ref.find( shot + '.') == 0 or ref.find( shot + '_') == 0:
			src_refs.append( os.path.join( Options.refs, ref))

	shot_name = shot
	if Options.padding != '':
		words = re.findall(r'\D+', shot)
		digits = re.findall(r'\d+', shot)
		if len( words) == len( digits):
			shot_name = ''
			num = 0
			for word in words:
				padding = '1'
				if num < len( Options.padding): padding = Options.padding[num]
				shot_name += word
				shot_name += ('%0' + padding + 'd') % int( digits[num])
				num += 1
			#print('"%s"->"%s"' % (shot, shot_name))

	shot_dest = os.path.join( Options.dest, shot_name)

	print('{"shot":"%s","sources":[' % shot_name)
	for src in src_sources: print('\t"%s",' % src)
	print('\t,""]')
	if len( src_refs):
		print('\t"refs":[')
		for ref in src_refs: print('\t"%s",' % ref)
		print('\t,""]')
	print('\t},')

	if Options.test: continue

	shutil.copytree( Options.template, shot_dest)
	for scr in src_sources:
		shutil.move( src, os.path.join( shot_dest, Options.shot_src))
	for ref in src_refs:
		shutil.move( ref, os.path.join( shot_dest, Options.shot_ref))


print('{"status":"success"}]}')

