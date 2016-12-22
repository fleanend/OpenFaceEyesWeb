#pragma once
#include "StdAfx.h"
#include "BaseCatalog/EywGeometricVector3D.h"
#include <opencv2/core/mat.hpp>
#include "LandmarkDetectorParameters.h"
#include "LandmarkDetectorModel.h"


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
	Eyw::bool_ptr m_parParameterPinPtr;
	Eyw::image_ptr m_inFrameImagePtr;
	Eyw::vector3d_double_ptr m_outGazeEstimateLeftPtr;
	Eyw::vector3d_double_ptr m_outGazeEstimateRightPtr;
	void PrepareCvImage(const Eyw::image_ptr& sourceImagePtr, cv::Mat& destinationImage);

	// For subpixel accuracy drawing
	const int gaze_draw_shiftbits = 4;
	const int gaze_draw_multiplier = 1 << 4;

	float fx, fy, cx, cy; //focal length e optical axis centre

	LandmarkDetector::CLNF clnf_model;
	LandmarkDetector::FaceModelParameters det_parameters;

	cv::Point3f leftEyeVector; 
	cv::Point3f rightEyeVector;

	cv::Mat captured_image;

};
