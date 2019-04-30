
#include "stdafx.h"

#ifdef __linux__
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#endif

#include <wchar.h>
#include <clocale>
#include "AddInNative.h"

uint32_t convToShortWchar(WCHAR_T** Dest, const wchar_t* Source, uint32_t len = 0);
uint32_t convFromShortWchar(wchar_t** Dest, const WCHAR_T* Source, uint32_t len = 0);
uint32_t getLenShortWcharStr(const WCHAR_T* Source);
static size_t WriteMemoryCallback(void* contents, size_t size, size_t nmemb, void* userp);
char* convWcharToChar(size_t* len, const wchar_t* wstr);

static const wchar_t g_kClassNames[] = L"SMSAddIn";

static const wchar_t *g_MethodNamesRu[] = {
										L"ОтправитьСМС",
										L"ПолучитьСтатусДоставки",
										L"ПолучитьКороткуюСсылку"
};

static const wchar_t *g_MethodNames[] = {
										L"SendSMS",
										L"GetDeliveryStatus",
										L"GetShortLink"
};

static const wchar_t *g_PropNamesRu[] = {
	L"URL",
	L"Логин",
	L"Пароль",
	L"КодОтвета",
	L"ТекстОтвета",
	L"Статус",
	L"Дата",
	L"ТипСообщения",
	L"Ответ",
	L"ВерсияAPI",
	L"Подпись",
	L"ИД",
	L"ДополнительныйСтатус",
	L"ПорядокОтправки",
	L"КлючиAPI"
};

static const wchar_t *g_PropNames[] = {
	L"URL",
	L"Login",
	L"Password",
	L"ReqCode",
	L"ReqText",
	L"ReqStatus",
	L"ReqDate",
	L"ReqMessageType",
	L"ReqSourceText",
	L"ApiVersion",
	L"SN",
	L"ID",
	L"ExtendedStatus",
	L"OrderList",
	L"ApiKeys"
};

