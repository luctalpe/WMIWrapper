// WmiBackup.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
// wmiback.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <Windows.h>
#include <wbemcli.h>

#pragma comment( lib , "Wbemuuid.lib" )


HRESULT EnableAllPrivileges()
{
    // Open thread token
    // =================

    HANDLE hToken = NULL;
    BOOL bRes = FALSE;

    bRes = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, &hToken);
    if (!bRes)            return E_FAIL;

    DWORD dwLen;
    BYTE* pBuffer = NULL;
    bRes = GetTokenInformation(hToken, TokenPrivileges, NULL, 0, &dwLen);

    pBuffer = new BYTE[dwLen];
    if (pBuffer == NULL)
    {
        CloseHandle(hToken);
        return E_FAIL;
    }

    bRes = GetTokenInformation(hToken, TokenPrivileges, pBuffer, dwLen, &dwLen);
    if (!bRes)
    {
        CloseHandle(hToken);
        delete[] pBuffer;
        return E_FAIL;
    }
    TOKEN_PRIVILEGES* pPrivs = (TOKEN_PRIVILEGES*)pBuffer;
    for (DWORD i = 0; i < pPrivs->PrivilegeCount; i++)
    {
        pPrivs->Privileges[i].Attributes |= SE_PRIVILEGE_ENABLED;
    }

    bRes = AdjustTokenPrivileges(hToken, FALSE, pPrivs, 0, NULL, NULL);
    delete[] pBuffer;
    CloseHandle(hToken);

    if (!bRes)
        return E_FAIL;
    else
        return S_OK;
}

using namespace std;


int main()
{
    CoInitialize(nullptr);

    EnableAllPrivileges();
    IWbemBackupRestoreEx* pBackup = NULL;
    HRESULT hr = CoCreateInstance(CLSID_WbemBackupRestore, NULL, CLSCTX_ALL, IID_IWbemBackupRestoreEx, (void**)&pBackup);
    std::cout << "hr " << hr << endl;
    hr = CoSetProxyBlanket(pBackup, RPC_C_AUTHN_DEFAULT, RPC_C_AUTHZ_DEFAULT, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_STATIC_CLOAKING);
    std::cout << "CoSetProxyBlanket hr " << hr << " . Will call Pause" << std::endl;
    hr = pBackup->Pause();
    std::cout << "Pause hr " << hr <<  " Will sleep 20 seconds " << endl;
    Sleep(20000);
    std::cout << "Will resume" << hr << endl;
    hr = pBackup->Resume();
    std::cout << "resume result " << hr << endl;

    pBackup->Release();


}
