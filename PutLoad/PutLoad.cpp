// PutLoad.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <iostream>
#include <string>
#include <sstream>
#include <stdexcept>
#include <string>
#include <cstdarg>
#include <locale>
#include <codecvt>
#include <regex>


#include "..\WMIWrapper\IWbemComWrapper.h"
#include "..\WMIWrapper\Instances.h"

#pragma comment(lib, "wbemuuid.lib")

HANDLE hEventToDo = NULL;
HANDLE hEventDone = NULL;
volatile DWORD  bStop = FALSE;
std::stringstream eMessage;

std::wstring Server;


class LTTimer {
	LARGE_INTEGER frequency;
	LARGE_INTEGER t1, t2;
	double elapsedTime;
public:
	LTTimer() : elapsedTime(0), t1({ 0 }), t2({ 0 }) { QueryPerformanceFrequency(&frequency); }
	void Start() {
		QueryPerformanceCounter(&t1);
	}
	double Stop() {
		QueryPerformanceCounter(&t2);
		elapsedTime = (float)(t2.QuadPart - t1.QuadPart) / frequency.QuadPart;
		return elapsedTime;
	}
	double Elapsed() { return elapsedTime; }
} WMIElapsedOPeration;

void Error(const char* msg, HRESULT hres) {
	if (FAILED(hres)) {
		eMessage << msg << " Error code = 0x" << std::hex << hres << std::endl;
		throw std::runtime_error(eMessage.str().c_str());
	}

}

OLECHAR* Win32_Process = const_cast<OLECHAR*>(L"Win32_Process");

volatile HRESULT hres;
DWORD WINAPI WMIPing(
	_In_ LPVOID lpParameter
) {
	DWORD retval = 0;

	try
	{
		
		/* Initialize WMI COM object.*/
		IWbemComWrapper iWbem;
		_bstr_t Namespace;

		Namespace = L"ROOT\\CIMV2";
		hres = iWbem.ConnectServer(Namespace);
		Error("Could not connect. ", hres);


		hres = iWbem.SetSecurity();
		Error("Could not set proxy blanket ", hres);
	
		IWbemClassObject* p = NULL;
		hres = iWbem.GetObjectW(Win32_Process, &p);

		Error("Get Win32_Process class failed ", hres);
		if (p) p->Release();
		std::cout << "Done\n" << std::flush; 

	}

	catch (std::exception& e)
	{
		std::cout << e.what() << std::flush;;
		retval = 1;
		
	}

	FreeComLibrary();

	bStop = true;
	return retval;
}

class Commandline {
	static std::string MakeRegex(std::string s, bool bString, int min = 1) {
		size_t i = s.length();
		std::string expression("-(");
		expression += s.substr(0, min);
		for (int j = min + 1; j < i + 1; j++) {
			expression += "|";
			expression += s.substr(0, j);
		}
		if (!bString) expression += ")[:=](\\d*)";
		else expression += ")[:=](\\w*)";
		return expression;
	}

	static std::wstring MakeRegex(std::wstring s, bool bString, int min = 1) {
		size_t i = s.length();
		std::wstring expression(L"-(");
		expression += s.substr(0, min);
		for (int j = min + 1; j < i + 1; j++) {
			expression += L"|";
			expression += s.substr(0, j);
		}
		if (!bString) expression += L"):(\\d*)";
		else expression += L")[=:](\\w*)";
		return expression;
	}
public:
	static std::string GetStringOption(const char* a) {
		std::smatch m;
		std::regex regex(MakeRegex(a, true, 3));
		std::string cl;
		cl = GetCommandLineA();
		std::regex_search(cl, m, regex);
		if (m.size() == 3) {
			return m[2];
		}
		return "";
	}
	static std::wstring GetStringWOption(const wchar_t* a) {
		std::wsmatch m;
		std::wregex regex(MakeRegex(a, true, 3));
		std::wstring cl;
		cl = GetCommandLineW();
		std::regex_search(cl, m, regex);
		if (m.size() == 3) {
			return m[2];
		}
		return L"";
	}
	static ULONG GetULongOption(const char* a) {
		std::smatch m;
		std::string expression = MakeRegex(a, false, 3);
		std::regex regex(expression);
		std::string cl;
		cl = GetCommandLineA();
		std::regex_search(cl, m, regex);

		if (m.size() == 3) {
			std::string  r = m[2];
			return atol(r.c_str());
		}
		return 0;

	}
};

int main()
{

	/* Initialize COM env.*/
	hres = InitComLibrary();
	Error("Failed to initialize COM Library", hres);


	HANDLE hThread;
	int i; 
	DWORD dwThreadId = 0;
	for ( i = 0 ; i < 800 ; i++ ) {
		hThread = CreateThread(NULL, 0, WMIPing, NULL, NULL, &dwThreadId);
		if (hThread == NULL) { std::cerr << L"Unable to create a workerthread " << std::endl;  return GetLastError(); }
		CloseHandle(hThread);
	}

	std::cout << "Press enter to stop" << std::endl << std::flush;
	std::cin >> i;

	return 1;

}