//---------------------------------------------------------------------------//
long GetClassObject(const wchar_t* wsName, IComponentBase** pInterface)
{
    if(!*pInterface)
    {
        *pInterface= new SMSAddIn();
        return (long)*pInterface;
    }
    return 0;
}
//---------------------------------------------------------------------------//
long DestroyObject(IComponentBase** pIntf)
{
   if(!*pIntf)
      return -1;

   delete *pIntf;
   *pIntf = 0;
   return 0;
}
//---------------------------------------------------------------------------//
const WCHAR_T* GetClassNames()
{
    static WCHAR_T* names = 0;
    if (!names)
        ::convToShortWchar(&names, g_kClassNames);
    return names;
}
//---------------------------------------------------------------------------//
//CAddInNative
SMSAddIn::SMSAddIn()
{
	m_iMemory = 0;
	m_iConnect = 0;
	url = L"";
	login = L"";
	password = L"";
	orderList = L"V,S";
	rs.clear();
	ApiVersion = 1;
	countKey = 0;

	countReqest = 0;
	timeStart = std::chrono::system_clock::now();

	curl_global_init(CURL_GLOBAL_ALL);
	curl = curl_easy_init();	

	for (size_t i = 0; i < MAX_COUNT_THREAD; ++i)
	{
		mcurl[i] = curl_easy_init();
	}
}
//---------------------------------------------------------------------------//
SMSAddIn::~SMSAddIn()
{
	if (curl) curl_easy_cleanup(curl);
	for (size_t i = 0; i < MAX_COUNT_THREAD; ++i)
	{
		if (mcurl[i]) curl_easy_cleanup(mcurl[i]);
	}
	curl_global_cleanup();
}
//---------------------------------------------------------------------------//
bool SMSAddIn::Init(void* pConnection)
{ 
	m_iConnect = (IAddInDefBase*)pConnection;
	return m_iConnect != NULL;
}
//---------------------------------------------------------------------------//
long SMSAddIn::GetInfo()
{ 
    return 2000; 
}
//---------------------------------------------------------------------------//
void SMSAddIn::Done()
{
}
//---------------------------------------------------------------------------//
bool SMSAddIn::RegisterExtensionAs(WCHAR_T** wsLanguageExt)
{ 
	wchar_t *wsExtension = L"SMSAddIn";
	int iActualSize = ::wcslen(wsExtension) + 1;
	WCHAR_T* dest = 0;

	if (m_iMemory)
	{
		if (m_iMemory->AllocMemory((void**)wsLanguageExt, iActualSize * sizeof(WCHAR_T)))
			::convToShortWchar(wsLanguageExt, wsExtension, iActualSize);
		return true;
	}

	return false;
}
//---------------------------------------------------------------------------//
long SMSAddIn::GetNProps()
{ 
    return eLastProp;
}
//---------------------------------------------------------------------------//
long SMSAddIn::FindProp(const WCHAR_T* wsPropName)
{ 
	long plPropNum = -1;
	wchar_t* propName = 0;

	::convFromShortWchar(&propName, wsPropName);
	plPropNum = findName(g_PropNames, propName, eLastProp);

	if (plPropNum == -1)
		plPropNum = findName(g_PropNamesRu, propName, eLastProp);

	delete[] propName;

	return plPropNum;
}
//---------------------------------------------------------------------------//
const WCHAR_T* SMSAddIn::GetPropName(long lPropNum, long lPropAlias)
{ 
	if (lPropNum >= eLastProp)
		return NULL;

	wchar_t *wsCurrentName = NULL;
	WCHAR_T *wsPropName = NULL;
	int iActualSize = 0;

	switch (lPropAlias)
	{
	case 0: // First language
		wsCurrentName = (wchar_t*)g_PropNames[lPropNum];
		break;
	case 1: // Second language
		wsCurrentName = (wchar_t*)g_PropNamesRu[lPropNum];
		break;
	default:
		return 0;
	}

	iActualSize = wcslen(wsCurrentName) + 1;

	if (m_iMemory && wsCurrentName)
	{
		if (m_iMemory->AllocMemory((void**)&wsPropName, iActualSize * sizeof(WCHAR_T)))
			::convToShortWchar(&wsPropName, wsCurrentName, iActualSize);
	}

	return wsPropName;
}
//---------------------------------------------------------------------------//
bool SMSAddIn::GetPropVal(const long lPropNum, tVariant* pvarPropVal)
{ 
	std::wstring *wStr = nullptr;
	std::string *Str = nullptr;
	int num = -1;

	switch (lPropNum)
	{
	case ePropUrl:
		wStr = &url;
		break;
	case ePropID:
		Str = &rs.id;
		break;
	case ePropSN:
		wStr = &sn;
		break;
	case ePropLogin:
		wStr = &login;
		break;
	case ePropPassword:
		wStr = &password;
		break;
	case ePropOrderList:
		wStr = &orderList;
		break;
	case ePropApiKeys:
		wStr = &jApiKeys;
		break;
	case ePropReqDate:
		Str = &rs.date;
		break;
	case ePropReqMessageType:
		Str = &rs.messageType;
		break;
	case ePropReqSourceText:
		Str = &rs.sourceText;
		break;
	case ePropReqStatus:
		num = rs.status;
		break;
	case ePropReqExStatus:
		num = rs.exstatus;
		break;
	case ePropReqText:
		Str = &rs.text;
		break;
	case ePropReqCode:
		Str = &rs.code;
		break;
	case ePropApiVersion:
		num = ApiVersion;
		break;
	default:
		return false;
	}

	if (wStr != nullptr)
	{
		TV_VT(pvarPropVal) = VTYPE_PWSTR;
		m_iMemory->AllocMemory((void**)&pvarPropVal->pwstrVal, wStr->length() * sizeof(wchar_t));
		memcpy((void*)pvarPropVal->pwstrVal, (void*)wStr->c_str(), wStr->length() * sizeof(wchar_t));
		pvarPropVal[0].wstrLen = wStr->length();
	}
	else if (Str != nullptr)
	{
		TV_VT(pvarPropVal) = VTYPE_PSTR;
		m_iMemory->AllocMemory((void**)&pvarPropVal->pstrVal, Str->length() * sizeof(char));
		memcpy((void*)pvarPropVal->pstrVal, (void*)Str->c_str(), Str->length() * sizeof(char));
		pvarPropVal[0].strLen = Str->length();
	}
	else if (num != -1)
	{
		TV_VT(pvarPropVal) = VTYPE_I4;
		TV_I4(pvarPropVal) = num;
	}

	return true;
}
//---------------------------------------------------------------------------//
bool SMSAddIn::SetPropVal(const long lPropNum, tVariant* varPropVal)
{ 
	switch (lPropNum)
	{
	case ePropUrl:
		if (TV_VT(varPropVal) != VTYPE_PWSTR)
			return false;		
		url = TV_WSTR(varPropVal);
		if (url.back() == L'/') url.pop_back();
		break;
	case ePropLogin:
		if (TV_VT(varPropVal) != VTYPE_PWSTR)
			return false;
		login = TV_WSTR(varPropVal);
		break;
	case ePropPassword:
		if (TV_VT(varPropVal) != VTYPE_PWSTR)
			return false;
		password = TV_WSTR(varPropVal);
		break;
	case ePropOrderList:
		if (TV_VT(varPropVal) != VTYPE_PWSTR)
			return false;
		orderList = TV_WSTR(varPropVal);
		break;
	case ePropSN:
		if (TV_VT(varPropVal) != VTYPE_PWSTR)
			return false;
		sn = TV_WSTR(varPropVal);
		break;
	case ePropApiKeys:
	{
		if (TV_VT(varPropVal) != VTYPE_PWSTR)
			return false;
		jApiKeys = TV_WSTR(varPropVal);

		size_t lenApiKeys = 0;
		char* strApiKeys = convWcharToChar(&lenApiKeys, jApiKeys.c_str());
		if (!strApiKeys) return false;
		countKey = ParseJsonLongLink(strApiKeys, mApiKeys);
		free(strApiKeys);
		break; 
	}
	case ePropApiVersion:
		if (TV_VT(varPropVal) != VTYPE_I4)
			return false;
		ApiVersion = TV_INT(varPropVal);
		if (ApiVersion <= 0 || ApiVersion > 2) ApiVersion = 1;
		break;
	default:
		return false;
	}

	return true;
}
//---------------------------------------------------------------------------//
bool SMSAddIn::IsPropReadable(const long lPropNum)
{ 
	//switch (lPropNum)
	//{
	//	return false;
	//default:
	//	return true;
	//}

	return true;
}
//---------------------------------------------------------------------------//
bool SMSAddIn::IsPropWritable(const long lPropNum)
{
	switch (lPropNum)
	{
	case ePropUrl:
	case ePropLogin:
	case ePropPassword:
	case ePropApiVersion:
	case ePropSN:
	case ePropOrderList:
	case ePropApiKeys:
		return true;
	default:
		return false;
	}

	return false;
}
//---------------------------------------------------------------------------//
long SMSAddIn::GetNMethods()
{ 
    return eLastMethod;
}
//---------------------------------------------------------------------------//
long SMSAddIn::FindMethod(const WCHAR_T* wsMethodName)
{ 
	long plMethodNum = -1;
	wchar_t* name = 0;

	::convFromShortWchar(&name, wsMethodName);

	plMethodNum = findName(g_MethodNames, name, eLastMethod);

	if (plMethodNum == -1)
		plMethodNum = findName(g_MethodNamesRu, name, eLastMethod);

	return plMethodNum;
}
//---------------------------------------------------------------------------//
const WCHAR_T* SMSAddIn::GetMethodName(const long lMethodNum, 
                            const long lMethodAlias)
{ 
	if (lMethodNum >= eLastMethod)
		return NULL;

	wchar_t *wsCurrentName = NULL;
	WCHAR_T *wsMethodName = NULL;
	int iActualSize = 0;

	switch (lMethodAlias)
	{
	case 0: // First language
		wsCurrentName = (wchar_t*)g_MethodNames[lMethodNum];
		break;
	case 1: // Second language
		wsCurrentName = (wchar_t*)g_MethodNamesRu[lMethodNum];
		break;
	default:
		return 0;
	}

	iActualSize = wcslen(wsCurrentName) + 1;

	if (m_iMemory && wsCurrentName)
	{
		if (m_iMemory->AllocMemory((void**)&wsMethodName, iActualSize * sizeof(WCHAR_T)))
			::convToShortWchar(&wsMethodName, wsCurrentName, iActualSize);
	}

	return wsMethodName;
}
//---------------------------------------------------------------------------//
long SMSAddIn::GetNParams(const long lMethodNum)
{ 
	int rez = 0;

	switch (lMethodNum)
	{
	case eMethGetDeliveryStatus:
		rez = 1;
		break;
	case eMethGetShortLink:
		rez = 1;
		break;
	case eMethSendSMS:
		rez = 3;
		break;
	default:
		break;
	}

	return rez;
}
//---------------------------------------------------------------------------//
bool SMSAddIn::GetParamDefValue(const long lMethodNum, const long lParamNum,
                          tVariant *pvarParamDefValue)
{ 
    return false;
} 
//---------------------------------------------------------------------------//
bool SMSAddIn::HasRetVal(const long lMethodNum)
{ 
	switch (lMethodNum)
	{
	case eMethGetDeliveryStatus:
	case eMethSendSMS:
	case eMethGetShortLink:
		return true;
		break;
	default:
		return false;
	}

    return false;
}
//---------------------------------------------------------------------------//
bool SMSAddIn::CallAsProc(const long lMethodNum,
                    tVariant* paParams, const long lSizeArray)
{ 
	bool rez = false;
	//switch (lMethodNum)
	//{

	//default:
	//	rez = false;
	//	break;
	//}
	return rez;
}
//---------------------------------------------------------------------------//
bool SMSAddIn::CallAsFunc(const long lMethodNum,
                tVariant* pvarRetValue, tVariant* paParams, const long lSizeArray)
{
	bool rez = false;

	switch (lMethodNum)
	{
	case eMethGetDeliveryStatus:
		if (TV_VT(paParams) == VTYPE_PWSTR && paParams[0].wstrLen > 0)
			rez = GetDeliveryStatus(TV_WSTR(paParams), pvarRetValue);
		break;
	case eMethGetShortLink:
		if (lSizeArray == 1
			&& TV_VT(&paParams[0]) == VTYPE_PWSTR && paParams[0].wstrLen > 0)
			rez = GetShortLink(TV_WSTR(&paParams[0]), pvarRetValue);
		break;
	case eMethSendSMS:
		if (lSizeArray == 3
			&& TV_VT(&paParams[0]) == VTYPE_PWSTR && paParams[0].wstrLen > 0
			&& TV_VT(&paParams[1]) == VTYPE_PWSTR && paParams[1].wstrLen > 0
			&& TV_VT(&paParams[2]) == VTYPE_BOOL)
		{
			rez = SendSMS(TV_WSTR(&paParams[0]), TV_WSTR(&paParams[1]), TV_BOOL(&paParams[2]), pvarRetValue);
		}
		break;
	default:
		break;
	}

	return rez;
}
//---------------------------------------------------------------------------//
void SMSAddIn::SetLocale(const WCHAR_T* loc)
{
#ifndef __linux__
    _wsetlocale(LC_ALL, loc);
#else
    int size = 0;
    char *mbstr = 0;
    wchar_t *tmpLoc = 0;
    convFromShortWchar(&tmpLoc, loc);
    size = wcstombs(0, tmpLoc, 0)+1;
    mbstr = new char[size];

    if (!mbstr)
    {
        delete[] tmpLoc;
        return;
    }

    memset(mbstr, 0, size);
    size = wcstombs(mbstr, tmpLoc, wcslen(tmpLoc));
    setlocale(LC_ALL, mbstr);
    delete[] tmpLoc;
    delete[] mbstr;
#endif
}
//---------------------------------------------------------------------------//
bool SMSAddIn::setMemManager(void* mem)
{
	m_iMemory = (IMemoryManager*)mem;
	return m_iMemory != 0;
}
//---------------------------------------------------------------------------//
uint32_t convToShortWchar(WCHAR_T** Dest, const wchar_t* Source, uint32_t len)
{
    if (!len)
        len = ::wcslen(Source)+1;

    if (!*Dest)
        *Dest = new WCHAR_T[len];

    WCHAR_T* tmpShort = *Dest;
    wchar_t* tmpWChar = (wchar_t*) Source;
    uint32_t res = 0;

    ::memset(*Dest, 0, len*sizeof(WCHAR_T));
    do
    {
        *tmpShort++ = (WCHAR_T)*tmpWChar++;
        ++res;
    }
    while (len-- && *tmpWChar);

    return res;
}
//---------------------------------------------------------------------------//
uint32_t convFromShortWchar(wchar_t** Dest, const WCHAR_T* Source, uint32_t len)
{
    if (!len)
        len = getLenShortWcharStr(Source)+1;

    if (!*Dest)
        *Dest = new wchar_t[len];

    wchar_t* tmpWChar = *Dest;
    WCHAR_T* tmpShort = (WCHAR_T*)Source;
    uint32_t res = 0;

    ::memset(*Dest, 0, len*sizeof(wchar_t));
    do
    {
        *tmpWChar++ = (wchar_t)*tmpShort++;
        ++res;
    }
    while (len-- && *tmpShort);

    return res;
}
//---------------------------------------------------------------------------//
uint32_t getLenShortWcharStr(const WCHAR_T* Source)
{
    uint32_t res = 0;
    WCHAR_T *tmpShort = (WCHAR_T*)Source;

    while (*tmpShort++)
        ++res;

    return res;
}
//---------------------------------------------------------------------------//

