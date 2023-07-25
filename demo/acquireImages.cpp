// testing code for MAGIS TDAQ by Nemi Nayak (nayak03@stanford.edu) with help from Spinnaker API example code. 
//
// testing preparing the camera parameters in one executable and triggering and exiting acquisition mode in a different executable (TRIGGER AND EXIT)
// begin acquisition, trigger, grab and save image from buffer. runs until buffer is empty & error is thrown. 
// upon exit, re-sets the trigger settings to the default state (not line 3 or rising edge).
//

#include "Spinnaker.h"
#include "SpinGenApi/SpinnakerGenApi.h"
#include <iostream>
#include <sstream>
#include <chrono>
#include <thread>
#include <fstream>
#include <json.hpp>


using namespace Spinnaker;
using namespace Spinnaker::GenApi;
using namespace Spinnaker::GenICam;
using namespace std;
using namespace nlohmann::json_abi_v3_11_2;



#define EXPERIMENT_START std::chrono::system_clock::now()

int ConfigureTrigger(CameraPtr pCam, json configJson) {

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
	// Set TriggerSelector to the selected entry
	CEnumerationPtr ptrTriggerSelector = nodeMap.GetNode("TriggerSelector");
	if (!IsAvailable(ptrTriggerSelector) || !IsWritable(ptrTriggerSelector))
	{
		cout << "Unable to set trigger selector (node retrieval). Aborting..." << endl;
		return -1;
	}

	string triggerSelector = "FrameStart"; 
	if (!configJson["TriggerSelector"].is_null()) triggerSelector = configJson["TriggerSelector"];

	CEnumEntryPtr ptrTriggerSelectorEntry = ptrTriggerSelector->GetEntryByName(gcstring(triggerSelector.c_str()));
	if (!IsAvailable(ptrTriggerSelectorEntry) || !IsReadable(ptrTriggerSelectorEntry))
	{
		cout << "Unable to set trigger selector (enum entry retrieval). Aborting..." << endl;
		return -1;
	}

	ptrTriggerSelector->SetIntValue(ptrTriggerSelectorEntry->GetValue());

	cout << "Trigger selector set to " << triggerSelector << "..."  << endl;

	//
	// Select trigger source
	//
	// *** NOTES ***
	// The trigger source must be set to hardware or software while trigger
	// mode is off.
	//
	
	string triggerSource = "Software"; 
	if (!configJson["TriggerSource"].is_null()) triggerSource = configJson["TriggerSource"];

	CEnumerationPtr ptrTriggerSource = nodeMap.GetNode("TriggerSource");
	if (!IsAvailable(ptrTriggerSource) || !IsWritable(ptrTriggerSource))
	{
		cout << "Unable to set trigger mode (node retrieval). Aborting..." << endl;
		return -1;
	}

	if (triggerSource == "Hardware") {

		// Set trigger mode to hardware ('Line3', apparently)
		CEnumEntryPtr ptrTriggerSourceHardware = ptrTriggerSource->GetEntryByName("Line3");
		if (!IsAvailable(ptrTriggerSourceHardware) || !IsReadable(ptrTriggerSourceHardware))
		{
			cout << "Unable to set trigger mode (enum entry retrieval). Aborting..." << endl;
			return -1;
		}

		ptrTriggerSource->SetIntValue(ptrTriggerSourceHardware->GetValue());

		cout << "Trigger source set to hardware (Line 3)..." << endl;


		// HARDWARE ONLY: set activation type 
		string triggerActivationType = "RisingEdge";
		if (!configJson["TriggerActivationType"].is_null()) triggerActivationType = configJson["TriggerActivationType"];

		CEnumerationPtr ptrTriggerActivation = nodeMap.GetNode("TriggerActivation");	
		if (!IsAvailable(ptrTriggerActivation) || !IsWritable(ptrTriggerActivation)) {
			cout << "Unable to set trigger activation (node retrieval). Aborting..." << endl;
			return -1;	
		}

		CEnumEntryPtr ptrTriggerActivationEntry = ptrTriggerActivation->GetEntryByName(gcstring(triggerActivationType.c_str()));
		if (!IsAvailable(ptrTriggerActivationEntry) || !IsReadable(ptrTriggerActivationEntry))
		{
			cout << "Unable to set trigger activation type (enum entry retrieval). Aborting..." << endl;
			return -1;
		}

		ptrTriggerActivation->SetIntValue(ptrTriggerActivationEntry->GetValue());
		cout << "Trigger activation mode set to " << triggerActivationType << "..."  << endl;
	}

	else {
		// Set trigger mode to software 
		CEnumEntryPtr ptrTriggerSourceSoftware = ptrTriggerSource->GetEntryByName("Software");
		if (!IsAvailable(ptrTriggerSourceSoftware) || !IsReadable(ptrTriggerSourceSoftware))
		{
			cout << "Unable to set trigger mode (enum entry retrieval). Aborting..." << endl;
			return -1;
		}

		ptrTriggerSource->SetIntValue(ptrTriggerSourceSoftware->GetValue());

		cout << "Trigger source set to software..." << endl;

	}

	// turn trigger mode back on after configuring the trigger
	CEnumEntryPtr ptrTriggerModeOn = ptrTriggerMode->GetEntryByName("On");
	if (!IsAvailable(ptrTriggerModeOn) || !IsReadable(ptrTriggerModeOn))
	{
		cout << "Unable to enable trigger mode (enum entry retrieval). Aborting..." << endl;
		return -1;
	}

	ptrTriggerMode->SetIntValue(ptrTriggerModeOn->GetValue());

	cout << "Trigger mode reenabled..." << endl;

	return 0;

}



