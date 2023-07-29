// testing code for MAGIS TDAQ by Nemi Nayak (nayak03@stanford.edu) with help from Spinnaker API example code. 
//
// testing preparing the camera parameters in one executable and triggering and exiting acquisition mode in a different executable (PREPARATION)
// init, configure (trigger only, no camera settings), set acquisition mode to continuous (does NOT begin acquisition)
// 
//
#include "Spinnaker.h"
#include "SpinGenApi/SpinnakerGenApi.h"
#include <iostream>
#include <sstream>
#include <typeinfo>

using namespace Spinnaker;
using namespace Spinnaker::GenApi;
using namespace Spinnaker::GenICam;
using namespace std;

// configures the camera by setting various trigger-related parameters that can be 
// easily observed by attempting to acquire an image with the camera
int ConfigureCamera(CameraPtr pCam) {

	INodeMap& nodeMap = pCam->GetNodeMap();

	// Ensure trigger mode off
	//
	// *** NOTES ***
	// The trigger must be disabled in order to configure whether the source
	// is software or hardware.
	//
	CEnumerationPtr ptrTriggerMode = nodeMap.GetNode("TriggerMode");
	if (!IsAvailable(ptrTriggerMode) || !IsReadable(ptrTriggerMode))
	{
		cout << "Unable to disable trigger mode (node retrieval). Aborting..." << endl;
		return -1;
	}

	CEnumEntryPtr ptrTriggerModeOff = ptrTriggerMode->GetEntryByName("Off");
	if (!IsAvailable(ptrTriggerModeOff) || !IsReadable(ptrTriggerModeOff))
	{
		cout << "Unable to disable trigger mode (enum entry retrieval). Aborting..." << endl;
		return -1;
	}

	ptrTriggerMode->SetIntValue(ptrTriggerModeOff->GetValue());

	cout << "Trigger mode disabled..." << endl;

	//
	// Set TriggerSelector to FrameStart
	CEnumerationPtr ptrTriggerSelector = nodeMap.GetNode("TriggerSelector");
	if (!IsAvailable(ptrTriggerSelector) || !IsWritable(ptrTriggerSelector))
	{
		cout << "Unable to set trigger selector (node retrieval). Aborting..." << endl;
		return -1;
	}

	CEnumEntryPtr ptrTriggerSelectorFrameStart = ptrTriggerSelector->GetEntryByName("FrameStart");
	if (!IsAvailable(ptrTriggerSelectorFrameStart) || !IsReadable(ptrTriggerSelectorFrameStart))
	{
		cout << "Unable to set trigger selector (enum entry retrieval). Aborting..." << endl;
		return -1;
	}

	ptrTriggerSelector->SetIntValue(ptrTriggerSelectorFrameStart->GetValue());

	cout << "Trigger selector set to frame start..." << endl;

	//
	// Select trigger source
	//
	// *** NOTES ***
	// The trigger source must be set to hardware or software while trigger
	// mode is off.
	//
	CEnumerationPtr ptrTriggerSource = nodeMap.GetNode("TriggerSource");
	if (!IsAvailable(ptrTriggerSource) || !IsWritable(ptrTriggerSource))
	{
		cout << "Unable to set trigger mode (node retrieval). Aborting..." << endl;
		return -1;
	}


	// Set trigger mode to hardware ('Line3', apparently)
	CEnumEntryPtr ptrTriggerSourceHardware = ptrTriggerSource->GetEntryByName("Line3");
	if (!IsAvailable(ptrTriggerSourceHardware) || !IsReadable(ptrTriggerSourceHardware))
	{
		cout << "Unable to set trigger mode (enum entry retrieval). Aborting..." << endl;
		return -1;
	}

	ptrTriggerSource->SetIntValue(ptrTriggerSourceHardware->GetValue());

	cout << "Trigger source set to hardware (Line 3)..." << endl;

	// set Activation Mode (level high for debugging, rising edge preferred)	
	CEnumerationPtr ptrTriggerActivation = nodeMap.GetNode("TriggerActivation");	
	if (!IsAvailable(ptrTriggerActivation) || !IsWritable(ptrTriggerActivation)) {
		cout << "Unable to set trigger activation (node retrieval). Aborting..." << endl;
		return -1;	
	}

	ptrTriggerActivation->SetIntValue(ptrTriggerActivation->GetEntryByName("RisingEdge")->GetValue());

	cout << "Trigger activation mode set to rising edge..." << endl;

	// set trigger delay 
	CFloatPtr ptrTriggerDelay = nodeMap.GetNode("TriggerDelay");
	if (!IsAvailable(ptrTriggerActivation) || !IsWritable(ptrTriggerActivation)) {
		cout << "Unable to set trigger activation (node retrieval). Aborting..." << endl;
		return -1;	
	}

	ptrTriggerDelay->SetValue(65520); // this is the max value (unsure why) 

	cout << "Trigger delay set to 0.06552 seconds" << endl;


	// turn trigger mode back on after configuring the trigger
	CEnumEntryPtr ptrTriggerModeOn = ptrTriggerMode->GetEntryByName("On");
	if (!IsAvailable(ptrTriggerModeOn) || !IsReadable(ptrTriggerModeOn))
	{
		cout << "Unable to enable trigger mode (enum entry retrieval). Aborting..." << endl;
		return -1;
	}

	ptrTriggerMode->SetIntValue(ptrTriggerModeOn->GetValue());

	cout << "Trigger mode turned back on..." << endl;


	// set acquisition mode to continuous, to take images without having to re-start the camera each image 
	CEnumerationPtr ptrAcquisitionMode = nodeMap.GetNode("AcquisitionMode"); 
	if (!IsAvailable(ptrAcquisitionMode) || !IsWritable(ptrAcquisitionMode))
	{
		cout << "Unable to set acquisition mode to continuous (node retrieval). Aborting..." << endl << endl;
		return -1;
	}

	CEnumEntryPtr ptrAcquisitionModeContinuous = ptrAcquisitionMode->GetEntryByName("Continuous");
	if (!IsAvailable(ptrAcquisitionModeContinuous) || !IsReadable(ptrAcquisitionModeContinuous))
	{
		cout << "Unable to set acquisition mode to continuous (entry 'continuous' retrieval). Aborting..." << endl
			<< endl;
		return -1;
	}

	int64_t acquisitionModeContinuous = ptrAcquisitionModeContinuous->GetValue();

	ptrAcquisitionMode->SetIntValue(acquisitionModeContinuous);

	cout << "Acquisition mode set to continuous..." << endl;

	// beginning acquisition is NOT required in this function (in fact, do not call it or the next function to acquire images will segfault)

	// set the buffer handling mode to oldest first (FIFO), which is the default 
	Spinnaker::GenApi::INodeMap& sNodeMap = pCam->GetTLStreamNodeMap();
	CEnumerationPtr ptrHandlingMode = sNodeMap.GetNode("StreamBufferHandlingMode");
	if (!IsAvailable(ptrHandlingMode) || !IsWritable(ptrHandlingMode))
	{
		cout << "Unable to set Buffer Handling mode (node retrieval). Aborting..." << endl << endl;
		return -1;
	}
	CEnumEntryPtr ptrHandlingModeEntry = ptrHandlingMode->GetCurrentEntry();
	if (!IsAvailable(ptrHandlingModeEntry) || !IsReadable(ptrHandlingModeEntry))
	{
		cout << "Unable to set Buffer Handling mode (Entry retrieval). Aborting..." << endl << endl;
		return -1;
	}
	ptrHandlingModeEntry = ptrHandlingMode->GetEntryByName("OldestFirst");
	ptrHandlingMode->SetIntValue(ptrHandlingModeEntry->GetValue());

	cout << "Buffer Handling Mode set to oldest first..." << endl << endl;

	return 0;

	  



}

