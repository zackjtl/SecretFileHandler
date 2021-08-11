// WinMergeScript.cpp : Implementation of CWinMergeScript
#include "stdafx.h"
#include "SecretFileHandler.h"
#include "WinMergeScript.h"
#include "resource.h"
#include <atlstr.h>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <iostream>
#include <cctype>
#include <cstdlib>
#include "randomc.h"

using namespace std;
/////////////////////////////////////////////////////////////////////////////
// CWinMergeScript

/** 
 * @brief Get the name of the current dll
 */
LPTSTR GetDllFilename(LPTSTR name, int len)
{
	// careful for the last char, the doc does not give this detail
	name[len] = 0;
	GetModuleFileName(_Module.GetModuleInstance(), name, len-1);
	// find last backslash
	TCHAR * lastslash = _tcsrchr(name, '/');
	if (lastslash == 0)		
		lastslash = name;
	else
		lastslash ++;
	TCHAR * lastslash2 = _tcsrchr(lastslash, '\\');
	if (lastslash2 == 0)		
		lastslash2 = name;
	else
		lastslash2 ++;
	if (lastslash2 != name)
		lstrcpy(name, lastslash2);
	return name;
}

CString KeyName()
{
	TCHAR szKeyName[256];
	TCHAR name[256+1];
	GetDllFilename(name, 256);
	lstrcpy(szKeyName, _T("Software\\Thingamahoochie\\WinMerge\\Plugins\\"));
	lstrcat(szKeyName, name);
	return szKeyName;
}

CString RegReadString(const CString& key, const CString& valuename, const CString& defaultValue)
{
	CRegKey reg;
	if (reg.Open(HKEY_CURRENT_USER, key, KEY_READ) == ERROR_SUCCESS)
	{
		TCHAR value[512] = {0};
		DWORD dwSize = sizeof(value) / sizeof(value[0]);
		reg.QueryStringValue(valuename, value, &dwSize);
		return value;
	}
	return defaultValue;
}
int HexStringToInteger(const string& Text)
{
	if (Text.empty()) {
		return -1;
	}
	int            value;
	istringstream  is(Text);

	is >> hex >> ((unsigned int&)value);

	if (!is.good() && !is.eof()) {
		return -1;
	}
	return value;
}

int HexStringToInteger(const wstring& Text)
{
	if (Text.empty()) {
		return -1;
	}
	int             value;
	wistringstream  is(Text);

	is >> hex >> ((unsigned int&)value);

	if (!is.good() && !is.eof()) {
		return -1;
	}
	return value;
}

string IntegerToHexString(unsigned int Value, unsigned int Digits)
{
	ostringstream  text;

	text << hex << setw(Digits) << uppercase << setfill('0') << right << Value;

	return string(text.str());
}

wstring IntegerToHexWideString(unsigned int Value, unsigned int Digits)
{
	wostringstream  text;

	text << hex << setw(Digits) << uppercase << setfill(L'0') << right << Value;

	return text.str();
}

int CreateArrayFromRangeString(const TCHAR *rangestr, int (* value)[2])
{
	TCHAR name[256];

	lstrcpy(name, rangestr);

	if (name[0] == 0)
		return 0;

	// first pass : prepare the chunks
	int nValue = 0;
	TCHAR * token = _tcstok(name, _T(",_"));
	while( token != NULL )
	{
		nValue ++;
		/* Get next token: */
		token = _tcstok( NULL, _T(",_") );
	}

	if (value == 0)
		// just return the number of values
		return nValue;

	token = name;
	int i;
	for (i = 0 ; i < nValue ; i++)
	{
		value[i][0] = _tcstol(token, &token, 10);
		while (*token != 0 && !_istdigit(*token))
			token ++;
		if (token[0] == 0)
		{
			value[i][1] = value[i][0];
		}
		else
		{
			value[i][1] = _tcstol(token, &token, 10);
		}
		token = token + _tcslen(token) + 1;
	}

	return nValue;
}

CString CreateRangeStringFromArray(int nExcludedRanges, const int aExcludedRanges[][2])
{
	CString rangestr = _T("");
	for (int i = 0; i < nExcludedRanges; ++i)
	{
		TCHAR value[256];
		if (aExcludedRanges[i][0] > 0 && aExcludedRanges[i][1] > 0)
		{
			if (aExcludedRanges[i][0] == aExcludedRanges[i][1])
			{
				wsprintf(value, _T("%d"), aExcludedRanges[i][0]);
				rangestr += value;
			}
			else
			{
				wsprintf(value, _T("%d-%d"), aExcludedRanges[i][0], aExcludedRanges[i][1]);
				rangestr += value;
			}
			rangestr += _T(",");
		}
	}
	if (!rangestr.IsEmpty())
		rangestr.Delete(rangestr.GetLength() - 1);

	return rangestr;
}

