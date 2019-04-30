
#ifndef __ADDINNATIVE_H__
#define __ADDINNATIVE_H__

#ifndef __linux__
#include <wtypes.h>
#endif //__linux__

constexpr size_t MAX_COUNT_THREAD = 50;

#include "ComponentBase.h"
#include "AddInDefBase.h"
#include "IMemoryManager.h"
#include <curl/curl.h>
#include <string>
#include <sstream>
#include <iostream>
#include <chrono>
#include <ctime>
#include <rapidxml/rapidxml.hpp>
#include <rapidjson/writer.h>
#include <rapidjson/reader.h>
#include <rapidjson/stringbuffer.h>
///////////////////////////////////////////////////////////////////////////////

struct RequestStatus {
	std::string code;
	std::string text;
	int status;
	int exstatus;
	std::string date;
	std::string messageType;
	std::string sourceText;
	std::string id;

	void clear(void)
	{
		code.clear();
		text.clear();
		date.clear();
		messageType.clear();
		sourceText.clear();
		id.clear();
		status = 0;
		exstatus = 0;
	}
};

struct MyHandler {
	const char* type;
	std::string data;
	
	MyHandler() : type(), data() {}

	bool Null() { type = "Null"; data.clear(); return true; }
	bool Bool(bool b) { type = "Bool:"; data = b ? "true" : "false"; return true; }
	bool Int(int i) { type = "Int:"; data = std::to_string(i); return true; }
	bool Uint(unsigned u) { type = "Uint:"; data = std::to_string(u); return true; }
	bool Int64(int64_t i) { type = "Int64:"; data = std::to_string(i); return true; }
	bool Uint64(uint64_t u) { type = "Uint64:"; data = std::to_string(u); return true; }
	bool Double(double d) { type = "Double:"; data = std::to_string(d); return true; }
	bool RawNumber(const char* str, size_t length, bool) { type = "Number:"; data = std::string(str, length); return true; }
	bool String(const char* str, size_t length, bool) { type = "String:"; data = std::string(str, length); return true; }
	bool StartObject() { type = "StartObject"; data.clear(); return true; }
	bool Key(const char* str, size_t length, bool) { type = "Key:"; data = std::string(str, length); return true; }
	bool EndObject(size_t memberCount) { type = "EndObject:"; data = std::to_string(memberCount); return true; }
	bool StartArray() { type = "StartArray"; data.clear(); return true; }
	bool EndArray(size_t elementCount) { type = "EndArray:"; data = std::to_string(elementCount); return true; }
};

struct MemoryStruct {
	char* memory;
	size_t size;
};

// class CAddInNative
class SMSAddIn : public IComponentBase
{
public:
    enum Props
    {
		ePropUrl,
		ePropLogin,
		ePropPassword,
		ePropReqCode,
		ePropReqText,
		ePropReqStatus,
		ePropReqDate,
		ePropReqMessageType,
		ePropReqSourceText,
		ePropApiVersion,
		ePropSN,
		ePropID,
		ePropReqExStatus,
		ePropOrderList,
		ePropApiKeys,
        eLastProp      // Always last
    };

    enum Methods
    {
		eMethSendSMS,
		eMethGetDeliveryStatus,
		eMethGetShortLink,
        eLastMethod      // Always last
    };

    SMSAddIn(void);
    virtual ~SMSAddIn();
    // IInitDoneBase
    virtual bool ADDIN_API Init(void*);
    virtual bool ADDIN_API setMemManager(void* mem);
    virtual long ADDIN_API GetInfo();
    virtual void ADDIN_API Done();
    // ILanguageExtenderBase
    virtual bool ADDIN_API RegisterExtensionAs(WCHAR_T** wsLanguageExt);
    virtual long ADDIN_API GetNProps();
    virtual long ADDIN_API FindProp(const WCHAR_T* wsPropName);
    virtual const WCHAR_T* ADDIN_API GetPropName(long lPropNum, long lPropAlias);
    virtual bool ADDIN_API GetPropVal(const long lPropNum, tVariant* pvarPropVal);
    virtual bool ADDIN_API SetPropVal(const long lPropNum, tVariant* varPropVal);
    virtual bool ADDIN_API IsPropReadable(const long lPropNum);
    virtual bool ADDIN_API IsPropWritable(const long lPropNum);
    virtual long ADDIN_API GetNMethods();
    virtual long ADDIN_API FindMethod(const WCHAR_T* wsMethodName);
    virtual const WCHAR_T* ADDIN_API GetMethodName(const long lMethodNum, 
                            const long lMethodAlias);
    virtual long ADDIN_API GetNParams(const long lMethodNum);
    virtual bool ADDIN_API GetParamDefValue(const long lMethodNum, const long lParamNum,
                            tVariant *pvarParamDefValue);
    virtual bool ADDIN_API HasRetVal(const long lMethodNum);
    virtual bool ADDIN_API CallAsProc(const long lMethodNum,
                    tVariant* paParams, const long lSizeArray);
    virtual bool ADDIN_API CallAsFunc(const long lMethodNum,
                tVariant* pvarRetValue, tVariant* paParams, const long lSizeArray);
    operator IComponentBase*() { return (IComponentBase*)this; }
    // LocaleBase
    virtual void ADDIN_API SetLocale(const WCHAR_T* loc);
private:
	long findName(const wchar_t* names[], const wchar_t* name, const uint32_t size) const;
	
	CURL *curl;
	CURL* mcurl[MAX_COUNT_THREAD];

    // Attributes
	IAddInDefBase      *m_iConnect;
	IMemoryManager     *m_iMemory;
	
	std::string mApiKeys[MAX_COUNT_THREAD];
	size_t countKey;

	std::wstring url, login, password, sn, orderList, jApiKeys;
	RequestStatus rs;
	int ApiVersion;
	size_t countReqest;
	std::chrono::time_point<std::chrono::system_clock> timeStart;

	bool GetDeliveryStatus(const wchar_t*, tVariant*);
	bool SendSMS(const wchar_t* number, const wchar_t* message, bool Resend, tVariant*);
	bool GetShortLink(const wchar_t *jLongUrls, tVariant*);

	void ParseRequestStatus(char *);
	void ParseRequestStatusXML(char *);

	void ParseRequestCode(char *);
	void ParseRequestCodeVK(char *);

	std::string ParseRequestShortLink(MemoryStruct);
	void ParseRequestMShortLink(const MemoryStruct*, std::string[], size_t);
	size_t ParseJsonLongLink(const char * jsonStr, std::string * mStr);

	std::string GetJsonShortLink(const char* url);

};
#endif //__ADDINNATIVE_H__
