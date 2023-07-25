#include "Spinnaker.h"
#include "SpinGenApi/SpinnakerGenApi.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <typeinfo>
#include <ctime>
#include <vector>
#include <json.hpp>
#include <string>

using namespace Spinnaker;
using namespace Spinnaker::GenApi;
using namespace Spinnaker::GenICam;
using namespace std;
using namespace nlohmann::json_abi_v3_11_2;


/* takes in a json file as a command line argument and sets camera attributes accordingly
 * If no command arguments are given, the program will not set any settings
 * json file should be in the same directory as the executable, and the command line argument should specify the directory if it is not the same as the current one
 * std logic error usually due to not passing in a command line argument
 * this function can set Acquisition mode, exposure time, automatic exposure, gain, automatic gain, X and Y offset, width and height of the region of interest, 
 * sensor shutter mode, ADC bit depth, stream buffer handling mode, and trigger source, mode, overlap, delay, and activation if desired
 * All chunk data is enabled by default
 * Technical reference with names of all nodes and enumeration values : http://softwareservices.flir.com/BFS-U3-13Y3/latest/Model/public/index.html
 * on some RPis, needs to be run with sudo in order to detect cameras
 * Camera settings are saved to UserSet0 at the end of this program and will be automatically loaded the next time the camera is turned on
 */
// parameter name: AcquisitionMode
// options for its values: Continuous, SingleFrame, MultiFrame
int configureAcquisition(INodeMap& nodeMap,string acquisitionMode){
	int result = 0;
	CEnumerationPtr ptrAcquisitionMode = nodeMap.GetNode("AcquisitionMode");
	cout << "Before: " << ptrAcquisitionMode-> GetDisplayName() << ": " << ptrAcquisitionMode -> GetCurrentEntry() -> GetSymbolic() << endl; 
	// error if AcquisitionMode is not readable or writable
	if (!IsReadable(ptrAcquisitionMode) || !IsWritable(ptrAcquisitionMode))
        {
            cout << "Acquisition Mode enumeration node not readable or writable.  Aborting..." << endl << endl;
            return -1;
        }
	CEnumEntryPtr ptrAcquisitionModeDesired = ptrAcquisitionMode -> GetEntryByName(gcstring(acquisitionMode.c_str()));
	// error if mode is not readable
	if (!IsReadable(ptrAcquisitionModeDesired)){
		cout <<"Unable to get or set acquisition mode to " << acquisitionMode<< " . Aborting..." << endl << endl;
		return -1;
	}
	const int64_t acquisitionModeDesired = ptrAcquisitionModeDesired -> GetValue();
	ptrAcquisitionMode -> SetIntValue(acquisitionModeDesired);
	cout << "Acquisition mode set to " << acquisitionMode  << endl;
	cout << "After: " << ptrAcquisitionMode -> GetDisplayName() << ": " << ptrAcquisitionMode -> GetCurrentEntry() -> GetSymbolic() << endl << endl;
	return result;
}
// property name: Exposure
// Options: a double or Auto
int setExposure(INodeMap& nodeMap, double exposureTimeToSet){
	int result = 0;
	cout << endl << endl << "*** CONFIGURING EXPOSURE ***" << endl << endl;
	try
	{	
        // Turn off automatic exposure mode
        //
        // *** NOTES ***
        // Automatic exposure prevents the manual configuration of exposure
        // time and needs to be turned off. Some models have auto-exposure
        // turned off by default
        //
        // *** LATER ***
        // Exposure time can be set automatically or manually as needed. This
        // example turns automatic exposure off to set it manually and back
        // on in order to return the camera to its default state.
        //
        CEnumerationPtr ptrExposureAuto = nodeMap.GetNode("ExposureAuto");
        if (IsReadable(ptrExposureAuto) &&
            IsWritable(ptrExposureAuto))
        {
            CEnumEntryPtr ptrExposureAutoOff = ptrExposureAuto->GetEntryByName("Off");
            if (IsReadable(ptrExposureAutoOff))
            {
                ptrExposureAuto->SetIntValue(ptrExposureAutoOff->GetValue());
                cout << "Automatic exposure disabled..." << endl;
            }
        }
        else 
        {
            CEnumerationPtr ptrAutoBright = nodeMap.GetNode("autoBrightnessMode");
            if (!IsReadable(ptrAutoBright) ||
                !IsWritable(ptrAutoBright))
            {
                cout << "Unable to get or set exposure time. Aborting..." << endl << endl;
                return -1;
            }
            cout << "Unable to disable automatic exposure. Expected for some models... " << endl;
            cout << "Proceeding..." << endl;
            result = 1;
        }

        //
        // Set exposure time manually; exposure time recorded in microseconds
        //
        // *** NOTES ***
        // The node is checked for availability and writability prior to the
        // setting of the node. Further, it is ensured that the desired exposure
        // time does not exceed the maximum. Exposure time is counted in
        // microseconds. This information can be found out either by
        // retrieving the unit with the GetUnit() method or by checking SpinView.
        //
        CFloatPtr ptrExposureTime = nodeMap.GetNode("ExposureTime");
        if (!IsReadable(ptrExposureTime) ||
            !IsWritable(ptrExposureTime))
        {
            cout << "Unable to get or set exposure time. Aborting..." << endl << endl;
            return -1;
        }
	cout << "Before: " <<  ptrExposureTime-> GetDisplayName() << ": " << ptrExposureTime -> GetValue() << endl; 

        // Ensure desired exposure time does not exceed the maximum
        const double exposureTimeMax = ptrExposureTime->GetMax();

        if (exposureTimeToSet > exposureTimeMax)
        {
            exposureTimeToSet = exposureTimeMax;
        }

        ptrExposureTime->SetValue(exposureTimeToSet);

        cout << std::fixed << "Exposure time set to " << exposureTimeToSet << " us..." << endl << endl;
	cout << "After: " << ptrExposureTime -> GetDisplayName() << ": " << ptrExposureTime -> GetValue() << endl << endl;
    }
    catch (Spinnaker::Exception& e)
    {
        cout << "Error: " << e.what() << endl;
        result = -1;
    }

    return result;
}

