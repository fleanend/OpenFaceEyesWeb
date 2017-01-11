#include "stdafx.h"
#include "GazeEstimator.h"
#include "Signature.h"
#include "resource.h"
#include "include/LandmarkDetectorModel.h"
#include "include/GazeEstimation.h"
#include <opencv2/imgproc.hpp>

using namespace Eyw;

cv::Point3f GetPupilPosition(cv::Mat_<double> eyeLdmks3d); 

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
#define PAR_MODEL_LOCATION "model_locationPin"

#define PAR_LIMIT_POSE "limit_posePin" //bool
#define PAR_REFINE_HIERARCHICAL "refine_hierarchicalPin" //bool
#define PAR_REFINE_PARAMETERS "refine_parametersPin" //bool

#define PAR_REINIT_VIDEO_EVERY "reinit_video_everyPin" //int
#define PAR_NUM_OPTIMIZATION_ITERATION "num_optimisation_iterationPin" //int

#define PAR_VALIDATION_BOUNDARY "validation_boundaryPin" //double
#define PAR_SIGMA "sigmaPin" //double
#define PAR_REG_FACTOR "reg_factorPin" //double
#define PAR_WEIGHT_FACTOR "weight_factorPin" //double
#define PAR_FX "fxPin" //double
#define PAR_FY "fyPin" //double
#define PAR_CX "cxPin" //double
#define PAR_CY "cyPin" //double

