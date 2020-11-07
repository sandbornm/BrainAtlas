#include "itkImage.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkAffineTransform.h"
#include <string>
#include <sstream>
#include "itkMultiResolutionImageRegistrationMethod.h"
#include "itkLinearInterpolateImageFunction.h"
#include "itkRegularStepGradientDescentOptimizer.h"
#include "itkResampleImageFilter.h"
#include "itkMeanSquaresImageToImageMetric.h"
#include "itkDemonsRegistrationFilter.h"
#include "itkCommand.h"
#include "itkDivideImageFilter.h"
#include <ctime>


// constants
const unsigned int nDims = 3 ;
const unsigned int imageCount = 21 ;
const std::string PATH = "/home/sandbom/KKI2009-ALL-MPRAGE/" ;

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

class RegistrationIterationCallback : public itk::Command
{
public:
	typedef RegistrationIterationCallback Self ;
	typedef itk::Command Superclass ;
	typedef itk::SmartPointer<RegistrationIterationCallback> Pointer ;
	itkNewMacro(RegistrationIterationCallback);
	typedef itk::RegularStepGradientDescentOptimizer OptimizerType ;
	typedef OptimizerType * OptimizerPointerType ;

	typedef itk::MultiResolutionImageRegistrationMethod < ImageType, ImageType > RegistrationMethodType ;
	typedef RegistrationMethodType * RegistrationPointerType ;
	
	// non const
	void Execute(itk::Object * caller, const itk::EventObject & event) 
	{
		 RegistrationPointerType registration = dynamic_cast < RegistrationPointerType > ( caller ) ;
		 std::cout << "Level: " << registration->GetCurrentLevel () << std::endl ;
		 // get optimizer to output images at iteration i 

		 OptimizerPointerType optimizer = dynamic_cast < OptimizerPointerType > ( registration->GetModifiableOptimizer() ) ;
		 optimizer->SetMaximumStepLength ( optimizer->GetMaximumStepLength() * 0.5 ) ;
	} 	
	
	// const 
	void Execute(const itk::Object * caller, const itk::EventObject & event)
	{
	}
};


int main(int argc, char * argv[])
{
	// if (argc < 4)
	// {
	// 	// ./Registration fixedImage lower upper
	// 	std::cout << "missing parameters! usage: ./Registration [-fixedImage=file] [-lower=num] [-upper=num]" << std::endl;
	// 	std::cout << "(includes endpoints) 0 <= lower <= numberOfImages, lower < upper <= numberOfImages"
	// } else if (argc > 4) 
	// {
	// 	std::cout << "too many parameters! usage: ./Registration [-fixedImage=file] [-lower=num] [-upper=num]" << std::endl;
	// 	std::cout << " 0 <= lower <= numberOfImages, lower < upper <= numberOfImages"
	// }

	// std::string fixedImageFile = argv[1];
	// int lower = argv[2];
	// int upper = argv[3];

   std::string fixedImageFile = "KKI2009-12-MPRAGE.nii.gz"; 
   
   ImageReaderType::Pointer fixedReader = ImageReaderType::New();
   fixedReader->SetFileName(fixedImageFile);
   fixedReader->Update();
   // another add filter to add affinely transformed images
   AddFilterType::Pointer tAddFilter = AddFilterType::New() ; 	

   std::string fixedFileNum = fixedImageFile.substr(8,2)
   int ffn = std::stoi(fixedFileNum);
   // add fixedImage to tImages
	std::string pre = "KKI2009-" ;
	std::string post = "-MPRAGE.nii.gz" ;
	for (int i = 1; i <= 21; ++i)
	{
	if ( i != ffn ) {

		ImageReaderType::Pointer movingReader = ImageReaderType::New();
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
		is = pre + is;
		std::string fname = PATH + is + post;
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
		// parameters
		optimizer->MinimizeOn() ;
		optimizer->SetNumberOfIterations ( 100 ) ;		
		optimizer->SetMinimumStepLength( 0 ) ;
		optimizer->SetMaximumStepLength( 0.0125 ) ;
		transform->SetIdentity() ;
		registration->SetInitialTransformParameters( transform->GetParameters() ) ;
		
		// optimizer callback
		OptimizerIterationCallback::Pointer optCallback = OptimizerIterationCallback::New();
		optimizer->AddObserver(itk::IterationEvent(), optCallback); 
		
		// registration callback
		RegistrationIterationCallback::Pointer regCallback = RegistrationIterationCallback::New();
		registration->AddObserver(itk::IterationEvent(), regCallback);
	
		registration->SetFixedImageRegion ( fixedReader->GetOutput()->GetLargestPossibleRegion() ) ;
	
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
		
		// add image for affine template calculation
		tAddFilter->SetInput(i, resampleFilter->GetOutput());
		// store transformed image for deformable registration
		ImageWriterType::Pointer result = ImageReaderType::New();
		std::string resname = "af" + fname;
		result->SetFileName(resname);
		result->SetInput( resampleFilter->GetOutput() );
		result->Update();
		std::cout << "wrote result to " << std::to_string(resname) << resname;

		} // end if 
	} // end for

	tAddFilter->Update() ; 

	DivideFilterType::Pointer divFilter = DivideFilterType::New();
	divFilter->SetInput(tAddFilter->GetOutput())
	divFilter->SetConstant(imageCount);
	divFilter->Update();
// write intermediate result for affine template
	//std::string atname = PATH + "a" + std::to_string(lower) + std::to_string(upper) + "intermediate.nii.gz";
	std::string atname = PATH + "affineTemplate.nii.gz";
	ImageWriterType::Pointer atWriter = ImageWriterType::New() ;
	atWriter->SetFileName( atname ) ;
	atWriter->SetInput( divFilter->GetOutput) ;
	atWriter->Update();
	std::cout << "wrote affine template to file " << atname;
	// std::to_string(lower) " to " << std::to_string(upper) "to 

// done with affine registration.
return 0;
}