int ResetExposure(INodeMap& nodeMap)
{
    int result = 0;

    try
    {
        //
        // Turn automatic exposure back on
        //
        // *** NOTES ***
        // Automatic exposure is turned on in order to return the camera to its
        // default state.
        //
        CEnumerationPtr ptrExposureAuto = nodeMap.GetNode("ExposureAuto");
        if (!IsReadable(ptrExposureAuto) ||
            !IsWritable(ptrExposureAuto))
        {
            cout << "Unable to enable automatic exposure (node retrieval). Non-fatal error..." << endl << endl;
            return -1;
        }

        CEnumEntryPtr ptrExposureAutoContinuous = ptrExposureAuto->GetEntryByName("Continuous");
        if (!IsReadable(ptrExposureAutoContinuous))
        {
            cout << "Unable to enable automatic exposure (enum entry retrieval). Non-fatal error..." << endl << endl;
            return -1;
        }

        ptrExposureAuto->SetIntValue(ptrExposureAutoContinuous->GetValue());

        cout << "Automatic exposure enabled..." << endl << endl;
    }
    catch (Spinnaker::Exception& e)
    {
        cout << "Error: " << e.what() << endl;
        result = -1;
    }

    return result;
}
// parameter name: Gain
// options: a double or Auto
int setGain(INodeMap& nodeMap, double gainToSet){
	// Turn off automatic gain
        // gain auto: once or continuous
	// camera automatically adjusts gain to maximize dynamic range
	// once: automatic gain adapts device, then manual again
	// continuous: camera continually adjusts
	// or off
        //
	int result = 0;
	try{
       		 CEnumerationPtr ptrGainAuto = nodeMap.GetNode("GainAuto");
       		 if (IsReadable(ptrGainAuto) && IsWritable(ptrGainAuto))
        	{		
            		CEnumEntryPtr ptrGainAutoOff = ptrGainAuto->GetEntryByName("Off");
            		if (IsReadable(ptrGainAutoOff))
            		{
                		ptrGainAuto->SetIntValue(ptrGainAutoOff->GetValue());
                		cout << "Automatic gain disabled..." << endl;
            		}
        	}	
        

        	//
        	// Set gain manually
        	//
        	// *** NOTES ***
        	// The node is checked for availability and writability prior to the
        	// setting of the node.

        	CFloatPtr ptrGain= nodeMap.GetNode("Gain");
        	if (!IsReadable(ptrGain) || !IsWritable(ptrGain))
        	{
            	cout << "Unable to get or set exposure time. Aborting..." << endl << endl;
            	return -1;
        	}
		cout << "Before: " <<  ptrGain -> GetDisplayName() << ": " << ptrGain -> GetValue() << endl;
        	ptrGain->SetValue(gainToSet);

        	cout << std::fixed << "Gain set to " << gainToSet << " us"  << endl << endl;
    	}
    	catch (Spinnaker::Exception& e)
    	{
        	cout << "Error: " << e.what() << endl;
        	return -1;
    	}

    	return result;
}
int ResetGain(INodeMap& nodeMap){
    int result = 0;

    try
    {
        //
        // Turn automatic gain on
        CEnumerationPtr ptrGainAuto = nodeMap.GetNode("GainAuto");
        if (!IsReadable(ptrGainAuto) ||
            !IsWritable(ptrGainAuto))
        {
            cout << "Unable to enable automatic gain (enumeration node retrieval). Non-fatal error..." << endl << endl;
            return -1;
        }

        CEnumEntryPtr ptrGainAutoContinuous = ptrGainAuto->GetEntryByName("Continuous");
        if (!IsReadable(ptrGainAutoContinuous))
        {
            cout << "Unable to enable automatic gain (enum entry retrieval). Non-fatal error..." << endl << endl;
            return -1;
        }

        ptrGainAuto->SetIntValue(ptrGainAutoContinuous->GetValue());

        cout << "Automatic gain  enabled..." << endl << endl;
    }
    catch (Spinnaker::Exception& e)
    {
        cout << "Error: " << e.what() << endl;
        result = -1;
    }
    return result;

}
//parameter name: PixelFormat
int setPixelFormat(INodeMap& nodeMap, string pixelFormat){
	int result = 0;
	try
    	{     
        	// Retrieve the enumeration node from the nodemap
        	CEnumerationPtr ptrPixelFormat = nodeMap.GetNode("PixelFormat");
		if (!IsReadable(ptrPixelFormat) || ! IsWritable(ptrPixelFormat))
		{
			cout << "Unable to get or set pixel format (Enumeration node retrieval). Aborting... ";
			return -1;
		}
        	

            	//Retrieve the desired entry node from the enumeration node
            	CEnumEntryPtr ptrDesiredPixelFormat = ptrPixelFormat->GetEntryByName(gcstring(pixelFormat.c_str()));
            	if (IsReadable(ptrDesiredPixelFormat))
            	{
                	// Retrieve the integer value from the entry node
                	int64_t DesiredPixelFormat = ptrDesiredPixelFormat->GetValue();

                	// Set integer as new value for enumeration node
               		ptrPixelFormat->SetIntValue(DesiredPixelFormat);

                	cout << "Pixel format set to " << ptrPixelFormat->GetCurrentEntry()->GetSymbolic()  << endl;
            	}
            	else
            	{
               	 		cout << pixelFormat << " not readable..." << endl;
				return -1;
            	}
	}
        
        
	catch (Spinnaker::Exception& e)
	{
		cout << "Error: " <<e.what() << endl;
		result = -1;
	}
	return result;

}
// X Offset: horizontal offset in pixels from the origin to the region of interest (ROI)
//parameter name: OffsetX
// options: an integer in pixels
int setOffsetX(INodeMap& nodeMap, int64_t OffsetXToSet){
	int result = 0;
	try{

		CIntegerPtr ptrOffsetX = nodeMap.GetNode("OffsetX");
		if (!IsReadable(ptrOffsetX) || !IsWritable(ptrOffsetX)){
			cout << "Cannot access OffsetX node (node retrieval). Aborting..." << endl;
			return -1;
		}
		int incrementRemainder = (OffsetXToSet - ptrOffsetX -> GetMin()) % ptrOffsetX -> GetInc();
		if (OffsetXToSet < ptrOffsetX -> GetMin()){
			OffsetXToSet = ptrOffsetX -> GetMin();
			cout << "OffsetX value below the minimum, and has been set to the minimum (" << ptrOffsetX -> GetMin() << "). Aborting..." << endl;
		}
		else if (OffsetXToSet > ptrOffsetX -> GetMax()){
			OffsetXToSet = ptrOffsetX -> GetMax();
			cout << "OffsetX value above the maximum, and has been set to the maximum(" << ptrOffsetX -> GetMax() << ")."  << endl;	
		}
		else if (incrementRemainder !=0){
				cout << "The different between OffsetX and the minimum is not a multiple of the increment (" << ptrOffsetX -> GetInc() << "). The X Offset has	been set to " << (OffsetXToSet - incrementRemainder) << endl;
		}	
		ptrOffsetX -> SetValue(OffsetXToSet);
		cout << "Offset X set to " << OffsetXToSet << "..." << endl;

	}
	catch (Spinnaker::Exception& e){
		cout << "Error: " <<e.what() << endl;
		result = -1;
	}
	return result;
}

