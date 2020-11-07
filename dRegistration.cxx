#include <cstdlib>
#include <string>
#include <sstream>
#include "itkDemonsRegistrationFilter.h"
#include "itkHistogramMatchingFilter.h"
#include "itkCastImageFilter.h"
#include "itkDisplacementFieldTransform.h"


// set up types
typedef itk::Image < double, nDims > ImageType ;
typedef itk::ImageFileReader < ImageType > ImageReaderType ; 
typedef itk::ImageFileWriter < ImageType > ImageWriterType ;
typedef itk::NaryAddImageFilter < ImageType, ImageType > AddFilterType ;
typedef itk::DivideImageFilter < ImageType, ImageType  > DivideFilterType ;
typedef itk::Image < float, nDims > InternalImageType;
typedef itk::CastImageFilter< ImageType, InternalImageType > ImageCasterType;
typedef itk::HistogramMatchingImageFilter <InternalImageType, InternalImageType> MatchingFilterType;
typedef itk::Vector<float, nDims> VectorPixelType;
typedef itk::Image<VectorPixelType, nDims> DisplacementFieldType;
typedef itk::DemonsRegistrationFilter<InternalImageType, InternalImageType, DisplacementFieldType> RegistrationFilterType;
typedef itk::Image<unsigned char, nDims> OutputImageType;
typedef itk::ResampleImageFilter<ImageType, ImageType, double, float> WarperType;
typedef itk::DisplacementFieldTransform<float, nDims> DisplacementFieldTransformType;

const std::string PATH = "/home/sandbom/KKI2009-ALL-MPRAGE/" ;

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
		 // get optimizer to output images at iteration i 
		 if optimizer->GetCurrentIteration() % 20 == 0 {

		 }
	} 	
	
	// const 
	void Execute(const itk::Object * caller, const itk::EventObject & event)
	{
	}
};


int main(int argc, char * argv[])
{

if (argc < 4)
	{
		// ./Registration fixedImage lower upper
		std::cout << "missing parameters! usage: ./dRegistration [-fixedImage=file] [-lower=num] [-upper=num]" << std::endl;
		std::cout << "(includes endpoints) 0 <= lower <= numberOfImages, lower < upper <= numberOfImages"
	} else if (argc > 4) 
	{
		std::cout << "too many parameters! usage: ./Registration [-fixedImage=file] [-lower=num] [-upper=num]" << std::endl;
		std::cout << " 0 <= lower <= numberOfImages, lower < upper <= numberOfImages"
	}

	std::string fixedImageFile = argv[1];
	int lower = argv[2];
	int upper = argv[3];


// based on https://itk.org/Doxygen/html/Examples_2RegistrationITKv4_2DeformableRegistration2_8cxx-example.html
	// demons deformable registration v4

// deformably register images to the affine template
// affine template is now fixed image and the tImages are the moving images
// go through all of the images in tImages

// yet another add filter to add deformably transformed images
	AddFilterType::Pointer dAddFilter = AddFilterType::New() ; 	
	// store affinely registered images for later
	ImageType::Pointer dImages[imageCount];
	//set fixed image as affine template

	std::string atname = "affineTemplate.nii.gz";
	ImageReaderType::Pointer fixedReader = ImageReaderType::New();
	fixedReader->SetFileName(atname);
	fixedReader->Update();

	ImageCasterType::Pointer fixedImageCaster = ImageCasterType::New();
	fixedImageCaster->SetInput( fixedReader->GetOutput() );
	std::string pre = "KKI2009-";
	std::string post = "-MPRAGE.nii.gz";
	// go over all tImages and register to fixedImage (affineTemplate)
	// add results to dImages to then be averaged for the deformable atlas
	for (int i = lower; i <= upper; ++i)
	{
		// get ith moving image and cast set up for demons
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
		std::cout << "deformably registering " << fname << std::endl;
		ImageReaderType::Pointer movingReader = ImageReaderType::New();
		movingReader->SetFileName( fname );
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

	} // end for
	dAddFilter->Update();
	// write intermediate result of deformable atlas
	std::string dname = PATH + "d" + std::to_string(lower) + std::to_string(upper) + "intermediate.nii.gz";
	ImageWriterType::Pointer dWriter = ImageWriterType::New() ;
	dWriter->SetFileName( dname ) ;
	dWriter->SetInput( dAddFilter->GetOutput() ) ;
	dWriter->Update();
	std::cout << "wrote added images from " << std::to_string(lower) " to " << std::to_string(upper) "to file " << dname;



} 



