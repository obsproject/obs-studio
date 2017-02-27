#!/usr/bin/env python
 
candidate_paths = "bin obs-plugins data".split()
 
plist_path = "../cmake/osxbundle/Info.plist"
icon_path = "../cmake/osxbundle/obs.icns"
run_path = "../cmake/osxbundle/obslaunch.sh"
 
#not copied
blacklist = """/usr /System""".split()
 
#copied
whitelist = """/usr/local""".split()
 
#
#
#
 
 
from sys import argv
from glob import glob
from subprocess import check_output, call
from collections import namedtuple
from shutil import copy, copytree, rmtree
from os import makedirs, rename, walk, path as ospath
import plistlib

import argparse

def _str_to_bool(s):
    """Convert string to bool (in argparse context)."""
    if s.lower() not in ['true', 'false']:
        raise ValueError('Need bool; got %r' % s)
    return {'true': True, 'false': False}[s.lower()]

def add_boolean_argument(parser, name, default=False):                                                                                               
    """Add a boolean argument to an ArgumentParser instance."""
    group = parser.add_mutually_exclusive_group()
    group.add_argument(
        '--' + name, nargs='?', default=default, const=True, type=_str_to_bool)
    group.add_argument('--no' + name, dest=name, action='store_false')

parser = argparse.ArgumentParser(description='obs-studio package util')
parser.add_argument('-d', '--base-dir', dest='dir', default='rundir/RelWithDebInfo')
parser.add_argument('-n', '--build-number', dest='build_number', default='0')
parser.add_argument('-k', '--public-key', dest='public_key', default='OBSPublicDSAKey.pem')
parser.add_argument('-f', '--sparkle-framework', dest='sparkle', default=None)
parser.add_argument('-b', '--base-url', dest='base_url', default='https://builds.catchexception.org/obs-studio')
parser.add_argument('-u', '--user', dest='user', default='jp9000')
parser.add_argument('-c', '--channel', dest='channel', default='master')
add_boolean_argument(parser, 'stable', default=False)
parser.add_argument('-p', '--prefix', dest='prefix', default='')
args = parser.parse_args()

def cmd(cmd):
    import subprocess
    import shlex
    return subprocess.check_output(shlex.split(cmd)).rstrip('\r\n')

LibTarget = namedtuple("LibTarget", ("path", "external", "copy_as"))
 
inspect = list()
 
inspected = set()
 
build_path = args.dir
build_path = build_path.replace("\\ ", " ")
 
def add(name, external=False, copy_as=None):
	if external and copy_as is None:
		copy_as = name.split("/")[-1]
	if name[0] != "/":
		name = build_path+"/"+name
	t = LibTarget(name, external, copy_as)
	if t in inspected:
		return
	inspect.append(t)
	inspected.add(t)


for i in candidate_paths:
	print("Checking " + i)
	for root, dirs, files in walk(build_path+"/"+i):
		for file_ in files:
			path = root + "/" + file_
			try:
				out = check_output("{0}otool -L '{1}'".format(args.prefix, path), shell=True,
						universal_newlines=True)
				if "is not an object file" in out:
					continue
			except:
				continue
			rel_path = path[len(build_path)+1:]
			print(repr(path), repr(rel_path))
			add(rel_path)
 
def add_plugins(path, replace):
	for img in glob(path.replace(
		"lib/QtCore.framework/Versions/5/QtCore",
		"plugins/%s/*"%replace).replace(
			"Library/Frameworks/QtCore.framework/Versions/5/QtCore",
			"share/qt5/plugins/%s/*"%replace)):
		if "_debug" in img:
			continue
		add(img, True, img.split("plugins/")[-1])

actual_sparkle_path = '@loader_path/Frameworks/Sparkle.framework/Versions/A/Sparkle'

