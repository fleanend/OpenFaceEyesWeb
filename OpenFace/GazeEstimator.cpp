#include "stdafx.h"
#include "GazeEstimator.h"
#include "Signature.h"
#include "resource.h"
#include "include/LandmarkDetectorModel.h"
#include "include/GazeEstimation.h"
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>

using namespace Eyw;

//////////////////////////////////////////////////////////
/// <summary>
/// Block Signature.
/// </summary>
Eyw::block_class_registrant g_GazeEstimator( 
	Eyw::block_class_registrant::block_id( "GazeEstimator" )
		.begin_language( EYW_LANGUAGE_US_ENGLISH )
			.name( "GazeEstimator" )
			.description( "This Block retrieves 2 vectors corresponding to the gaze of a single person inside an image or videoframe."	)
			.libraries( "MyLibrary" )
			.bitmap( IDB_GAZEESTIMATOR_BITMAP )
		.end_language()	
		.begin_authors()
			.author( EYW_OPENFACE_CATALOG_AUTHOR_ID )
		.end_authors()
		.begin_companies()
			.company( EYW_OPENFACE_COMPANY_ID )
		.end_companies()
		.begin_licences()
			.licence( EYW_OPENFACE_LICENSE_ID )
		.end_licences()
		.default_factory< CGazeEstimator >()
	);

//////////////////////////////////////////////////////////
// Identifiers
#define PAR_PARAMETERPIN "parameterPin"
#define IN_FRAMEIMAGE "Frame/Image"
#define OUT_GAZEESTIMATELEFT "GazeEstimateLeft"
#define OUT_GAZEESTIMATERIGHT "GazeEstimateRight"


//////////////////////////////////////////////////////////
/// <summary>
/// Constructor.
/// </summary>
//////////////////////////////////////////////////////////
CGazeEstimator::CGazeEstimator( const Eyw::OBJECT_CREATIONCTX* ctxPtr )
:	Eyw::CBlockImpl( ctxPtr )
{
	m_inFrameImagePtr=NULL;
	m_outGazeEstimateLeftPtr=NULL;
	m_outGazeEstimateRightPtr=NULL;

	_schedulingInfoPtr->SetActivationEventBased( true );
	_schedulingInfoPtr->GetEventBasedActivationInfo()->SetActivationOnInputChanged( IN_FRAMEIMAGE, true );

}

//////////////////////////////////////////////////////////
/// <summary>	
/// Destructor.
/// </summary>
//////////////////////////////////////////////////////////
CGazeEstimator::~CGazeEstimator()
{
}

//////////////////////////////////////////////////////////
/// <summary>
/// Block signature initialization.
/// </summary>
//////////////////////////////////////////////////////////
void CGazeEstimator::InitSignature()
{	 
	m_parParameterPinPtr= Eyw::Cast<Eyw::IBool*>(
						 SetParameter(Eyw::pin::id(PAR_PARAMETERPIN)
							 .name("parameterPin")
							 .description("Not sure if needed")
							 .type<Eyw::IBool>()
							 )->GetDatatype() );
	SetInput(Eyw::pin::id(IN_FRAMEIMAGE)
		.name("Frame/Image")
		.description("Input Image or Frame")
		.type<Eyw::IImage>()
		);
	SetOutput(Eyw::pin::id(OUT_GAZEESTIMATELEFT)
		.name("GazeEstimateLeft")
		.description("Vector estimating the left eye gaze direction")
		.type<Eyw::IVector3DDouble>()
		);
	SetOutput(Eyw::pin::id(OUT_GAZEESTIMATERIGHT)
		.name("GazeEstimateRight")
		.description("Vector estimating the right eye gaze direction")
		.type<Eyw::IVector3DDouble>()
		);

}

//////////////////////////////////////////////////////////
/// <summary>
/// Block signature check.
/// </summary>
//////////////////////////////////////////////////////////
void CGazeEstimator::CheckSignature()
{
	m_parParameterPinPtr=get_parameter_datatype<Eyw::IBool>(PAR_PARAMETERPIN);
	_signaturePtr->GetInputs()->FindItem( IN_FRAMEIMAGE );
	_signaturePtr->GetOutputs()->FindItem( OUT_GAZEESTIMATELEFT );
	_signaturePtr->GetOutputs()->FindItem( OUT_GAZEESTIMATERIGHT );

}

//////////////////////////////////////////////////////////
/// <summary>
/// Block signature deinitialization.
/// </summary>
//////////////////////////////////////////////////////////
void CGazeEstimator::DoneSignature()
{
	m_parParameterPinPtr=NULL;

}

/// Block Actions

//////////////////////////////////////////////////////////
/// <summary>
/// Block initialization action.
/// </summary>
/// <returns>
/// true if success, otherwise false.
/// </returns>
//////////////////////////////////////////////////////////
bool CGazeEstimator::Init() throw()
{
	try
	{
		/// TODO: Init data structures here 

		m_inFrameImagePtr = get_input_datatype<Eyw::IImage>( IN_FRAMEIMAGE );
		m_outGazeEstimateLeftPtr = get_output_datatype<Eyw::IVector3DDouble>( OUT_GAZEESTIMATELEFT );
		m_outGazeEstimateRightPtr = get_output_datatype<Eyw::IVector3DDouble>( OUT_GAZEESTIMATERIGHT );
		
		clnf_model = LandmarkDetector::CLNF(det_parameters.model_location);
		det_parameters.track_gaze = true;

		return true;
	}
	catch(...)
	{
		return false;
	}
}

