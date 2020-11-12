#include <iostream>
#include <string>
#include <cstdlib>
#include <stdlib.h>
#include <sstream>
#include "itkImage.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkAffineTransform.h"
#include "itkMultiResolutionImageRegistrationMethod.h"
#include "itkLinearInterpolateImageFunction.h"
#include "itkRegularStepGradientDescentOptimizer.h"
#include "itkResampleImageFilter.h"
#include "itkMeanSquaresImageToImageMetric.h"
#include "itkNaryAddImageFilter.h"
#include "itkDemonsRegistrationFilter.h"
#include "itkCommand.h"
#include "itkDivideImageFilter.h"

// constants
const unsigned int nDims = 3 ;

// set up types
typedef itk::Image < double, nDims > ImageType ;
typedef itk::ImageFileReader < ImageType > ImageReaderType ; 
typedef itk::ImageFileWriter < ImageType > ImageWriterType ;
typedef itk::NaryAddImageFilter < ImageType, ImageType > AddFilterType ;
typedef itk::DivideImageFilter < ImageType, ImageType, ImageType > DivideFilterType ;
typedef itk::MultiResolutionImageRegistrationMethod < ImageType, ImageType > RegistrationMethodType ;
typedef itk::AffineTransform < double, nDims > AffineTransformType ;
typedef itk::RegularStepGradientDescentOptimizer OptimizerType ;
typedef itk::LinearInterpolateImageFunction < ImageType > InterpolatorType ;
typedef itk::ResampleImageFilter < ImageType, ImageType > ResampleFilterType ;
typedef itk::MeanSquaresImageToImageMetric <ImageType, ImageType > MetricType ;

// callback for optimizer like in class
class OptimizerIterationCallback : public itk::Command
{
public:
	typedef OptimizerIterationCallback Self ;
	typedef itk::Command Superclass ;
	typedef itk::SmartPointer<OptimizerIterationCallback> Pointer ;
	itkNewMacro(OptimizerIterationCallback);

	typedef const OptimizerType * OptimizerPointerType ;
	// non-const if want to change 
	void Execute(itk::Object * caller, const itk::EventObject & event)
	{
		Execute((const itk::Object *) caller, event) ;
	}
	// const if just observing
	void Execute(const itk::Object * caller, const itk::EventObject & event)
	{
		OptimizerPointerType optimizer = dynamic_cast < OptimizerPointerType > ( caller ) ;
		std::cout << " iteration: " << optimizer->GetCurrentIteration() << " value: " << optimizer->GetValue() << std::endl;
	}
};

