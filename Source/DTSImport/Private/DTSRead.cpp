

#include "DtsFactory.h"

#include <string>
#include <vector>


// http://docs.garagegames.com/torque-3d/official/content/documentation/Artist%20Guide/Formats/dts_format.html


enum DTSMeshType : uint32_t
{
	StandardMeshType = 0,
	SkinMeshType = 1,
	DecalMeshType = 2,
	SortedMeshType = 3,
	NullMeshType = 4,
	// flags stored with meshType:
	//UseEncodedNormals = BIT(28),
	//BillboardZAxis = BIT(29),
	//HasDetailTexture = BIT(30),
	//Billboard = BIT(31),
};



template<typename T>
T GetValue(uint8*& data, int64& dataSize)
{
	//uint32_t sizeMemBuffer = *((uint32_t*)data); data += sizeof(uint32_t); dataSize -= sizeof(uint32_t);
	checkf(dataSize >= sizeof(T), TEXT("Buffer empty"));
	if (dataSize >= sizeof(T))
	{
		T t = *((T*)data); data += sizeof(T); dataSize -= sizeof(T);
		return t;
	}
	return 0;
}


template<typename T, typename B>
T GetValue(B*& memBuffer, uint32_t& sizeMemBuffer)
{
	checkf(sizeMemBuffer > 0, TEXT("Buffer empty"));
	if (sizeMemBuffer > 0)
	{
		T t = *((T*)memBuffer); memBuffer++; sizeMemBuffer--;
		return t;
	}
	return 0;
}


std::vector<uint32_t> GetBitset(uint8*& data, int64& dataSize)
{
	std::vector<uint32_t> out;
	int32_t dummy = GetValue<int32_t>(data, dataSize);
	int32_t numWords = GetValue<int32_t>(data, dataSize);
	for (auto i = 0; i < numWords; i++)
	{
		uint32_t value = GetValue<uint32_t>(data, dataSize);
		out.push_back(value);
	}
	return out;
}


std::string GetPascalString(uint8*& data, int64& dataSize)
{
	std::string out;
	uint8_t numBytes = GetValue<uint8_t>(data, dataSize);
	for (auto i = 0; i < numBytes; i++)
	{
		char c = GetValue<char>(data, dataSize);
		out += c;
	}
	return out;
}


FVector GetVector(uint32_t*& memBuffer32, uint32_t& sizeMemBuffer32)
{
	float x = GetValue<float>(memBuffer32, sizeMemBuffer32);
	float y = GetValue<float>(memBuffer32, sizeMemBuffer32);
	float z = GetValue<float>(memBuffer32, sizeMemBuffer32);
	return FVector(x, y, z);
}


FBox GetBox(uint32_t*& memBuffer32, uint32_t& sizeMemBuffer32)
{
	FVector min = GetVector(memBuffer32, sizeMemBuffer32);
	FVector max = GetVector(memBuffer32, sizeMemBuffer32);
	return FBox(min, max);
}


FQuat GetQuat16(uint16_t*& memBuffer16, uint32_t& sizeMemBuffer16)
{
	int16_t x = GetValue<int16_t>(memBuffer16, sizeMemBuffer16);
	int16_t y = GetValue<int16_t>(memBuffer16, sizeMemBuffer16);
	int16_t z = GetValue<int16_t>(memBuffer16, sizeMemBuffer16);
	int16_t w = GetValue<int16_t>(memBuffer16, sizeMemBuffer16);
	return FQuat(x, y, z, w);
}


std::string GetString(uint8_t*& memBuffer8, uint32_t& sizeMemBuffer8)
{
	std::string out;
	for (;;)
	{
		char c = GetValue<char>(memBuffer8, sizeMemBuffer8);
		if (c == 0)
		{
			break;
		}
		out += c;
	}
	return out;
}