bool SMSAddIn::SendSMS(const wchar_t *number, const wchar_t *message, bool Resend, tVariant* rez)
{
	rs.clear();

	if (url.empty() || sn.empty() || !curl || ((login.empty() || password.empty() || orderList.empty()) && ApiVersion == 2))
	{
		rs.code = "901";
		rs.sourceText = "Не установлены параметры";
		return true;
	}

	CURLcode res;
	struct MemoryStruct chunk;
	chunk.memory = (char*)malloc(1);
	chunk.memory[0] = 0;
	chunk.size = 0;

	std::wstring wstr = L"";

	switch (ApiVersion)
	{
	case 1: //Отправка вайбер и СМС
		wstr.append(L"type=text&sn=").append(sn).
			append(L"&msisdn=").append(number).
			append(L"&resend_by_sms=").append(Resend ? L"Y" : L"N").
			append(L"&message=").append(message);
		break;
	case 2: //Отправка вконтакте, вайбер и СМС
		wstr.append(L"order_list=").append(orderList).
			append(L"&serviceid=").append(login).
			append(L"&pass=").append(password).
			append(L"&message=").append(message).
			append(L"&clientId=").append(number).
			append(L"&v_resendCond=").append(Resend ? L"S" : L"N").
			append(L"&i_resendCond=Y&v_resendValid=000000020000000R&i_resendValid=000000020000000R").
			append(L"&sn=").append(sn);
		break;
	default:
		free(chunk.memory);
		return false;
		break;
	}

	std::setlocale(LC_ALL, "ru_RU.utf8");

	size_t len = 0, lenUrl = 0;
	char* str = convWcharToChar(&len, wstr.c_str());
	char* strUrl = convWcharToChar(&lenUrl, url.c_str());

	curl_easy_setopt(curl, CURLOPT_URL, strUrl);

	if ((int)url.find(L"https") == 0)
	{
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
	}

	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, str);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, len);

	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

	res = curl_easy_perform(curl);
	if (res != CURLE_OK)
	{
		rs.sourceText = curl_easy_strerror(res);
		rs.text = rs.sourceText;
	}

	if (chunk.size > 0)
	switch (ApiVersion)
	{
	case 1:
		ParseRequestCode(chunk.memory);
		break;
	case 2:
		ParseRequestCodeVK(chunk.memory);
		break;
	default:
		break;
	}
	else
	{
		rs.code = "902";
	}

	if (!rs.code.empty())
	{
		TV_VT(rez) = VTYPE_PSTR;
		m_iMemory->AllocMemory((void**)&rez->pstrVal, rs.code.length() * sizeof(char));
		memcpy((void*)rez->pstrVal, (void*)rs.code.c_str(), rs.code.length() * sizeof(char));
		rez->strLen = rs.code.length();
	}

	free(chunk.memory);
	free(str);
	free(strUrl);

	return true;
}

