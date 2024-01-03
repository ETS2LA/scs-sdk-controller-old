/**
 * @brief Simple semantical input device
 *
 * Generates a simple device which toggles lights based on time
 */

// Windows stuff.

#ifdef _WIN32
#  define WINVER 0x0500
#  define _WIN32_WINNT 0x0500
#  include <windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <array>
#include <sstream>
const time_t startTime = time(NULL);

// Management of the log file.
FILE* log_file = NULL;

bool init_log(void)
{
	if (log_file) {
		return true;
	}
	log_file = fopen("input.log", "wt");
	fprintf(log_file, "Log opened\n");
	if (!log_file) {
		return false;
	}
	return true;
}

void finish_log(void)
{
	if (!log_file) {
		return;
	}
	fprintf(log_file, "Log ended\n");
	fclose(log_file);
	log_file = NULL;
}

void log_print(const char* const text, ...)
{
	if (!log_file) {
		return;
	}
	va_list args;
	va_start(args, text);
	vfprintf(log_file, text, args);
	va_end(args);
}

void log_line(const char* const text, ...)
{
	if (!log_file) {
		return;
	}
	va_list args;
	va_start(args, text);
	vfprintf(log_file, text, args);

	fprintf(log_file, "\n");
	va_end(args);
}

// SDK
#include "scssdk_input.h"
#include "eurotrucks2/scssdk_eut2.h"
#include "eurotrucks2/scssdk_input_eut2.h"
#include "amtrucks/scssdk_ats.h"
#include "amtrucks/scssdk_input_ats.h"

// Shared Memory
const int axisCount = 4;
const int buttonCount = 15;
const wchar_t* memname = L"Local\\SCSControls";
const size_t buttonSize = buttonCount * sizeof(bool);
const size_t axisSize = axisCount * sizeof(float);
const size_t size = axisSize + buttonSize;
HANDLE hMapFile;

// Function to initialize shared memory
void initialize_mem() {
	hMapFile = CreateFileMapping(
		INVALID_HANDLE_VALUE,    // use paging file
		NULL,                    // default security
		PAGE_READWRITE,          // read/write access
		0,                       // maximum object size (high-order DWORD)
		size,                    // maximum object size (low-order DWORD)
		memname);                // name of mapping object

	if (hMapFile == NULL) {
		DWORD dw = GetLastError();
		std::stringstream ss;
		ss << dw;
		std::string message = "Failed to create file mapping. Error code: " + ss.str();
		log_line(message.c_str());
		return;
	}

	void* pBuf = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, size);

	if (pBuf == NULL) {
		log_line("Failed to map view of file.");
		CloseHandle(hMapFile);
		hMapFile = NULL;
		return;
	}

	float data[buttonCount + axisCount] = {};
	for (int i = 0; i < buttonCount + axisCount; i++)
	{
		if (i < axisCount)
		{
			data[i] = 0.0;
		}
		else
		{
			data[i] = false;
		}
	}
	memcpy(pBuf, data, size);

	UnmapViewOfFile(pBuf);

	log_line("Successfully opened shared mem file.");
}

// Function to read shared memory
std::pair<std::array<float, axisCount>, std::array<bool, buttonCount>> read_mem() {
	if (hMapFile == NULL) {
		log_line("Shared mem file not open.");
		return std::make_pair(std::array<float, axisCount>{}, std::array<bool, buttonCount>{});
	}

	void* pBuf = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, size);

	std::array<float, axisCount> floatData;
	std::array<bool, buttonCount> boolData;

	memcpy(floatData.data(), pBuf, axisSize);
	memcpy(boolData.data(), static_cast<char*>(pBuf) + size, buttonSize);

	UnmapViewOfFile(pBuf);
	return std::make_pair(floatData, boolData);
}

#define UNUSED(x)

int inputNumber = 0;
scs_float_t inputValues[4];
scs_value_bool_t inputBools[15];
scs_input_device_t device_info;
SCSAPI_RESULT input_event_callback(scs_input_event_t* const event_info, const scs_u32_t flags, const scs_context_t UNUSED(context))
{
	if (inputNumber >= device_info.input_count) {
		inputNumber = 0;
		return SCS_RESULT_not_found;
	}

	if (flags & SCS_INPUT_EVENT_CALLBACK_FLAG_first_after_activation) {
		log_line("First call after activation");
	}

	if (SCS_INPUT_EVENT_CALLBACK_FLAG_first_in_frame == 1 && inputNumber == 0) {
		inputNumber = 0;

		// Read the floats from shared memory
		std::pair<std::array<float, axisCount>, std::array<bool, buttonCount>> data = read_mem();
		std::array<float, axisCount> values = data.first;
		std::array<bool, buttonCount> bools = data.second;

		for (int i = 0; i < axisCount; i++)
		{
			if (values[i] > 1.0) {
				values[i] = 1.0;
			}
			if (values[i] < -1.0) {
				values[i] = -1.0;
			}
			inputValues[i] = values[i];
		}

		// Set the bools
		for (int i = 0; i < buttonCount; i++)
		{
			inputBools[i].value = bools[i];
		}
	}

	event_info->input_index = inputNumber;

	if (inputNumber < axisCount)
	{
		event_info->value_float.value = inputValues[inputNumber];
	}
	else
	{
		// TODO: This code is not running, well atleast the value is not true.
		if (inputBools[inputNumber].value == true)
		{
			log_line("Button %d pressed", inputNumber);
		}
		event_info->value_bool.value = inputBools[inputNumber].value;
	}

	inputNumber++;

	return SCS_RESULT_ok;
}