bool CheckGuard(uint32_t& guardValue, uint32_t*& memBuffer32, uint32_t& sizeMemBuffer32, uint16_t*& memBuffer16, uint32_t& sizeMemBuffer16, uint8_t*& memBuffer8, uint32_t& sizeMemBuffer8)
{
	uint32_t val32 = GetValue<uint32_t>(memBuffer32, sizeMemBuffer32);
	uint16_t val16 = GetValue<uint16_t>(memBuffer16, sizeMemBuffer16);
	uint8_t val8 = GetValue<uint8_t>(memBuffer8, sizeMemBuffer8);
	guardValue++;
	checkf(val32 == val16 && val16 == val8 && val8 == (guardValue - 1), TEXT("Guard failed"));
	if (val32 != val16 || val16 != val8 || val8 != (guardValue - 1))
	{
		return false;
	}
	return true;
}


bool UDtsFactory::parseDtsData(UObject*& createdObject, uint8* data, int64 dataSize)
{
	uint32_t version = GetValue<uint32_t>(data, dataSize) & 0xFFFF;
	if (version < 19)
	{
		return false;
	}

	uint32_t sizeMemBuffer = GetValue<uint32_t>(data, dataSize);
	uint32_t startU16      = GetValue<uint32_t>(data, dataSize);
	uint32_t startU8       = GetValue<uint32_t>(data, dataSize);
	uint32_t sizeMemBuffer32 = startU16 * 4;
	uint32_t sizeMemBuffer16 = startU8 * 4 - startU16 * 4;
	uint32_t sizeMemBuffer8 = sizeMemBuffer * 4 - startU8 * 4;
	uint32_t* memBuffer32  = (uint32_t*)data; data += sizeMemBuffer32;
	uint16_t* memBuffer16  = (uint16_t*)data; data += sizeMemBuffer16;
	uint8_t* memBuffer8    = (uint8_t*)data;  data += sizeMemBuffer8;
	parseMembuffers(version, memBuffer32, sizeMemBuffer32, memBuffer16, sizeMemBuffer16, memBuffer8, sizeMemBuffer8);
	dataSize -= sizeMemBuffer * 4;

	int32_t numSequences   = GetValue<int32_t>(data, dataSize);
	for (auto num = 0; num < numSequences; num++)
	{
		parseSequence(version, data, dataSize);
	}

	int8_t matStreamType = GetValue<int8_t>(data, dataSize);
	if (matStreamType == 1)
	{
		int32_t numMaterials = GetValue<int32_t>(data, dataSize);
		for (auto num = 0; num < numMaterials; num++)
		{
			std::string matName = GetPascalString(data, dataSize);			// Names of the materials in the shape. Each name is stored as a 4-byte length followed by the N characters in the string (terminating NULL is not included in the length or N characters).
		}
		for (auto num = 0; num < numMaterials; num++)
		{
			uint32_t matFlags = GetValue<uint32_t>(data, dataSize);			// Flags for each material*
		}
		for (auto num = 0; num < numMaterials; num++)
		{
			int32_t matReflectanceMaps = GetValue<int32_t>(data, dataSize);	// Index of the material to use as a reflectance map for each material* (or -1 for none)
		}
		for (auto num = 0; num < numMaterials; num++)
		{
			int32_t matBumpMaps = GetValue<int32_t>(data, dataSize);		// Index of the material to use as a bump map for each material* (or -1 for none)
		}
		for (auto num = 0; num < numMaterials; num++)
		{
			int32_t matDetailMaps = GetValue<int32_t>(data, dataSize);		// Index of the material to use as a detail map for each material* (or -1 for none)
		}
		if (version == 25)
		{
			for (auto num = 0; num < numMaterials; num++)
			{
				int32_t dummy = GetValue<int32_t>(data, dataSize);			// Dummy value. Only present in DTS v25.
			}
		}
		for (auto num = 0; num < numMaterials; num++)
		{
			float matDetailScales = GetValue<float>(data, dataSize);		// Detail scale for each material*
		}
		for (auto num = 0; num < numMaterials; num++)
		{
			float matReflectance = GetValue<float>(data, dataSize);			// Reflectance value for each material*
		}
	}

	return true;
}