CString GetSeedString()
{
	TCHAR name[256+1];
	GetDllFilename(name, 256);
	CString rangestr;
	TCHAR * token = _tcspbrk(name, _T(",_"));
	if (!token)
		rangestr = RegReadString(KeyName(), _T("SecretSeed"), _T(""));
	else
		rangestr = token + 1;
	
	int seed = HexStringToInteger(rangestr.GetString());
	
	if (seed < 0) {
		rangestr = "0xFFFFFFFF";
	}
	return rangestr;
}


STDMETHODIMP CWinMergeScript::get_PluginEvent(BSTR *pVal)
{
	*pVal = SysAllocString(L"FILE_PACK_UNPACK");
	return S_OK;
}

STDMETHODIMP CWinMergeScript::get_PluginDescription(BSTR *pVal)
{
	*pVal = SysAllocString(L"Encrypt and Decrypt Database");
	return S_OK;
}

STDMETHODIMP CWinMergeScript::get_PluginFileFilters(BSTR *pVal)
{
	*pVal = SysAllocString(L"\\.txt;\\.cfg;\\.ini;\\.*$");
	//*pVal = SysAllocString(L"");
	return S_OK;
}

STDMETHODIMP CWinMergeScript::get_PluginIsAutomatic(VARIANT_BOOL *pVal)
{
	*pVal = VARIANT_TRUE;
	return S_OK;
}

STDMETHODIMP CWinMergeScript::get_PluginArguments(BSTR *pVal)
{
	*pVal = m_bstrArguments.Copy();
	return S_OK;
}

STDMETHODIMP CWinMergeScript::put_PluginArguments(BSTR val)
{
	m_bstrArguments = val;
	return S_OK;
}

STDMETHODIMP CWinMergeScript::UnpackBufferA(SAFEARRAY** pBuffer, INT* pSize, VARIANT_BOOL* pbChanged, INT* pSubcode, VARIANT_BOOL* pbSuccess)
{
	// We don't need it
	CString seedStr = GetSeedString();

	int seed = HexStringToInteger(seedStr.GetString());

	TRandomMersenne  pattern(seed);


	//wchar_t* pText = (wchar_t*)((byte*)(*pBuffer)->pvData + 2);
	//int txtCnt = ((*pSize) - 2) / 2;

	int txtCnt = (*pBuffer)->cbElements / 2;

	wchar_t HUGEP* pdFreq;
	HRESULT hr = SafeArrayAccessData(*pBuffer, (void HUGEP * FAR*) & pdFreq);
	if (SUCCEEDED(hr))
	{
		++pdFreq;
		
		for (int i = 1; i < txtCnt; ++i)
		{
			wchar_t ch = pdFreq[i];

			switch (ch) {
			case L'\r': case L'\n':
				break;
			default:
				pdFreq[i] = (ch & 0xfff0) + ((ch ^ pattern.IRandom(0, 255)) & 0x000f);
				break;
			}
			//if (i == 4096)break;
		}
		SafeArrayUnaccessData(*pBuffer);
	}
	*pSize = (*pBuffer)->cbElements;

	*pbChanged = VARIANT_TRUE;
	*pbSuccess = VARIANT_TRUE;
	return S_OK;
}

STDMETHODIMP CWinMergeScript::PackBufferA(SAFEARRAY** pBuffer, INT* pSize, VARIANT_BOOL* pbChanged, INT subcode, VARIANT_BOOL* pbSuccess)
{
	// We don't need it
	CString seedStr = GetSeedString();

	int seed = HexStringToInteger(seedStr.GetString());

	TRandomMersenne  pattern(seed);


	//wchar_t* pText = (wchar_t*)((byte*)(*pBuffer)->pvData + 2);
	//int txtCnt = ((*pSize) - 2) / 2;

	int txtCnt = (*pBuffer)->cbElements / 2;

	wchar_t HUGEP* pdFreq;
	HRESULT hr = SafeArrayAccessData(*pBuffer, (void HUGEP * FAR*) & pdFreq);
	if (SUCCEEDED(hr))
	{
		++pdFreq;

		for (int i = 1; i < txtCnt; ++i)
		{
			wchar_t ch = pdFreq[i];

			switch (ch) {
			case L'\r': case L'\n':
				break;
			default:
				pdFreq[i] = (ch & 0xfff0) + ((ch ^ pattern.IRandom(0, 255)) & 0x000f);
				break;
			}
			//if (i == 4096)break;
		}
		SafeArrayUnaccessData(*pBuffer);
	}
	*pSize = (*pBuffer)->cbElements;

	*pbChanged = VARIANT_TRUE;
	*pbSuccess = VARIANT_TRUE;
	return S_OK;
}

