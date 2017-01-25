# OpenFace LandmarksDetector EyesWeb Block

Developed by [Edoardo Ferrante](https://github.com/fleanend), based on [OpenFace](https://github.com/TadasBaltrusaitis/OpenFace) - [Wiki](https://github.com/TadasBaltrusaitis/OpenFace/wiki).

Source for this block available at: [https://github.com/fleanend/OpenFaceEyesWeb](https://github.com/fleanend/OpenFaceEyesWeb).

EyesWeb Platform: [http://www.infomus.org/eyesweb_ita.php](http://www.infomus.org/eyesweb_ita.php).

OpenFace LandmarksDetector is an EyewWeb Block based on the Landmarks detector classes of face tracking software **OpenFace**, mainly developed by [Tadas Baltrusaitis](https://www.cl.cam.ac.uk/research/rainbow/projects/openface/). 

## Specifications

### Input

- **Frame/Image**: image or frame from a video which contains a face whose facial landmarks are to be detected. If more than one person is present in the frame, the block will choose randomly the one upon which the algorithm will run.

### Output

- **Landmarks**, type:**Graphic labelled set of 2D doubles**: Set of all facial landmarks as 2D normalized coordinates on the image.
- **Eyeball Landmarks**, type: **Graphic labelled set of 2D doubles**: Set of landmarks of the eyeballs as 2D normalized coordinates on the image.
### Parameters

- ``model_location``: It's the location for the landmark detection model used by OpenFace, it can assume 4 values:
    - ``main_clnf_general`` (default), trained on Multi-PIE of varying pose and illumination and In-the-wild data, works well for head pose tracking (CLNF model);
	- ``main_clnf_wild``, trained on In-the-wild data, works better in noisy environments (not very well suited for head pose tracking), (CLNF in-the-wild model);
	- ``main_clm_general``, a less accurate but slightly faster CLM model trained on Multi-PIE of varying pose and illumination and In-the-wild data, works well for head pose tracking;
	- ``main_clm-z``, trained on Multi-PIE and BU-4DFE datasets, works with both intensity and depth signals (CLM-Z). 

#### Boolean parameters

- ``limit_pose``: it regulates whether pose should be limited to 180 degrees frontal, defaults to **true**;
- ``refine_hierarchical``: it regulates whether the model should be refined hierarchically, defaults to **true**;
- ``refine_parameters``: it regulates whether the parameters should be refined for different scales, defaults to **true**.

#### Integer parameters

- ``num_optimisation iterations``: Number of RLMS (Regularized Least Mean Squares) or NU-RLMS iterations, defaults to **5**;
- ``reinit_video_every``: How often should face detection be used to attempt reinitialisation: every n frames (set to negative not to reinit), defaults to **4**.

#### Double parameters

- ``validation_boundary``: Landmark detection validator boundary for correct detection, the regressor output -1 (perfect alignment) 1 (bad alignment), defaults to **-0.45**;
- ``sigma``: used for the smooting of response maps (KDE sigma), defaults to **1.5**;
- ``reg_factor``: weight put to regularization, defaults to **25**;
- ``weight_factor``: Factor for weighted least squares. By default **0**, as for videos doesn't work well;
- ``fx, fy, cx, cy``: respectively: focal length $x$ and $y$ coordinates, optical $x$ and $y$ axis center. These are camera-related parameters and, by default, if any of them is 0 at the start of the patch execution, they are initialised as follows:

        cx = frame_rows / 2.0f;
	    cy = frame_columns / 2.0f;
        
        fx = 500 * ( rows / 640.0);
		fy = 500 * ( cols / 480.0);

        //fx and fy are equal by default
		fx = (fx + fy) / 2.0;
		fy = fx;