// vertical offset from the origin to ROI
// parameter: OffsetY
// options: an integer in pixels
int setOffsetY(INodeMap& nodeMap, int64_t OffsetYToSet){
	int result = 0;
	try{

		CIntegerPtr ptrOffsetY = nodeMap.GetNode("OffsetY");
		if (!IsReadable(ptrOffsetY) || !IsWritable(ptrOffsetY)){
			cout << "Cannot access OffsetY node (node retrieval). Aborting..." << endl;
			return -1;
		}
		int incrementRemainder = (OffsetYToSet - ptrOffsetY -> GetMin()) % ptrOffsetY -> GetInc();
		if (OffsetYToSet < ptrOffsetY -> GetMin()){
			OffsetYToSet = ptrOffsetY -> GetMin();
			cout << "OffsetY value below the minimum, and has been set to the minimum (" << ptrOffsetY -> GetMin() << "). Aborting..." << endl;
		}
		else if (OffsetYToSet > ptrOffsetY -> GetMax()){
			OffsetYToSet = ptrOffsetY -> GetMax();
			cout << "OffsetY value above the maximum, and has been set to the maximum(" << ptrOffsetY -> GetMax() << ")."  << endl;	
		}
		else if (incrementRemainder !=0){
				cout << "The different between OffsetY and the minimum is not a multiple of the increment (" << ptrOffsetY -> GetInc() << "). The Y Offset has	been set to " << (OffsetYToSet - incrementRemainder) << endl;
		}
			
		ptrOffsetY -> SetValue(OffsetYToSet);
		cout << "Offset Y set to " << OffsetYToSet << "..." << endl;

	}
	catch (Spinnaker::Exception& e)
	{
		cout << "Error: " <<e.what() << endl;
		result = -1;
	}
	return result;
}
// width of image in pixels provided by device reflecting the region of interest: only the pixel information from ROI is processed
int setWidth(INodeMap& nodeMap, int64_t widthToSet){
	int result = 0;
	try{
		CIntegerPtr ptrWidth = nodeMap.GetNode("Width");
		if (!IsReadable(ptrWidth) || ! IsWritable(ptrWidth)){
			cout << "Cannot get or set Width node. Aborting... " << endl;
			return -1;
		}
		int incrementRemainder =(widthToSet - ptrWidth-> GetMin() ) % ptrWidth -> GetInc();
		if (widthToSet > ptrWidth->GetMax()){
			widthToSet = ptrWidth->GetMax();
			cout << "Desired width is larger than the maximum, and has been set to " << ptrWidth -> GetMax() << "." << endl;
		}		
		else if (widthToSet < ptrWidth -> GetMin()){
			widthToSet = ptrWidth -> GetMin();
			cout << "Desired width is smaller than the minimum, and has been set to " << ptrWidth -> GetMin() << "." << endl;
		}
		
		else if (incrementRemainder != 0){
			widthToSet -= incrementRemainder;
			cout << "Desired width is not a multiple of the increment, and has been set to " << widthToSet << "." << endl;
		}

		ptrWidth->SetValue(widthToSet);

		cout << "Width set to " << ptrWidth->GetValue() << "..." << endl;	
	}

	catch (Spinnaker::Exception& e)
	{
		cout << "Error: " <<e.what() << endl;
		result = -1;
	}
	return result;

}
int setHeight(INodeMap& nodeMap, int64_t heightToSet){
	int result = 0;
	try{
		CIntegerPtr ptrHeight = nodeMap.GetNode("Height");
		if (!IsReadable(ptrHeight) || ! IsWritable(ptrHeight)){
			cout << "Cannot get or set Width node. Aborting... " << endl;
			return -1;
		}
		int incrementRemainder =(heightToSet - ptrHeight-> GetMin() ) % ptrHeight -> GetInc();
		if (heightToSet > ptrHeight -> GetMax()){
			heightToSet = ptrHeight -> GetMax();
			cout << "Desired height is larger than the maximum, and has been set to " << ptrHeight -> GetMax() << "." << endl;
		}		
		else if (heightToSet < ptrHeight -> GetMin()){
			heightToSet = ptrHeight -> GetMin();
			cout << "Desired height  is smaller  than the minimum, and has been set to " << ptrHeight -> GetMin() << "." << endl;
		}
		
		else if (incrementRemainder != 0){
			heightToSet -= incrementRemainder;
			cout << "Desired height  is not a multiple of the increment, and has been set to " << heightToSet << "." << endl;
		}

		ptrHeight -> SetValue(heightToSet);

		cout << "Height set to " << ptrHeight -> GetValue() << "..." << endl;
	
	}

	catch (Spinnaker::Exception& e)
	{
		cout << "Error: " <<e.what() << endl;
		result = -1;
	}
	return result;

}
// parameter name: AdcBitDepth
// Options: Bit10, etc.
int setADCBitDepth(INodeMap& nodeMap, string  bitDepth){
	int result = 0;
	try
	{
		CEnumerationPtr ptrAdcBitDepth = nodeMap.GetNode("AdcBitDepth");
		// error if node is not readable or writable
		if (!IsReadable(ptrAdcBitDepth) || !IsWritable(ptrAdcBitDepth))
		{
			cout << "Unable to set ADC Bit Depth (enum retrieval). Aborting..." << endl << endl;
			return -1;
		}

		CEnumEntryPtr ptrAdcBitDepthDesired = ptrAdcBitDepth->GetEntryByName(gcstring(bitDepth.c_str()));
		// error if mode is not readable
		if (!IsReadable(ptrAdcBitDepthDesired)){
			cout << "Unable to get or set the ADC bit depth to the desired mode. Aborting..." << endl << endl;
			return -1;
		}
		const int64_t AdcBitDepthDesired = ptrAdcBitDepthDesired -> GetValue();
		ptrAdcBitDepth -> SetIntValue(AdcBitDepthDesired);
		cout << "ADC Bit Depth set to" <<  bitDepth  << endl;	
	}
	catch (Spinnaker::Exception& e) { 
		cout << "Error: " << e.what() << endl;
		result = -1;
	}
	return result;

}
int configureLUT(INodeMap&nodeMap, char* LUT){
	// use LUT Selector (Enumeration) to choose which LUT to control
	// LUT1 is the only lookup table that allows user access and control
	// LUT Enable (Boolean) - disable selected LUT before customizing (need to disable before customizing)
	// LUT Index(Integer) - find index of coefficient
	// LUT Value(Integer) - gets value and LUT index, set new value
	// Enable LUT to use new changes

	int result = 0;

	cout << endl << endl << "*** CONFIGURING LOOKUP TABLES ***" << endl << endl;

	try
	{
		//
		// Select lookup table type
		//
		// *** NOTES ***
		// Setting the lookup table selector. It is important to note that this
		// does not enable lookup tables.
		//
		CEnumerationPtr ptrLUTSelector = nodeMap.GetNode("LUTSelector");
		if (!IsReadable(ptrLUTSelector) ||
				!IsWritable(ptrLUTSelector))
		{
			// Some cameras have an LUT type that must be "User Defined"
			CEnumerationPtr ptrLUTType = nodeMap.GetNode("lutType");
			CEnumEntryPtr ptrLUTTypeType = nodeMap.GetNode("UserDefined"); // look for actual name??
			if (!IsReadable(ptrLUTTypeType) ||
					!IsWritable(ptrLUTTypeType)) 
			{
				cout << "Unable to set Lookup Table (node retrieval). Aborting... " << endl;
				//PrintRetrieveNodeFailure("node", "LUTSelector");
				return -1;
			}
			ptrLUTType->SetIntValue(static_cast<int64_t>(ptrLUTTypeType->GetValue()));
		}

		CEnumEntryPtr ptrLUTSelectorLUT = ptrLUTSelector->GetEntryByName(gcstring(LUT));
		// Try out alternate node name
		if (!IsReadable(ptrLUTSelectorLUT))
		{
			ptrLUTSelectorLUT = ptrLUTSelector->GetEntryByName("UserDefined1");
			if (!IsReadable(ptrLUTSelectorLUT))
			{			
				cout << "Unable to set Lookup Table Type (enum entry retrieval)" << endl;
				return -1;
			}
			cout << "Lookup table selector set to User Defined 1..." << endl;
		}
		else
		{
			cout << "Lookup table selector set to LUT 1..." << endl;
		}
		ptrLUTSelector->SetIntValue(static_cast<int64_t>(ptrLUTSelectorLUT->GetValue()));

		//
		// Determine pixel increment and set indexes and values as desired
		//
		// *** NOTES ***
		// To get the pixel increment, the maximum range of the value node must
		// first be retrieved. The value node represents an index, so its value
		// should be one less than a power of 2 (e.g. 511, 1023, etc.). Add 1 to
		// this index to get the maximum range. Divide the maximum range by 512
		// to calculate the pixel increment.
		//
		// Finally, all values (in the value node) and their corresponding
		// indexes (in the index node) need to be set. The goal of this example
		// is to set the lookup table linearly. As such, the slope of the values
		// should be set according to the increment, but the slope of the
		// indexes is inconsequential.
		//
		// Retrieve value node
		CIntegerPtr ptrLUTValue = nodeMap.GetNode("LUTValue");
		if (!IsReadable(ptrLUTValue) ||
				!IsWritable(ptrLUTValue))
		{
			cout << "Unable to set Lookup Table Value (node retrieval)" << endl; 
			//PrintRetrieveNodeFailure("node", "LUTValue");
			return -1;
		}
		// Retrieve index node
		CIntegerPtr ptrLUTIndex = nodeMap.GetNode("LUTIndex");
		if (!IsWritable(ptrLUTIndex))
		{	
			cout << "Unable to set Lookup Table Index (node retrieval). aborting." << endl; 

			//PrintRetrieveNodeFailure("node", "LUTIndex");
			return -1;
		}

		// Retrieve maximum value
		int maxVal = (int)ptrLUTValue->GetMax();
		cout << "\tMaximum Value: " << maxVal << endl;

		// Retrieve maximum index
		int maxIndex = (int)ptrLUTIndex->GetMax();
		cout << "\tMaximum Index: " << maxIndex << endl;

		// Calculate increment
		int increment = maxVal / maxIndex;

		// Set LUT Values
		if (increment > 0) 
		{
			cout << "\tIncrement: " << increment << endl;
			for(int i = 0; i < maxIndex; i++)
			{
				ptrLUTIndex->SetValue(i);
				ptrLUTValue->SetValue(i * increment);
			}

		}
		else 
		{
			int denominator = maxIndex / maxVal;
			cout << "\tIncrement: 1/" << denominator << endl;
			for(int i = 0; i < maxIndex; i++)
			{
				ptrLUTIndex->SetValue(i);
				ptrLUTValue->SetValue(i / denominator);
			}
		}

		cout << "All lookup table values set..." << endl;

		//
		// Enable lookup tables
		//
		// *** NOTES ***
		// Once lookup tables have been configured, don't forget to enable them
		// with the appropriate node.
		//
		// *** LATER ***
		// Once the images with lookup tables have been collected, turn the
		// feature off with the same node.
		//
		CBooleanPtr ptrLUTEnable = nodeMap.GetNode("LUTEnable");
		if (!IsWritable(ptrLUTEnable))
		{
			// Try alternate name
			CEnumerationPtr ptrLUTMode = nodeMap.GetNode("lutMode");
			if (!IsWritable(ptrLUTMode)) 
			{
				cout << "Unable to set Lookup Table Mode (node retrieval). Aborting." << endl; 

				// PrintRetrieveNodeFailure("node", "LUTEnable");
				return -1;
			}
			CEnumEntryPtr ptrLUTModeActive = ptrLUTMode->GetEntryByName("Active");
			ptrLUTMode->SetIntValue(ptrLUTModeActive->GetValue());
		}
		else
		{
			ptrLUTEnable->SetValue(true);
		}



		cout << "Lookup tables enabled..." << endl << endl;
	}
	catch (Spinnaker::Exception& e)
	{
		cout << "Error: " << e.what() << endl;
		result = -1;
	}

	return result;
}
//parameter name: SensorShutterMode
// options: Rolling for some devices, Global for some
int setShutterMode(INodeMap& nodeMap, string shutterMode){
	CEnumerationPtr ptrSensorShutterMode = nodeMap.GetNode("SensorShutterMode");
	if (!IsReadable(ptrSensorShutterMode) || ! IsWritable(ptrSensorShutterMode)){
		cout <<"Unable to read or write sensor shutter mode" << endl;
		return -1;
	}
	cout << "Before: " << ptrSensorShutterMode -> GetDisplayName() << ": " << ptrSensorShutterMode -> GetCurrentEntry() -> GetSymbolic() << endl; 

	CEnumEntryPtr ptrSensorShutterModeDesired = ptrSensorShutterMode-> GetEntryByName(gcstring(shutterMode.c_str()));
	if (!IsReadable(ptrSensorShutterModeDesired) || !IsWritable(ptrSensorShutterModeDesired)){
		cout << "Unable to retrieve " << shutterMode << " shutter mode" << endl;
		return -1;
	}
	ptrSensorShutterMode -> SetIntValue(ptrSensorShutterModeDesired->GetValue());
	cout << "Sensor shutter mode set to " << shutterMode << endl;
	cout << "After: " << ptrSensorShutterMode -> GetDisplayName() << ": " << ptrSensorShutterMode -> GetCurrentEntry() -> GetSymbolic() << endl << endl;
	return 0;
}