STDMETHODIMP CWinMergeScript::UnpackFile(BSTR fileSrc, BSTR fileDst, VARIANT_BOOL* pbChanged, INT* pSubcode, VARIANT_BOOL* pbSuccess)
{
	USES_CONVERSION;
	CString seedStr = GetSeedString();
	int seed = HexStringToInteger(seedStr.GetString());
	TRandomMersenne  pattern(seed);

	ifstream input(W2T(fileSrc), ios::in | ios::binary);
	ofstream output(W2T(fileDst), ios::out | ios::binary);

	input.seekg(0L, ios::end);
	int len = input.tellg();
	input.seekg(0L, ios::beg);

	char buffer[4096];
	// Check for Unicode BOM (byte order mark)
	// Only matter if file has 3 or more bytes
	// Files with <3 bytes are empty if they are UCS-2

	buffer[0] = 0xff;
	buffer[1] = 0xfe;

	output.write(buffer, 2);
	input.read(buffer, 2);

	len -= 2;

	while (len)
	{
		int curlen = len;
		if (curlen > 4096)
			curlen = 4096;
		// align on 4byte boundary, in case doing unicode encoding
		if (curlen > 2 && ((curlen % 2) != 0))
			curlen -= (curlen % 2);		

		input.read(buffer, curlen);
		int i = 0;

		wchar_t* pText = (wchar_t*)buffer;

		int wcharCnt = curlen / 2;

		for (int idx = 0; idx < wcharCnt; ++idx) {
			wchar_t ch = pText[idx];

			switch (ch) {
			case L'\r': case L'\n':
				break;
			default:
				pText[idx] = (ch & 0xfff0) + ((ch ^ pattern.IRandom(0, 255)) & 0x000f);
				break;
			}
		}		

		output.write(buffer, curlen);
		len -= curlen;
	}

	input.close();
	output.close();

	*pbChanged = VARIANT_TRUE;
	*pbSuccess = VARIANT_TRUE;
	return S_OK;
}

STDMETHODIMP CWinMergeScript::PackFile(BSTR fileSrc, BSTR fileDst, VARIANT_BOOL* pbChanged, INT pSubcode, VARIANT_BOOL* pbSuccess)
{
	USES_CONVERSION;
	CString seedStr = GetSeedString();
	int seed = HexStringToInteger(seedStr.GetString());
	TRandomMersenne  pattern(seed);

	ifstream input(W2T(fileSrc), ios::in | ios::binary);
	ofstream output(W2T(fileDst), ios::out | ios::binary);

	input.seekg(0L, ios::end);
	int len = input.tellg();
	input.seekg(0L, ios::beg);

	char buffer[4096];
	// Check for Unicode BOM (byte order mark)
	// Only matter if file has 3 or more bytes
	// Files with <3 bytes are empty if they are UCS-2

	buffer[0] = 0xff;
	buffer[1] = 0xfe;

	output.write(buffer, 2);
	input.read(buffer, 2);

	len -= 2;

	while (len)
	{
		int curlen = len;
		if (curlen > 4096)
			curlen = 4096;
		// align on 4byte boundary, in case doing unicode encoding
		if (curlen > 2 && ((curlen % 2) != 0))
			curlen -= (curlen % 2);

		input.read(buffer, curlen);
		int i = 0;

		wchar_t* pText = (wchar_t*)buffer;

		int wcharCnt = curlen / 2;

		for (int idx = 0; idx < wcharCnt; ++idx) {
			wchar_t ch = pText[idx];

			switch (ch) {
			case L'\r': case L'\n':
				break;
			default:
				pText[idx] = (ch & 0xfff0) + ((ch ^ pattern.IRandom(0, 255)) & 0x000f);
				break;
			}
		}

		output.write(buffer, curlen);
		len -= curlen;
	}

	input.close();
	output.close();

	*pbChanged = VARIANT_TRUE;
	*pbSuccess = VARIANT_TRUE;
	return S_OK;
}