// in line with Spinnaker API examples, RunSingleCamera initializes the camera, configures it and returns 
// it takes in a camera ptr and can thus be run with multiple cameras 
int RunSingleCamera(CameraPtr pCam) {

	int result = 0;

	// initialize camera
	pCam->Init();
	cout << "Camera Initialized" << endl;

	// CONFIGURE THE TRIGGER HERE 	 
	cout << "Configuring Camera..." << endl << endl;
	result = ConfigureCamera(pCam); 
	cout << "Camera Configured." << endl << endl;

	pCam->DeInit();

	return result;

}

// loops through the cameras, configures them and exits.
int main(int /*argc*/, char** /*argv*/) {

	// Retrieve singleton reference to system object
	SystemPtr system = System::GetInstance();

	// Print out current library version
	const LibraryVersion spinnakerLibraryVersion = system->GetLibraryVersion();
	cout << "Spinnaker library version: " << spinnakerLibraryVersion.major << "." << spinnakerLibraryVersion.minor
		<< "." << spinnakerLibraryVersion.type << "." << spinnakerLibraryVersion.build << endl
		<< endl;

	CameraList camList = system->GetCameras();

	const unsigned int numCameras = camList.GetSize();

	cout << "Number of cameras detected: " << numCameras << endl << endl;

	// Finish if there are no cameras
	if (numCameras == 0) {
		// Clear camera list before releasing system
		camList.Clear();

		// Release system
		system->ReleaseInstance();

		cout << "Not enough cameras! Press Enter to exit." << endl;
		getchar();
		return -1;
	}   

	// run the camera and exit without closing 
	else {
		int result = 0;
		for (unsigned int camN = 0; camN < numCameras; camN++) {
			CameraPtr pCam = camList.GetByIndex(camN);
			result = RunSingleCamera(pCam);
		}

		cout << "Run the Trigger to Acquire Images." << endl;

		return result;

	} 




}