// in the case of hardware trigger, this function does nothing, in the case of 
// software trigger, it will hang and wait for trigger from the keyboard. 
int GrabNextImageByTrigger(INodeMap& nodeMap, CameraPtr pCam, string triggerSource)
{
	int result = 0;

	try
	{
		//
		// Use trigger to capture image
		//
		// *** NOTES ***
		// The software trigger only feigns being executed by the Enter key;
		// what might not be immediately apparent is that there is not a
		// continuous stream of images being captured; in other examples that
		// acquire images, the camera captures a continuous stream of images.
		// When an image is retrieved, it is plucked from the stream.
		//
		if (triggerSource == "Software")
		{
			// Get user input
			cout << "Press the Enter key to initiate software trigger." << endl;
			getchar();


			// Execute software trigger
			CCommandPtr ptrSoftwareTriggerCommand = nodeMap.GetNode("TriggerSoftware");
			if (!IsAvailable(ptrSoftwareTriggerCommand) || !IsWritable(ptrSoftwareTriggerCommand))
			{
				cout << "Unable to execute trigger. Aborting..." << endl;
				result = -1;
			}

			ptrSoftwareTriggerCommand->Execute();
		}
		else if (triggerSource == "Hardware")
		{
			/* do nothing */
		}
		else {
			cout << "Trigger type invalid, please check your config file." << endl; 
			result = -1;
			
		}
	}
	catch (Spinnaker::Exception& e)
	{
		cout << "Error: " << e.what() << endl;
		result = -1;
	}

	return result;
}


