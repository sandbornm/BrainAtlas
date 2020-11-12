#include <iostream>
#include <string>
#include <cstdlib>
#include <sstream>
#include "itkImage.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkDivideImageFilter.h"
#include "itkNaryAddImageFilter.h"

// constants
const unsigned int nDims = 3 ;
const unsigned int imageCount = 21 ;

// set up types
typedef itk::Image < double, nDims > ImageType ;
typedef itk::ImageFileReader < ImageType > ImageReaderType ;
typedef itk::ImageFileWriter < ImageType > ImageWriterType ;
typedef itk::NaryAddImageFilter < ImageType, ImageType > AddFilterType ;
typedef itk::DivideImageFilter < ImageType, ImageType, ImageType  > DivideFilterType ;

int main(int argc, char * argv[])
{
	// assume parameters are expected types - # args varies
	if (argc < 2)
       	{
		// example ./Setup i -> get initial template of 21 KKI2009-XX-MPRAGE.nii.gz
		// example ./Setup a 3 21 file1.nii.gz file2.nii.gz file3.nii.gz --> get affine template made of (file1 + file2 + file3) / 21
		std::cout << "check parameters! usage: ./Setup [-templateType={i, a, d}] [-numImages=num] [-constant=num] [-filenames= {numImages} strings]" << std::endl;
		std::cout << "example ./Setup a 3 21 file1.nii.gz file2.nii.gz file3.nii.gz" << std::endl;
		exit(EXIT_FAILURE);
	}

	// assume necessary images are in the build directory 
	// i/a/d = get initial/affine/deformable template/template/atlas	
	std::string templateType = argv[1];

	if (templateType == "i") {
		// make initial template
		AddFilterType::Pointer addFilter = AddFilterType::New();
		std::cout << "loading images for initial template..." << std::endl;
		// adapted from https://discourse.itk.org/t/beginner-in-itk-averaging-image/2328/4
		for (int i = 1; i <= imageCount; i++)
	 	{
			// filename setup
			std::string is = "";
			std::stringstream o;
			o << i;
			std::string istr = o.str();
			if (i < 10)
			{
				is = "0" + istr;
			} else
			{
				is = istr;
			}
			std::string pre = "KKI2009-";
			std::string post = "-MPRAGE.nii.gz";
			ImageReaderType::Pointer reader = ImageReaderType::New();
			reader->SetFileName(pre + is + post);
			reader->Update();
			addFilter->SetInput(i-1, reader->GetOutput());
			std::cout << "added image " << pre + is + post << std::endl;
		} // end for
		// all 21 images added together
  		addFilter->Update();
		// divide by imageCount
		DivideFilterType::Pointer divFilter = DivideFilterType::New();
		divFilter->SetInput(addFilter->GetOutput());
		divFilter->SetConstant( imageCount );
		divFilter->Update();
	        // write image	
		ImageWriterType::Pointer writer = ImageWriterType::New();
		writer->SetFileName( "initialTemplate.nii.gz" );
		writer->SetInput( divFilter->GetOutput());
		writer->Update();
		std::cout << "wrote initialTemplate.nii.gz " << std::endl; 
	
	} else if (templateType == "a" || templateType == "d") { // can be affine or deformable 
		// the number of files to load
		int numImages = atoi(argv[2]);
		// the constant in the division 
		int constant  = atoi(argv[3]);
		if (templateType == "a"){
			std::cout << "loading images for affine template..." << std::endl;
		} else {
			std::cout << "loading images for deformable atlas..." << std::endl;
		}
		// the files to divide
		AddFilterType::Pointer addFilter = AddFilterType::New();
		for (int j = 0; j < numImages; j++){	
			std::string fname = argv[4+j];
			ImageReaderType::Pointer reader = ImageReaderType::New();
			reader->SetFileName( fname );
			reader->Update();
			addFilter->SetInput(j, reader->GetOutput());
			std::cout << "added image " << fname << std::endl;
		} // end for
		addFilter->Update();
		// divide by constant (may or may not be equal to numImages)
		DivideFilterType::Pointer divFilter = DivideFilterType::New();
		divFilter->SetInput(addFilter->GetOutput());
		divFilter->SetConstant(constant);
		divFilter->Update();
		// write image
		ImageWriterType::Pointer writer = ImageWriterType::New();
		if (templateType == "a") { // affine template
			writer->SetFileName("affineTemplate.nii.gz");
		} else { // deformable template
			writer->SetFileName("deformableTemplate.nii.gz");
		}
		writer->SetInput( divFilter->GetOutput());
		writer->Update();
		std::cout << "writing template..." << std::endl;
		if (templateType == "a") {
			std::cout << "wrote affineTemplate.nii.gz" << std::endl;
		} else {
			std::cout << "wrote deformableTemplate.nii.gz" << std::endl;
		}
	}
std::cout << "done" << std::endl;
return 0;
}