int setTrigger(INodeMap& nodeMap, string source, string triggerType, string activation, string overlap, double delay){
	int result = 0;

    try
    {
        //
        // Ensure trigger mode off
        //
        // *** NOTES ***
        // The trigger must be disabled in order to configure whether the source
        // is software or hardware.
        //
        CEnumerationPtr ptrTriggerMode = nodeMap.GetNode("TriggerMode");
        if (!IsReadable(ptrTriggerMode))
        {
            cout << "Unable to disable trigger mode (node retrieval). Aborting..." << endl;
            return -1;
        }

        CEnumEntryPtr ptrTriggerModeOff = ptrTriggerMode->GetEntryByName("Off");
        if (!IsReadable(ptrTriggerModeOff))
        {
            cout << "Unable to disable trigger mode (enum entry retrieval). Aborting..." << endl;
            return -1;
        }

        ptrTriggerMode->SetIntValue(ptrTriggerModeOff->GetValue());

        cout << "Trigger mode disabled..." << endl;
        //
        // Select trigger source
        //
        // *** NOTES ***
        // The trigger source must be set to hardware or software while trigger
        // mode is off.// set trigger activation to rising edge by default
        //
        CEnumerationPtr ptrTriggerSource = nodeMap.GetNode("TriggerSource");
        if (!IsReadable(ptrTriggerSource) ||
            !IsWritable(ptrTriggerSource))
        {
            cout << "Unable to get or set trigger mode (node retrieval). Aborting..." << endl;
            return -1;
        }

        if (source == "Software" )
        {
            // Set trigger mode to software
            CEnumEntryPtr ptrTriggerSourceSoftware = ptrTriggerSource->GetEntryByName("Software");
            if (!IsReadable(ptrTriggerSourceSoftware))
            {
                cout << "Unable to set trigger mode (enum entry retrieval). Aborting..." << endl;
                return -1;
            }

            ptrTriggerSource->SetIntValue(ptrTriggerSourceSoftware->GetValue());

            cout << "Trigger source set to software..." << endl;
        }
        else if (source == "Hardware")
        {
            // Set trigger mode to hardware ('Line0')
            CEnumEntryPtr ptrTriggerSourceHardware = ptrTriggerSource->GetEntryByName("Line0");
            if (!IsReadable(ptrTriggerSourceHardware))
            {
                cout << "Unable to set trigger mode (enum entry retrieval). Aborting..." << endl;
                return -1;
            }

            ptrTriggerSource->SetIntValue(ptrTriggerSourceHardware->GetValue());

            cout << "Trigger source set to hardware..." << endl;
        }
	//
        // Set TriggerSelector
        //default is FrameStart for most cameras
	//Options:
	//AcquisitionStart - trigger stats acquisition in the selected activation mode
	//FrameStart - a trigger is required for each individual that is required
	//FrameBurstStart - trigger acquires a specified number of images, usually used in continuous mode
	if (triggerType != ""){

        	 CEnumerationPtr ptrTriggerSelector = nodeMap.GetNode("TriggerSelector");
       		 if (!IsReadable(ptrTriggerSelector) ||
            	!IsWritable(ptrTriggerSelector))
        	{
            	cout << "Unable to get or set trigger selector (node retrieval). Aborting..." << endl;
            	return -1;
        	}

        	CEnumEntryPtr ptrTriggerSelectorDesired = ptrTriggerSelector->GetEntryByName(gcstring(triggerType.c_str()));
        	if (!IsReadable(ptrTriggerSelectorDesired))
        	{
            	cout << "Unable to get trigger selector (enum entry retrieval). Aborting..." << endl;
            	return -1;
        	}

        	ptrTriggerSelector->SetIntValue(ptrTriggerSelectorDesired->GetValue());

        	cout << "Trigger selector set to " << triggerType  << endl;
	}
	// set trigger overlap: whether a trigger responds while the readout of a previously acquired image is still occuring
	// Off: trigger is ignored during readout
	// ReadOut: trigger can acquire another image during readout
	if (overlap != ""){
	 	CEnumerationPtr ptrTriggerOverlap = nodeMap.GetNode("TriggerOverlap");
        	if (!IsReadable(ptrTriggerOverlap) ||
            	!IsWritable(ptrTriggerOverlap))
        	{
            		cout << "Unable to get or set trigger overlap. Aborting..." << endl;
           		 return -1;
        	}

        	CEnumEntryPtr ptrTriggerDesiredOverlap = ptrTriggerOverlap->GetEntryByName(gcstring(overlap.c_str()));
        	if (!IsReadable(ptrTriggerDesiredOverlap))
        	{
            		cout << "Unable to get " <<overlap<< ". Aborting..." << endl;
            		return -1;
        	}

        	ptrTriggerOverlap->SetIntValue(ptrTriggerDesiredOverlap->GetValue());

        	cout << "Trigger overlap  set to " <<  overlap  << endl;
	}
	// set trigger delay in microseconds
	// test cameras had a minimum delay of 177 and maximum of 65520
	if (delay >=0){
		CFloatPtr ptrTriggerDelay = nodeMap.GetNode("TriggerDelay");
		if (!IsReadable(ptrTriggerDelay) || !IsWritable(ptrTriggerDelay)){
			cout << "Unable to access Trigger Delay mode. Aborting...";
			return -1;
		}
		if (delay < ptrTriggerDelay -> GetMin()){
			cout << "Delay is smaller than minimum(" << ptrTriggerDelay-> GetMin() << "). Aborting.. " << endl;
			return -1;
		}
		else if (delay > ptrTriggerDelay -> GetMax()){
			cout << "Delay is larger than maximum(" << ptrTriggerDelay->GetMax() << "). Aborting.. " << endl;
			return -1;
		}
		ptrTriggerDelay -> SetValue(delay);
		cout << "Trigger delay set to " << delay << endl;
	}

	// set trigger activation mode
	// trigger activation only available when source is set to hardware
	//Options: LevelLow, LevelHigh, FallingEdge, RisingEdge, AnyEdge
	if (activation != ""){

		CEnumerationPtr ptrTriggerActivation = nodeMap.GetNode("TriggerActivation");
		if (!IsReadable(ptrTriggerActivation) || !IsWritable(ptrTriggerActivation)){
			cout << "Unable to set trigger activation (enumeration  node retrieval). Aborting..." << endl;
			return -1;
		}


		CEnumEntryPtr ptrTriggerActivationDesired = ptrTriggerActivation -> GetEntryByName(gcstring(activation.c_str()));
		if (!IsReadable(ptrTriggerActivationDesired)){
			cout << "Unable to get trigger activation mode (enum entry retrieval). Aborting..." << endl;
			return -1;
		}
		ptrTriggerActivation -> SetIntValue(ptrTriggerActivationDesired-> GetValue());
		cout << "Trigger activation mode set to " << activation  << endl;
	}

        //
        // Turn trigger mode on
        //
        // *** LATER ***
        // Once the appropriate trigger source has been set, turn trigger mode
        // on in order to retrieve images using the trigger.
        //

        CEnumEntryPtr ptrTriggerModeOn = ptrTriggerMode->GetEntryByName("On");
        if (!IsReadable(ptrTriggerModeOn))
        {
            cout << "Unable to enable trigger mode (enum entry retrieval). Aborting..." << endl;
            return -1;
        }

        ptrTriggerMode->SetIntValue(ptrTriggerModeOn->GetValue());

        // NOTE: Blackfly and Flea3 GEV cameras need 1 second delay after trigger mode is turned on

        cout << "Trigger mode turned back on..." << endl << endl;
    }
    catch (Spinnaker::Exception& e)
    {
        cout << "Error: " << e.what() << endl;
        result = -1;
    }

    return result;		
}
// activate all chunks
/*chunks:
	Image - can't be disabled
	CRC (cyclic redundancy check) - can't be disabled
	FrameID
	OffsetX
	OffsetY
	Width
	Height
	Exposure Time
	Gain
	Black Level
	Pixel Format
	ImageTimestamp
	Sequencer Set Active
	*/

