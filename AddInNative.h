
#ifndef __ADDINNATIVE_H__
#define __ADDINNATIVE_H__

#ifndef __linux__
#include <wtypes.h>
#endif //__linux__

#include "ComponentBase.h"
#include "AddInDefBase.h"
#include "IMemoryManager.h"
#include <curl/curl.h>
#include <string>
#include <sstream>
#include <iostream>

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
        eLastProp      // Always last
    };

    enum Methods
    {
		eMethSendSMS,
		eMethGetDeliveryStatus,
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

    // Attributes
	IAddInDefBase      *m_iConnect;
	IMemoryManager     *m_iMemory;

	std::wstring url, login, password, sn;
	RequestStatus rs;
	int ApiVersion;

	bool GetDeliveryStatus(const wchar_t*, tVariant*);
	bool SendSMS(const wchar_t* number, const wchar_t* message, bool Resend, tVariant*);

	void ParseRequestStatus(char *);
	void ParseRequestStatusXML(char *);

	void ParseRequestCode(char *);
	void ParseRequestCodeVK(char *);
};
#endif //__ADDINNATIVE_H__
