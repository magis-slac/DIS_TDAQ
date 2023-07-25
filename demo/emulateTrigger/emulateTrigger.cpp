/* emulate DAQ trigger from central control. 
 * running the executable once will trigger N signals 
 * (can be defined by changing the macro N_SIGNALS) with 
 * a rate that should be taken from the same config file 
 * that acquireImages.cpp uses. Toggles the 
 * appropriate GPIO signal for input, turns it 
 * off, and exits */
 
 // like all PiGPIO programs, user needs to run with sudo 

#include <chrono>
#include <thread>
#include <iostream>
#include <pigpio.h>
#include <json.hpp>
#include <fstream>

using namespace std; 
using namespace nlohmann::json_abi_v3_11_2;



#define PIN 18 // in broadcom numbering 
#define N_SIGNALS 10

// sends a signal to the given pin number (must be configured as output)
void sendSignal(unsigned int pin, unsigned int length) {
	// send a high signal for length amount of time i
	//
	cout << "sending signal..." << endl;

	gpioWrite(pin, PI_HIGH);

	this_thread::sleep_for(chrono::milliseconds(length/2));
			
	gpioWrite(pin, PI_LOW);

	cout << "signal sent successfully" << endl << endl; 

	this_thread::sleep_for(chrono::milliseconds(length/2));

}



int main (int argc, char** argv) { 
	if (gpioInitialise() < 0) {
	cout << "pigipio failed to initialise. aborting..." << endl; 
		return -1; 
	}	

	gpioSetMode(PIN, PI_OUTPUT);

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


	for (int i = 0; i < N_SIGNALS; i++) {

	sendSignal(PIN, configJson["TriggerTiming"]);

	}

	gpioTerminate();

	return 0;
}
