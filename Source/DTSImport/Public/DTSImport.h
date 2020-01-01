

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"


#define AssertRet(cond, ret)                  \
	do {                                      \
		if (static_cast<bool>(cond) == false) \
		{                                     \
			return (ret);                     \
		}                                     \
	} while (0)

#define AssertErrorRet(cond, ret, ...)        \
	do {                                      \
		if (static_cast<bool>(cond) == false) \
		{                                     \
			GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, __VA_ARGS__);  \
			return (ret);                     \
		}                                     \
	} while (0)


class FDTSImportModule : public IModuleInterface
{
public:

	void StartupModule() override;
	void ShutdownModule() override;

	static inline FDTSImportModule& Get()
	{
		return FModuleManager::LoadModuleChecked<FDTSImportModule>("DTSImport");
	}
	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("DTSImport");
	}
};
