#pragma once
#include "StdAfx.h"
#include "BaseCatalog/EywGeometricVector3D.h"
#include "BaseCatalog/EywGeometricPoint2D.h"
#include "EywGraphicPoint2D.h"
#include <opencv2/core/mat.hpp>
#include "./include/LandmarkDetectorParameters.h"
#include "./include/LandmarkDetectorModel.h"


class CGazeEstimator : public Eyw::CBlockImpl
{
public:
	//////////////////////////////////////////////////////////
	/// <summary>
	/// Constructor.
	/// </summary>
	//////////////////////////////////////////////////////////
	CGazeEstimator( const Eyw::OBJECT_CREATIONCTX* ctxPtr );
	
	//////////////////////////////////////////////////////////
	/// <summary>	
	/// Destructor.
	/// </summary>
	//////////////////////////////////////////////////////////
	~CGazeEstimator();

protected:

	//////////////////////////////////////////////////////////
	/// <summary>
	/// Block signature initialization.
	/// </summary>
	//////////////////////////////////////////////////////////
	virtual void InitSignature();	// should also initialize layout and private data
	
	//////////////////////////////////////////////////////////
	/// <summary>
	/// Block signature check.
	/// </summary>
	//////////////////////////////////////////////////////////
	virtual void CheckSignature();
	
	//////////////////////////////////////////////////////////
	/// <summary>
	/// Block signature deinitialization.
	/// </summary>
	//////////////////////////////////////////////////////////
	virtual void DoneSignature();

	//////////////////////////////////////////////////////////
	/// Block Actions
	/// <summary>
	/// Block initialization action.
	/// </summary>
	/// <returns>
	/// true if success, otherwise false.
	/// </returns>
	//////////////////////////////////////////////////////////
	virtual bool Init() throw();

	//////////////////////////////////////////////////////////
	/// <summary>
	/// Block start action.
	/// </summary>
	/// <returns>
	/// true if success, otherwise false.
	/// </returns>
	//////////////////////////////////////////////////////////
	virtual bool Start() throw();

	//////////////////////////////////////////////////////////
	/// <summary>
	/// Block execution action.
	/// </summary>
	/// <returns>
	/// true if success, otherwise false.
	/// </returns>
	//////////////////////////////////////////////////////////
	virtual bool Execute() throw();
	


	//////////////////////////////////////////////////////////
	/// <summary>
	/// Block stop action.
	/// </summary>
	//////////////////////////////////////////////////////////
	virtual void Stop() throw();

	//////////////////////////////////////////////////////////
	/// <summary>
	/// Block deinitialization action.
	/// </summary>
	//////////////////////////////////////////////////////////
	virtual void Done() throw();

	//////////////////////////////////////////////////////////
	/// optionals
	/// <summary>
	/// Manage the ChangedParameter event.
	/// </summary>
	/// <param name="csParameterID">
	/// [in] id of the changed parameter.
	/// </param>
	//////////////////////////////////////////////////////////
	void OnChangedParameter( const std::string& csParameterID );

private:
	/*
	 *
	 *	PARAMETERS
	 *
	 */
	Eyw::int_ptr m_model_locationPinPtr;

	//bool ptrs
	Eyw::bool_ptr m_limit_posePinPtr;
	Eyw::bool_ptr m_refine_hierarchicalPinPtr;
	Eyw::bool_ptr m_refine_parametersPinPtr;

	//int ptrs
	Eyw::int_ptr m_num_optimisation_iterationPinPtr;
	Eyw::int_ptr m_reinit_video_everyPinPtr;

	//double ptrs
	Eyw::double_ptr m_validation_boundaryPinPtr;
	Eyw::double_ptr m_sigmaPinPtr;
	Eyw::double_ptr m_reg_factorPinPtr;
	Eyw::double_ptr m_weight_factorPinPtr;
	Eyw::double_ptr m_fxPinPtr;
	Eyw::double_ptr m_fyPinPtr;
	Eyw::double_ptr m_cxPinPtr;
	Eyw::double_ptr m_cyPinPtr;

	/*
	 *
	 *	INPUTS AND OUTPUTS
	 *
	 */
	Eyw::image_ptr m_inFrameImagePtr;

	Eyw::vector3d_double_ptr m_outGazeEstimateLeftPtr;
	Eyw::vector3d_double_ptr m_outGazeEstimateRightPtr;

	Eyw::graphic_point2d_int_ptr m_pupilLeftPtr;
	Eyw::graphic_point2d_int_ptr m_pupilRightPtr;


	//Eyw::image_ptr m_outProcessedImagePtr;

	//utility function
	void PrepareCvImage(const Eyw::image_ptr& sourceImagePtr, cv::Mat& destinationImage);
	void visualise_tracking(cv::Mat& captured_image, const LandmarkDetector::CLNF& face_model, const LandmarkDetector::FaceModelParameters& det_parameters, cv::Point3f gazeDirection0, cv::Point3f gazeDirection1, int frame_count, double fx, double fy, double cx, double cy);
	void fillPupilPosition();

	/*
	 *
	 *	INTERNAL DATA
	 *
	 */

	// For subpixel accuracy drawing
	const int gaze_draw_shiftbits = 4;
	const int gaze_draw_multiplier = 1 << 4;

	double fx, fy, cx, cy; //focal length e optical axis centre

	LandmarkDetector::CLNF clnf_model;
	LandmarkDetector::FaceModelParameters det_parameters = LandmarkDetector::FaceModelParameters();

	cv::Point3f leftEyeVector; 
	cv::Point3f rightEyeVector;

	cv::Mat captured_image;

	int frame_count = 0;

	bool cx_undefined = false;
	bool fx_undefined = false;

};