void UDtsFactory::parseMembuffers(uint32_t version, uint32_t* memBuffer32, uint32_t sizeMemBuffer32, uint16_t* memBuffer16, uint32_t sizeMemBuffer16, uint8_t* memBuffer8, uint32_t sizeMemBuffer8)
{
	uint32_t guardValue = 0;

	int32_t numNodes = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);				// Number of nodes in the shape
	int32_t numObjects = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);			// Number of objects in the shape
	int32_t numDecals = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);			// Number of decals in the shape
	int32_t numSubShapes = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);			// Number of subshapes in the shape
	int32_t numIFLs = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);				// Number of IFL materials in the shape
	int32_t numNodeRotations = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);		// Number of node rotation keyframes
	int32_t numNodeTranslations = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);	// Number of node translation keyframes
	int32_t numNodeUniformScales = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);	// Number of node uniform scale keyframes
	int32_t numNodeAlignedScales = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);	// Number of node aligned scale keyframes
	int32_t numNodeArbScales = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);		// Number of node arbitrary scale keyframes
	int32_t numGroundFrames = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);		// Number of ground transform keyframes
	int32_t numObjectStates = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);		// Number of object state keyframes
	int32_t numDecalStates = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);		// Number of decal state keyframes
	int32_t numTriggers = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);			// Number of triggers (all sequences)
	int32_t numDetails = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);			// Number of detail levels in the shape
	int32_t numMeshes = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);			// Number of meshes (all detail levels) in the shape
	int32_t numNames = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);				// Number of name strings in the shape
	float smallestVisibleSize = GetValue<float>(memBuffer32, sizeMemBuffer32);		// Size of the smallest visible detail level
	int32_t smallestVisibleDL = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);	// Index of the smallest visible detail level

	CheckGuard(guardValue, memBuffer32, sizeMemBuffer32, memBuffer16, sizeMemBuffer16, memBuffer8, sizeMemBuffer8);

	float radius = GetValue<float>(memBuffer32, sizeMemBuffer32);					// Shape bounding sphere radius
	float tubeRadius = GetValue<float>(memBuffer32, sizeMemBuffer32);				// Shape bounding cylinder radius
	FVector center = GetVector(memBuffer32, sizeMemBuffer32);						// Center of the shape bounds
	FBox bounds = GetBox(memBuffer32, sizeMemBuffer32);								// Shape bounding box

	CheckGuard(guardValue, memBuffer32, sizeMemBuffer32, memBuffer16, sizeMemBuffer16, memBuffer8, sizeMemBuffer8);

	for (auto i = 0; i < numNodes; i++)												// Array of numNodes Nodes
	{
		int32_t nameIndex = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);
		int32_t parentIndex = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);
		int32_t firstObject = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);
		int32_t firstChild = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);
		int32_t nextSibling = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);
	}

	CheckGuard(guardValue, memBuffer32, sizeMemBuffer32, memBuffer16, sizeMemBuffer16, memBuffer8, sizeMemBuffer8);

	for (auto i = 0; i < numObjects; i++)											// Array of numObjects Objects
	{
		int32_t nameIndex = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);
		int32_t numMeshes_ = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);
		int32_t startMeshIndex = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);
		int32_t nodeIndex = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);
		int32_t nextSibling = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);
		int32_t firstDecal = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);
	}

	CheckGuard(guardValue, memBuffer32, sizeMemBuffer32, memBuffer16, sizeMemBuffer16, memBuffer8, sizeMemBuffer8);

	for (auto i = 0; i < numDecals; i++)											// Array of numDecals Decals. Note that decals are deprecated.
	{
		int32_t dummy0 = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);
		int32_t dummy1 = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);
		int32_t dummy2 = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);
		int32_t dummy3 = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);
		int32_t dummy4 = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);
	}

	CheckGuard(guardValue, memBuffer32, sizeMemBuffer32, memBuffer16, sizeMemBuffer16, memBuffer8, sizeMemBuffer8);

	for (auto i = 0; i < numIFLs; i++)												// Array of numIFLs IflMaterials
	{
		int32_t nameIndex = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);
		int32_t materialSlot = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);
		int32_t firstFrame = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);
		int32_t firstFrameOffTimeIndex = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);
		int32_t numFrames = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);
	}

	CheckGuard(guardValue, memBuffer32, sizeMemBuffer32, memBuffer16, sizeMemBuffer16, memBuffer8, sizeMemBuffer8);

	for (auto i = 0; i < numSubShapes; i++)												// Array of numSubShapes ints representing the index of the first node in each subshape
	{
		int32_t subShapeFirstNode = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);
	}
	for (auto i = 0; i < numSubShapes; i++)												// Array of numSubShapes ints representing the index of the first object in each subshape
	{
		int32_t subShapeFirstObject = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);
	}
	for (auto i = 0; i < numSubShapes; i++)												// Array of numSubShapes ints representing the index of the first decal in each subshape
	{
		int32_t subShapeFirstDecal = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);
	}

	CheckGuard(guardValue, memBuffer32, sizeMemBuffer32, memBuffer16, sizeMemBuffer16, memBuffer8, sizeMemBuffer8);

	for (auto i = 0; i < numSubShapes; i++)
	{
		int32_t subShapeNumNodes = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);
	}
	for (auto i = 0; i < numSubShapes; i++)
	{
		int32_t subShapeNumObjects = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);
	}
	for (auto i = 0; i < numSubShapes; i++)
	{
		int32_t subShapeNumDecals = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);
	}

	CheckGuard(guardValue, memBuffer32, sizeMemBuffer32, memBuffer16, sizeMemBuffer16, memBuffer8, sizeMemBuffer8);

	for (auto i = 0; i < numNodes; i++)													// Array of numNodes quaternions for default node rotations
	{
		FQuat defaultRotation = GetQuat16(memBuffer16, sizeMemBuffer16);
	}
	for (auto i = 0; i < numNodes; i++)													// Array of numNodes points for default node translations
	{
		FVector defaultTranslation = GetVector(memBuffer32, sizeMemBuffer32);
	}
	for (auto i = 0; i < numNodeRotations; i++)											// Array of numNodeRotations quaternions for node rotation keyframes (all sequences)
	{
		FQuat nodeRotation = GetQuat16(memBuffer16, sizeMemBuffer16);
	}
	for (auto i = 0; i < numNodeTranslations; i++)										// Array of numNodeTranslations points for node translation keyframes (all sequences)
	{
		FVector nodeTranslation = GetVector(memBuffer32, sizeMemBuffer32);
	}

	CheckGuard(guardValue, memBuffer32, sizeMemBuffer32, memBuffer16, sizeMemBuffer16, memBuffer8, sizeMemBuffer8);

	for (auto i = 0; i < numNodeUniformScales; i++)										// Array of numNodeUniformScales floats for node uniform scale keyframes (all sequences)
	{
		float nodeUniformScale = GetValue<float>(memBuffer32, sizeMemBuffer32);
	}
	for (auto i = 0; i < numNodeAlignedScales; i++)										// Array of numNodeAlignedScales points for node aligned scale keyframes (all sequences)
	{
		FVector nodeAlignedScale = GetVector(memBuffer32, sizeMemBuffer32);
	}
	for (auto i = 0; i < numNodeArbScales; i++)											// Array of numNodeArbScales points for node arbitrary scale factor keyframes (all sequences)
	{
		FVector nodeArbScaleFactor = GetVector(memBuffer32, sizeMemBuffer32);
	}
	for (auto i = 0; i < numNodeArbScales; i++)											// Array of numNodeArbScales quaternions for node arbitrary scale rotation keyframes (all sequences)
	{
		FQuat nodeArbScaleRot = GetQuat16(memBuffer16, sizeMemBuffer16);
	}

	CheckGuard(guardValue, memBuffer32, sizeMemBuffer32, memBuffer16, sizeMemBuffer16, memBuffer8, sizeMemBuffer8);

	for (auto i = 0; i < numGroundFrames; i++)											// Array of numGroundFrames points for ground transform keyframes (all sequences)
	{
		FVector groundTranslation = GetVector(memBuffer32, sizeMemBuffer32);
	}
	for (auto i = 0; i < numGroundFrames; i++)											// Array of numGroundFrames quaternions for ground transform keyframes (all sequences)
	{
		FQuat groundRotation = GetQuat16(memBuffer16, sizeMemBuffer16);
	}

	CheckGuard(guardValue, memBuffer32, sizeMemBuffer32, memBuffer16, sizeMemBuffer16, memBuffer8, sizeMemBuffer8);

	for (auto i = 0; i < numObjectStates; i++)											// Array of numObjectStates ObjectStates
	{
		float vis = GetValue<float>(memBuffer32, sizeMemBuffer32);
		int32_t frameIndex = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);
		int32_t matFrame = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);
	}

	CheckGuard(guardValue, memBuffer32, sizeMemBuffer32, memBuffer16, sizeMemBuffer16, memBuffer8, sizeMemBuffer8);

	for (auto i = 0; i < numDecalStates; i++)											// Array of numDecalStates dummy integers for decal states
	{
		int32_t decalState = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);
	}

	CheckGuard(guardValue, memBuffer32, sizeMemBuffer32, memBuffer16, sizeMemBuffer16, memBuffer8, sizeMemBuffer8);

	for (auto i = 0; i < numTriggers; i++)												// Array of numTriggers sequence triggers (all sequences)
	{
		uint32_t state = GetValue<uint32_t>(memBuffer32, sizeMemBuffer32);
		uint32_t pos = GetValue<uint32_t>(memBuffer32, sizeMemBuffer32);
	}

	CheckGuard(guardValue, memBuffer32, sizeMemBuffer32, memBuffer16, sizeMemBuffer16, memBuffer8, sizeMemBuffer8);

	for (auto i = 0; i < numDetails; i++)												// Array of numDetails Details
	{
		int32_t nameIndex = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);
		int32_t subShapeNum = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);
		int32_t objectDetailNum = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);
		float size = GetValue<float>(memBuffer32, sizeMemBuffer32);
		float averageError = GetValue<float>(memBuffer32, sizeMemBuffer32);
		float maxError = GetValue<float>(memBuffer32, sizeMemBuffer32);
		int32_t polyCount = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);
		if (version >= 26)
		{
			int32_t bbDimension = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);
			int32_t bbDetailLevel = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);
			uint32_t bbEquatorSteps = GetValue<uint32_t>(memBuffer32, sizeMemBuffer32);
			uint32_t bbPolarSteps = GetValue<uint32_t>(memBuffer32, sizeMemBuffer32);
			float bbPolarAngle = GetValue<float>(memBuffer32, sizeMemBuffer32);
			uint32_t bbIncludePoles = GetValue<uint32_t>(memBuffer32, sizeMemBuffer32);
		}
	}

	CheckGuard(guardValue, memBuffer32, sizeMemBuffer32, memBuffer16, sizeMemBuffer16, memBuffer8, sizeMemBuffer8);

	for (auto i = 0; i < numMeshes; i++)												// Array of numMeshes Meshes
	{
		parseMesh(version, guardValue, memBuffer32, sizeMemBuffer32, memBuffer16, sizeMemBuffer16, memBuffer8, sizeMemBuffer8);
	}

	CheckGuard(guardValue, memBuffer32, sizeMemBuffer32, memBuffer16, sizeMemBuffer16, memBuffer8, sizeMemBuffer8);

	for (auto i = 0; i < numNames; i++)												// Array of numNames strings, stored as N characters followed by a terminating NULL for each string.
	{
		std::string name = GetString(memBuffer8, sizeMemBuffer8);
	}

	CheckGuard(guardValue, memBuffer32, sizeMemBuffer32, memBuffer16, sizeMemBuffer16, memBuffer8, sizeMemBuffer8);

	for (auto i = 0; i < numDetails; i++)												// Array of numDetails floats representing alpha-in value for each detail
	{
		float alphaIn = GetValue<float>(memBuffer32, sizeMemBuffer32);
	}
	for (auto i = 0; i < numDetails; i++)												// Array of numDetails floats representing alpha-out value for each detail
	{
		float alphaOut = GetValue<float>(memBuffer32, sizeMemBuffer32);
	}
}


