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
const wchar_t* memname = L"Local\\SCSControls";
const size_t size = 3 * sizeof(float);
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

	float data[3] = { 0.0, 0.0, 0.0 };
	memcpy(pBuf, data, size);

	UnmapViewOfFile(pBuf);

	log_line("Successfully opened shared mem file.");
}

// Function to read shared memory
std::array<float, 3> read_mem() {
	if (hMapFile == NULL) {
		log_line("Shared mem file not open.");
		return std::array<float, 3>{};
	}

	void* pBuf = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, size);

	std::array<float, 3> data;
	memcpy(data.data(), pBuf, size);

	UnmapViewOfFile(pBuf);
	return data;
}

#define UNUSED(x)

int inputNumber = 0;
scs_float_t inputValues[3];
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

		std::array<float, 3> values = read_mem();

		// Check that none of the values are too high or too low.
		if (values[0] > 1) 
		{
			values[0] = 1;
		}
		if (values[0] < -1)
		{
			values[0] = -1;
		}
		if (values[1] > 1)
		{
			values[1] = 1;
		}
		if (values[1] < -1)
		{
			values[1] = -1;
		}
		if (values[2] > 1)
		{
			values[2] = 1;
		}
		if (values[2] < -1)
		{
			values[2] = -1;
		}

		char buffer[64];
		int ret = snprintf(buffer, sizeof buffer, "%f", values[0]);

		for (int i = 0; i < device_info.input_count; i++) {
			inputValues[i] = values[i];
		}

		if (ret < 0) {
			return EXIT_FAILURE;
		}
		
		log_line(buffer);
	}

	event_info->input_index = inputNumber;

	if (inputNumber == 0)
	{
		event_info->value_float.value = inputValues[0];
		log_line("Updated steering");
	}
	if (inputNumber == 1)
	{
		event_info->value_float.value = inputValues[1];
		log_line("Updated acceleration");
	}
	if (inputNumber == 2)
	{
		event_info->value_float.value = inputValues[2];
		log_line("Updated braking");
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

	const scs_input_device_input_t inputs[] = { 
		{"steering", "ETS2LA Steering", SCS_VALUE_TYPE_float }, 
		{"aforward", "ETS2LA Forward", SCS_VALUE_TYPE_float }, 
		{"abackward", "ETS2LA Backward", SCS_VALUE_TYPE_float } 
	};
	device_info.input_count = 3;
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
