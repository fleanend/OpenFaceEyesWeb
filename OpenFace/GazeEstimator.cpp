#include "stdafx.h"
#include "GazeEstimator.h"
#include "Signature.h"
#include "resource.h"
#include "LandmarkDetectorModel.h"

using namespace Eyw;


// For subpixel accuracy drawing
const int gaze_draw_shiftbits = 4;
const int gaze_draw_multiplier = 1 << 4;

float fx, fy, cx, cy; //focal length e optical axis centre

LandmarkDetector::CLNF clnf_model;
LandmarkDetector::FaceModelParameters det_parameters;

cv::Matx33d Euler2RotationMatrix(const vector3d_double_ptr eulerAngles)
{
	CDoubleMatrixInitInfoPtr init_info_ptr = CDoubleMatrixInitInfoPtr();
	init_info_ptr->_numColumns = 3;
	init_info_ptr->_numRows = 3;
	CDoubleMatrixPtr rotation_matrix = CDoubleMatrixPtr();

	double s1 = sin(eulerAngles->GetVersorX());
	double s2 = sin(eulerAngles->GetVersorY());
	double s3 = sin(eulerAngles->GetVersorZ());

	double c1 = cos(eulerAngles->GetVersorX());
	double c2 = cos(eulerAngles->GetVersorY());
	double c3 = cos(eulerAngles->GetVersorZ());

	rotation_matrix(0,0) = c2 * c3;
	rotation_matrix(0,1) = -c2 *s3;
	rotation_matrix(0,2) = s2;
	rotation_matrix(1,0) = c1 * s3 + c3 * s1 * s2;
	rotation_matrix(1,1) = c1 * c3 - s1 * s2 * s3;
	rotation_matrix(1,2) = -c2 * s1;
	rotation_matrix(2,0) = s1 * s3 - c1 * c3 * s2;
	rotation_matrix(2,1) = c3 * s1 + c1 * s2 * s3;
	rotation_matrix(2,2) = c1 * c2;

	return rotation_matrix;
}

cv::Vec6d GetPoseCamera(const LandmarkDetector::CLNF& clnf_model, double fx, double fy, double cx, double cy)
{
	if(!clnf_model.detected_landmarks.empty() && clnf_model.params_global[0] != 0)
	{
		double Z = fx / clnf_model.params_global[0];
	
		double X = ((clnf_model.params_global[4] - cx) * (1.0/fx)) * Z;
		double Y = ((clnf_model.params_global[5] - cy) * (1.0/fy)) * Z;
	
		return cv::Vec6d(X, Y, Z, clnf_model.params_global[1], clnf_model.params_global[2], clnf_model.params_global[3]);
	}
	else
	{
		return cv::Vec6d(0,0,0,0,0,0);
	}
}


vector3d_double_ptr RaySphereIntersect(vector3d_double_ptr rayOrigin, vector3d_double_ptr rayDir, vector3d_double_ptr sphereOrigin, float sphereRadius){

	float dx = rayDir->GetVersorX();
	float dy = rayDir->GetVersorY();
	float dz = rayDir->GetVersorZ();
	float x0 = rayOrigin->GetVersorX();
	float y0 = rayOrigin->GetVersorY();
	float z0 = rayOrigin->GetVersorZ();
	float cx = sphereOrigin->GetVersorX();
	float cy = sphereOrigin->GetVersorY();
	float cz = sphereOrigin->GetVersorZ();
	float r = sphereRadius;

	float a = dx*dx + dy*dy + dz*dz;
	float b = 2*dx*(x0-cx) + 2*dy*(y0-cy) + 2*dz*(z0-cz);
	float c = cx*cx + cy*cy + cz*cz + x0*x0 + y0*y0 + z0*z0 + -2*(cx*x0 + cy*y0 + cz*z0) - r*r;

	float disc = b*b - 4*a*c;

	float t = (-b - sqrt(b*b - 4*a*c))/2*a;

	// This implies that the lines did not intersect, point straight ahead
	if (b*b - 4 * a*c < 0)
		return cv::Point3f(0, 0, -1);

	rayDir->MultiplyInt(t);
	rayOrigin->Add(rayDir);
	return rayOrigin;
}

vector3d_double_ptr GetPupilPosition(CDoubleMatrixPtr eyeLdmks3d){

	eyeLdmks3d->Transpose(eyeLdmks3d);
	//eyeLdmks3d = eyeLdmks3d.t();

	for(int i =0; i<8; i++)
		eyeLdmks3d->GetRow(i);

	cv::Mat_<double> irisLdmks3d = eyeLdmks3d.rowRange(0,8);


	cv::Point3f p (mean(irisLdmks3d.col(0))[0], mean(irisLdmks3d.col(1))[0], mean(irisLdmks3d.col(2))[0]);
	return p;
}


void EstimateGaze(const LandmarkDetector::CLNF& clnf_model, vector3d_double_ptr gaze_absolute, float fx, float fy, float cx, float cy, bool left_eye)
{
	cv::Vec6d headPose = GetPoseCamera(clnf_model, fx, fy, cx, cy);
	cv::Vec3d eulerAngles(headPose(3), headPose(4), headPose(5));
	cv::Matx33d rotMat = Euler2RotationMatrix(eulerAngles);

	int part = -1;
	for (size_t i = 0; i < clnf_model.hierarchical_models.size(); ++i)
	{
		if (left_eye && clnf_model.hierarchical_model_names[i].compare("left_eye_28") == 0)
		{
			part = i;
		}
		if (!left_eye && clnf_model.hierarchical_model_names[i].compare("right_eye_28") == 0)
		{
			part = i;
		}
	}

	if (part == -1)
	{
		std::cout << "Couldn't find the eye model, something wrong" << std::endl;
	}


	cv::Mat eyeLdmks3d = clnf_model.hierarchical_models[part].GetShape(fx, fy, cx, cy);

	vector3d_double_ptr pupil = GetPupilPosition(eyeLdmks3d);
	vector3d_double_ptr rayDir = pupil / norm(pupil);

	cv::Mat faceLdmks3d = clnf_model.GetShape(fx, fy, cx, cy);
	faceLdmks3d = faceLdmks3d.t();
	cv::Mat offset = (cv::Mat_<double>(3, 1) << 0, -3.50, 0);
	int eyeIdx = 1;
	if (left_eye)
	{
		eyeIdx = 0;
	}

	cv::Mat eyeballCentreMat = (faceLdmks3d.row(36+eyeIdx*6) + faceLdmks3d.row(39+eyeIdx*6))/2.0f + (cv::Mat(rotMat)*offset).t();

	vector3d_double_ptr eyeballCentre = cv::Point3f(eyeballCentreMat);

	vector3d_double_ptr gazeVecAxis = RaySphereIntersect(cv::Point3f(0,0,0), rayDir, eyeballCentre, 12) - eyeballCentre;
	
	gaze_absolute = gazeVecAxis / norm(gazeVecAxis);
}

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

		EstimateGaze(clnf_model, m_outGazeEstimateLeftPtr, fx, fy, cx, cy, true);
		EstimateGaze(clnf_model, m_outGazeEstimateRightPtr, fx, fy, cx, cy, false);


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