#if 0
STDMETHODIMP CWinMergeScript::PrediffBufferW(BSTR *pText, INT *pSize, VARIANT_BOOL *pbChanged, VARIANT_BOOL *pbHandled)
{
	WCHAR * pBeginText = *pText;
	long nSize = *pSize;
	WCHAR * pEndText = pBeginText + nSize;

	int argc = 0;
	wchar_t **argv = CommandLineToArgvW(m_bstrArguments.m_str, &argc);
	CString rangestr = (m_bstrArguments.Length() > 0 && argc > 0) ? argv[0] : GetSeedString();
	if (argv)
		LocalFree(argv);

	int nExcludedRanges = CreateArrayFromRangeString(rangestr, NULL);
	int (* aExcludedRanges)[2] = new int[nExcludedRanges][2];
	if (aExcludedRanges == NULL)
		nExcludedRanges = 0;
	else
		nExcludedRanges = CreateArrayFromRangeString(rangestr, aExcludedRanges);

	if (nExcludedRanges == 0)
	{
		*pbChanged = VARIANT_FALSE;
		*pbHandled = VARIANT_TRUE;
		return S_OK;
	}

	// character position begins at 1 for user, but at 0 here
	int i;
	for (i = 0 ; i < nExcludedRanges ; i++)
	{
		aExcludedRanges[i][0] --;
		aExcludedRanges[i][1] --;
	}


	WCHAR * pDst = pBeginText;
	WCHAR * zoneBegin;
	WCHAR * lineBegin;

	for (zoneBegin = lineBegin = pBeginText; lineBegin < pEndText ; lineBegin = zoneBegin)
	{
		// next excluded range in the current line
		int nextExcludedRange;
		for (nextExcludedRange = 0 ; nextExcludedRange < nExcludedRanges ; nextExcludedRange ++)
		{
			// look for the end of the included zone
			WCHAR * zoneEnd = zoneBegin;
			WCHAR * zoneMaxEnd = lineBegin + aExcludedRanges[nextExcludedRange][0];
			while (zoneEnd < pEndText && zoneEnd < zoneMaxEnd &&
					*zoneEnd != L'\n' && *zoneEnd != L'\r')
				zoneEnd ++;

			// copy the characters of included columns
			wcsncpy(pDst, zoneBegin, zoneEnd - zoneBegin);
			pDst += zoneEnd - zoneBegin;

			// advance the cursor
			zoneBegin = zoneEnd;

			if (zoneEnd < zoneMaxEnd)
				break;

			// look for the end of the excluded zone
			zoneEnd = zoneBegin;
			zoneMaxEnd = lineBegin + aExcludedRanges[nextExcludedRange][1] + 1;
			while (zoneEnd < pEndText && zoneEnd < zoneMaxEnd &&
					*zoneEnd != L'\n' && *zoneEnd != L'\r')
				zoneEnd ++;

			// advance the cursor
			zoneBegin = zoneEnd;

			if (zoneEnd < zoneMaxEnd)
				break;
		}

		if (nextExcludedRange == nExcludedRanges)
		{
			// treat the trailing included zone

			// look for the end of the included zone
			WCHAR * zoneEnd = zoneBegin;
			while (zoneEnd < pEndText &&
					*zoneEnd != L'\n' && *zoneEnd != L'\r')
				zoneEnd ++;

			// copy the characters of included columns
			wcsncpy(pDst, zoneBegin, zoneEnd - zoneBegin);
			pDst += zoneEnd - zoneBegin;

			// advance the cursor
			zoneBegin = zoneEnd;
		}
		
		// keep possible EOL characters 
		WCHAR * eolEnd = zoneBegin;
		while (eolEnd < pEndText && 
			  (*eolEnd == L'\n' || *eolEnd == L'\r'))
		{
			eolEnd ++;
		}

		if (eolEnd > zoneBegin)
		{
			// copy the EOL characters
			wcsncpy(pDst, zoneBegin, eolEnd - zoneBegin);
			pDst += eolEnd - zoneBegin;
			// advance the cursor
			zoneBegin = eolEnd;
		}

	}


	delete [] aExcludedRanges;

	// set the new size
	*pSize = pDst - pBeginText;

	if (*pSize == nSize)
		*pbChanged = VARIANT_FALSE;
	else
		*pbChanged = VARIANT_TRUE;

	*pbHandled = VARIANT_TRUE;
	return S_OK;
}
#endif
INT_PTR CALLBACK DlgProc(HWND hWnd, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uiMsg) {
	case WM_INITDIALOG:
		SetDlgItemText(hWnd, IDC_EDIT1, GetSeedString());
		return TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK)
		{
			CRegKey	reg;
			if (reg.Create(HKEY_CURRENT_USER, KeyName(), REG_NONE, REG_OPTION_NON_VOLATILE, KEY_WRITE) != ERROR_SUCCESS)
				return FALSE;
			TCHAR value[512] = {0};
			GetDlgItemText(hWnd, IDC_EDIT1, value, sizeof(value)/sizeof(TCHAR));
			reg.SetStringValue(_T("SecretSeed"), value);
			EndDialog(hWnd, IDOK);
		}
		else if (LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hWnd, IDCANCEL);
		}
		return TRUE;
		break;

	default:
		break;
	}
	return FALSE;
}

STDMETHODIMP CWinMergeScript::ShowSettingsDialog(VARIANT_BOOL *pbHandled)
{
	*pbHandled = (DialogBox(_Module.GetModuleInstance(), MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgProc) == IDOK);
	return S_OK;
}
