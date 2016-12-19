#include "stdafx.h"
#include "GazeEstimator.h"
#include "Signature.h"
#include "resource.h"
#include "include/LandmarkDetectorModel.h"
#include "GazeEstimation.h"

using namespace Eyw;


// For subpixel accuracy drawing
const int gaze_draw_shiftbits = 4;
const int gaze_draw_multiplier = 1 << 4;

float fx, fy, cx, cy; //focal length e optical axis centre

LandmarkDetector::CLNF clnf_model;
LandmarkDetector::FaceModelParameters det_parameters;

cv::Point3f leftEyeVector; 
cv::Point3f rightEyeVector;

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
		clnf_model = LandmarkDetector::CLNF(det_parameters.model_location);

		cx = m_inFrameImagePtr->GetWidth() / 2.0f;
		cy = m_inFrameImagePtr->GetHeight() / 2.0f;

		fx = 500 * ( m_inFrameImagePtr->GetWidth() / 640.0);
		fy = 500 * ( m_inFrameImagePtr->GetHeight() / 480.0);

		fx = (fx + fy) / 2.0;
		fy = fx;

		leftEyeVector = cv::Point3f(0, 0, -1);
		rightEyeVector = cv::Point3f(0, 0, -1);

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
		/// TODO: add your logic

		FaceAnalysis::EstimateGaze(clnf_model, leftEyeVector, fx, fy, cx, cy, true);
		FaceAnalysis::EstimateGaze(clnf_model, rightEyeVector, fx, fy, cx, cy, false);


		m_outGazeEstimateLeftPtr->SetValue(leftEyeVector.x, leftEyeVector.y, leftEyeVector.z );
		m_outGazeEstimateRightPtr->SetValue(rightEyeVector.x, rightEyeVector.y, rightEyeVector.z);

	}
	catch(...)
	{
	}
	return true;
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