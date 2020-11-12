#include <cstdlib>
#include <stdlib.h>
#include <string>
#include <sstream>
#include <typeinfo>
#include "itkImage.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkVector.h"
#include "itkSymmetricForcesDemonsRegistrationFilter.h"
#include "itkHistogramMatchingImageFilter.h"
#include "itkWarpImageFilter.h"
#include "itkLinearInterpolateImageFunction.h"
#include "itkCommand.h"
#include "itkNaryAddImageFilter.h"
#include "itkDivideImageFilter.h"

const unsigned int nDims = 3;

// set up types
typedef itk::Image < double, nDims > ImageType ;
typedef itk::ImageFileReader < ImageType > ImageReaderType ; 
typedef itk::ImageFileWriter < ImageType > ImageWriterType ;
typedef itk::HistogramMatchingImageFilter <ImageType, ImageType> MatchingFilterType;
typedef itk::Vector<double, nDims> VectorPixelType;
typedef itk::Image<VectorPixelType, nDims> DisplacementFieldType;
typedef itk::SymmetricForcesDemonsRegistrationFilter<ImageType, ImageType, DisplacementFieldType> RegistrationFilterType;
typedef itk::WarpImageFilter<ImageType, ImageType, DisplacementFieldType> WarperType;
typedef itk::LinearInterpolateImageFunction< ImageType, double> InterpolatorType;
typedef itk::NaryAddImageFilter < ImageType, ImageType > AddFilterType ;
typedef itk::DivideImageFilter<ImageType, ImageType, ImageType> DivideFilterType;

// used code from: https://itk.org/Doxygen/html/Examples_2RegistrationITKv3_2DeformableRegistration3_8cxx-example.html to set up the registration and part of callback
// the callback for writing images during deformable registration- observer added if observe flag is set by user
class CommandIterationUpdate : public itk::Command
{
	public:
	typedef CommandIterationUpdate Self ;
	typedef itk::Command Superclass ;
	typedef itk::SmartPointer<Self> Pointer ;	
	itkNewMacro(CommandIterationUpdate);
		
	protected:
	typedef itk::Image <double, nDims > ImageType;
	typedef itk::Vector<double, nDims> VectorPixelType;
	typedef itk::Image<VectorPixelType, nDims> DisplacementFieldType;
	typedef itk::SymmetricForcesDemonsRegistrationFilter<ImageType, ImageType, DisplacementFieldType> RegistrationFilterType;
	typedef itk::WarpImageFilter<ImageType, ImageType, DisplacementFieldType> WarperType;
	typedef itk::LinearInterpolateImageFunction< ImageType, double> InterpolatorType;

	public:
	void Execute(itk::Object * caller, const itk::EventObject & event) 
	{
		Execute( (const itk::Object *)caller, event);
	}

	void Execute(const itk::Object * object, const itk::EventObject & event)
	{
		const RegistrationFilterType * filter =  static_cast< const RegistrationFilterType * >( object );
		int currentIteration = filter->GetElapsedIterations();  
		std::cout << "elapsed iterations " << currentIteration << std::endl;
		if (currentIteration == 1 || currentIteration % 20 == 0){
			
			// grab images from current registration	
			const ImageType * movingImage = static_cast <const ImageType *>(filter->GetMovingImage());
			const ImageType * fixedImage = static_cast <const ImageType *>(filter->GetFixedImage());

			// set up warper
			InterpolatorType::Pointer interpolator = InterpolatorType::New();
			WarperType::Pointer warper = WarperType::New();
			warper->SetInput(movingImage);
			warper->SetInterpolator(interpolator) ;
			warper->SetOutputSpacing(fixedImage->GetSpacing());
			warper->SetOutputOrigin(fixedImage->GetOrigin());
			warper->SetOutputDirection(fixedImage->GetDirection());
			warper->SetDisplacementField(filter->GetOutput());
			warper->Update();			
			std::stringstream itnum;
			itnum << currentIteration;
			std::string fname = "out" + itnum.str() + ".nii.gz";
			std::cout << "writing " << fname << "..." << std::endl;	
			// write image with transform at current iteration
			ImageWriterType::Pointer writer = ImageWriterType::New();	
			writer->SetFileName(fname);
			writer->SetInput(warper->GetOutput());
			writer->Update();	
			std::cout << "wrote " << fname << std::endl;
		}	
	}
};			       