bool SMSAddIn::GetShortLink(const wchar_t *jLongUrls, tVariant *rez)
{
	rs.clear();

	if (url.empty())
	{
		rs.code = "901";
		rs.sourceText = "Не установлены параметры";
		return true;
	}

	std::setlocale(LC_ALL, "ru_RU.utf8");

	size_t len = 0 , lenUrl = 0;
	char* str = convWcharToChar(&len, jLongUrls);
	if (!str) return false;

	char* strUrl = convWcharToChar(&lenUrl, url.c_str());
	if (!strUrl) return false;

	char** mStrUrl = (char**) malloc(countKey == 0 ? 1 : countKey * sizeof(mStrUrl));
	if (!mStrUrl) return false;

	for (size_t s = 0; s < countKey == 0 ? 0 : countKey - 1; ++s)
	{
		std::string t = "";
		t.append(strUrl);
		if (countKey > 0)
		{
			t.append("?key=").append(mApiKeys[s]);
		}

		mStrUrl[s] = (char*)malloc(t.length() * sizeof(mStrUrl[s]));
		if (!mStrUrl[s]) return false;
		strcpy_s(mStrUrl[s], t.length() + 1, t.c_str());

	}

	long response_code = 200;

	struct curl_slist* headers = NULL;
	headers = curl_slist_append(headers, "Accept: application/json");
	headers = curl_slist_append(headers, "Content-Type: application/json");
	headers = curl_slist_append(headers, "charsets: utf-8");
	
	std::string mstr[MAX_COUNT_THREAD];
	size_t countOfLinks = ParseJsonLongLink(str, mstr);

	countReqest += countOfLinks;

	if (countReqest >= MAX_COUNT_THREAD)
	{
		std::chrono::duration<double> elapsed_seconds = std::chrono::system_clock::now() - timeStart;
		int ms = (int)(1000 - elapsed_seconds.count() * 1000);
		if (ms > 0 && ms <= 1000)
			Sleep(ms);

		countReqest = 0;
	}

	timeStart = std::chrono::system_clock::now();

	if (countOfLinks == 0)
	{
		rs.code = "0";
		rs.sourceText = "Error read params";

		for (size_t s = 0; s < countKey == 0 ? 0 : countKey - 1; ++s)
			free(mStrUrl[s]);
		free(mStrUrl);

		free(str);
		free(strUrl);
		return true;
	}

	MemoryStruct* mchunk = (MemoryStruct*)malloc(countOfLinks * sizeof (* mchunk));
	
	for (size_t i = 0; i < countOfLinks; ++i)
	{
		mchunk[i].memory = (char*)malloc(1);
		mchunk[i].size = 0;
	}

	int ct = 0;
	for (size_t i = 0; i < countOfLinks; ++i)
	{

		mstr[i] = GetJsonShortLink(mstr[i].c_str());

		if (ct == 0) ct = countKey == 0 ? 1 : countKey;
		curl_easy_setopt(mcurl[i], CURLOPT_URL, mStrUrl[--ct]);
		
		//curl_easy_setopt(mcurl[i], CURLOPT_URL, strUrl);

		curl_easy_setopt(mcurl[i], CURLOPT_HTTPHEADER, headers);

		if ((int)url.find(L"https") == 0)
		{
			curl_easy_setopt(mcurl[i], CURLOPT_SSL_VERIFYPEER, 0L);
			curl_easy_setopt(mcurl[i], CURLOPT_SSL_VERIFYHOST, 0L);
		}

		curl_easy_setopt(mcurl[i], CURLOPT_POSTFIELDS, mstr[i].c_str());
		curl_easy_setopt(mcurl[i], CURLOPT_POSTFIELDSIZE, mstr[i].length());

		curl_easy_setopt(mcurl[i], CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
		curl_easy_setopt(mcurl[i], CURLOPT_WRITEDATA, (void*)& mchunk[i]);
	}

	CURLM* multi_handle = curl_multi_init();

	for (size_t i = 0; i < countOfLinks; ++i)
	{
		curl_multi_add_handle(multi_handle, mcurl[i]);
	}

	int still_running = 0;

	curl_multi_perform(multi_handle, &still_running);

	while (still_running)
	{
		Sleep(100);
		curl_multi_perform(multi_handle, &still_running);
	}

	int msgq = 0;
	CURLcode res;
	std::string mError[MAX_COUNT_THREAD];

	for (size_t i = 0; i < countOfLinks; ++i)
	{
		res = curl_easy_getinfo(mcurl[i], CURLINFO_HTTP_CODE, &msgq);
		if (res != CURLE_OK)
		{
			mError[i] = curl_easy_strerror(res);
		}
		else if (msgq != 200)
		{	
			mError[i] = std::to_string(msgq);
			//if (mchunk[i].size > 0)
			//	mError[i].append(" (").append(mchunk[i].memory).append(")");
		}
	}

	curl_multi_cleanup(multi_handle);

	ParseRequestMShortLink(mchunk, mError, countOfLinks);

	TV_VT(rez) = VTYPE_INT;
	TV_INT(rez) = response_code;

	for (size_t i = 0; i < countOfLinks; ++i)
	{
		free(mchunk[i].memory);
	}

	free(mchunk);

	for (size_t s = 0; s < countKey == 0 ? 0 : countKey - 1; ++s)
		free(mStrUrl[s]);

	free(mStrUrl);
	free(str);
	free(strUrl);

	return true;
}

long SMSAddIn::findName(const wchar_t* names[], const wchar_t* name,
	const uint32_t size) const
{
	long ret = -1;
	for (uint32_t i = 0; i < size; i++)
	{
		if (!wcscmp(names[i], name))
		{
			ret = i;
			break;
		}
	}
	return ret;
}

bool SMSAddIn::GetDeliveryStatus(const wchar_t *message, tVariant* rez)
{
	rs.clear();

	if (url.empty() || !curl || ((login.empty() || password.empty()) && ApiVersion == 2))
	{		
		rs.code = "901";
		rs.sourceText = "Не установлены параметры";
		return true;
	}

	CURLcode res;
	struct MemoryStruct chunk;
	chunk.memory = (char*)malloc(1);
	chunk.size = 0;

	std::wstring wstr = L"";

	switch (ApiVersion)
	{
	case 1: //Проверка статуса вайбер и СМС
		wstr.append(L"show_date=Y&mt_num=").append(message);
		break;
	case 2: //Проверка статуса вконтакте, вайбер и СМС
		wstr.append(L"serviceid=").append(login).append(L"&pass=").append(password).append(L"&id=").append(message);
		break;
	default:
		free(chunk.memory);
		return false;
		break;
	}

	std::setlocale(LC_ALL, "ru_RU.utf8");

	size_t len = 0, lenUrl = 0;
	char* str = convWcharToChar(&len, wstr.c_str());
	char* strUrl = convWcharToChar(&lenUrl, url.c_str());

	curl_easy_setopt(curl, CURLOPT_URL, strUrl);

	if ((int)url.find(L"https") == 0)
	{
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
	}

	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, str);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, len);

	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

	res = curl_easy_perform(curl);
	if (res != CURLE_OK)
		rs.sourceText = curl_easy_strerror(res);

	if (chunk.size > 0)
	switch (ApiVersion)
	{
	case 1:
		ParseRequestStatus(chunk.memory);
		break;
	case 2:
		ParseRequestStatusXML(chunk.memory);
		break;
	default:
		break;
	}
	else
	{
		rs.code = "902";
	}

	if (!rs.code.empty())
	{
		TV_VT(rez) = VTYPE_PSTR;
		m_iMemory->AllocMemory((void**)&rez->pstrVal, rs.code.length() * sizeof(char));
		memcpy((void*)rez->pstrVal, (void*)rs.code.c_str(), rs.code.length() * sizeof(char));
		rez->strLen = rs.code.length();
	}

	free(chunk.memory);
	free(str);
	free(strUrl);

	return true;
}