int enableChunkData(INodeMap& nodeMap){
	int result = 0;
	cout << endl << endl << "*** CONFIGURING CHUNK DATA ***" << endl << endl;
	try
	{
		// activate chunk mode
		CBooleanPtr ptrChunkModeActive = nodeMap.GetNode("ChunkModeActive");
		if (!IsWritable(ptrChunkModeActive))
		{
			cout << "Unable to activate chunk mode. Aborting.." << endl << endl;
			return -1;
		}
		ptrChunkModeActive -> SetValue(true);
		cout << "Chunk mode activated..." << endl;
		// iterate through and enable chunk data
		NodeList_t entries;
		CEnumerationPtr ptrChunkSelector = nodeMap.GetNode("ChunkSelector");
		if (!IsReadable(ptrChunkSelector)){
			cout << "Unable to retrieve chunk selector. Aborting..." << endl << endl;
			return -1;
		}
		ptrChunkSelector -> GetEntries(entries);
		cout << "Enabling entries..." << endl;
		for (size_t i = 0; i < entries.size(); i++){
			//access the current chunk in list
			CEnumEntryPtr ptrChunkSelectorEntry = entries.at(i);
			// Skip to next node if the node is not readable
			if (!IsReadable(ptrChunkSelectorEntry))
			{
				continue;
			}
			ptrChunkSelector -> SetIntValue(ptrChunkSelectorEntry -> GetValue());
			cout << "\t" << ptrChunkSelectorEntry -> GetSymbolic() << ": ";
			// retrieve chunk enable boolean node
			CBooleanPtr ptrChunkEnable = nodeMap.GetNode("ChunkEnable");
			if (!IsAvailable(ptrChunkEnable)){
				cout << "not available" << endl;
				result = -1;
			}
			else if (ptrChunkEnable -> GetValue())
			{
				cout << "enabled" << endl;
			}
			else if (IsWritable(ptrChunkEnable))
			{
				ptrChunkEnable-> SetValue(true);
				cout << "enabled" << endl;
			}
			else{
				cout << "not writable" << endl;
				result = -1;
			}
		}
		
	}
	catch (Spinnaker:: Exception& e)
	{
		cout << "Error: " << e.what() << endl;
		result = -1;
	}
	return result;
}