/**
 * @brief Input API initialization function.
 *
 * See scssdk_input.h
 */
SCSAPI_RESULT scs_input_init(const scs_u32_t version, const scs_input_init_params_t *const params)
{
	init_log();

	// We currently support only one version.
	if (version != SCS_INPUT_VERSION_1_00) {
		return SCS_RESULT_unsupported;
	}

	initialize_mem();

	const scs_input_init_params_v100_t *const version_params = static_cast<const scs_input_init_params_v100_t *>(params);

	// Setup the device information. The name of the input matches the name of the
	// mix as seen in controls.sii. Note that only some inputs are supported this way.
	// See documentation of SCS_INPUT_DEVICE_TYPE_semantical

	memset(&device_info, 0, sizeof(device_info));
	device_info.name = "laneassist";
	device_info.display_name = "ETS2 Lane Assist";
	device_info.type = SCS_INPUT_DEVICE_TYPE_semantical;

	// These are all the somewhat useful commands from the controls
	// Name, Index, Type, Control Name In File
	// Steering, 0, float, steering
	// Acceleration, 1, float, aforward
	// Braking, 2, float, abackward
	// Clutch, 3, float, clutch
	// Pause Game, 4, bool,
	// Parking Brake, 5, bool, parkingbrake
	// Wipers, 6, bool, wipers
	// Cruise Control, 7, bool, cruiectrl
	// Cruise Control Increase, 8, bool, cruiectrlinc
	// Cruise Control Decrease, 9, bool, cruiectrldec
	// Cruise Control Reset, 10, bool, cruiectrlres
	// Lights, 11, bool, light
	// High Beams, 12, bool, hblight
	// Left Blinker, 13, bool, lblinker
	// Right Blinker, 14, bool, rblinker
	// Quickpark, 15, bool, quickpark
	// Drive(Gear), 16, bool, drive
	// Reverse(Gear), 17, bool, reverse
	// Cycle Zoom(map ? ), 18, bool, cycl_zoom

	const scs_input_device_input_t inputs[] = { 
		{"steering", "ETS2LA Steering", SCS_VALUE_TYPE_float }, 
		{"aforward", "ETS2LA Forward", SCS_VALUE_TYPE_float }, 
		{"abackward", "ETS2LA Backward", SCS_VALUE_TYPE_float },
		{"clutch", "ETS2LA Clutch", SCS_VALUE_TYPE_float },
		{"pause", "ETS2LA Pause", SCS_VALUE_TYPE_bool },
		{"parkingbrake", "ETS2LA Parking Brake", SCS_VALUE_TYPE_bool },
		{"wipers", "ETS2LA Wipers", SCS_VALUE_TYPE_bool },
		{"cruiectrl", "ETS2LA Cruise Control", SCS_VALUE_TYPE_bool },
		{"cruiectrlinc", "ETS2LA Cruise Control Increase", SCS_VALUE_TYPE_bool },
		{"cruiectrldec", "ETS2LA Cruise Control Decrease", SCS_VALUE_TYPE_bool },
		{"cruiectrlres", "ETS2LA Cruise Control Reset", SCS_VALUE_TYPE_bool },
		{"light", "ETS2LA Lights", SCS_VALUE_TYPE_bool },
		{"hblight", "ETS2LA High Beams", SCS_VALUE_TYPE_bool },
		{"lblinker", "ETS2LA Left Blinker", SCS_VALUE_TYPE_bool },
		{"rblinker", "ETS2LA Right Blinker", SCS_VALUE_TYPE_bool },
		{"quickpark", "ETS2LA Quickpark", SCS_VALUE_TYPE_bool },
		{"drive", "ETS2LA Drive", SCS_VALUE_TYPE_bool },
		{"reverse", "ETS2LA Reverse", SCS_VALUE_TYPE_bool },
		{"cycl_zoom", "ETS2LA Cycle Zoom", SCS_VALUE_TYPE_bool },	
	};
	device_info.input_count = axisCount + buttonCount;
	device_info.inputs = inputs;

	device_info.input_event_callback = input_event_callback;
	device_info.callback_context = NULL;

	if (version_params->register_device(&device_info) != SCS_RESULT_ok) {
		version_params->common.log(SCS_LOG_TYPE_error, "Unable to register device");
		return SCS_RESULT_generic_error;
	}

	log_line("Successfully created and initialized controller.");

	return SCS_RESULT_ok;
}
/**
 * @brief Input API deinitialization function.
 *
 * See scssdk_input.h
 */
SCSAPI_VOID scs_input_shutdown(void)
{
	finish_log();
}

// Cleanup
#ifdef _WIN32
BOOL APIENTRY DllMain(
	HMODULE module,
	DWORD  reason_for_call,
	LPVOID reseved
)
{
	return TRUE;
}
#endif
