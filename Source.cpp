#include <iostream>
#include <WS2tcpip.h>
#include <string>
#include<stdio.h>
#include<windows.h>
#include<Xinput.h>


#pragma comment (lib, "ws2_32.lib")
#pragma comment(lib, "xinput.lib")



// XBOX Controller Class Definition
class CXBOXController
{
private:
	XINPUT_STATE _controllerState;
	int _controllerNum;
public:
	CXBOXController(int playerNumber);
	XINPUT_STATE GetState();
	bool IsConnected();
	void Vibrate(int leftVal = 0, int rightVal = 0);
};
CXBOXController::CXBOXController(int playerNumber)
{
	// Set the Controller Number
	_controllerNum = playerNumber - 1;
}

XINPUT_STATE CXBOXController::GetState()
{
	// Zeroise the state
	ZeroMemory(&_controllerState, sizeof(XINPUT_STATE));

	// Get the state
	XInputGetState(_controllerNum, &_controllerState);

	return _controllerState;
}

bool CXBOXController::IsConnected()
{
	// Zeroise the state
	ZeroMemory(&_controllerState, sizeof(XINPUT_STATE));

	// Get the state
	DWORD Result = XInputGetState(_controllerNum, &_controllerState);

	if (Result == ERROR_SUCCESS)
	{
		return true;
	}
	else
	{
		return false;
	}
}

void CXBOXController::Vibrate(int leftVal, int rightVal)
{
	// Create a Vibraton State
	XINPUT_VIBRATION Vibration;

	// Zeroise the Vibration
	ZeroMemory(&Vibration, sizeof(XINPUT_VIBRATION));

	// Set the Vibration Values
	Vibration.wLeftMotorSpeed = leftVal;
	Vibration.wRightMotorSpeed = rightVal;

	// Vibrate the controller
	XInputSetState(_controllerNum, &Vibration);
}
CXBOXController* Player1;
using namespace std;

void main(int argc, char* argv[])
{
	int re_flag = 0; //FLAG TO DETERMINE IF CONNECTION HAS BEEN LOST BEFORE
	string ipAddress;  //ip address of the server
	int port = 55100;//Listening port # on the server
	cout << "What is the ip address?" << endl;
	getline(cin, ipAddress);

	Player1 = new CXBOXController(1);
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

	XINPUT_GAMEPAD js;
	//Do-While loop to send and receive data
	char accl[4096], bra[4096], steer[4096];
	string userInput1, userInput2, userInput3;
	int bytesReceived1, bytesReceived2, bytesReceived3;
	int input1, input2, input3;
	do
	{
		js = Player1->GetState().Gamepad;
		input1 = js.sThumbLX+32768;
		input2 = (js.bLeftTrigger * 65535) / 255;
		input3 = (js.bRightTrigger * 65535) / 255;
	userInput1 = to_string(input1);
	userInput2 = to_string(input2);
	userInput3 = to_string(input3);
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