//////////////////////////////////////////////////////////
/// <summary>
/// Block start action.
/// </summary>
/// <returns>
/// true if success, otherwise false.
/// </returns>
//////////////////////////////////////////////////////////
bool CGazeEstimator::Start() throw()
{
	try
	{
		/// TODO: add your logic
		return true;
	}
	catch(...)
	{
		return false;
	}
}

//////////////////////////////////////////////////////////
/// <summary>
/// Block execution action.
/// </summary>
/// <returns>
/// true if success, otherwise false.
/// </returns>
//////////////////////////////////////////////////////////
bool CGazeEstimator::Execute() throw()
{
	try
	{
		int rows = m_inFrameImagePtr->GetHeight();
		int cols = m_inFrameImagePtr->GetWidth();

		PrepareCvImage(m_inFrameImagePtr, captured_image);

		cx = rows / 2.0f;
		cy = cols / 2.0f;

		fx = 500 * ( rows / 640.0);
		fy = 500 * ( cols / 480.0);

		fx = (fx + fy) / 2.0;
		fy = fx;

		if(!captured_image.empty())
		{
			// Reading the images
			cv::Mat_<uchar> grayscale_image;

			if(captured_image.channels() == 3)
			{
				cv::cvtColor(captured_image, grayscale_image, CV_BGR2GRAY);				
			}
			else
			{
				grayscale_image = captured_image.clone();				
			}


			bool detection_success = DetectLandmarksInVideo(grayscale_image, clnf_model, det_parameters);
			double detection_certainty = clnf_model.detection_certainty;

			leftEyeVector = cv::Point3f(0, 0, -1);
			rightEyeVector = cv::Point3f(0, 0, -1);


			if (det_parameters.track_gaze && detection_success && clnf_model.eye_model)
			{
				FaceAnalysis::EstimateGaze(clnf_model, leftEyeVector, fx, fy, cx, cy, true);
				FaceAnalysis::EstimateGaze(clnf_model, rightEyeVector, fx, fy, cx, cy, false);
			}

			m_outGazeEstimateLeftPtr->SetValue(leftEyeVector.x, leftEyeVector.y, leftEyeVector.z );
			m_outGazeEstimateRightPtr->SetValue(rightEyeVector.x, rightEyeVector.y, rightEyeVector.z);

			m_outGazeEstimateLeftPtr->SetCreationTime(_clockPtr->GetTime());
			m_outGazeEstimateRightPtr->SetCreationTime(_clockPtr->GetTime());

			Notify_DebugString("Completing execute()");

		}

		frame_count++;

		/// TODO: add your logic



	}
	catch(...)
	{
		Notify_ErrorString("Exception thrown on execution");
	}
	return true;
}

	void CGazeEstimator::PrepareCvImage(const image_ptr& sourceImagePtr, cv::Mat &destinationImage)
	{
		BOOST_ASSERT(sourceImagePtr);		

		try
		{
			if (!sourceImagePtr)
				return;
			const RECT_2D_INT roi = sourceImagePtr->GetROI();

			int width, height, step;
			width = sourceImagePtr->GetWidth();
			height = sourceImagePtr->GetHeight();
			step = sourceImagePtr->GetStepSize();
			if (sourceImagePtr->GetColorModel() == Eyw::ecmBW)
				destinationImage = *(new cv::Mat(height, width, CV_8UC1, sourceImagePtr->GetBuffer(), step));
			if ((sourceImagePtr->GetColorModel() == Eyw::ecmRGB) || (sourceImagePtr->GetColorModel() == Eyw::ecmBGR))
				destinationImage = *(new cv::Mat(height, width, CV_8UC3, sourceImagePtr->GetBuffer(), step));
			return;
			
		}
		catch (const IException&)
		{
			return;
		}
	}


//////////////////////////////////////////////////////////
/// <summary>
/// Block stop action.
/// </summary>
//////////////////////////////////////////////////////////
void CGazeEstimator::Stop() throw()
{
	try
	{
		Notify_DebugString("Stopping...\n");
	}
	catch(...)
	{
	}
}

//////////////////////////////////////////////////////////
/// <summary>
/// Block deinitialization action.
/// </summary>
//////////////////////////////////////////////////////////
void CGazeEstimator::Done() throw()
{
	try
	{
		m_inFrameImagePtr = NULL;
		m_outGazeEstimateLeftPtr = NULL;
		m_outGazeEstimateRightPtr = NULL;

		Notify_DebugString("We are done\n");

	}
	catch(...)
	{
	}
}

/// optionals

//////////////////////////////////////////////////////////
/// <summary>
/// Manage the ChangedParameter event.
/// </summary>
/// <param name="csParameterID">
/// [in] id of the changed parameter.
/// </param>
//////////////////////////////////////////////////////////
void CGazeEstimator::OnChangedParameter( const std::string& csParameterID )
{
	/// TODO: manage the changed parameters
}