int main(int argc, char * argv[])
{
	 // assume parameters are expected types and fixed file is in the build directory i.e. can be accessed directly by filename
	 if (argc < 6 || argc > 6)
	 {
	 	// example ./Registration KKI2009-05-MPRAGE.nii.gz 1 10 1 0 --> affinely register images 1 to 10 to KKI2009-05-MPRAGE.nii.gz, divide the result and don't add an observer --> output file is a1_10intermediate.nii.gz
		// usage: ./Registration  [-fixedImage=file] [-lower=num] [-upper=num] [-doDivide=num{0,1}] [-observe=num{0,1}] 
	 	std::cout << "check parameters! usage: ./Registration  [-fixedImage=file] [-lower=num] [-upper=num] [-doDivide=num{0,1}] [-observe=num{0,1}]" << std::endl;
	 	std::cout << "(includes endpoints) 0 <= lower <= numberOfImages, lower < upper <= numberOfImages, doDivide = 0 --> don't divide final image, = 1 --> divide final image" << std::endl;
		std::cout << "observe = 0 --> don't add observer, observe = 1 --> add observer" << std::endl; 
		std::cout << "example: ./Registration KKI2009-05-MPRAGE.nii.gz 1 10 1 0 --> affinely register images 1 to 10 to KKI2009-05-MPRAGE.nii.gz, divide the result and don't add an observer --> output file is a1_10intermediate.nii.gz" << std::endl;
		exit(EXIT_FAILURE);
	 }

	// parse args
	std::string fixedImageFile = argv[1];
	int lower = atoi(argv[2]);
	int upper = atoi(argv[3]);
	int doDivide = atoi(argv[4]);
	int observer = atoi(argv[5]);

   	AddFilterType::Pointer tAddFilter = AddFilterType::New() ; 	
   	ImageReaderType::Pointer fixedReader = ImageReaderType::New();
  	fixedReader->SetFileName(fixedImageFile);
   	fixedReader->Update();
   	// assumes file name is of the form KKI2009-05-MPRAGE.nii.gz  
	std::string fixedFileNum = fixedImageFile.substr(8,2);
   	std::cout << "fixedFileNum " << fixedFileNum << std::endl;
	std::stringstream f(fixedFileNum);
	int ffn = 0;
	f >> ffn;
	std::string pre = "KKI2009-" ;
	std::string post = "-MPRAGE.nii.gz" ;
	for (int i = lower; i <= upper; ++i)
	{
	if ( i != ffn ) {
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
		is = pre + is;
		std::string fname = is + post;
		ImageReaderType::Pointer movingReader = ImageReaderType::New();
		movingReader->SetFileName( fname );
		movingReader->Update() ;
		std::cout << "now registering " << fname << std::endl;  
		// gather registration materials 		
		RegistrationMethodType::Pointer registration = RegistrationMethodType::New(); 
		AffineTransformType::Pointer transform = AffineTransformType::New();
		MetricType::Pointer metric = MetricType::New() ;
		OptimizerType::Pointer optimizer = OptimizerType::New();		
		InterpolatorType::Pointer interpolator = InterpolatorType::New() ;
	
		// set up affine registration
		registration->SetFixedImage(fixedReader->GetOutput()) ;
		registration->SetMovingImage(movingReader->GetOutput() );
		registration->SetOptimizer ( optimizer ) ;
		registration->SetMetric ( metric ) ;
		registration->SetInterpolator ( interpolator ) ;
		registration->SetTransform( transform ) ;
		optimizer->MinimizeOn() ;
		optimizer->SetNumberOfIterations ( 100 ) ;		
		optimizer->SetMinimumStepLength( 0 ) ;
		optimizer->SetMaximumStepLength( 0.0125 ) ;
		transform->SetIdentity() ;
		registration->SetInitialTransformParameters( transform->GetParameters() ) ;
		registration->SetFixedImageRegion ( fixedReader->GetOutput()->GetLargestPossibleRegion() ) ;
		// add callbacks based on observer flag argv[5]	
		if (observer) {
			OptimizerIterationCallback::Pointer optCallback = OptimizerIterationCallback::New();
			optimizer->AddObserver(itk::IterationEvent(), optCallback); 
		}

		// try to do registration
		try {
		registration->Update();
		std::cout << "stopped because " << optimizer->GetStopConditionDescription() << std::endl;
		}
		catch ( itk::ExceptionObject & err )
		{
		std::cerr << "Exception caught" << std::endl;
		std::cerr << err << std::endl;
		return EXIT_FAILURE;
		}

		// apply transform
		ResampleFilterType::Pointer resampleFilter = ResampleFilterType::New() ;
		resampleFilter->SetInput ( movingReader->GetOutput() ) ;
		resampleFilter->SetTransform ( transform ) ;
		resampleFilter->SetReferenceImage( fixedReader->GetOutput()) ;
		resampleFilter->UseReferenceImageOn() ;		
	        resampleFilter->Update() ; 
			
		// add registered image to add filter for affine template calculation- depends on lower
		if (lower == 1){
			// have to set input at zero index so no complaining (itk::ERROR:
			// NaryAddImageFilter Input Primary is required but not set.)
			tAddFilter->SetInput(i-1, resampleFilter->GetOutput());
		} else if (lower > 1 && i == lower){
			tAddFilter->SetInput(0, resampleFilter->GetOutput());
		} else {
			tAddFilter->SetInput(i-lower, resampleFilter->GetOutput());
		}

		// store affinely registered image for deformable registration moving image
		ImageWriterType::Pointer result = ImageWriterType::New();
		std::string resname = is + post;
		result->SetFileName(resname);
		result->SetInput( resampleFilter->GetOutput() );
		result->Update();
		std::cout << "wrote result to " << resname << std::endl;
		} // end if 
	} // end for
// update add filter
tAddFilter->Update();

// file range to strings
std::stringstream l;
std::stringstream u;
l << lower;
u << upper;
std::string lo = l.str();
std::string up = u.str();
		
// doDivide = 1 --> divide added images by upper - lower + 1 for affine template
// doDivide = 0 --> just output the added images from lower to upper (for distributed runs, 
// eg run1: lower = 1 and upper = 11; run2: lower = 12 and upper = 21)
ImageWriterType::Pointer writer = ImageWriterType::New() ;
if (doDivide) {
	DivideFilterType::Pointer divFilter = DivideFilterType::New();
	divFilter->SetInput(tAddFilter->GetOutput());
	divFilter->SetConstant(upper - lower + 1);
	divFilter->Update();
	std::string aname = lo + "_" + up + "affineTemplate" + ".nii.gz";
	writer->SetFileName( aname ) ;
	writer->SetInput( divFilter->GetOutput()) ;
	writer->Update();
	std::cout << "wrote " << aname << std::endl;
} else {
	// just write added images
	std::string aname = "a" + lo + "_" + up + "intermediate.nii.gz";
	std::cout << "writing " + aname + "..." << std::endl;
	writer->SetFileName(aname);
	writer->SetInput(tAddFilter->GetOutput());
        writer->Update();	
	std::cout <<  "wrote " << aname << std::endl;
}
// done with affine registration.
return 0;
}
