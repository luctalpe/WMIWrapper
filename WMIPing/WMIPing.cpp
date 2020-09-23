// WMIPing.cpp : This file contains the 'main' function. Program execution begins and ends there.
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
#include <iomanip>



#include "..\WMIWrapper\IWbemComWrapper.h"
#include "..\WMIWrapper\Instances.h"

HANDLE hEventToDo = NULL;
HANDLE hEventDone = NULL;
volatile DWORD  bStop = FALSE;
std::stringstream eMessage; 

std::wstring Server;


class LTTimer {
	LARGE_INTEGER frequency;
	LARGE_INTEGER t1, t2;
	double elapsedTime;
	double minimum;
	double maximum;
public:
	LTTimer() : minimum(0), maximum(0), elapsedTime(0), t1({ 0 }), t2({ 0 }) { QueryPerformanceFrequency(&frequency); }
	void Start() {
		QueryPerformanceCounter(&t1);
	}
	double Stop(bool bFinalStop ) {
		QueryPerformanceCounter(&t2);
		elapsedTime = (float)(t2.QuadPart - t1.QuadPart) / frequency.QuadPart;
		if (elapsedTime > maximum) { maximum = elapsedTime; }
		if (minimum == 0 || elapsedTime < minimum) {
			minimum = elapsedTime;
		}
		return elapsedTime;
	}
	double Elapsed() {		return elapsedTime; 	}
	void DisplayStats( ) {
		time_t now = time(0);
		struct tm ltm;
	
		localtime_s(&ltm, &now);
		std::cout << ltm.tm_mon +1 << "/" << ltm.tm_mday +1 << " " << ltm.tm_hour << ":" << ltm.tm_min << ":" << ltm.tm_sec << " " << 
			std::setprecision(4)  << minimum << " ->" << elapsedTime << " " << maximum << std::endl << std::flush;
	}
} WMIElapsedOPeration ;

void Error(const char* msg, HRESULT hres) {
	if (FAILED(hres)) {
		eMessage << msg <<  " Error code = 0x" << std::hex  <<  hres << std::endl;
		throw std::runtime_error(eMessage.str().c_str());
	}

}

int ReturnStatus = 0;

OLECHAR* Win32_Process = const_cast<OLECHAR*>(L"Win32_Process");

volatile HRESULT hres;
DWORD WINAPI WMIPing(
    _In_ LPVOID lpParameter
) {
	DWORD retval = 0;

	try
	{
		/* Initialize COM env.*/
		hres = InitComLibrary();
		Error("Failed to initialize COM Library", hres);

		/* Initialize WMI COM object.*/
		IWbemComWrapper iWbem;
		_bstr_t Namespace;
		if (Server == L"") {
			Namespace = L"ROOT\\CIMV2";
		}
		else {
			Server = std::wstring(L"\\\\") + Server + L"\\ROOT\\CIMV2";
			Namespace = Server.data();
		}
		WMIElapsedOPeration.Start();

		hres = iWbem.ConnectServer(Namespace);
		Error("Could not connect. ",hres);
		

		hres = iWbem.SetSecurity();
		Error("Could not set proxy blanket ", hres);
		int i = 0;
		while (!bStop) {
			IWbemClassObject* p = NULL;
			WaitForSingleObject(hEventToDo, INFINITE);
			if (!bStop) {
				WMIElapsedOPeration.Start();
				hres = iWbem.GetObjectW(Win32_Process, &p);

				Error("Get Win32_Process class failed ", hres);
				WMIElapsedOPeration.Stop(true);
				SetEvent(hEventDone);
	

			}
			if (p) p->Release();
		}

	}

	catch (std::exception& e)
	{
		std::cout << e.what();
		retval = 1;
		SetEvent(hEventDone);
		ReturnStatus = 2;
	}

	FreeComLibrary();

	bStop = true;
    return retval;
}

class Commandline {
	static std::string MakeRegex(std::string s,  bool bString ,int min = 1 ) {
		size_t i = s.length();
		std::string expression("-(");
		expression += s.substr(0, min);
		for (int j = min+1 ; j < i+1; j++) {
			expression += "|";
			expression += s.substr(0, j);
		}
		if( !bString ) expression += ")[:=](\\d*)";
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
		std::regex regex( MakeRegex(a,true, 3) );
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
    
    hEventDone = CreateEventW(NULL, false, false, NULL);
    hEventToDo = CreateEventW(NULL, false, false, NULL);
    if (hEventDone == nullptr || hEventToDo == nullptr) {
        std::cerr << L"Unable to create event  " << std::endl;  return GetLastError();
    }
    HANDLE hThread;
	DWORD dwThreadId = 0;
	int i = 0;

	DWORD TimeWait = (i = Commandline::GetULongOption("ReportWaitMs") ) ? i : 1000 ;

	DWORD MaxWait = (i = Commandline::GetULongOption("MaxWaitMs")) ? i : 10000;
	DWORD PingInterval = (i = Commandline::GetULongOption("IntervalMs")) ? i : 2000; 
	DWORD PingCount = (i = Commandline::GetULongOption("Count")) ? i : INFINITE;

	Server = Commandline::GetStringWOption(L"Server");

	using convert_type = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_type, wchar_t> converter;
	std::string aServer = converter.to_bytes(Server);

	std::cout << "will ping -server:" << aServer << " -ReportWaitMs:" << TimeWait << " -MaxWaitMs:" << MaxWait << " -Count:" << PingCount << " -IntervalMs:" << PingInterval << std::endl << std::flush; 
	std::cout << "\n-MaxWaitMs :\tif a WMI query is not finished within MaxWaits , WMI server will be considered as unresposnive => WMIPing will stop with Status = 1";
	std::cout << "\n-ReportWaitMs\t: if a WMI query takes more than ReportWaitMs, a warning will be displayed";
	std::cout << "\n-IntervalMs\t: will wait IntervalMs before any new query\n\n"; 


    hThread = CreateThread(NULL, 0, WMIPing, NULL, NULL, &dwThreadId);
    if (hThread == NULL) { std::cerr << L"Unable to create a workerthread " << std::endl;  return GetLastError();  }

	i = 0;
	do {
		SetEvent(hEventToDo);
		DWORD dwStatus = WaitForSingleObject(hEventDone, TimeWait);

		if (dwStatus == WAIT_TIMEOUT) {	
			double waittime = WMIElapsedOPeration.Stop(false);
			std::cout << "Slow Response detected " << waittime <<  " seconds without answer "  << std::endl << std::flush ;
			if (waittime*1000 > MaxWait ) {
				std::cout << "WMI unresponsive. Stopping" << std::endl << std::flush;
				bStop = true;
				ReturnStatus = 1;
				TerminateThread(hThread, 10);
			}
		}
		else {
			WMIElapsedOPeration.DisplayStats();
			if (!bStop) Sleep(PingInterval);
		}
		i++;
	} while (!bStop && i < PingCount);

    bStop = true;
    WaitForSingleObject(hThread, 4000);
    CloseHandle(hThread);
    CloseHandle(hEventToDo);
    CloseHandle(hEventDone);
	return ReturnStatus;
}

