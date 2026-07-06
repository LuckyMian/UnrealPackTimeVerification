#pragma once
#include <string>

class TimeVerificationOnline
{
public:
	static std::string GetDeviceUniqueIDByWMI();
	static FString EncryptWithDES(const FString& PlainText,const FString& Key);
	static void ShowWindowsMessageBox(const FString& Title, const FString& Message);

};
