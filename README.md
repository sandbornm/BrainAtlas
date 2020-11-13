# Overview

## Setup.cxx

### Obtain an initial template, affine template, or deformable atlas based on input number of images, file names, and the constant by which to divide the images (useful for parallel processing)

Assumptions: The required images are in the build directory and named uniformly by KKI2009-XX-MPRAGE.nii.gz.

Usage: `./Setup [-templateType={i, a, d}] [-numImages=num] [-constant=num] [-filenames= {numImages} strings]` \
Example1: `./Setup a 3 21 file1.nii.gz file2.nii.gz file3.nii.gz` \
Example1 meaning: get the affine template of (file1 + file2 + file3) / 21 \
Example2: `./Setup i` \
Example2 meaning: get the initial template 

## Registration.cxx 

### Affinely register some number of images to a designated fixed image with the option of dividing the resulting image or adding an observer to view the iteration number and metric value at each of the 100 iterations.

Assumptions: The required images (fixed and moving) are in the build directory and named uniformly by KKI2009-XX-MPRAGE.nii.gz. When the number of command line args is correct (6), they are assumed to be acceptable values.

Usage: `./Registration  [-fixedImage=file] [-lower=num] [-upper=num] [-doDivide=num{0,1}] [-observe=num{0,1}]` \
Example: `./Registration KKI2009-05-MPRAGE.nii.gz 1 10 1 0` \
Example meaning: affinely register KKI2009-01-MPRAGE.nii.gz through KKI2009-10-MPRAGE.nii.gz to KKI2009-05-MPRAGE.nii.gz, divide the result, and don't add an observer to the registration process.

## dRegistration.cxx 

### Deformably register some number of images to a designated fixed image with the option of dividing the resulting image or adding an observer to generate deformable templates at iteration 1 and subsequent 20 iteration intervals.

Assumptions: The required images (fixed and moving) are in the build directory and named uniformly by KKI2009-XX-MPRAGE.nii.gz. When the number of command line args is correct (6), they are assumed to be acceptable values.

Usage: `./dRegistration [-fixedImage=filename] [-lower=num] [-upper=num] [-doDivide=num{0,1}] [-observe=num{0,1}]` \
Example: `./dRegistration affineTemplate.nii.gz 1 12 0 1` \
Example meaning: affinely register KKI2009-01-MPRAGE.nii.gz through KKI2009-12-MPRAGE.nii.gz to KKI2009-05-MPRAGE.nii.gz, don't divide the result, and add an observer to the registration process.

## divide.py

### Obtain an initial template, affine template, or deformable atlas based on input number of images, file names, and the constant by which to divide the images (useful for parallel processing)

Assumptions: The required images are in the script directory and named uniformly by KKI2009-XX-MPRAGE.nii.gz. When the number of command line args is correct (6), they are assumed to be acceptable values.

Usage: `python divide.py [-templateType={a, d}] [-numImages=num] [-constant=num] [-filenames= (numImages) strings]` \
Example: `python divide.py d 3 21 file1.nii.gz file2.nii.gz file3.nii.gz` \
Example meaning: get the deformable atlas of (file1 + file2 + file3) / 21 

## registration.py

### Affinely or deformably register some number of images to a designated fixed image with the option of generating deformable templates at 20 iteration intervals, getting the initial template, and dividing the resulting image.

Assumptions: The required images are in the script directory and named uniformly by KKI2009-XX-MPRAGE.nii.gz. When the number of command line args is correct (8), they are assumed to be acceptable values.

Usage: `python registration.py [-fixedImage=filepath] [-lower=num] [-upper=num] [-a=affine | -d=deformable] [-observe=num{0,1}] [-initialize=num{0,1}] [-doDivide=num{0,1}]` \
Example: `python registration.py affineTemplate.nii.gz 1 10 d 0 0 1`\
Example meaning: deformably register KKI2009-01-MPRAGE.nii.gz through KKI2009-10-MPRAGE.nii.gz to affineTemplate.nii.gz, don't observe, don't get the initial template, and divide the final result.