void SMSAddIn::ParseRequestStatus(char *message)
{
	rs.clear();
	rs.sourceText = message;
	rs.code = "100";
	if (strlen(message) <= 20)
	{
		char *str = strstr(message, " ");
		if (str) {
			rs.date = str;
			str[0] = 0;
			rs.status = atoi(message);
			rs.code = "200";
		}
		else
		{
			rs.code = "100";
		}
	}
}

void SMSAddIn::ParseRequestStatusXML(char *message)
{
	rs.clear();
	rs.sourceText = message;

	try {
		rapidxml::xml_document<> xml;
		xml.parse<0>(message);
		
		rapidxml::xml_node<> *node = xml.first_node("response", 0, false);
		if (!node) throw 1;

		rapidxml::xml_node<> *attr = node->first_node("code", 0, false);
		if (!attr) throw 1;
		
		rs.code = attr->value();

		attr = node->first_node("text", 0, false);
		if (!attr) throw 1;
		
		rs.text = attr->value();

		if (rs.code.find("20") == 0)
		{
			node = node->first_node("payload", 0, false);
			if (!node) throw 1;

			attr = node->first_node("status", 0, false);
			if (!attr) throw 1;
			rs.status = atoi(attr->value());

			attr = node->first_node("extended-status", 0, false);
			if (!attr) throw 1;
			rs.exstatus = atoi(attr->value());

			attr = node->first_node("date", 0, false);
			if (!attr) throw 1;
			rs.date = attr->value();
			
			attr = node->first_node("message-type", 0, false);
			if (!attr) throw 1;
			rs.messageType = attr->value();
		}
	}
	catch (int)
	{
		rs.code = "905";
	}
	catch (rapidxml::parse_error parseErr)
	{
		rs.code = "906";
	}
}

