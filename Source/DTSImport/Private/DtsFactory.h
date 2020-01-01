
#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Factories/Factory.h"
#include "DtsFactory.generated.h"

class IImportSettingsParser;

UCLASS(hidecategories=Object)
class DTSIMPORT_API UDtsFactory : public UFactory
{
	GENERATED_UCLASS_BODY()

	//~ Begin UObject Interface
	void CleanUp() override;
	bool ConfigureProperties() override;
	void PostInitProperties() override;
	//~ End UObject Interface

	//~ Begin UFactory Interface
	bool DoesSupportClass(UClass* Class) override;
	UClass* ResolveSupportedClass() override;
	UObject* FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, const FString& Filename, const TCHAR* Parms, FFeedbackContext* Warn, bool& bOutOperationCanceled) override;
	bool FactoryCanImport(const FString& Filename) override;
	IImportSettingsParser* GetImportSettingsParser() override;
	//~ End UFactory Interface

private:
	bool parseDtsData(UObject*& createdObject, uint8* data, int64 dataSize);
	void parseSequence(uint32_t version, uint8*& data, int64& dataSize);
	void parseMembuffers(uint32_t version, uint32_t* memBuffer32, uint32_t sizeMemBuffer32, uint16_t* memBuffer16, uint32_t sizeMemBuffer16, uint8_t* memBuffer8, uint32_t sizeMemBuffer8);
	void parseMesh(uint32_t version, uint32_t& guardValue, uint32_t*& memBuffer32, uint32_t& sizeMemBuffer32, uint16_t*& memBuffer16, uint32_t& sizeMemBuffer16, uint8_t*& memBuffer8, uint32_t& sizeMemBuffer8);
	
};

DECLARE_LOG_CATEGORY_EXTERN(LogDts, Log, All);




