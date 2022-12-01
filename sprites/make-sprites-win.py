#!/bin/python3

# From: https://github.com/15-466/15-466-f19-base6/tree/master/sprites
#This exists mainly to work around the (many) problems with doing (simple) CLI things on windows. Like lack of shell globbing.

# intended to be run from root of repo

name = "the-planet" # name of spritees to be extracted and packed (must be shared by .xcf and .list file in the same directory)
gimp = "C:\\Program Files\\GIMP 2\\bin\\gimp-console-2.10.exe"

import subprocess
import os
import shutil

if os.path.exists("./dist/assets/sprites/" + name + ".png"):
	os.unlink("./dist/assets/sprites/" + name + ".png")

if os.path.exists("./dist/assets/sprites/" + name + ".atlas"):
	os.unlink("./dist/assets/sprites/" + name + ".atlas")

if os.path.exists("./sprites/" + name):
	shutil.rmtree("./sprites/" + name)

subprocess.run([
	"python3",
	"./sprites/extract-sprites.py",
	"./sprites/" + name + ".list",
	"./dist/assets/sprites/"  + name,
	"--gimp", gimp
], check=True)

pngs = []
for root, dirs, files in os.walk("./dist/assets/sprites/" + name):
	for f in files:
		if f.endswith(".png"):
			pngs.append(root + "/" + f)
	break

print(pngs)

subprocess.run([
	"./sprites/pack-sprites.exe",
	"./dist/assets/sprites/"
] + pngs, check=True)
