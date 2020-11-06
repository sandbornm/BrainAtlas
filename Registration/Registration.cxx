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
#include <ctime>
#include <cstdlib>

#include "itkDemonsRegistrationFilter.h"
#include "itkHistogramMatchingFilter.h"
#include "itkCastImageFilter.h"
#include "itkDisplacementFieldTransform.h"



// constants
const unsigned int nDims = 3 ;
const unsigned int imageCount = 21 ;
const std::string PATH = "/home/sandbom/itkants/KKI2009-ALL-MPRAGE/" ;

// set up types
typedef itk::Image < double, nDims > ImageType ;
typedef itk::ImageFileReader < ImageType > ImageReaderType ; 
typedef itk::ImageFileWriter < ImageType > ImageWriterType ;
typedef itk::NaryAddImageFilter < ImageType, ImageType > AddFilterType ;
typedef itk::ShiftScaleImageFilter < ImageType, ImageType  > ScaleFilterType ;
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
		std::cout << optimizer->GetCurrentIteration() << " " << optimizer->GetValue() << std::endl;
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

   unsigned int maxIterations = 200;
   std::string pathToRandomFixedImage = "../randomFixedImage12.nii.gz";
   ImageReaderType::Pointer fixedReader = ImageReaderType::New();
   // the index associated with the randomFixedImage 
   int randSubject = 11;
    

   // another add filter to add affinely transformed images
   AddFilterType::Pointer tAddFilter = AddFilterType::New() ; 	
   // store affinely registered images for later
   ImageType::Pointer tImages[imageCount];
   // add fixedImage to tImages
   tImages[randSubject] = fixedReader->GetOutput();	

	for (int i = 0; i < imageCount; ++i)
	{
	if ( i != randSubject ) {
		std::string fname = readers[i]->GetFileName() ; 
		std::cout << "now registering " << fname << std::endl;  
		// gather registration materials 
		ImageReaderType::Pointer movingReader = ImageReaderType::New();
		movingReader->SetFileName( fname );
		movingReader->Update() ;
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
		optimizer->SetNumberOfIterations ( 50 ) ;		
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
	
		registration->SetNumberOfLevels( 3 ) ;
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
		
		tAddFilter->SetInput(i, resampleFilter->GetOutput());
		// store transformed image for deformable registration
		tImages[i] = resampleFilter->GetOutput();

		} // end if 
	} // end for

// average the aff registered images and the fixed image = affine template
	tAddFilter->Update() ; 
	DivideFilterType::Pointer aDivFilter = DivideFilterType::New() ;
	aDivFilter->SetInput( tAddFilter->GetOutput()) ; 
	aDivFilter->SetConstant(imageCount) ; 
	aDivFilter->Update() ;
// write affine template
	std::string atname = PATH + "affineTemplate.nii.gz";
	ImageWriterType::Pointer atWriter = ImageWriterType::New() ;
	atWriter->SetFileName( atname ) ;
	atWriter->SetInput( aDivFilter->GetOutput() ) ;
	atWriter->Update();

// deformably register images to the affine template
// affine template is now fixed image and the tImages are the moving images
// go through all of the images in tImages

// yet another add filter to add deformably transformed images
	AddFilterType::Pointer dAddFilter = AddFilterType::New() ; 	
	// store affinely registered images for later
	ImageType::Pointer dImages[imageCount];
	//set fixed image as affine template
	ImageReaderType::Pointer fixedReader = ImageReaderType::New();
	fixedReader->SetFileName(atname);
	fixedReader->Update();

// based on https://itk.org/Doxygen/html/Examples_2RegistrationITKv4_2DeformableRegistration2_8cxx-example.html
	// demons deformable registration v4
	typedef itk::Image < float, nDims > InternalImageType;
	typedef itk::CastImageFilter< ImageType, InternalImageType > ImageCasterType;
	typedef itk::HistogramMatchingImageFilter <InternalImageType, InternalImageType> MatchingFilterType;

	typedef itk::Vector<float, nDims> VectorPixelType;
	typedef itk::Image<VectorPixelType, nDims> DisplacementFieldType;
	typedef itk::DemonsRegistrationFilter<InternalImageType, InternalImageType, DisplacementFieldType> RegistrationFilterType;
	typedef itk::Image<unsigned char, nDims> OutputImageType;
	typedef itk::ResampleImageFilter<ImageType, ImageType, double, float> WarperType;
	typedef itk::DisplacementFieldTransform<float, nDims> DisplacementFieldTransformType;
	
	ImageCasterType::Pointer fixedImageCaster = ImageCasterType::New();
	fixedImageCaster->SetInput( fixedReader->GetOutput() );
	// go over all tImages and register to fixedImage (affineTemplate)
	// add results to dImages to then be averaged for the deformable atlas
	for (int i = 0; i < imageCount; ++i)
	{
		// get ith moving image and cast set up for demons
		std::string fnamei = readers[i]->GetFileName() ; 
		std::cout << "deformably registering " << fnamei << std::endl;
		ImageReaderType::Pointer movingReader = ImageReaderType::New();
		movingReader->SetFileName( fnamei );
		movingReader->Update();
	        ImageCasterType::Pointer movingImageCaster = ImageCasterType::New();
		movingImageCaster->SetInput(movingReader->GetOutput());
		
		MatchingFilterType::Pointer matcher = MatchingImageFilterType:New();
		matcher->SetInput(movingImageCaster->GetOutput());
		matcher->SetReferenceImage(fixedImageCaster->GetOutput());
		matcher->SetNumberOfHistogramLevels(1024);
		matcher->SetNumberOfMatchPoints(7);
		matcher->ThresholdAtMeanIntensityOn();
		RegistrationFilterType::Pointer dregistration = RegistrationFilterType::New();
		// dont forget iteration update callback

		// set up demons
		dregistration->SetFixedImage(fixedImageCaster->GetOutput());
		dregistration->SetMovingImage(matcher->GetOutput());
		dregistration->SetNumberOfIterations(50);
		dregistration->SetStandardDeviations(1.0);
		
		// do registration
		dregistration->Update();

		// do transformation
		WarperType::Pointer warper = WarperType::New();
		warper->SetInput(movingImageReader->GetOutput());
		warper->UseReferenceImageOn();
		warper->SetReferenceImage(fixedImageReader->GetOutput());
		
		DisplacementFieldTransformType::Pointer transform =  DisplacementFieldTransformType::New();
		transform->SetDisplacementField(dregistration->GetOutput());
		warper->SetTransform(transform);
		
		dAddFilter->SetInput(warper->GetOutput());



	}








// set up deformable registration

//for (int i = 1; i <= imageCount; ++i)




 return 0;
}