// options: NewestFirst, NewestOnly, OldestFirst, OldestFirstOverwrite
// default is usually OldestFirst
int setBufferHandlingMode(INodeMap& sNodeMap, string bufferHandlingMode){
	int result = 0;
	
	CEnumerationPtr ptrHandlingMode = sNodeMap.GetNode("StreamBufferHandlingMode");
	if (!IsReadable(ptrHandlingMode) || ! IsWritable(ptrHandlingMode)){
		cout << "Unable to set Buffer Handling mode (Enumeration node retrieval). Aborting.." << endl;
		return -1;
	}
	cout << "Before: " << ptrHandlingMode -> GetDisplayName() <<  ": " << ptrHandlingMode -> GetCurrentEntry() -> GetSymbolic() << endl;
	CEnumEntryPtr  ptrHandlingModeEntry = ptrHandlingMode -> GetEntryByName(gcstring(bufferHandlingMode.c_str()));
	if (!IsReadable(ptrHandlingModeEntry)){
		cout << "Unable to get Buffer Handling mode (entry retrieval). Aborting.." << endl;
		return -1;
	}

	ptrHandlingMode-> SetIntValue(ptrHandlingModeEntry->GetValue());
	cout << "Stream Buffer Handling Mode set to " << bufferHandlingMode << endl << endl;
	cout <<"After: " << ptrHandlingMode -> GetDisplayName() << ": " <<  ptrHandlingMode -> GetCurrentEntry()-> GetSymbolic() << endl;

	return result;
}
// print all nodemap info from /usr/src/spinnaker/src/NodeMapInfo
int prepareCameras(CameraList camList,const string fileName){
	int result = 0;
	ifstream configFile(fileName);
	json configurations = json::parse(configFile);
	if (!configurations.is_null()){	
		CameraPtr pCam = nullptr;
		json cameras = configurations["Cameras"];
		try {
			for (unsigned int i = 0; i < camList.GetSize(); i++)
			{
				pCam = camList.GetByIndex(i);
				pCam-> Init();
				INodeMap& nodeMap = pCam -> GetNodeMap();
				// get device serial number
				INodeMap& nodeMapTLDevice = pCam -> GetTLDeviceNodeMap();
				gcstring deviceSerialNumber("");
				CStringPtr ptrStringSerial = nodeMapTLDevice.GetNode("DeviceSerialNumber");
				if (IsReadable(ptrStringSerial)){
					deviceSerialNumber = ptrStringSerial -> GetValue();
				}		
				else{
					cout << "device serial number not readable" << endl;
					return -1;
				}	
				// access the settings from the config file associated with the current camera
				json currentCam;
				for (json::iterator it = cameras.begin(); it != cameras.end(); it++)
				{
					// find serial number of each device described in file
					if (it.value()["DeviceID"] == deviceSerialNumber){
						currentCam = it.value();
						cout << "Current camera: " << it.key()<< endl;	
						cout << "Serial number: " << deviceSerialNumber << endl;
					}
				}

				if (currentCam.is_null()){
					cout << "No json configuration found for device with serial number" << deviceSerialNumber << endl;
					continue;
				}
				// check is camera is specified as InUse
				json inUse = currentCam["InUse"];
				if (!inUse.is_boolean()){
					cout << "InUse parameter is not a boolean, trying next camera..." << endl;
					continue;
				}
			
				if (!inUse){
					cout << "Camera not in use: trying next camera.." << endl;
				}
			
				// set acquisition mode to continuous by default, and will change if the configuration file specifies otherwise
				string acquisitionMode = "Continuous";
				if (!currentCam["AcquisitionMode"].is_null()){
					acquisitionMode = currentCam["AcquisitionMode"];
				}	
				configureAcquisition(nodeMap,acquisitionMode);
					
				// set attributes
				if (!currentCam["Exposure"].is_null()){
				//sets exposure to the specified time
					if (currentCam["Exposure"].is_number()){
					double exposureTime = currentCam["Exposure"];
					setExposure(nodeMap, exposureTime);
					}
					// puts exposure to continuous automatic if Exposure is specified in the json file but not a number
					else{
						ResetExposure(nodeMap);
					}
				} 			
				if (!currentCam["Gain"].is_null()){
					if (currentCam["Gain"].is_number())
					{
						double gain = currentCam["Gain"];
						setGain(nodeMap, gain);
					}
					// sets gain to continuous automatic is Gain is specified but not a number
					else{
						ResetGain(nodeMap);
					}
				}
				if (!currentCam["PixelFormat"].is_null()){
					string pixelFormat = currentCam["PixelFormat"];		
					setPixelFormat(nodeMap, pixelFormat);
				}
				if (!currentCam["OffsetX"].is_null()){
					int64_t OffsetXToSet = currentCam["OffsetX"];
					setOffsetX(nodeMap, OffsetXToSet);
				}	
				
				if (!currentCam["OffsetY"].is_null()){
					int64_t OffsetYToSet = currentCam["OffsetY"];
					setOffsetY(nodeMap, OffsetYToSet);
				}
				
				if (!currentCam["Width"].is_null()){
					int64_t width= currentCam["Width"];
					setWidth(nodeMap, width);
				}
				if (!currentCam["Height"].is_null()){
					int64_t	height = currentCam["Height"];
					setHeight(nodeMap, height);
				}
				
				if (!currentCam["AdcBitDepth"].is_null()){
					string bitDepth = currentCam["AdcBitDepth"];
					setADCBitDepth(nodeMap, bitDepth);
				}
				if (!currentCam["SensorShutterMode"].is_null()){
					cout << currentCam << endl;
					string shutterMode = currentCam["SensorShutterMode"];
					setShutterMode(nodeMap, shutterMode);
				}
				string chosenTrigger;
				if (!currentCam["TriggerSource"].is_null()){
					chosenTrigger = currentCam["TriggerSource"];
				}
				string triggerType;
				if (!currentCam["TriggerSelector"].is_null()){
					triggerType = currentCam["TriggerSelector"];
				}	
				string triggerActivation;
				if (!currentCam["TriggerActivation"].is_null()){
					triggerActivation = currentCam["TriggerActivation"];
				}
				string overlap;
				if (!currentCam["TriggerOverlap"].is_null()){
					overlap = currentCam["TriggerOverlap"];
				}
				double delay = -1;
				if(!currentCam["TriggerDelay"].is_null()){
				     	delay = currentCam["TriggerDelay"];
				}
				// will not do anything if no trigger parameters specified in the config file
				setTrigger(nodeMap, chosenTrigger,triggerType, triggerActivation,  overlap, delay);
				// enables data to be sent along with image data
				enableChunkData(nodeMap);
				// sets buffer handling mode
				if (!currentCam["StreamBufferHandlingMode"].is_null()){
					INodeMap& sNodeMap = pCam -> GetTLStreamNodeMap();
					string bufferHandlingMode = currentCam["StreamBufferHandlingMode"];
					setBufferHandlingMode(sNodeMap, bufferHandlingMode);
				}
				//save current settings to userset, and set them to default so that they load the next time the camera is turned on
				CEnumerationPtr ptrUserSelector = nodeMap.GetNode("UserSetSelector");
				ptrUserSelector -> SetIntValue(ptrUserSelector -> GetEntryByName("UserSet0") -> GetValue());
				CCommandPtr ptrUserSetSave = nodeMap.GetNode("UserSetSave");
				ptrUserSetSave -> Execute();
				CEnumerationPtr ptrUserSetDefault = nodeMap.GetNode("UserSetDefault");
				CEnumEntryPtr ptrUserSetDefault0 = ptrUserSetDefault -> GetEntryByName("UserSet0");
				ptrUserSetDefault -> SetIntValue(ptrUserSetDefault0 -> GetValue());
				cout << "Settings saved to " << ptrUserSetDefault0 -> GetSymbolic() << endl;
				pCam-> DeInit();
			}
		}
		catch (Spinnaker::Exception& e)
		{
        		cout << "Error: " << e.what() << endl;
        		return -1;
		}
	}
	else{
		cout << "File could not be parsed" << endl;
	}
		
	return result;
}

int main(int argc, char** argv)
{
    // Since this application saves images in the current folder
    // we must ensure that we have permission to write to this folder.
    // If we do not have permission, fail right away.
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

    // Print application build information
    cout << "Application build date: " << __DATE__ << " " << __TIME__ << endl << endl;

    // Retrieve singleton reference to system object
    SystemPtr system = System::GetInstance();

    // Print out current library version
    const LibraryVersion spinnakerLibraryVersion = system->GetLibraryVersion();
    cout << "Spinnaker library version: " << spinnakerLibraryVersion.major << "." << spinnakerLibraryVersion.minor
        << "." << spinnakerLibraryVersion.type << "." << spinnakerLibraryVersion.build << endl
        << endl;
	int result = 0;
    // Retrieve list of cameras from the system
    CameraList camList = system->GetCameras();

    const unsigned int numCameras = camList.GetSize();

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
	else{
		// if no arguments given, do nothing
		if (argc < 2){
			cout << "No configuration file given, so no settings will be changed." << endl;
			return -1;
		}
		else{
			const string filename = argv[1];
			prepareCameras(camList,filename);
		}
	}
	camList.Clear();
	system -> ReleaseInstance();
	return result;	
}
