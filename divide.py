#!/Users/michael/envs/jup/bin/python

import ants as a
import os
import sys

# ensure proper number of args, assume types and value ranges are valid
if len(sys.argv) < 3:
	print("check parameters! usage: python divide.py [-templateType={a, d}] [-numImages=num] [-constant=num] [-filenames= (numImages) strings]")
	print("example: python divide.py a 3 21 file1.nii.gz file2.nii.gz file3.nii.gz --> get affine template made of (file1 + file2 + file3) / 21")
	print("try again")
	sys.exit(0)

# parse command line args
templateType = sys.argv[1]
numImages = int(sys.argv[2])
constant = int(sys.argv[3])
firstImage = a.image_read(sys.argv[4])
imgToDivide = firstImage

if templateType == "a": # affine template
	print("loading images for affine template")
elif templateType == "d": # deformable atlas
	print("loading images for deformable atlas")	
for i in range(1, numImages):
	img = a.image_read(sys.argv[4 + i])
	imgToDivide += img
outImage = imgToDivide / constant
if templateType == "a":
	print("writing affine template...")
	a.image_write(outImage, "affineTemplate.nii.gz")
elif templateType == "d":
	print("writing deformable atlas...")
	a.image_write(outImage, "deformableAtlas.nii.gz")
else:
 	print("invalid option")
 	exit(0)
print("done")