while inspect:
	target = inspect.pop()
	print("inspecting", repr(target))
	path = target.path
	if path[0] == "@":
		continue
	out = check_output("{0}otool -L '{1}'".format(args.prefix, path), shell=True,
			universal_newlines=True)
 
	if "QtCore" in path:
		add_plugins(path, "platforms")
		add_plugins(path, "imageformats")
		add_plugins(path, "accessible")
 
 
	for line in out.split("\n")[1:]:
		new = line.strip().split(" (")[0]
		if '@' in new and "sparkle.framework" in new.lower():
			actual_sparkle_path = new
			print "Using sparkle path:", repr(actual_sparkle_path)
		if not new or new[0] == "@" or new.endswith(path.split("/")[-1]):
			continue
		whitelisted = False
		for i in whitelist:
			if new.startswith(i):
				whitelisted = True
		if not whitelisted:
			blacklisted = False
			for i in blacklist:
				if new.startswith(i):
					blacklisted = True
					break
			if blacklisted:
				continue
		add(new, True)

changes = list()
for path, external, copy_as in inspected:
	if not external:
		continue #built with install_rpath hopefully
	changes.append("-change '%s' '@rpath/%s'"%(path, copy_as))
changes = " ".join(changes)

info = plistlib.readPlist(plist_path)

latest_tag = cmd('git describe --tags --abbrev=0')
log = cmd('git log --pretty=oneline {0}...HEAD'.format(latest_tag))

from os import path
# set version
if args.stable:
    info["CFBundleVersion"] = latest_tag
    info["CFBundleShortVersionString"] = latest_tag
    info["SUFeedURL"] = '{0}/stable/updates.xml'.format(args.base_url)
else:
    info["CFBundleVersion"] = args.build_number
    info["CFBundleShortVersionString"] = '{0}.{1}'.format(latest_tag, args.build_number)
    info["SUFeedURL"] = '{0}/{1}/{2}/updates.xml'.format(args.base_url, args.user, args.channel)

info["SUPublicDSAKeyFile"] = path.basename(args.public_key)
info["OBSFeedsURL"] = '{0}/feeds.xml'.format(args.base_url)

app_name = info["CFBundleName"]+".app"
icon_file = "tmp/Contents/Resources/%s"%info["CFBundleIconFile"]

copytree(build_path, "tmp/Contents/Resources/", symlinks=True)
copy(icon_path, icon_file)
plistlib.writePlist(info, "tmp/Contents/Info.plist")
makedirs("tmp/Contents/MacOS")
copy(run_path, "tmp/Contents/MacOS/%s"%info["CFBundleExecutable"])
try:
	copy(args.public_key, "tmp/Contents/Resources")
except:
	pass

if args.sparkle is not None:
    copytree(args.sparkle, "tmp/Contents/Frameworks/Sparkle.framework", symlinks=True)

prefix = "tmp/Contents/Resources/"
sparkle_path = '@loader_path/{0}/Frameworks/Sparkle.framework/Versions/A/Sparkle'

cmd('{0}install_name_tool -change {1} {2} {3}/bin/obs'.format(
    args.prefix, actual_sparkle_path, sparkle_path.format('../..'), prefix))



for path, external, copy_as in inspected:
	id_ = ""
	filename = path
	rpath = ""
	if external:
		id_ = "-id '@rpath/%s'"%copy_as
		filename = prefix + "bin/" +copy_as
		rpath = "-add_rpath @loader_path/ -add_rpath @executable_path/"
		if "/" in copy_as:
			try:
				dirs = copy_as.rsplit("/", 1)[0]
				makedirs(prefix + "bin/" + dirs)
			except:
				pass
		copy(path, filename)
	else:
		filename = path[len(build_path)+1:]
		id_ = "-id '@rpath/../%s'"%filename
		if not filename.startswith("bin"):
			print(filename)
			rpath = "-add_rpath '@loader_path/{}/'".format(ospath.relpath("bin/", ospath.dirname(filename)))
		filename = prefix + filename
 
	cmd = "{0}install_name_tool {1} {2} {3} '{4}'".format(args.prefix, changes, id_, rpath, filename)
	call(cmd, shell=True)

try:
	rename("tmp", app_name)
except:
	print("App already exists")
	rmtree("tmp")
