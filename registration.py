#!/Users/michael/envs/jup/bin/python

import ants as a
import os
import sys

## ----- INITIAL SETUP -----

# change path as needed
path = '/Users/michael/itkants/itkants/KKI2009-ALL-MPRAGE/'
fileArr = sorted([x for x in os.listdir(path) if not x.startswith('.')])
imageCount = len(fileArr)

# ensure proper number of args, assume types and value ranges are valid
if len(sys.argv) < 8 or len(sys.argv) > 8:
	print("check parameters! usage: [-fixedImage=filepath] [-lower=num] [-upper=num] [-a=affine | -d=deformable] [-observe=num{0,1}] [-initialize=num{0,1}] [-doDivide=num{0,1}]")
	print("0 <= lower <= numberOfImages, lower < upper <= numberOfImages (includes endpoints)")
	print("observe = 0/1 -> don't/do output intermediate transformed images")
	print("initialize = 0/1 -> don't get/get initial template")
	print("doDivide = 0/1 -> don't/do divide final image")
	print("example: python registration.py affineTemplate.nii.gz 1 10 d 0 0 1 --> deformably register images 1 to 10 to affineTemplate.nii.gz without intermediate images without initial template and divide final image")
	print("try again")
	sys.exit(0)

# parse command line args
fixed = sys.argv[1]
lower = int(sys.argv[2])
upper = int(sys.argv[3])
which = sys.argv[4]
observe = int(sys.argv[5])
initialize = int(sys.argv[6])
doDivide = int(sys.argv[7])

## ----- Load Images from Directory -----
# Do anyway
imageArr = []
for filename in fileArr:
	f = os.path.join(path,filename)
	img = a.image_read(f)
	imageArr.append(img)
print("loaded {} images".format(len(imageArr)))

## ----- Get Initial Template -----
# Do if initialize is set
if initialize:
	baseImage = imageArr[0]
	for i in range(1,imageCount):
		baseImage += imageArr[i]
	iniTemplate = baseImage / imageCount
	initmp = "initialTemplate.nii.gz"
	initialTemplate = a.image_write(iniTemplate, initmp, True)
	print("wrote initial template to {}".format(initmp))

## ----- Get Affine Template -----
# (chosen fixed image for affine template KKI2009-05-MPRAGE.nii.gz)
# assume fixed image is in the same directory as the script
if which == 'a':
	print("affine registration of images {} to {} to fixed image {}".format(lower, upper, fixed))
	affFixedImage = a.image_read(path + fixed)
	baseImage2 = affFixedImage
	for i in range(lower-1, upper):
		if i != int(fixed[8:10]):
			affTransforms = a.registration(affFixedImage, imageArr[i], type_of_transform='AffineFast')
			print("affinely registered {}".format(fileArr[i]))
			affTransformedImage = a.apply_transforms(fixed=affFixedImage, moving=imageArr[i], transformlist=affTransforms['fwdtransforms'])
			print("applied transforms to {}".format(fileArr[i]))
			a.image_write(affTransformedImage, "af" + fileArr[i])
			print("wrote transformed image to {}".format("af" + fileArr[i]))
			baseImage2 += affTransformedImage
			print("added transformed image to baseImage".format(fileArr[i]))
	if doDivide:
		avgAffImage = baseImage2 / (upper - lower + 1) 
		affTmp = str(lower) + "_" + str(upper) + "affineTemplate.nii.gz"
		affineTemplate = a.image_write(avgAffImage, affTmp, True)
		print("wrote affine template for images {} to {} to {}".format(lower, upper, affTmp))
	else:
		affInt = str(lower) + "_" + str(upper) + "affIntermediate.nii.gz"
		affineIntermediate = a.image_write(baseImage2, affInt, True)
		print("wrote affine intermediate result for images {} to {} to {}".format(lower, upper, affInt))