#define IN_FRAMEIMAGE "Frame/Image"
#define OUT_GAZEESTIMATELEFT "GazeEstimateLeft"
#define OUT_GAZEESTIMATERIGHT "GazeEstimateRight"
#define OUT_PUPILLEFT "PupilLeft"
#define OUT_PUPILRIGHT "PupiRight"
#define OUT_PROCESSEDIMAGE "ProcessedImage"


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
	m_outProcessedImagePtr = NULL;

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
	const char* model_location_description =
		R"( "Location for the landmark detection model used by OpenFace: 
	- "main_clnf_general" (default), trained on Multi-PIE of varying pose and illumination and In-the-wild data, works well for head pose tracking (CLNF model);
	- "main_clnf_wild", trained on In-the-wild data, works better in noisy environments (not very well suited for head pose tracking), (CLNF in-the-wild model);
	- "main_clm_general", a less accurate but slightly faster CLM model trained on Multi-PIE of varying pose and illumination and In-the-wild data, works well for head pose tracking;
	- "main_clm-z", trained on Multi-PIE and BU-4DFE datasets, works with both intensity and depth signals (CLM-Z).)";

	m_model_locationPinPtr = Eyw::Cast<Eyw::IInt*>(
	                     SetParameter(Eyw::pin::id(PAR_MODEL_LOCATION)
	                         .name("Model location")
	                         .description(model_location_description)
	                         .type<Eyw::IInt>()
	                         .set_combo_layout(4) // change the number to the number of items
	                             .item(0, "model/main_clnf_general.txt")
	                             .item(1, "model/main_clnf_wild.txt")
	                             .item(2, "model/main_clm_general.txt")
								 .item(3, "model/main_clm-z.txt")
	                         )->GetDatatype() );
	m_model_locationPinPtr->SetValue(0);

	m_limit_posePinPtr= Eyw::Cast<Eyw::IBool*>(
						 SetParameter(Eyw::pin::id(PAR_LIMIT_POSE)
							 .name("Limit Pose")
							 .description("Should pose be limited to 180 degrees frontal")
							 .type<Eyw::IBool>()
							 )->GetDatatype() );
	m_limit_posePinPtr->SetValue(true);

	m_refine_hierarchicalPinPtr= Eyw::Cast<Eyw::IBool*>(
						 SetParameter(Eyw::pin::id(PAR_REFINE_HIERARCHICAL)
							 .name("Refine Hierarchical")
							 .description("Should the model be refined hierarchically (if available)")
							 .type<Eyw::IBool>()
							 )->GetDatatype() );
	m_refine_hierarchicalPinPtr->SetValue(true);

	m_refine_parametersPinPtr= Eyw::Cast<Eyw::IBool*>(
						 SetParameter(Eyw::pin::id(PAR_REFINE_PARAMETERS)
							 .name("Refine Parameters")
							 .description("Should the parameters be refined for different scales")
							 .type<Eyw::IBool>()
							 )->GetDatatype() );
	m_refine_parametersPinPtr->SetValue(true);

	m_num_optimisation_iterationPinPtr= Eyw::Cast<Eyw::IInt*>(
						 SetParameter(Eyw::pin::id(PAR_NUM_OPTIMIZATION_ITERATION)
							 .name("Number of optimisation iterations")
							 .description("A number of RLMS or NU-RLMS iterations")
							 .type<Eyw::IInt>()
							 .set_int_domain()
							 .min(0)
							 )->GetDatatype() );
	m_num_optimisation_iterationPinPtr->SetValue(5);

	m_reinit_video_everyPinPtr= Eyw::Cast<Eyw::IInt*>(
						 SetParameter(Eyw::pin::id(PAR_NUM_OPTIMIZATION_ITERATION)
							 .name("Reinit video every n frames")
							 .description("How often should face detection be used to attempt reinitialisation, every n frames (set to negative not to reinit)")
							 .type<Eyw::IInt>()
							 )->GetDatatype() );
	m_reinit_video_everyPinPtr->SetValue(4);



	m_validation_boundaryPinPtr= Eyw::Cast<Eyw::IDouble*>(
						 SetParameter(Eyw::pin::id(PAR_VALIDATION_BOUNDARY)
							 .name("Validation boundary")
							 .description("Landmark detection validator boundary for correct detection, the regressor output -1 (perfect alignment) 1 (bad alignment),")
							 .type<Eyw::IDouble>()
							 )->GetDatatype() );
	m_validation_boundaryPinPtr->SetValue(-0.45);

	m_sigmaPinPtr= Eyw::Cast<Eyw::IDouble*>(
						 SetParameter(Eyw::pin::id(PAR_SIGMA)
							 .name("Sigma")
							 .description("Used for the smooting of response maps (KDE sigma)")
							 .type<Eyw::IDouble>()
							 .set_double_domain()
							 .min(0)
							 )->GetDatatype() );
	m_sigmaPinPtr->SetValue(1.5);

	m_reg_factorPinPtr= Eyw::Cast<Eyw::IDouble*>(
						 SetParameter(Eyw::pin::id(PAR_REG_FACTOR)
							 .name("Regularization factor")
							 .description("Weight put to regularisation")
							 .type<Eyw::IDouble>()
							 .set_double_domain()
							 .min(0)
							 )->GetDatatype() );
	m_reg_factorPinPtr->SetValue(25);

	m_weight_factorPinPtr= Eyw::Cast<Eyw::IDouble*>(
						 SetParameter(Eyw::pin::id(PAR_WEIGHT_FACTOR)
							 .name("Weight factor")
							 .description("Factor for weighted least squares. By default 0, as for videos doesn't work well")
							 .type<Eyw::IDouble>()
							 .set_double_domain()
							 .min(0)
							 )->GetDatatype() );

	m_fxPinPtr= Eyw::Cast<Eyw::IDouble*>(
						 SetParameter(Eyw::pin::id(PAR_FX)
							 .name("Focal X")
							 .description("Focal length X coordinate")
							 .type<Eyw::IDouble>()
							 .set_double_domain()
							 .min(0)
							 )->GetDatatype() );


	m_fyPinPtr= Eyw::Cast<Eyw::IDouble*>(
						 SetParameter(Eyw::pin::id(PAR_FY)
							 .name("Focal Y")
							 .description("Focal length Y coordinate")
							 .type<Eyw::IDouble>()
							 .set_double_domain()
							 .min(0)
							 )->GetDatatype() );

	m_cxPinPtr= Eyw::Cast<Eyw::IDouble*>(
						 SetParameter(Eyw::pin::id(PAR_CX)
							 .name("Optical center X")
							 .description("Optical X axis center")
							 .type<Eyw::IDouble>()
							 .set_double_domain()
							 .min(0)
							 )->GetDatatype() );

	m_cyPinPtr= Eyw::Cast<Eyw::IDouble*>(
						 SetParameter(Eyw::pin::id(PAR_CY)
							 .name("Optical center Y")
							 .description("Optical Y axis center")
							 .type<Eyw::IDouble>()
							 .set_double_domain()
							 .min(0)
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
	SetOutput(Eyw::pin::id(OUT_PROCESSEDIMAGE)
		.name("Processed Image")
		.description("Image processed showing the estimated gaze")
		.type<Eyw::IImage>()
		);
	SetOutput(Eyw::pin::id(OUT_PUPILLEFT)
		.name("Left pupil position")
		.description("Vector estimating the right eye gaze direction")
		.type<Eyw::IVector3DInt>()
		);
	SetOutput(Eyw::pin::id(OUT_PUPILRIGHT)
		.name("Right Pupil Position")
		.description("Vector estimating the right eye gaze direction")
		.type<Eyw::IVector3DInt>()
		);
	

}

//////////////////////////////////////////////////////////
/// <summary>
/// Block signature check.
/// </summary>
//////////////////////////////////////////////////////////
void CGazeEstimator::CheckSignature()
{
	m_model_locationPinPtr = get_parameter_datatype<Eyw::IInt>(PAR_MODEL_LOCATION);

	m_limit_posePinPtr=get_parameter_datatype<Eyw::IBool>(PAR_LIMIT_POSE);
	m_refine_hierarchicalPinPtr=get_parameter_datatype<Eyw::IBool>(PAR_REFINE_HIERARCHICAL);
	m_refine_parametersPinPtr=get_parameter_datatype<Eyw::IBool>(PAR_REFINE_PARAMETERS);

	//int ptrs
	m_num_optimisation_iterationPinPtr=get_parameter_datatype<Eyw::IInt>(PAR_NUM_OPTIMIZATION_ITERATION);
	m_reinit_video_everyPinPtr=get_parameter_datatype<Eyw::IInt>(PAR_REINIT_VIDEO_EVERY);

	//double ptrs
	m_validation_boundaryPinPtr=get_parameter_datatype<Eyw::IDouble>(PAR_VALIDATION_BOUNDARY);
	m_sigmaPinPtr=get_parameter_datatype<Eyw::IDouble>(PAR_SIGMA);
	m_reg_factorPinPtr=get_parameter_datatype<Eyw::IDouble>(PAR_REG_FACTOR);
	m_weight_factorPinPtr=get_parameter_datatype<Eyw::IDouble>(PAR_WEIGHT_FACTOR);
	m_fxPinPtr=get_parameter_datatype<Eyw::IDouble>(PAR_FX);
	m_fyPinPtr=get_parameter_datatype<Eyw::IDouble>(PAR_FY);
	m_cxPinPtr=get_parameter_datatype<Eyw::IDouble>(PAR_CX);
	m_cyPinPtr=get_parameter_datatype<Eyw::IDouble>(PAR_CY);


	_signaturePtr->GetInputs()->FindItem( IN_FRAMEIMAGE );
	_signaturePtr->GetOutputs()->FindItem( OUT_GAZEESTIMATELEFT );
	_signaturePtr->GetOutputs()->FindItem( OUT_GAZEESTIMATERIGHT );
	_signaturePtr->GetOutputs()->FindItem( OUT_PROCESSEDIMAGE );
	_signaturePtr->GetOutputs()->FindItem( OUT_PUPILLEFT );
	_signaturePtr->GetOutputs()->FindItem( OUT_PUPILRIGHT );

}

//////////////////////////////////////////////////////////
/// <summary>
/// Block signature deinitialization.
/// </summary>
//////////////////////////////////////////////////////////
void CGazeEstimator::DoneSignature()
{
	m_limit_posePinPtr=NULL;
	m_refine_hierarchicalPinPtr=NULL;
	m_refine_parametersPinPtr=NULL;

	//int ptrs
	m_num_optimisation_iterationPinPtr=NULL;
	m_reinit_video_everyPinPtr=NULL;

	//double ptrs
	m_validation_boundaryPinPtr=NULL;
	m_sigmaPinPtr=NULL;
	m_reg_factorPinPtr=NULL;
	m_weight_factorPinPtr=NULL;
	m_fxPinPtr=NULL;
	m_fyPinPtr=NULL;
	m_cxPinPtr=NULL;
	m_cyPinPtr=NULL;

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
		m_outProcessedImagePtr = get_output_datatype<Eyw::IImage>( OUT_PROCESSEDIMAGE );
		m_pupilLeftPtr = get_output_datatype<Eyw::IVector3DInt>( OUT_PUPILLEFT );
		m_pupilRightPtr = get_output_datatype<Eyw::IVector3DInt>( OUT_PUPILRIGHT );
		
		det_parameters.model_location = GetComboParameterItem(PAR_MODEL_LOCATION, m_model_locationPinPtr->GetValue());

		clnf_model = LandmarkDetector::CLNF(det_parameters.model_location);

		det_parameters.track_gaze = true;
		det_parameters.limit_pose = m_limit_posePinPtr->GetValue();
		det_parameters.refine_hierarchical = m_refine_hierarchicalPinPtr->GetValue();
		det_parameters.refine_parameters = m_refine_parametersPinPtr->GetValue();

		det_parameters.num_optimisation_iteration = m_num_optimisation_iterationPinPtr->GetValue();
		det_parameters.reinit_video_every = m_reinit_video_everyPinPtr->GetValue();

		det_parameters.reg_factor = m_reg_factorPinPtr->GetValue();
		det_parameters.weight_factor = m_weight_factorPinPtr->GetValue();
		det_parameters.sigma = m_sigmaPinPtr->GetValue();
		det_parameters.validation_boundary = m_validation_boundaryPinPtr->GetValue();

		if (m_fxPinPtr->GetValue() == 0.0 || m_fyPinPtr->GetValue() == 0.0)
		{
			Notify_DebugString("fx_undefined is true\n");
			fx_undefined = true;
		}
			

		if (m_cxPinPtr->GetValue() == 0.0 || m_cyPinPtr->GetValue() == 0.0)
		{
			Notify_DebugString("cx_undefined is true\n");
			cx_undefined = true;
		}
			


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

		if(cx_undefined)
		{
			cx = rows / 2.0f;
			cy = cols / 2.0f;
			
		} else
		{
			cx = m_cxPinPtr->GetValue();
			cy = m_cyPinPtr->GetValue();
		}


		if(fx_undefined)
		{
			fx = 500 * ( rows / 640.0);
			fy = 500 * ( cols / 480.0);

			fx = (fx + fy) / 2.0;
			fy = fx;	
		} else
		{
			fx = m_fxPinPtr->GetValue();
			fy = m_fyPinPtr->GetValue();

		}


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

			visualise_tracking(captured_image, clnf_model, det_parameters, leftEyeVector, rightEyeVector, frame_count, fx, fy, cx, cy);

			m_outGazeEstimateLeftPtr->SetValue(leftEyeVector.x, leftEyeVector.y, leftEyeVector.z );
			m_outGazeEstimateRightPtr->SetValue(rightEyeVector.x, rightEyeVector.y, rightEyeVector.z);
			//riempire 	m_outProcessedImagePtr

			m_outGazeEstimateLeftPtr->SetCreationTime(_clockPtr->GetTime());
			m_outGazeEstimateRightPtr->SetCreationTime(_clockPtr->GetTime());
			m_outProcessedImagePtr->SetCreationTime(_clockPtr->GetTime());

			fillPupilPosition();

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
		m_outProcessedImagePtr = NULL;
		m_pupilLeftPtr = NULL;
		m_pupilRightPtr = NULL;

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
	if(IsRunTime())
	{
		clnf_model.Reset();
		if (csParameterID == PAR_LIMIT_POSE)
			det_parameters.limit_pose = m_limit_posePinPtr->GetValue();
		else if (csParameterID == PAR_NUM_OPTIMIZATION_ITERATION)
			det_parameters.num_optimisation_iteration = m_num_optimisation_iterationPinPtr->GetValue();
		else if (csParameterID == PAR_REFINE_HIERARCHICAL)
			det_parameters.refine_hierarchical = m_refine_hierarchicalPinPtr->GetValue();

		else if (csParameterID == PAR_REFINE_PARAMETERS)
			det_parameters.refine_parameters = m_refine_parametersPinPtr->GetValue();

		else if (csParameterID == PAR_REG_FACTOR)
			det_parameters.reg_factor = m_reg_factorPinPtr->GetValue();

		else if (csParameterID == PAR_REINIT_VIDEO_EVERY)
			det_parameters.reinit_video_every = m_reinit_video_everyPinPtr->GetValue();

		else if (csParameterID == PAR_CX)
			cx = m_cxPinPtr->GetValue();
		else if (csParameterID == PAR_CY)
			cy = m_cyPinPtr->GetValue();
		else if (csParameterID == PAR_FX)
			fx = m_fxPinPtr->GetValue();
		else if (csParameterID == PAR_FY)
			fy = m_fyPinPtr->GetValue();

		else if(csParameterID == PAR_SIGMA)
			det_parameters.sigma = m_sigmaPinPtr->GetValue();
		else if(csParameterID == PAR_VALIDATION_BOUNDARY)
			det_parameters.validation_boundary = m_validation_boundaryPinPtr->GetValue();
		else if(csParameterID == PAR_WEIGHT_FACTOR)
			det_parameters.weight_factor = m_weight_factorPinPtr->GetValue();

		cx_undefined = cx == 0.0 || cy == 0.0;
		fx_undefined = fx == 0.0 || fy == 0.0;

	}
		
	
}

void CGazeEstimator::visualise_tracking(cv::Mat& captured_image, const LandmarkDetector::CLNF& face_model, const LandmarkDetector::FaceModelParameters& det_parameters, cv::Point3f gazeDirection0, cv::Point3f gazeDirection1, int frame_count, double fx, double fy, double cx, double cy)
{

	// Drawing the facial landmarks on the face and the bounding box around it if tracking is successful and initialised
	double detection_certainty = face_model.detection_certainty;
	bool detection_success = face_model.detection_success;

	double visualisation_boundary = 0.2;

	// Only draw if the reliability is reasonable, the value is slightly ad-hoc
	if (detection_certainty < visualisation_boundary)
	{
		LandmarkDetector::Draw(captured_image, face_model);

		double vis_certainty = detection_certainty;
		if (vis_certainty > 1)
			vis_certainty = 1;
		if (vis_certainty < -1)
			vis_certainty = -1;

		vis_certainty = (vis_certainty + 1) / (visualisation_boundary + 1);

		// A rough heuristic for box around the face width
		int thickness = (int)std::ceil(2.0* ((double)captured_image.cols) / 640.0);

		cv::Vec6d pose_estimate_to_draw = LandmarkDetector::GetCorrectedPoseWorld(face_model, fx, fy, cx, cy);

		// Draw it in reddish if uncertain, blueish if certain
		//LandmarkDetector::DrawBox(captured_image, pose_estimate_to_draw, cv::Scalar((1 - vis_certainty)*255.0, 0, vis_certainty * 255), thickness, fx, fy, cx, cy);

		if (det_parameters.track_gaze && detection_success && face_model.eye_model)
		{
			FaceAnalysis::DrawGaze(captured_image, face_model, gazeDirection0, gazeDirection1, fx, fy, cx, cy);
		}
	}

	/*
	// Work out the framerate
	if (frame_count % 10 == 0)
	{
		double t1 = cv::getTickCount();
		fps_tracker = 10.0 / (double(t1 - t0) / cv::getTickFrequency());
		t0 = t1;
	}

	// Write out the framerate on the image before displaying it
	char fpsC[255];
	std::sprintf(fpsC, "%d", (int)fps_tracker);
	string fpsSt("FPS:");
	fpsSt += fpsC;
	cv::putText(captured_image, fpsSt, cv::Point(10, 20), CV_FONT_HERSHEY_SIMPLEX, 0.5, CV_RGB(255, 0, 0), 1, CV_AA);

	if (!det_parameters.quiet_mode)
	{
		cv::namedWindow("tracking_result", 1);
		cv::imshow("tracking_result", captured_image);
	}*/
}


void CGazeEstimator::fillPupilPosition()
{
	int part_left = -1;
	int part_right = -1;
	for (size_t i = 0; i < clnf_model.hierarchical_models.size(); ++i)
	{
		if (clnf_model.hierarchical_model_names[i].compare("left_eye_28") == 0)
		{
			part_left = i;
		}
		if (clnf_model.hierarchical_model_names[i].compare("right_eye_28") == 0)
		{
			part_right = i;
		}
	}

	//3D representation of pupil position
	cv::Mat eyeLdmks3d_left = clnf_model.hierarchical_models[part_left].GetShape(fx, fy, cx, cy);
	cv::Point3f pupil_left = GetPupilPosition(eyeLdmks3d_left);

	cv::Mat eyeLdmks3d_right = clnf_model.hierarchical_models[part_right].GetShape(fx, fy, cx, cy);
	cv::Point3f pupil_right = GetPupilPosition(eyeLdmks3d_right);


	vector<cv::Point3d> points_left;
	points_left.push_back(cv::Point3d(pupil_left));
	points_left.push_back(cv::Point3d(pupil_left + leftEyeVector*50.0));

	vector<cv::Point3d> points_right;
	points_right.push_back(cv::Point3d(pupil_right));
	points_right.push_back(cv::Point3d(pupil_right + rightEyeVector*50.0));

	//Projection on a 2D image
	cv::Mat_<double> proj_points;
	cv::Mat_<double> left_mesh = (cv::Mat_<double>(2, 3) << points_left[0].x, points_left[0].y, points_left[0].z, points_left[1].x, points_left[1].y, points_left[1].z);
	LandmarkDetector::Project(proj_points, left_mesh, fx, fy, cx, cy);

	cv::Point projectedPupilLeft = cv::Point(cvRound(proj_points.at<double>(0, 0) /** (double)gaze_draw_multiplier*/), cvRound(proj_points.at<double>(0, 1) /** (double)gaze_draw_multiplier*/));

	cv::Mat_<double> right_mesh = (cv::Mat_<double>(2, 3) << points_right[0].x, points_right[0].y, points_right[0].z, points_right[1].x, points_right[1].y, points_right[1].z);
	LandmarkDetector::Project(proj_points, right_mesh, fx, fy, cx, cy);

	cv::Point projectedPupilRight = cv::Point(cvRound(proj_points.at<double>(0, 0) /** (double)gaze_draw_multiplier*/), cvRound(proj_points.at<double>(0, 1) /** (double)gaze_draw_multiplier*/));


	m_pupilLeftPtr->SetValue(projectedPupilLeft.x, projectedPupilLeft.y, 0);
	m_pupilRightPtr->SetValue(projectedPupilRight.x, projectedPupilRight.y, 0);
	m_pupilLeftPtr->SetCreationTime(_clockPtr->GetTime());
	m_pupilRightPtr->SetCreationTime(_clockPtr->GetTime());


}