void UDtsFactory::parseSequence(uint32_t version, uint8*& data, int64& dataSize)
{
	int32_t nameIndex = GetValue<int32_t>(data, dataSize);					// The name of this sequence as in index into the names array
	uint32_t flags = GetValue<uint32_t>(data, dataSize);					// Sequence flags
	int32_t numKeyframes = GetValue<int32_t>(data, dataSize);				// Number of keyframes in this sequence
	float duration = GetValue<float>(data, dataSize);						// Duration of the sequence (in seconds)
	int32_t priority = GetValue<int32_t>(data, dataSize);					// Sequence priority
	int32_t firstGroundFrame = GetValue<int32_t>(data, dataSize);			// First ground transform keyframe in this sequence (index into the groundTranslations and groundRotation arrays)
	int32_t numGroundFrames = GetValue<int32_t>(data, dataSize);			// Number of ground transform keyframes in this sequence
	int32_t baseRotation = GetValue<int32_t>(data, dataSize);				// First node rotation keyframe in this sequence (index into the nodeRotations array)
	int32_t baseTranslation = GetValue<int32_t>(data, dataSize);			// First node translation keyframe in this sequence (index into the nodeTranslations array)
	int32_t baseScale = GetValue<int32_t>(data, dataSize);					// First node scale keyframe in this sequence (index into the nodeXXXScales arrays)
	int32_t baseObjectState = GetValue<int32_t>(data, dataSize);			// First object state keyframe in this sequence (index into the objectStates array)
	int32_t baseDecalState = GetValue<int32_t>(data, dataSize);				// First decal state keyframe in this sequence (index into the decalStates array). Note that DTS decals are deprecated, and this value should be 0.
	int32_t firstTrigger = GetValue<int32_t>(data, dataSize);				// First trigger in this sequence (index into the triggers array)
	int32_t numTriggers = GetValue<int32_t>(data, dataSize);				// Number of triggers in this sequence
	float toolBegin = GetValue<float>(data, dataSize);						// Value representing the start of this sequence in the exporting tool's timeline (can usually by ignored)
	std::vector<uint32_t> rotationMatters = GetBitset(data, dataSize);		// BitSet indicating which node rotations are animated by this sequence.
	std::vector<uint32_t> translationMatters = GetBitset(data, dataSize);	// BitSet indicating which node translations are animated by this sequence.
	std::vector<uint32_t> scaleMatters = GetBitset(data, dataSize);			// BitSet indicating which node scales are animated by this sequence.
	std::vector<uint32_t> decalMatters = GetBitset(data, dataSize);			// BitSet indicating which decal states are animated by this sequence. Note that DTS decals are deprecated.
	std::vector<uint32_t> iflMatters = GetBitset(data, dataSize);			// BitSet indicating which IFL materials are animated by this sequence.
	std::vector<uint32_t> visMatters = GetBitset(data, dataSize);			// BitSet indicating which object's visibility is animated by this sequence.
	std::vector<uint32_t> frameMatters = GetBitset(data, dataSize);			// BitSet indicating which mesh's verts are animated by this sequence.
	std::vector<uint32_t> matFrameMatters = GetBitset(data, dataSize);		// BitSet indicating which mesh's UV coords are animated by this sequence.
}


