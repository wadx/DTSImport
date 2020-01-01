

#include "DtsFactory.h"

#include "Misc/Paths.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimSequence.h"
#include "Editor/EditorEngine.h"
#include "Engine/StaticMesh.h"
#include "Editor.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/FeedbackContext.h"
//#include "SkelImport.h"
//#include "EditorReimportHandler.h"
//#include "Logging/TokenizedMessage.h"
//#include "AssetRegistryModule.h"
//#include "ObjectTools.h"
//#include "AssetImportTask.h"
//#include "LODUtilities.h"


DEFINE_LOG_CATEGORY(LogDts);

#define LOCTEXT_NAMESPACE "DTSFactory"


UDtsFactory::UDtsFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = nullptr;
	Formats.Add(TEXT("dts;DTS meshes and animations"));

	bCreateNew = false;
	bText = false;
	bEditorImport = true;
}


void UDtsFactory::PostInitProperties()
{
	Super::PostInitProperties();
	bEditorImport = true;
	bText = false;
}


bool UDtsFactory::DoesSupportClass(UClass* Class)
{
	return (Class == UStaticMesh::StaticClass() || Class == USkeletalMesh::StaticClass() || Class == UAnimSequence::StaticClass());
}


UClass* UDtsFactory::ResolveSupportedClass()
{
	UClass* ImportClass = nullptr;

//	if (ImportUI->MeshTypeToImport == FBXIT_SkeletalMesh)
//	{
//		ImportClass = USkeletalMesh::StaticClass();
//	}
//	else if (ImportUI->MeshTypeToImport == FBXIT_Animation)
//	{
//		ImportClass = UAnimSequence::StaticClass();
//	}
//	else
//	{
		ImportClass = UStaticMesh::StaticClass();
//	}

	return ImportClass;
}


bool UDtsFactory::ConfigureProperties()
{
	return true;
}


UObject* UDtsFactory::FactoryCreateFile
(
	UClass* Class,
	UObject* InParent,
	FName Name,
	EObjectFlags Flags,
	const FString& InFilename,
	const TCHAR* Parms,
	FFeedbackContext* Warn,
	bool& bOutOperationCanceled
 )
{
	FString FileExtension = FPaths::GetExtension(InFilename);
	const TCHAR* Type = *FileExtension;
	if (!IFileManager::Get().FileExists(*InFilename))
	{
		UE_LOG(LogDts, Error, TEXT("Failed to load file [%s]"), *InFilename)
		return nullptr;
	}
	ParseParms(Parms);
	CA_ASSUME(InParent);
	GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPreImport(this, Class, InParent, Name, Type);
	Warn->BeginSlowTask(NSLOCTEXT("DtsFactory", "BeginImportingDtsMeshTask", "Importing DTS mesh"), true);

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	IFileHandle* FileHandle = PlatformFile.OpenRead(*InFilename);
	if (!FileHandle)
	{
		UE_LOG(LogDts, Error, TEXT("Can't open file [%s]"), *InFilename)
		Warn->EndSlowTask();
		GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPostImport(this, nullptr);
		return nullptr;
	}
	uint8* ByteArray = static_cast<uint8*>(FMemory::Malloc(FileHandle->Size()));
	if (!ByteArray)
	{
		UE_LOG(LogDts, Error, TEXT("Can't allocate memory for file [%s] size [%li]"), *InFilename, FileHandle->Size());
		Warn->EndSlowTask();
		GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPostImport(this, nullptr);
		return nullptr;
	}
	if (!FileHandle->Read(ByteArray, FileHandle->Size()))
	{
		UE_LOG(LogDts, Error, TEXT("Can't read from file [%s] size [%li]"), *InFilename, FileHandle->Size());
		Warn->EndSlowTask();
		GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPostImport(this, nullptr);
		return nullptr;
	}
	UObject* CreatedObject = nullptr;
	if (!parseDtsData(CreatedObject, ByteArray, FileHandle->Size()) || !CreatedObject)
	{
		UE_LOG(LogDts, Error, TEXT("Can't parse file [%s] size [%li]"), *InFilename, FileHandle->Size());
		Warn->EndSlowTask();
		GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPostImport(this, nullptr);
		return nullptr;
	}
	FMemory::Free(ByteArray);
	delete FileHandle;

	Warn->EndSlowTask();
	GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPostImport(this, CreatedObject);
	return CreatedObject;
}


void UDtsFactory::CleanUp() 
{
}


bool UDtsFactory::FactoryCanImport(const FString& Filename)
{
	const FString Extension = FPaths::GetExtension(Filename);
	if (Extension == TEXT("dts"))
	{
		return true;
	}
	return false;
}


IImportSettingsParser* UDtsFactory::GetImportSettingsParser()
{
	return nullptr;
}


#undef LOCTEXT_NAMESPACE