// Acquires images FROM THE IMAGE STORAGE BUFFER of the current camera in the loop
int AcquireImages(CameraPtr pCam, INodeMap& nodeMap, INodeMap& nodeMapTLDevice, int imageCnt, string triggerSource)
{
	int result = 0;

	cout << endl << "*** IMAGE ACQUISITION ***" << endl;

	try
	{

		// Retrieve device serial number for filename
		gcstring deviceSerialNumber("");

		CStringPtr ptrStringSerial = nodeMapTLDevice.GetNode("DeviceSerialNumber");
		if (IsAvailable(ptrStringSerial) && IsReadable(ptrStringSerial))
		{
			deviceSerialNumber = ptrStringSerial->GetValue();

			cout << "Device serial number retrieved as " << deviceSerialNumber << "..." << endl;
		}
		cout << endl;

		// Retrieve the next image from the trigger (in the case of hardware trigger, this does nothing)
		result = result | GrabNextImageByTrigger(nodeMap, pCam, triggerSource);

		// Retrieve the next received image from the image storage buffer
		ImagePtr pResultImage = pCam->GetNextImage(1);


		// NOTE:
		// the camera has an internal buffer.
		// trying to grab the second image after sending only one trigger throws an error, grabbing the second 
		// image after two triggers works as expected (grabs the second image). you MUST clear the ENTIRE buffer manually (ie, 
		// manually save all the images in the buffer one-by-one) if you want all the images in it. 

		if (pResultImage->IsIncomplete())
		{
			cout << "Image incomplete with image status " << pResultImage->GetImageStatus() << "..." << endl
				<< endl;
		}
		else
		{
			// Print image information
			cout << "Grabbed image. " << imageCnt << ", width = " << pResultImage->GetWidth()
				<< ", height = " << pResultImage->GetHeight() << endl;

			// Convert image to mono 8
			ImagePtr convertedImage = pResultImage->Convert(PixelFormat_Mono8, HQ_LINEAR);

			// Create a unique filename
			ostringstream filename;

			filename << "Trigger-";
			if (deviceSerialNumber != "")
			{
				filename << deviceSerialNumber.c_str() << "-";
			}
			filename << imageCnt << ".jpg";

			// Save image
			convertedImage->Save(filename.str().c_str());

			cout << "Image saved at " << filename.str() << endl;
		}

		// Release image
		pResultImage->Release();

		cout << endl;
	}

	catch (Spinnaker::Exception& e)
	{
		// print the error for nothing in buffer 
		cout << "Error: " << e.what() << endl;
		result = -1;
	}

	return result;
}


// as the name suggests, inits and configures all cameras given a camera list and a json file 
int InitializeAndConfigure(CameraList camList, json configJson)
{
	int result = 0;
	try
	{
		for (unsigned int i = 0; i < camList.GetSize(); i++) {
			CameraPtr pCam = camList.GetByIndex(i);
			// Initialize camera
			pCam->Init();

			// configure each trigger
			result |= ConfigureTrigger(pCam, configJson);
			cout << "Trigger configured for camera " << i << endl;
		}

		if (result != 0) {
			for (unsigned int i = 0; i < camList.GetSize(); i++) {
				CameraPtr pCam = camList.GetByIndex(i);

				// Deinitialize camera 
				pCam->DeInit();
			}

			cout << "Camera Configuration Error. Aborting..." << endl;
			return result;

		}

		// Begin Acquisition on all Cameras 
		for (unsigned int i = 0; i < camList.GetSize(); i++) {
			CameraPtr pCam = camList.GetByIndex(i);
			pCam->BeginAcquisition();

			cout << "Began acquisition for camera " << i << endl;
		}

	}
	catch (Spinnaker::Exception& e)
	{
		cout << "Error: " << e.what() << endl;
		result |= -1;
	}

	return result;
}

// Since this application saves images in the current folder
// we must ensure that we have permission to write to this folder.
// If we do not have permission, fail right away.
int checkPermissions() {
	FILE* tempFile = fopen("test.txt", "w+");
	if (tempFile == nullptr)
	{
		cout << "Failed to create file in current folder.  Please check "
			"permissions."
			<< endl;
		cout << "Press Enter to exit..." << endl;
		getchar();
		return -1;
	}
	fclose(tempFile);
	remove("test.txt");
	return 0;
}