void UDtsFactory::parseMesh(uint32_t version, uint32_t& guardValue, uint32_t*& memBuffer32, uint32_t& sizeMemBuffer32, uint16_t*& memBuffer16, uint32_t& sizeMemBuffer16, uint8_t*& memBuffer8, uint32_t& sizeMemBuffer8)
{

	uint32_t meshType = GetValue<uint32_t>(memBuffer32, sizeMemBuffer32);		// Type of mesh

	if (meshType == DTSMeshType::NullMeshType)
	{
		return;
	}

	CheckGuard(guardValue, memBuffer32, sizeMemBuffer32, memBuffer16, sizeMemBuffer16, memBuffer8, sizeMemBuffer8);

	int32_t numFrames = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);		// Number of vertex position keyframes
	int32_t numMatFrames = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);		// Number of vertex UV keyframes
	int32_t parentMesh = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);		// Index of this mesh's parent (usually -1 for none)
	FBox bounds = GetBox(memBuffer32, sizeMemBuffer32);							// Bounding box for this mesh
	FVector center = GetVector(memBuffer32, sizeMemBuffer32);					// Bounds center for this mesh
	float radius = GetValue<float>(memBuffer32, sizeMemBuffer32);				// Bounding sphere radius for this mesh
	int32_t numVerts = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);			// Number of vertex positions
	for (auto i = 0; i < numVerts; i++)											// Array of numVerts vertex positions (all keyframes)
	{
		FVector vert = GetVector(memBuffer32, sizeMemBuffer32);
	}
	int32_t numTVerts = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);		// Number of UV coordinates
	for (auto i = 0; i < numTVerts; i++)										// Array of numTVerts UV coordinates (all keyframes)
	{
		float u = GetValue<float>(memBuffer32, sizeMemBuffer32);				// Point2F u
		float v = GetValue<float>(memBuffer32, sizeMemBuffer32);				// Point2F v
	}
	if (version >= 26)
	{
		int32_t numTVerts2 = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);	// Number of 2nd UV coordinates (DTS v26+ only)
		for (auto i = 0; i < numTVerts2; i++)									// Array of numTVerts2 2nd UV coordinates (DTS v26+ only)
		{
			float u = GetValue<float>(memBuffer32, sizeMemBuffer32);			// Point2F u
			float v = GetValue<float>(memBuffer32, sizeMemBuffer32);			// Point2F v
		}

		int32_t numVColors = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);	// Number of vertex color values (DTS v26+ only)
		for (auto i = 0; i < numVColors; i++)									// Array of numVColors vertex colors (DTS v26+ only)
		{
			uint32_t color = GetValue<uint32_t>(memBuffer32, sizeMemBuffer32);	// ColorI { U8 red, U8 green, U8 blue, U8 alpha }
		}
	}
	for (auto i = 0; i < numVerts; i++)											// Array of numVerts vertex normals
	{
		FVector norm = GetVector(memBuffer32, sizeMemBuffer32);
	}
	for (auto i = 0; i < numVerts; i++)											// Array of numVerts encoded normal indices
	{
		uint8_t encodedNorm = GetValue<uint8_t>(memBuffer8, sizeMemBuffer8);
	}

	int32_t numPrimitives = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);	// Number of mesh primitives (triangles, triangle lists etc)
	if (version <= 24)
	{
		for (auto i = 0; i < numPrimitives; i++)								// primitives (v24-) 16-bit S16 Array of numPrimitives 16-bit Primitive struct data { start, numElements }
		{
			int16_t start = GetValue<int16_t>(memBuffer16, sizeMemBuffer16);
			int16_t numElements = GetValue<int16_t>(memBuffer16, sizeMemBuffer16);
		}
		for (auto i = 0; i < numPrimitives; i++)								// primitives (v24-) 32-bit U32 Array of numPrimitives 32-bit Primitive struct data { maxIndex }
		{
			uint32_t maxIndex = GetValue<uint32_t>(memBuffer32, sizeMemBuffer32);
		}
	}
	else
	{
		for (auto i = 0; i < numPrimitives; i++)								// primitives (v25+) 32-bit Primitive { S32 start, S32 numElements, U32 matIndex } Array of numPrimitives Primitives
		{
			int32_t start = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);
			int32_t numElements = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);
			uint32_t matIndex = GetValue<uint32_t>(memBuffer32, sizeMemBuffer32);
		}
	}

	int32_t numIndices = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);		// Total number of vertex indices (all primitives)
	if (version <= 25)
	{
		for (auto i = 0; i < numIndices; i++)									// indices (DTS v25-) 16-bit S16 Array of numIndices vertex indices
		{
			int16_t vertex = GetValue<int16_t>(memBuffer16, sizeMemBuffer16);
		}
	}
	else
	{
		for (auto i = 0; i < numIndices; i++)									// indices (DTS v25+) 32-bit S32 Array of numIndices vertex indices
		{
			int32_t vertex = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);
		}
	}

	int32_t numMergeIndices = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);	// Number of merge indices. Note that merge indices have been deprecated.
	for (auto i = 0; i < numMergeIndices; i++)									// Array of numMergeIndices merge indices
	{
		int16_t vertex = GetValue<int16_t>(memBuffer16, sizeMemBuffer16);
	}

	int32_t vertsPerFrame = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);	// Number of vertices in each keyframe (position or UV)
	uint32_t flags = GetValue<uint32_t>(memBuffer32, sizeMemBuffer32);			// Mesh flags

	CheckGuard(guardValue, memBuffer32, sizeMemBuffer32, memBuffer16, sizeMemBuffer16, memBuffer8, sizeMemBuffer8);

	if (meshType == DTSMeshType::SkinMeshType)
	{
		int32_t numInitialVerts = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);	// Number of intial vert positions and normals
		for (auto i = 0; i < numInitialVerts; i++)									// Array of numInitialVerts positions
		{
			int32_t initialVert = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);
		}
		for (auto i = 0; i < numInitialVerts; i++)									// Array of numInitialVerts vertex normals
		{
			FVector norm = GetVector(memBuffer32, sizeMemBuffer32);
		}
		for (auto i = 0; i < numInitialVerts; i++)									// Array of numInitialVerts encoded initial normal indices
		{
			uint8_t encodedNorm = GetValue<uint8_t>(memBuffer8, sizeMemBuffer8);
		}
		int32_t numInitialTransforms = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);	// Number of initial transforms
		for (auto i = 0; i < numInitialTransforms; i++)									// Array of numInitialTransforms transforms
		{
			for (auto n = 0; n < 16; n++)												// MatrixF { F32 m[16] }
			{
				float value = GetValue<float>(memBuffer32, sizeMemBuffer32);
			}
		}
		int32_t numVertIndices = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);	// Number of vertex indices
		for (auto i = 0; i < numVertIndices; i++)									// Array of numVertIndices vertex indices
		{
			int32_t vertIndice = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);
		}
		int32_t numBoneIndices = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);	// Number of bone indices
		for (auto i = 0; i < numBoneIndices; i++)									// Array of numBoneIndices bone indices
		{
			int32_t boneIndice = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);
		}
		int32_t numWeights = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);		// Number of weights
		for (auto i = 0; i < numWeights; i++)										// Array of numWeights bone weights
		{
			float weight = GetValue<float>(memBuffer32, sizeMemBuffer32);
		}
		int32_t numNodeIndices = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);	// Number of node indices
		for (auto i = 0; i < numNodeIndices; i++)									// Array of node indices
		{
			int32_t nodeIndice = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);
		}

		CheckGuard(guardValue, memBuffer32, sizeMemBuffer32, memBuffer16, sizeMemBuffer16, memBuffer8, sizeMemBuffer8);
	}

	if (meshType == DTSMeshType::SortedMeshType)
	{

		int32_t numClusters = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);		// Number of clusters
		for (auto i = 0; i < numClusters; i++)										// Array of numClusters Clusters
		{
			int32_t startPrimitive = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);
			int32_t endPrimitive = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);
			FVector norm = GetVector(memBuffer32, sizeMemBuffer32);
			float k = GetValue<float>(memBuffer32, sizeMemBuffer32);
			int32_t frontCluster = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);
			int32_t backCluster = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);
		}
		int32_t numStartClusters = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);	// Number of start cluster indices
		for (auto i = 0; i < numStartClusters; i++)									// Array of numStartClusters start cluster indices
		{
			int32_t startCluster = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);
		}
		int32_t numFirstVerts = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);	// Number of first vertex indices
		for (auto i = 0; i < numFirstVerts; i++)									// Array of numFirstVerts first vertex indices
		{
			int32_t firstVert = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);
		}
		int32_t numNumVerts = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);		// Number of numVert counts
		for (auto i = 0; i < numNumVerts; i++)										// Array of numVert counts
		{
			int32_t count = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);
		}
		int32_t numFirstTVerts = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);	// Number of first TVert indices
		for (auto i = 0; i < numFirstTVerts; i++)									// Array of numFIrstTVerts first TVert indices
		{
			int32_t firstTVert = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);
		}
		int32_t alwaysWriteDepth = GetValue<int32_t>(memBuffer32, sizeMemBuffer32);	// Always write depth flag

		CheckGuard(guardValue, memBuffer32, sizeMemBuffer32, memBuffer16, sizeMemBuffer16, memBuffer8, sizeMemBuffer8);
	}
}

