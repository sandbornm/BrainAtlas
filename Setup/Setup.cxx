#include "itkImage.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkDivideImageFilter.h"
#include "itkNaryAddImageFilter.h"
#include <string>
#include <sstream>
#include <ctime>
#include <cstdlib>

// constants
const unsigned int nDims = 3 ;
const unsigned int imageCount = 21 ;
const std::string PATH = "../KKI2009-ALL-MPRAGE/" ;

// set up types
typedef itk::Image < double, nDims > ImageType ;
typedef itk::ImageFileReader < ImageType > ImageReaderType ;
typedef itk::ImageFileWriter < ImageType > ImageWriterType ;
typedef itk::NaryAddImageFilter < ImageType, ImageType > AddFilterType ;
typedef itk::DivideImageFilter < ImageType, ImageType, ImageType  > DivideFilterType ;

int main(int argc, char * argv[])
{
	 // array of readers and file paths
	 ImageReaderType::Pointer readers[imageCount];
	 std::string pre = "KKI2009-" ;
	 std::string post = "-MPRAGE.nii.gz" ;
	 // initialize readers - start at 1 for image 01 and add images to reader array
	 // so the index for the random fixed image can be used to access the reader array later
	 AddFilterType::Pointer addFilter = AddFilterType::New();
	 std::cout << "loading images..." << std::endl;
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
		readers[i-1] = ImageReaderType::New();
		readers[i-1]->SetFileName(PATH + pre + is + post);
		readers[i-1]->Update();
		addFilter->SetInput(i-1, readers[i-1]->GetOutput());
	     }//end for

	// from https://discourse.itk.org/t/beginner-in-itk-averaging-image/2328/4
	// all 21 images added together
  	addFilter->Update();
	// divide by imageCount
	DivideFilterType::Pointer divFilter = DivideFilterType::New();
	divFilter->SetInput(addFilter->GetOutput());
	divFilter->SetConstant( imageCount );
	divFilter->Update();
	// write initial template
	std::string tname = "initialTemplate.nii.gz";
	ImageWriterType::Pointer writer = ImageWriterType::New();
	writer->SetFileName( tname );
	writer->SetInput( divFilter->GetOutput());
	writer->Update();
	std::cout << "wrote initialTemplate " << tname << std::endl; 
	// random subject as fixed image
	srand((unsigned int) time(NULL)); // seed rand with current time
	int randSubject = rand() % imageCount; // rand int in range 0-20
	std::cout << "random subject is index: " << randSubject << std::endl;
	// write to a new image to save time later
	ImageWriterType::Pointer writer2 = ImageWriterType::New();
	std::stringstream r;
	r << randSubject + 1;
	std::string rfname = "randomFixedImage" + r.str() + ".nii.gz";
	writer2->SetFileName( rfname );
	writer2->SetInput( readers[randSubject]->GetOutput());
	writer2->Update();
	std::cout << "wrote random subject " +  r.str() + " to " +  tname << std::endl; 
	// set up done, have initialTemplate and randomFixedImage
	return 0;
}