int main(int argc, char * argv[])
{
	// assume parameters are expected types and the fixed file is in the build directory i.e. can be accessed directly by filename 
	if (argc < 6 || argc > 6)
	{
		// example: ./dRegistration affineTemplate.nii.gz 1 12 0 1 --> deformably register images 1 to 12 to affineTemplate.nii.gz, don't divide the result and add an observer --> output file includes intermediate deformable templates and  d1_12intermediate.nii.gz 
		std::cout << "check parameters! usage: ./dRegistration [-fixedImage=filename] [-lower=num] [-upper=num] [-doDivide=num{0,1}] [-observe=num{0,1}]" << std::endl;
		std::cout << "(includes endpoints) 0 <= lower <= numberOfImages, lower < upper <= numberOfImages , doDivide = 0/1 --> don't divide/do divide final image; observe = 0/1 don't add/do add observer for intermediate deformable templates" << std::endl;
		std::cout << "./dRegistration affineTemplate.nii.gz 1 12 0 1 --> deformably register images 1 to 12 to affineTemplate.nii.gz, don't divide the result and add an observer --> output file includes intermediate deformable templates and  d1_12intermediate.nii.gz" << std::endl;
		exit(0);	
	}	
	
	// parse args
	std::string atname = argv[1];
	int lower = atoi(argv[2]);
	int upper = atoi(argv[3]);
	int doDivide = atoi(argv[4]);
	int observer = atoi(argv[5]);

	// affinely registered files for moving images- assume they are in build directory, change as needed
	std::string pre = "afKKI2009-";
	std::string post = "-MPRAGE.nii.gz";

	AddFilterType::Pointer dAddFilter = AddFilterType::New() ; 	
	ImageReaderType::Pointer fixedReader = ImageReaderType::New();
	fixedReader->SetFileName(atname);
	fixedReader->Update();
       
	for (int i = lower; i <= upper; ++i)
	{
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
		std::cout << "deformably registering " << fname << std::endl;
	
		ImageReaderType::Pointer movingReader = ImageReaderType::New();
		movingReader->SetFileName( fname );
		movingReader->Update();
		// intensity matching of fixed and moving images
		MatchingFilterType::Pointer matcher = MatchingFilterType::New();
		matcher->SetInput(movingReader->GetOutput());
		matcher->SetReferenceImage(fixedReader->GetOutput());
		matcher->SetNumberOfHistogramLevels(1024);
		matcher->SetNumberOfMatchPoints(7);
		matcher->ThresholdAtMeanIntensityOn();
		RegistrationFilterType::Pointer dregistration = RegistrationFilterType::New();

		// add callback based on observer flag argv[5]
		if (observer) {
			CommandIterationUpdate::Pointer observer = CommandIterationUpdate::New();
			dregistration->AddObserver(itk::IterationEvent(), observer);
		}
		dregistration->SetFixedImage(fixedReader->GetOutput());
		dregistration->SetMovingImage(matcher->GetOutput());
		dregistration->SetNumberOfIterations(60);
		dregistration->SetStandardDeviations(1.0);

		// (try to) do registration
		try {
			dregistration->Update();
		}
		catch (itk::ExceptionObject & err)
		{
			std::cout << err << std::endl;
			return EXIT_FAILURE;
		}

		// do transformation
		WarperType::Pointer warper = WarperType::New();
		InterpolatorType::Pointer interpolator = InterpolatorType::New();	
		warper->SetInput(movingReader->GetOutput());
		warper->SetInterpolator(interpolator);
		warper->SetOutputSpacing(fixedReader->GetOutput()->GetSpacing());
		warper->SetOutputOrigin(fixedReader->GetOutput()->GetOrigin());
		warper->SetOutputDirection(fixedReader->GetOutput()->GetDirection());
		warper->SetDisplacementField(dregistration->GetOutput());
		warper->Update();
		std::cout << "adding img " << i << " to add filter" << std::endl;
		if (lower == 1) {
			// have to set input at zero index so no complaining (itk::ERROR:
			// NaryAddImageFilter Input Primary is required but not set.)
			dAddFilter->SetInput(i-1, warper->GetOutput());
		} else if (lower > 1 && i == lower) {
			dAddFilter->SetInput(0, warper->GetOutput());
		} else {
		dAddFilter->SetInput(i-lower, warper->GetOutput());
		}
	} // end for
// update add filter
dAddFilter->Update();

// file range to strings
std::stringstream l;
std::stringstream u;
l << lower;
u << upper;
std::string lo = l.str();
std::string up = u.str();

// doDivide = 1 --> divide added images by upper - lower + 1 for deformable atlas
// doDivide = 0 --> just output the added images from lower to upper (for distributed runs,
// eg run1: lower = 1 and upper = 11; run2: lower = 12 and upper = 21)
ImageWriterType::Pointer writer = ImageWriterType::New() ;
if (doDivide) {
	std::cout << "dividing added images " + lo + " to " + up + " by " << lower-upper+1 << std::endl;
	DivideFilterType::Pointer divFilter = DivideFilterType::New();
	divFilter->SetInput(dAddFilter->GetOutput());
	divFilter->SetConstant(upper - lower + 1);
	divFilter->Update();
	std::string dname = lo + "_" + up + "deformableAtlas" + ".nii.gz";
	std::cout << "writing " + dname + "..." << std::endl;
	writer->SetFileName( dname ) ;
	writer->SetInput( divFilter->GetOutput()) ;
	writer->Update();
	std::cout << "wrote " + dname << std::endl;
 } else {
	// just write added images
	std::string dname = "d" + lo + "_" + up + "intermediate.nii.gz";
	std::cout << "writing " + dname + "..." << std::endl;
	writer->SetFileName( dname ) ;
	writer->SetInput( dAddFilter->GetOutput() ) ;
	writer->Update();
	std::cout << "wrote " + dname << std::endl;
}
// done with deformable registration 
 return 0;
} 