// begins acquisition for all cameras, waits for a trigger + calls the function to save the image on success. 
// if no trigger is sent, the executable will reset all cameras, close the system and exit. 
int main(int argc, char** argv)
{
	
	if (checkPermissions() != 0) return -1;

	// Print application build information	
	cout << "Application build date: " << __DATE__ << " " << __TIME__ << endl << endl;

	// Retrieve singleton reference to system object
	SystemPtr system = System::GetInstance();

	// Print out current library version
	const LibraryVersion spinnakerLibraryVersion = system->GetLibraryVersion();
	cout << "Spinnaker library version: " << spinnakerLibraryVersion.major << "." << spinnakerLibraryVersion.minor
		<< "." << spinnakerLibraryVersion.type << "." << spinnakerLibraryVersion.build << endl
		<< endl;

	// Retrieve list of cameras from the system
	CameraList camList = system->GetCameras();

	unsigned int numCameras = camList.GetSize();

	cout << "Number of cameras detected: " << numCameras << endl << endl;

	// Finish if there are no cameras
	if (numCameras == 0)
	{
		// Clear camera list before releasing system
		camList.Clear();

		// Release system
		system->ReleaseInstance();

		cout << "Not enough cameras!" << endl;
		cout << "Done! Press Enter to exit..." << endl;
		getchar();

		return -1;
	}

	int errorFlag = 0;

	// check for a cmd line argument, if not, use the default file 
	string rawFile; 

	if (argc >= 2) {
		rawFile = argv[1];
	}
	else {
		rawFile = "config.json";
	}

	// parse json 
	ifstream configFile(rawFile);
	json configJson = json::parse(configFile);

	if (configJson.is_null()) {
		cout << "Could not parse config file; aborting..." << endl;
		return -1;
       	}

	cout << endl << "***Reading from config " << configJson["TriggerName"] << " ***"  << endl << endl;

	// Init and configure all cameras, begin acquisition on them all 
	errorFlag |= InitializeAndConfigure(camList, configJson);

	if (errorFlag != 0) {
		camList.Clear();
		system->ReleaseInstance();

		cout << "Press Enter to Exit" << endl; 
		getchar(); 

		return -1;
	}

	int imageCnt = 1;

	// enter the acquisition loop
	while (true) { 

		cout << endl << "Waiting for trigger" << endl; 

		// sleep the function (send the trigger within this sleep time)
		std::this_thread::sleep_for(std::chrono::milliseconds(configJson["TriggerTiming"]));

		// grab from buffer for all cameras 
		for (unsigned int i = 0; i < numCameras; i++) { 
			CameraPtr pCam = camList.GetByIndex(i);

			INodeMap& nodeMapTLDevice = pCam->GetTLDeviceNodeMap();

			INodeMap& nodeMap = pCam->GetNodeMap();

			errorFlag |= AcquireImages(pCam, nodeMap, nodeMapTLDevice, imageCnt, configJson["TriggerSource"]);

			cout << "Grabbed image from camera " << i << endl;
		}

		// exit gracefully if errorFlag != 0 or the experiment has ended 
		if ((errorFlag != 0 && configJson["ExitOnError"]) || (std::chrono::system_clock::now() > EXPERIMENT_START + std::chrono::minutes(configJson["RunUntil"]))) {
			cout << endl << "***Stopping acquisition*** (no image in buffer)" << endl; 
			for (unsigned int i = 0; i < numCameras; i++) {
				CameraPtr pCam = camList.GetByIndex(i);

				// release the camera
				pCam->EndAcquisition(); 
				cout << "Ended Acquisition for camera " << i << endl;

				pCam->DeInit();
				cout << "DeInitialized camera " << i << endl << endl;
			}

			// release the system
			camList.Clear(); 
			cout << endl <<  "Cleared camera list" << endl;

			system->ReleaseInstance();

			cout << "System instance released" << endl;
			break; 

		}
		
		imageCnt++;

	} 

	cout << endl << "Done!"  << endl;

	return 0;
}
