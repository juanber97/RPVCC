#include<iostream>
#include<WS2tcpip.h>
#include<string>
#include <windows.h>
#include <basetsd.h>
#include <dinput.h>
#include <stdio.h>

#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")
class Joystick
{
private:
	unsigned int            id;
	unsigned int            device_counter;

	LPDIRECTINPUT8          di;
	LPDIRECTINPUTDEVICE8    joystick;

public:
	Joystick(unsigned int id);
	~Joystick();

	HRESULT deviceName(char* name);

	HRESULT open();
	HRESULT close();

	HRESULT poll(DIJOYSTATE2* js);

	BOOL CALLBACK enumCallback(const DIDEVICEINSTANCE* instance, VOID* context);

	// Device Querying
	static unsigned int deviceCount();
};

BOOL CALLBACK enumCallback(const DIDEVICEINSTANCE* instance, VOID* context);
BOOL CALLBACK countCallback(const DIDEVICEINSTANCE* instance, VOID* counter);


#define SAFE_RELEASE(p)     { if(p) { (p)->Release(); (p) = NULL; } }

Joystick::Joystick(unsigned int id)
{
	this->id = id;
	device_counter = 0;

	di = NULL;
	joystick = NULL;
}

Joystick::~Joystick()
{
	close();
}

HRESULT
Joystick::deviceName(char* name)
{
	HRESULT hr;
	DIDEVICEINSTANCE device;

	ZeroMemory(&device, sizeof(device));
	device.dwSize = sizeof(device);

	if (!di || !joystick) {
		return E_INVALIDARG;
	}

	if (FAILED(hr = joystick->GetDeviceInfo(&device))) {
		return hr;
	}


	return hr;
}

HRESULT
Joystick::open()
{
	HRESULT hr;

	// Create a DirectInput device
	if (FAILED(hr = DirectInput8Create(GetModuleHandle(NULL),
		DIRECTINPUT_VERSION,
		IID_IDirectInput8,
		(VOID * *)& di, NULL))) {
		return hr;
	}

	// Look for the first simple joystick we can find.
	if (FAILED(hr = di->EnumDevices(DI8DEVCLASS_GAMECTRL, ::enumCallback,
		(LPVOID)this, DIEDFL_ATTACHEDONLY))) {
		return hr;
	}

	// Make sure we got a joystick
	if (joystick == NULL) {
		return E_FAIL;
	}

	// Set the data format to "simple joystick" - a predefined data format 
	//
	// A data format specifies which controls on a device we are interested in,
	// and how they should be reported. This tells DInput that we will be
	// passing a DIJOYSTATE2 structure to IDirectInputDevice::GetDeviceState().
	if (FAILED(hr = joystick->SetDataFormat(&c_dfDIJoystick2))) {
		return hr;
	}

	// Set the cooperative level to let DInput know how this device should
	// interact with the system and with other DInput applications.
	if (FAILED(hr = joystick->SetCooperativeLevel(NULL, DISCL_EXCLUSIVE | DISCL_FOREGROUND))) {
		return hr;
	}

	return S_OK;
}

HRESULT
Joystick::close()
{
	if (joystick) {
		joystick->Unacquire();
	}

	SAFE_RELEASE(joystick);
	SAFE_RELEASE(di);

	return S_OK;
}

HRESULT
Joystick::poll(DIJOYSTATE2* js)
{
	HRESULT hr;

	if (joystick == NULL) {
		return S_OK;
	}

	// Poll the device to read the current state
	hr = joystick->Poll();
	if (FAILED(hr)) {

		// DirectInput is telling us that the input stream has been
		// interrupted.  We aren't tracking any state between polls, so we
		// don't have any special reset that needs to be done.  We just
		// re-acquire and try again.
		hr = joystick->Acquire();
		while (hr == DIERR_INPUTLOST) {
			hr = joystick->Acquire();
		}

		// If we encounter a fatal error, return failure.
		if ((hr == DIERR_INVALIDPARAM) || (hr == DIERR_NOTINITIALIZED)) {
			return E_FAIL;
		}

		// If another application has control of this device, return success.
		// We'll just have to wait our turn to use the joystick.
		if (hr == DIERR_OTHERAPPHASPRIO) {
			return S_OK;
		}
	}

	// Get the input's device state
	if (FAILED(hr = joystick->GetDeviceState(sizeof(DIJOYSTATE2), js))) {
		return hr;
	}

	return S_OK;
}

BOOL CALLBACK
Joystick::enumCallback(const DIDEVICEINSTANCE* instance, VOID* context)
{
	// If this is the requested device ID ...
	if (device_counter == this->id) {

		// Obtain an interface to the enumerated joystick.  Stop the enumeration
		// if the requested device was created successfully.
		if (SUCCEEDED(di->CreateDevice(instance->guidInstance, &joystick, NULL))) {
			return DIENUM_STOP;
		}
	}

	// Otherwise, increment the device counter and continue with
	// the device enumeration.
	device_counter++;

	return DIENUM_CONTINUE;
}

BOOL CALLBACK
enumCallback(const DIDEVICEINSTANCE* instance, VOID* context)
{
	if (context != NULL) {
		return ((Joystick*)context)->enumCallback(instance, context);
	}
	else {
		return DIENUM_STOP;
	}
}