void SMSAddIn::ParseRequestCode(char * message)
{
	rs.clear();
	rs.sourceText = message;
	
	if (rs.sourceText.length() <= 13 && isdigit(message[0]))
	{
		for (int a = 0; message[a]; ++a)
			if (!isdigit(message[a]))
			{
				message[a] = 0;
				break;
			}
		rs.id = message;
		rs.code = "200";
	}
	else
	{
		rs.text = message;
		rs.code = "904";
	}
}

void SMSAddIn::ParseRequestCodeVK(char * message)
{
	rs.clear();
	rs.sourceText = message;

	const int maxLen = 256;
	char line1[maxLen], line2[maxLen];
	line1[0] = 0;
	line2[0] = 0;

	std::istringstream stream(rs.sourceText);
	stream.setf(std::ios_base::skipws);
	stream.getline(line1, maxLen);
	
	while (!stream.eof() && strlen(line2) < 2)
	{
		stream.getline(line2, maxLen);
	}

	if (strlen(line1) > 0)
	{
		rs.text = line1;
		if (rs.text.back() == '\r')
			rs.text.pop_back();
	}

	if (strlen(line2) > 0)
	{
		rs.id = line2;
		rs.code = "200";
	}
	else
	{
		rs.code = "904";
	}

}

std::string SMSAddIn::ParseRequestShortLink(MemoryStruct chunk)
{
	std::string str = "";

	if (chunk.size == 0) return str;

	MyHandler handler;
	rapidjson::Reader reader;
	rapidjson::StringStream ss(chunk.memory);
	reader.IterativeParseInit();

	bool flag = false;
	while (!reader.IterativeParseComplete()) {
		reader.IterativeParseNext<rapidjson::kParseDefaultFlags>(ss, handler);
		
		if (reader.HasParseError()) return str;

		if (flag)
		{
			if (handler.data.length() > 0)
				str = handler.data;
			break;
		}
		if (strcmp(handler.type, "Key:") == 0 && handler.data.compare("shortLink") == 0) flag = true;
	}

	return str;
}