## ----- STEP 5: Deformable Registration -----
# using original image for movingImage here, ElasticSyn does Affine + Deformable
if which == 'd':
	print("deformable registration of images {} to {} to fixed image {}".format(lower, upper, fixed))
	if observe: 
		defFixedImage = a.image_read(fixed)
		baseImage3 = defFixedImage
		print("observe = 1 --> writing intermediate transforms to images at iterations 20, 40, 60")
		for i in range(lower-1, upper):		
			defMovingImage = imageArr[i]
			# to 20 iterations
			defTransforms = a.registration(defFixedImage, defMovingImage, type_of_transform='ElasticSyN', reg_iterations=((20,)))
			print("deformably registered {}".format(fileArr[i]))
			defTransformedImage = a.apply_transforms(fixed=defFixedImage, moving=defMovingImage, transformlist=defTransforms['fwdtransforms'])
			print("applied transforms to {}".format(fileArr[i]))
			initname = "twenty" + str(i+1) + ".nii.gz"
			print("writing result after 20 iterations to {}".format(initname))
			a.image_write(defTransformedImage, initname)
			print("wrote result after 20 iterations to {}".format(initname))
			# to 40 iterations
			defTransforms2 = a.registration(defFixedImage, defTransformedImage, type_of_transform='ElasticSyN', reg_iterations=((20,)))
			print("deformably registered intermediate 1")
			defTransformedImage2 = a.apply_transforms(fixed=defFixedImage, moving=defTransformedImage, transformlist=defTransforms2['fwdtransforms'])
			print("applied intermediate transforms 1")
			print("applied transforms2 to {}".format(fileArr[i]))
			intr1 = "forty" + str(i+1) + ".nii.gz"
			print("writing result after 40 iterations to {}".format(intr1))
			a.image_write(defTransformedImage2, intr1)
			print("wrote result after 40 iterations to {}".format(intr1))
			# to 60 iterations
			defTransforms3 = a.registration(defFixedImage, defTransformedImage2, type_of_transform='ElasticSyN', reg_iterations=((20,)))
			print("deformably registered intermediate 2")
			defTransformedImage3 = a.apply_transforms(fixed=defFixedImage, moving=defTransformedImage2, transformlist=defTransforms3['fwdtransforms'])
			print("applied intermediate transforms 3")
			print("applied transforms3 to {}".format(fileArr[i]))
			intr2 = "sixty" + str(i+1) + ".nii.gz"
			print("writing result after 60 iterations to {}".format(intr2))
			a.image_write(defTransformedImage3, intr2)
			print("wrote result after 60 iterations to {}".format(intr2))
			# add the last image to the array
	else:
		# Observe not set, just a single call for 60 iterations
		defFixedImage = a.image_read(fixed)
		for i in range(lower-1, upper):
			defMovingImage = imageArr[i]
			defTransforms = a.registration(defFixedImage, defMovingImage, type_of_transform='ElasticSyN', reg_iterations=((60,)))
			print("deformably registered {}".format(fileArr[i]))
			defTransformedImage = a.apply_transforms(fixed=defFixedImage, moving=defMovingImage, transformlist=defTransforms['fwdtransforms'])
			print("applied transforms to {}".format(fileArr[i]))
			if i == lower - 1:
				# first time through for loop
				baseImage3 = defTransformedImage
			baseImage3 += defTransformedImage
		if doDivide:
			avgDefImage = baseImage3 / (upper - lower + 1)
			defAtls = str(lower) + "_" + str(upper) + "deformableAtlas.nii.gz"
			defAtlas = a.image_write(avgDefImage, defAtls, True)
			print("wrote deformable atlas for images {} to {} to {}".format(lower, upper, defAtls))
		else:
			defInt = str(lower) + "_" + str(upper) + "defIntermediate.nii.gz"
			deformableIntermediate = a.image_write(baseImage3, defInt, True)
			print("wrote affine intermediate result for images {} to {} to {}".format(lower, upper, defInt))


		