unsigned int
Joystick::deviceCount()
{
	unsigned int counter = 0;
	LPDIRECTINPUT8 di = NULL;
	HRESULT hr;

	if (SUCCEEDED(hr = DirectInput8Create(GetModuleHandle(NULL),
		DIRECTINPUT_VERSION,
		IID_IDirectInput8,
		(VOID * *)& di, NULL))) {
		di->EnumDevices(DI8DEVCLASS_GAMECTRL, ::countCallback,
			&counter, DIEDFL_ATTACHEDONLY);
	}

	return counter;
}

BOOL CALLBACK
countCallback(const DIDEVICEINSTANCE* instance, VOID* counter)
{
	if (counter != NULL) {
		unsigned int* tmpCounter = (unsigned int*)counter;
		(*tmpCounter)++;
		counter = tmpCounter;
	}

	return DIENUM_CONTINUE;
}
using namespace std;

void main()
{
	Joystick* joysticks[4];
	unsigned int i;
	int re_flag = 0; //FLAG TO DETERMINE IF CONNECTION HAS BEEN LOST BEFORE
	string ipAddress;  //ip address of the server
	int port = 55100;//Listening port # on the server
	cout << "What is the ip address?" << endl;
	getline(cin, ipAddress);
	unsigned int numJoysticks = Joystick::deviceCount();
	printf("Found %d joysticks:\n", numJoysticks);
	//Initialize WinSock
	WSAData data;
	WORD ver = MAKEWORD(2, 2);
	int wsResult = WSAStartup(ver, &data);
	if (wsResult != 0)
	{
		cerr << "Can't Start Winsock, Err #" << wsResult << endl;
		return;
	}
	cout << "WINSOCK STARTED" << endl;
	for (i = 0; i < numJoysticks; i++) {
		joysticks[i] = new Joystick(i);
		joysticks[i]->open();

		// Print the name of the joystick.
		char name[MAX_PATH];
		joysticks[i]->deviceName(name);
		printf("  Joystick %d: %s\n", i, name);
	}
loop:

	//Create Socket
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET)
	{
		cerr << "Can't create socket, Err #" << WSAGetLastError() << endl;
		WSACleanup();
		return;
	}
	cout << "SOCKET CREATED" << endl;

	//Fill in a hint structure
	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(port);
	inet_pton(AF_INET, ipAddress.c_str(), &hint.sin_addr);

	//Connect to server
	int connResult = connect(sock, (sockaddr*)&hint, sizeof(hint));
	for (int i = 0; i < 20; i++)
	{

		if (connResult == SOCKET_ERROR)
		{
			cerr << "Can't connect to server, Err #" << WSAGetLastError() << endl;
			if (re_flag == 0) {
				cerr << "ATTEMPTING TO CONNECT...TIMEOUT: " << 20 - i << " seconds" << endl;
			}
			else if (re_flag == 1) {
				cerr << "ATTEMPTING TO RECONNECT...TIMEOUT: " << 20 - i << " seconds" << endl;
			}

			Sleep(1000);
			connResult = connect(sock, (sockaddr*)&hint, sizeof(hint));
			WSACleanup();
			return;
		}
		else {
			if (re_flag == 0) {
				cerr << "CONNECTION ESTABLISHED" << endl;
				break;
			}
			else if (re_flag == 1) {
				cerr << "CONNECTION REESTABLISHED" << endl;
				break;
			}
		}
	}
	cout << "Connected to server" << endl;
	//Do-While loop to send and receive data
	char accl[4096], bra[4096], steer[4096];
	string userInput1, userInput2, userInput3;
	int bytesReceived1, bytesReceived2, bytesReceived3;
	do
	{
		DIJOYSTATE2 js;
		Sleep(1);
		joysticks[0]->poll(&js);

		userInput1 = to_string(js.lX);
		userInput2 = to_string(js.lY);
		userInput3 = to_string(js.lRz);
		//send the text1 wait for response
		int sendResult1 = send(sock, userInput1.c_str(), userInput1.size() + 1, 0);
		if (sendResult1 != SOCKET_ERROR) {
			ZeroMemory(steer, 4096);
			bytesReceived1 = recv(sock, steer, 4096, 0);
		}
		else {
			cerr << "CONNECTION HAS BEEN LOST";
			re_flag = 1;
			goto loop;
		}
		int sendResult2 = send(sock, userInput2.c_str(), userInput2.size() + 1, 0);
		//send the text2 wair for response
		if (sendResult2 != SOCKET_ERROR) {
			ZeroMemory(bra, 4096);
			bytesReceived2 = recv(sock, bra, 4096, 0);
		}
		else {
			cerr << "CONNECTION HAS BEEN LOST";
			re_flag = 1;
			goto loop;
		}

		int sendResult3 = send(sock, userInput3.c_str(), userInput3.size() + 1, 0);
		if (sendResult3 != SOCKET_ERROR)
		{
			//wait for response
			bytesReceived3 = recv(sock, accl, 4096, 0);
		}
		else {
			cerr << "CONNECTION HAS BEEN LOST";
			re_flag = 1;
			goto loop;
		}
		if (bytesReceived1 > 0 ||
			bytesReceived2 > 0 ||
			bytesReceived3 > 0)
		{
			//Echo response to console
			cout << "Steering: " << string(steer, 0, bytesReceived1)
				<< " Brakes: " << string(bra, 0, bytesReceived2)
				<< " Acceleration: " << string(accl, 0, bytesReceived3) << endl;
		}
		else {
			cerr << "CONNECTION HAS BEEN LOST";
			re_flag = 1;
			goto loop;
		}

	} while (userInput1.size() > 0);
	//Gracefully close down everything
	closesocket(sock);
	WSACleanup();
}
