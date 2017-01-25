#include "stdafx.h"
#include "Landmarksdetector.h"
#include "Signature.h"
#include "resource.h"
#include "include/LandmarkDetectorModel.h"
#include "include/GazeEstimation.h"
#include <opencv2/imgproc.hpp>

using namespace Eyw;
 

//////////////////////////////////////////////////////////
/// <summary>
/// Block Signature.
/// </summary>
Eyw::block_class_registrant g_Landmarksdetector( 
	Eyw::block_class_registrant::block_id( "Landmarksdetector" )
		.begin_language( EYW_LANGUAGE_US_ENGLISH )
			.name( "Landmarks detector" )
			.description( "This blocks detects and outputs landmarks in an image containing a single face."	)
			.libraries( "MyLibrary" )
			.bitmap( IDB_LANDMARKSDETECTOR_BITMAP )
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
		.default_factory< CLandmarksdetector >()
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
#define OUT_LANDMARKS "Landmarks"
#define OUT_LANDMARKS_EYE "Eyeball Landmarks"

//////////////////////////////////////////////////////////
/// <summary>
/// Constructor.
/// </summary>
//////////////////////////////////////////////////////////
CLandmarksdetector::CLandmarksdetector( const Eyw::OBJECT_CREATIONCTX* ctxPtr )
:	Eyw::CBlockImpl( ctxPtr )
{
	m_inFrameImagePtr=NULL;
	m_outLandmarksPtr = NULL;
	m_outLandmarksEyePtr = NULL;
	_schedulingInfoPtr->SetActivationEventBased( true );
	_schedulingInfoPtr->GetEventBasedActivationInfo()->SetActivationOnInputChanged( IN_FRAMEIMAGE, true );
}

//////////////////////////////////////////////////////////
/// <summary>	
/// Destructor.
/// </summary>
//////////////////////////////////////////////////////////
CLandmarksdetector::~CLandmarksdetector()
{
}

//////////////////////////////////////////////////////////
/// <summary>
/// Block signature initialization.
/// </summary>
//////////////////////////////////////////////////////////
void CLandmarksdetector::InitSignature()
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
	
	SetOutput(Eyw::pin::id(OUT_LANDMARKS)
		.name("Landmarks' position")
		.description("Labelled set of 2D points of the landmarks")
		.type < Eyw::IGraphicLabelledSet2DDouble > ()
		);

	SetOutput(Eyw::pin::id(OUT_LANDMARKS_EYE)
		.name("Eye Landmarks' position")
		.description("Labelled set of 2D points of the landmarks of the eyeballs")
		.type < Eyw::IGraphicLabelledSet2DDouble > ()
		);

}

//////////////////////////////////////////////////////////
/// <summary>
/// Block signature check.
/// </summary>
//////////////////////////////////////////////////////////
void CLandmarksdetector::CheckSignature()
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
	_signaturePtr->GetOutputs()->FindItem( OUT_LANDMARKS );
	_signaturePtr->GetOutputs()->FindItem( OUT_LANDMARKS_EYE );
}

