#include "TimeVerificationOnline.h"
#include <vector>
#include <cstring>
namespace OpenSSLWrapper
{
#include <openssl/des.h>
#include <openssl/evp.h>
}

#if defined(_WIN32)

#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/AllowWindowsPlatformAtomics.h"
#	include <comdef.h>
#	include <Wbemidl.h>
#	include <wincrypt.h>
#	include <string>
#	pragma comment(lib, "wbemuuid.lib")
#	pragma comment(lib, "advapi32.lib")
#	pragma comment(lib, "crypt32.lib")
#include "Windows/HideWindowsPlatformAtomics.h"
#include "Windows/HideWindowsPlatformTypes.h"

#endif

using std::string;
using std::wstring;

#if defined(_WIN32)


static BYTE HexCharToNibble(char c)
{
	if (c >= '0' && c <= '9') return (BYTE)(c - '0');
	if (c >= 'a' && c <= 'f') return (BYTE)(c - 'a' + 10);
	if (c >= 'A' && c <= 'F') return (BYTE)(c - 'A' + 10);
	return 0;
}

// 简单的宽字符转 UTF-8
static string WideToUtf8(const wstring& wstr)
{
	if (wstr.empty())
	{
		return string();
	}
	int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
	if (sizeNeeded <= 0)
	{
		return string();
	}
	string result(sizeNeeded, '\0');
	WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), result.data(), sizeNeeded, nullptr, nullptr);
	return result;
}

static string BstrToUtf8(BSTR bstr)
{
	if (!bstr)
	{
		return string();
	}
	return WideToUtf8(wstring(bstr, SysStringLen(bstr)));
}

// 查询单个 WMI 类的一个属性，并追加到结果字符串中
static void AppendWmiProperty(
	IWbemServices* WmiServices,
	const wchar_t* WqlQuery,
	const wchar_t* PropertyName,
	const char* Label,
	string& Out)
{
	if (!WmiServices)
	{
		return;
	}

	IEnumWbemClassObject* Enumerator = nullptr;
	HRESULT Hr = WmiServices->ExecQuery(
		bstr_t(L"WQL"),
		bstr_t(WqlQuery),
		WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
		nullptr,
		&Enumerator);

	if (FAILED(Hr) || !Enumerator)
	{
		return;
	}

	IWbemClassObject* Obj = nullptr;
	ULONG Returned = 0;

	// 只取第一个结果即可
	Hr = Enumerator->Next(WBEM_INFINITE, 1, &Obj, &Returned);
	if (SUCCEEDED(Hr) && Returned)
	{
		VARIANT vtProp;
		VariantInit(&vtProp);

		Hr = Obj->Get(PropertyName, 0, &vtProp, 0, 0);
		if (SUCCEEDED(Hr))
		{
			string ValueStr;

			if (vtProp.vt == VT_BSTR)
			{
				ValueStr = BstrToUtf8(vtProp.bstrVal);
			}
			else
			{
				// 其他类型转成字符串
				_bstr_t Tmp(vtProp);
				ValueStr = BstrToUtf8(Tmp);
			}

			if (!ValueStr.empty())
			{
				Out.append(Label);
				Out.append("=");
				Out.append(ValueStr);
				Out.append(";");
			}
		}

		VariantClear(&vtProp);
	}

	if (Obj)
	{
		Obj->Release();
	}
	if (Enumerator)
	{
		Enumerator->Release();
	}
}

#endif // _WIN32