void SMSAddIn::ParseRequestMShortLink(const MemoryStruct * mchunk, std::string mError[], size_t countOfLinks)
{
	rs.clear();

	rapidjson::StringBuffer s;
	rapidjson::Writer<rapidjson::StringBuffer> json(s);

	json.StartArray();

	for (size_t i = 0; i < countOfLinks; ++i)
	{
		std::string jsonStr = ParseRequestShortLink(mchunk[i]).c_str();
		if (mError[i].empty() && !jsonStr.empty())
			json.String(jsonStr.c_str());
		else
		{
			json.StartObject();
			json.Key("Error");
			json.String(!jsonStr.empty() ? "Has parse error" : mError[i].c_str());
			json.EndObject();
		}
	}

	json.EndArray();

	rs.text = s.GetString();
}

size_t SMSAddIn::ParseJsonLongLink(const char * jsonStr, std::string * mstr)
{
	size_t countOfLinks = 0;

	MyHandler handler;
	rapidjson::Reader reader;
	rapidjson::StringStream ss(jsonStr);
	reader.IterativeParseInit();

	while (countOfLinks < MAX_COUNT_THREAD && !reader.IterativeParseComplete())
	{
		reader.IterativeParseNext<rapidjson::kParseDefaultFlags>(ss, handler);
		if (strcmp(handler.type, "String:") == 0)
		{
			if (handler.data.length() > 0)
				mstr[countOfLinks++] = handler.data;
		}
	}

	return countOfLinks;
}

std::string SMSAddIn::GetJsonShortLink(const char* url)
{
	rapidjson::StringBuffer s;
	rapidjson::Writer<rapidjson::StringBuffer> json(s);

	json.StartObject();

	json.Key("longDynamicLink");
	json.String(url);

	json.Key("suffix");
	json.StartObject();
	json.Key("option");
	json.String("SHORT");
	json.EndObject();

	json.EndObject();
	
	return (std::string)s.GetString();
}

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	struct MemoryStruct *mem = (struct MemoryStruct *)userp;

	char *ptr = (char *)realloc(mem->memory, mem->size + realsize + 1);
	if (!ptr) {
		/* out of memory! */
		fprintf(stderr, "not enough memory (realloc returned NULL)\n");
		return 0;
	}

	mem->memory = ptr;
	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;

	return realsize;
}

char* convWcharToChar(size_t *len, const wchar_t* wstr)
{
	wcstombs_s(len, NULL, 0, wstr, 0);
	if (len[0] <= 0) return false;
	char* str = (char*)malloc(len[0] + 1);
	wcstombs_s(NULL, str, len[0] + 1, wstr, len[0] + 1);

	return str;
}