//////////////////////////////////////////////////////////
/// <summary>
/// Block signature deinitialization.
/// </summary>
//////////////////////////////////////////////////////////
void CLandmarksdetector::DoneSignature()
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
bool CLandmarksdetector::Init() throw()
{
	
    try
	{
		/// TODO: Init data structures here 
		m_inFrameImagePtr = get_input_datatype<Eyw::IImage>( IN_FRAMEIMAGE );
		
		m_outLandmarksPtr = get_output_datatype<Eyw::IGraphicLabelledSet2DDouble>(OUT_LANDMARKS);

			list_init_info_ptr listInitInfoPtr = datatype_init_info<IListInitInfo>::create(_kernelServicesPtr);
			listInitInfoPtr->SetCatalogID(EYW_BASE_CATALOG_ID);
			listInitInfoPtr->SetClassID(EYW_BASE_CATALOG_GRAPHIC_POINT2D_DOUBLE_ID);
			
			m_outLandmarksPtr->InitInstance(listInitInfoPtr.get());

		m_outLandmarksEyePtr = get_output_datatype<Eyw::IGraphicLabelledSet2DDouble>(OUT_LANDMARKS_EYE);

			list_init_info_ptr listInitInfoPtr2 = datatype_init_info<IListInitInfo>::create(_kernelServicesPtr);
			listInitInfoPtr2->SetCatalogID(EYW_BASE_CATALOG_ID);
			listInitInfoPtr2->SetClassID(EYW_BASE_CATALOG_GRAPHIC_POINT2D_DOUBLE_ID);
			
			m_outLandmarksEyePtr->InitInstance(listInitInfoPtr2.get());

			m_pointPtr = datatype<Eyw::IGraphicPoint2DDouble>::create(_kernelServicesPtr);
		//m_outLandmarksPtr = get_output_datatype<Eyw::IGraphicPoint2DDouble>( OUT_LANDMARKS );
		
		/*list_init_info_ptr listInitInfoPtr = datatype_init_info<IListInitInfo>::create(_kernelServicesPtr);
			listInitInfoPtr->SetCatalogID(EYW_BASE_CATALOG_ID);
			listInitInfoPtr->SetClassID(EYW_BASE_CATALOG_POINT2D_INT_ID);
		m_outLandmarksPtr->InitInstance(listInitInfoPtr.get());*/
		
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
bool CLandmarksdetector::Start() throw()
{
	
    try
    {
		Notify_DebugString("Start\n");
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
bool CLandmarksdetector::Execute() throw()
{
    try
	{

		int rows = m_inFrameImagePtr->GetHeight();
		int cols = m_inFrameImagePtr->GetWidth();

		m_outLandmarksPtr->Clear();
		m_outLandmarksEyePtr->Clear();

		labelCount = 0;
		labelCount_2 = 0;

		cv::Mat output_image;

		PrepareCvImage(m_inFrameImagePtr, captured_image);
		
		captured_image.copyTo(output_image);

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

			//Notify_DebugString("Part 1\n");

			

			bool detection_success = DetectLandmarksInVideo(grayscale_image, clnf_model, det_parameters);
			double detection_certainty = clnf_model.detection_certainty;

			double threshold = 0.2;

			if (detection_certainty < threshold)
				DrawLandmark(clnf_model);

			/*landmarks = cv::Mat();

			landmarks = clnf_model.detected_landmarks;

			int mid = landmarks.rows / 2;

			for (int i = 0; i < mid;i++)
			{
				if (i < 10)
				{
					str = "Landmark_0" + i;
				}
				else
				{
					str = "Landmark_" + i;
				}
				m_pointPtr->SetValue(landmarks.at<double>(i, 0)/cols,landmarks.at<double>(mid+i,0 )/rows);
				m_outLandmarksPtr->Insert(str.c_str(), m_pointPtr.get());
			}

			//Notify_DebugString(str);*/
			
			m_outLandmarksPtr->SetCreationTime(_clockPtr->GetTime());
			m_outLandmarksEyePtr->SetCreationTime(_clockPtr->GetTime());
			return true;
			Notify_DebugString("Completing execute()");
		}
		else
		{
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

void CLandmarksdetector::PrepareCvImage(const image_ptr& sourceImagePtr, cv::Mat &destinationImage)
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
			normFacX = width;
			normFacY = height;
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
void CLandmarksdetector::Stop() throw()
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
void CLandmarksdetector::Done() throw()
{
    try
	{
		m_inFrameImagePtr = NULL;
		m_outLandmarksPtr = NULL;
		m_outLandmarksEyePtr = NULL;
		m_pointPtr = 0;
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
void CLandmarksdetector::OnChangedParameter( const std::string& csParameterID )
{
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

// Drawing detected landmarks on a face image
void CLandmarksdetector::DrawLandmark(const LandmarkDetector::CLNF& clnf_model)
{

	int idx = clnf_model.patch_experts.GetViewIdx(clnf_model.params_global, 0);

	// Because we only draw visible points, need to find which points patch experts consider visible at a certain orientation
	DrawLandmark(clnf_model.detected_landmarks, clnf_model.patch_experts.visibilities[0][idx]);

	// If the model has hierarchical updates draw those too
	for(size_t i = 0; i < clnf_model.hierarchical_models.size(); ++i)
	{
		if(clnf_model.hierarchical_models[i].pdm.NumberOfPoints() != clnf_model.hierarchical_mapping[i].size())
		{
			DrawLandmark(clnf_model.hierarchical_models[i]);
		}
	}
}

void CLandmarksdetector::DrawLandmark(const cv::Mat_<double>& shape2D,const cv::Mat_<int> &visibilities)
{
	int n = shape2D.rows/2;

	if(n > 66)
	{
		for( int i = 0; i < n; ++i)
		{		
			if(visibilities.at<int>(i))
			{
				labelCount++;

				m_pointPtr->SetValue(cvRound(shape2D.at<double>(i)*16-10)/(normFacX*16),cvRound(shape2D.at<double>(i + n)*16)/(normFacY*16));

				str = labelCount;

				//Notify_DebugString("labelCount %d",labelCount);

				m_outLandmarksPtr->Insert(str.c_str(), m_pointPtr.get());

			}
		}
	}
	else if(n == 28) // drawing eyes
	{
		for( int i = 0; i < n; ++i)
		{		
			labelCount_2++;

				m_pointPtr->SetValue(cvRound(shape2D.at<double>(i)*16-10)/(normFacX*16),cvRound(shape2D.at<double>(i + n)*16)/(normFacY*16));
			
				str = labelCount_2;

				//Notify_DebugString("labelCount %d",labelCount);

				m_outLandmarksEyePtr->Insert(str.c_str(), m_pointPtr.get());

		}
	}	
	else if (n == 6)
	{
		for( int i = 0; i < n; ++i)
		{		
					labelCount++;

					m_pointPtr->SetValue(cvRound(shape2D.at<double>(i)*16-10)/(normFacX*16),cvRound(shape2D.at<double>(i + n)*16)/(normFacY*16));
			
					if (labelCount < 10)
					{
						m_outLandmarksPtr->Insert("Landmark_0"+labelCount, m_pointPtr.get());
					}
					else
					{
						m_outLandmarksPtr->Insert("Landmark_"+labelCount, m_pointPtr.get());
					}

		}	
	}
}