string TimeVerificationOnline::GetDeviceUniqueIDByWMI()
{
#if !defined(_WIN32)
	return string("UnsupportedPlatform");
#else
	HRESULT Hr;

	// 初始化 COM
	Hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	bool bComInitialized = SUCCEEDED(Hr);

	// 有些情况下已经初始化过，忽略返回值
	if (FAILED(Hr) && Hr != RPC_E_CHANGED_MODE)
	{
		return string("COMInitFailed");
	}

	// 设置通用安全性
	Hr = CoInitializeSecurity(
		nullptr,
		-1,
		nullptr,
		nullptr,
		RPC_C_AUTHN_LEVEL_DEFAULT,
		RPC_C_IMP_LEVEL_IMPERSONATE,
		nullptr,
		EOAC_NONE,
		nullptr);

	// 如果已经设置过安全性，这里可能失败，但不致命
	if (FAILED(Hr) && Hr != RPC_E_TOO_LATE)
	{
		if (bComInitialized && Hr != RPC_E_TOO_LATE)
		{
			CoUninitialize();
		}
		return string("SecurityInitFailed");
	}

	IWbemLocator* Locator = nullptr;
	Hr = CoCreateInstance(
		CLSID_WbemLocator,
		nullptr,
		CLSCTX_INPROC_SERVER,
		IID_IWbemLocator,
		reinterpret_cast<LPVOID*>(&Locator));

	if (FAILED(Hr) || !Locator)
	{
		if (bComInitialized)
		{
			CoUninitialize();
		}
		return string("WmiLocatorFailed");
	}

	IWbemServices* Services = nullptr;
	Hr = Locator->ConnectServer(
		bstr_t(L"ROOT\\CIMV2"),
		nullptr,
		nullptr,
		nullptr,
		0,
		nullptr,
		nullptr,
		&Services);

	if (FAILED(Hr) || !Services)
	{
		Locator->Release();
		if (bComInitialized)
		{
			CoUninitialize();
		}
		return string("WmiConnectFailed");
	}

	// 设置代理安全性
	Hr = CoSetProxyBlanket(
		Services,
		RPC_C_AUTHN_WINNT,
		RPC_C_AUTHZ_NONE,
		nullptr,
		RPC_C_AUTHN_LEVEL_CALL,
		RPC_C_IMP_LEVEL_IMPERSONATE,
		nullptr,
		EOAC_NONE);

	if (FAILED(Hr))
	{
		Services->Release();
		Locator->Release();
		if (bComInitialized)
		{
			CoUninitialize();
		}
		return string("ProxyBlanketFailed");
	}

	string Result;

	// 常见的硬件信息，你可以按需要增删
	AppendWmiProperty(Services, L"SELECT ProcessorId FROM Win32_Processor", L"ProcessorId", "CPU", Result);
	//AppendWmiProperty(Services, L"SELECT SerialNumber FROM Win32_BaseBoard", L"SerialNumber", "BaseBoardSN", Result);
	//AppendWmiProperty(Services, L"SELECT SerialNumber FROM Win32_BIOS", L"SerialNumber", "BIOSSN", Result);
	AppendWmiProperty(Services, L"SELECT SerialNumber FROM Win32_DiskDrive WHERE MediaType LIKE 'Fixed%'", L"SerialNumber", "DiskSN", Result);
	//AppendWmiProperty(Services, L"SELECT UUID FROM Win32_ComputerSystemProduct", L"UUID", "UUID", Result);
	//AppendWmiProperty(Services, L"SELECT Caption FROM Win32_OperatingSystem", L"Caption", "OS", Result);
	//AppendWmiProperty(Services,L"SELECT MACAddress FROM Win32_NetworkAdapterConfiguration WHERE IPEnabled=True",L"MACAddress","MAC",Result);
	//AppendWmiProperty(Services, L"SELECT Name FROM Win32_ComputerSystem",L"Name","PC",Result);

	Services->Release();
	Locator->Release();
	if (bComInitialized)
	{
		CoUninitialize();
	}

	if (Result.empty())
	{
		return string("NoHardwareInfo");
	}

	// 使用 DES 加密硬件信息
	return Result;
#endif
}


#if defined(_WIN32)
// DES 加密辅助函数

FString TimeVerificationOnline::EncryptWithDES(const FString& PlainText, const FString& Key)
{
	// 1. FString -> UTF-8
	FTCHARToUTF8 PlainUtf8(*PlainText);
	const uint8* PlainData = reinterpret_cast<const uint8*>(PlainUtf8.Get());
	int32 PlainLen = PlainUtf8.Length();

	// 2. Key（64 bits = 8 bytes）
	uint8 KeyBytes[8] = { 0 };
	FTCHARToUTF8 KeyUtf8(*Key);
	FMemory::Memcpy(
		KeyBytes,
		KeyUtf8.Get(),
		FMath::Min(8, KeyUtf8.Length())
	);

	// 3. 创建 EVP 上下文
	OpenSSLWrapper::EVP_CIPHER_CTX* Ctx = OpenSSLWrapper::EVP_CIPHER_CTX_new();
	if (!Ctx)
	{
		return TEXT("");
	}

	// DES ECB + PKCS7（OpenSSL 默认）
	EVP_EncryptInit_ex(
		Ctx,
		OpenSSLWrapper::EVP_des_ecb(),
		nullptr,
		KeyBytes,
		nullptr
	);

	// 4. 加密
	TArray<uint8> Encrypted;
	Encrypted.SetNumZeroed(PlainLen + 8);

	int32 OutLen1 = 0;
	EVP_EncryptUpdate(
		Ctx,
		Encrypted.GetData(),
		&OutLen1,
		PlainData,
		PlainLen
	);

	int32 OutLen2 = 0;
	EVP_EncryptFinal_ex(
		Ctx,
		Encrypted.GetData() + OutLen1,
		&OutLen2
	);

	EVP_CIPHER_CTX_free(Ctx);

	Encrypted.SetNum(OutLen1 + OutLen2);

	// 5. 转 Hex 字符串
	FString HexResult;
	HexResult.Reserve(Encrypted.Num() * 2);

	for (uint8 Byte : Encrypted)
	{
		HexResult += FString::Printf(TEXT("%02X"), Byte);
	}

	return HexResult.ToLower();
}


#else
FString TimeVerificationOnline::EncryptWithDES(const FString& PlainText, const FString& Key)
{
	return PlainText;
}
#endif



void TimeVerificationOnline::ShowWindowsMessageBox(const FString& Title, const FString& Message)
{
#if PLATFORM_WINDOWS
	// MessageBoxW 支持 Unicode，所以用 *FString 转 const wchar_t*
	MessageBoxW(nullptr, *Message, *Title, MB_OK | MB_ICONINFORMATION);
#endif
}


