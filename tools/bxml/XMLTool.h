#pragma once

#include <iostream>
#include <vector>
#include <Windows.h>
#include <cstdio>
#include <comutil.h>

#include "UniqueStringTable.h"

//#define DEBUG_ASSERT(cond, expr, eq) if (cond == eq) { printf(#expr); DebugBreak(); }
#define DEBUG_ASSERT(cond, expr, eq) ((void)(0))
// Minimal utility structs.
struct Vector4 {
	float x, y, z, w;
};

struct Matrix4x4 {
	float m00, m01, m02, m03;
	float m10, m11, m12, m13;
	float m20, m21, m22, m23;
	float m30, m31, m32, m33;
};

// Macros.
#define MAX_ATTRIB_SEARCHES 20
#define MAX_CHILD_SEARCHES 1000

struct XMLToolAttrib;

#pragma pack(push, 1)

struct BXMLFileHeader
{
	int BXML; // characters: "BXML"
	int Version;
	int NumStrings;
	int StringSize; // size in bytes of entire string heap
	int BinarySize; // size of binary value heap in bytes
	int NumAttribs;
	int NumTags;
	int EndOfFileOffset;
};

struct BXMLToolTagItem
{
	int NameStringIndex;
	int ValueIndex; // string index if type string, -1 for none, otherwise byte offset into binary heap
	int ValueLen; // if type binary, num bytes, other wise number of elements, 0 for string/none
	short ValueType;
	short unk;
	int FirstChildIndex;
	int NumChildren;
	int FirstAttribIndex;
	int NumAttribs;
};

struct BXMLToolAttribItem
{
	int NameStringIndex;
	int ValueIndex;
	unsigned short ValueLen;
	unsigned short ValueType;
};

#pragma pack(pop)

class XMLToolTag {
public:
	std::string mName;
	int mValueType;
	short unk;
	void* mpValue;
	std::string mStringValue;
	int mNumValueElements;
	std::shared_ptr<XMLToolTag> mpParent;
	std::vector<std::shared_ptr<XMLToolTag>> mpChildren;
	int mNumChildren;
	std::vector<std::shared_ptr<XMLToolAttrib>> mpAttribs;
	int mNumAttribs;


	int mChildStartIndex;
	int mAttribStartIndex;
	int mBinaryValueIndex;

	XMLToolTag();
	~XMLToolTag();
};

struct XMLToolAttrib {
	std::string mName;
	void* mpValue;
	std::string mStringValue;
	int mBinaryValueIndex;
	int mNumValueElements;
	int mValueType;

	XMLToolAttrib();
};

struct AttribSearch {
	std::shared_ptr<XMLToolTag> mpParentTag;
	int mCurrentAttribIndex;
	char* mSearchString;
};

struct ChildSearch {
	std::shared_ptr<XMLToolTag> mpParentTag;
	int mCurrentChildIndex;
	char* mSearchString;
};

class XMLTool {
private:
	int mAvailableAttribSearches[MAX_ATTRIB_SEARCHES];
	AttribSearch mAttribSearches[MAX_ATTRIB_SEARCHES];
	int mNumAvailableAttribSearches;
	int mAvailableChildSearches[MAX_CHILD_SEARCHES];
	ChildSearch mChildSearches[MAX_CHILD_SEARCHES];
	int mNumAvailableChildSearches;

	char* m_loadedFile;
	std::shared_ptr<XMLToolTag> mpRootTag;
	UniqueStringTable mUniqueStringTable;
	
public:
	XMLTool();
	~XMLTool();
	BOOL LoadFromXML(char*);
	BOOL LoadFromBXML(char*);
	int LoadFromBXML(FILE*);
	BOOL WriteXML(char*);
	BOOL WriteXML(FILE*);
	BOOL WriteBinaryXML(char*);
	BOOL WriteBinaryXML(FILE*);
	void RecursivePrintTag(std::shared_ptr<XMLToolTag>, FILE*, int);
	void Unload();
	std::shared_ptr<XMLToolTag> AddTag(std::shared_ptr<XMLToolTag>, IXMLDOMNode*, bool);
	std::shared_ptr<XMLToolTag> AddTag(std::shared_ptr<XMLToolTag>, char*);
	std::shared_ptr<XMLToolAttrib> AddAttrib(std::shared_ptr<XMLToolTag>, char*, char*);
	BOOL RemoveTag(std::shared_ptr<XMLToolTag>);
	BOOL RemoveAttrib(std::shared_ptr<XMLToolTag>, std::shared_ptr<XMLToolAttrib>);
	std::shared_ptr<XMLToolTag> GetRootTag();
	int GetChildNum(std::shared_ptr<XMLToolTag>);
	std::shared_ptr<XMLToolTag> FindFirstChild(std::shared_ptr<XMLToolTag>, char*);
	std::shared_ptr<XMLToolTag> OpenChildSearch(std::shared_ptr<XMLToolTag>, char*, int*);
	void CloseChildSearch(int childSearchID);
	std::shared_ptr<XMLToolTag> GetNextChild(int childSearchID);
	std::shared_ptr<XMLToolTag> GetPrevChild(int childSearchID);
	/*
	std::shared_ptr<XMLToolTag> SetChildFilter(std::shared_ptr<XMLToolTag>, char*);
	std::shared_ptr<XMLToolTag> GetNextChild(std::shared_ptr<XMLToolTag>);
	std::shared_ptr<XMLToolTag> GetPrevChild(std::shared_ptr<XMLToolTag>);
	*/
	BOOL IsIntValueType(std::shared_ptr<XMLToolTag>);
	BOOL IsUnsignedIntValueType(std::shared_ptr<XMLToolTag>);
	BOOL IsFloatValueType(std::shared_ptr<XMLToolTag>);
	BOOL IsDoubleValueType(std::shared_ptr<XMLToolTag>);
	BOOL IsVector4ValueType(std::shared_ptr<XMLToolTag>);
	BOOL IsMatrix44ValueType(std::shared_ptr<XMLToolTag>);
	BOOL IsBinaryValueType(std::shared_ptr<XMLToolTag>);
	BOOL IsStringValueType(std::shared_ptr<XMLToolTag>);
	BOOL IsBOOLValueType(std::shared_ptr<XMLToolTag>);

	int GetTagNameLength(std::shared_ptr<XMLToolTag>);
	BOOL GetTagName(std::shared_ptr<XMLToolTag>, char*, int);
	char* GetTagPath(std::shared_ptr<XMLToolTag>, char*, int);
	int GetTagValueSize(std::shared_ptr<XMLToolTag>);
	int GetTagValue(std::shared_ptr<XMLToolTag>, void*, int);
	BOOL GetTagValue(std::shared_ptr<XMLToolTag>, char*, int);
	BOOL GetTagValue(std::shared_ptr<XMLToolTag>, unsigned int*, int);
	BOOL GetTagValue(std::shared_ptr<XMLToolTag>, int*, int);
	BOOL GetTagValue(std::shared_ptr<XMLToolTag>, float*, int);
	BOOL GetTagValue(std::shared_ptr<XMLToolTag>, double*, int);
	BOOL GetTagValue(std::shared_ptr<XMLToolTag>, Vector4*, int);
	BOOL GetTagValue(std::shared_ptr<XMLToolTag>, Matrix4x4*, int);
	BOOL SetTagName(std::shared_ptr<XMLToolTag>, char*);
	BOOL SetTagValue(std::shared_ptr<XMLToolTag>, void*, int);
	BOOL SetTagValue(std::shared_ptr<XMLToolTag>, char*);
	BOOL SetTagValue(std::shared_ptr<XMLToolTag>, unsigned int*, int);
	BOOL SetTagValue(std::shared_ptr<XMLToolTag>, int*, int);
	BOOL SetTagValue(std::shared_ptr<XMLToolTag>, float*, int);
	BOOL SetTagValue(std::shared_ptr<XMLToolTag>, double*, int);
	BOOL SetTagValue(std::shared_ptr<XMLToolTag>, Vector4*, int);
	BOOL SetTagValue(std::shared_ptr<XMLToolTag>, Matrix4x4*, int);
	BOOL SetTagValueBOOL(std::shared_ptr<XMLToolTag>, unsigned int*, int);
	int GetAttribNum(std::shared_ptr<XMLToolTag>);
	std::shared_ptr<XMLToolAttrib> FindFirstAttrib(std::shared_ptr<XMLToolTag>, char*);
	std::shared_ptr<XMLToolAttrib> OpenAttribSearch(std::shared_ptr<XMLToolTag>, char*, int*);
	void CloseAttribSearch(int);
	std::shared_ptr<XMLToolAttrib> GetNextAttrib(int);
	std::shared_ptr<XMLToolAttrib> GetPrevAttrib(int);
	/*
	std::shared_ptr<XMLToolAttrib> SetAttribFilter(std::shared_ptr<XMLToolTag>, char*);
	std::shared_ptr<XMLToolAttrib> GetNextAttrib(std::shared_ptr<XMLToolTag>);
	std::shared_ptr<XMLToolAttrib> GetPrevAttrib(std::shared_ptr<XMLToolTag>);
	*/
	BOOL GetAttribValueAsString(std::shared_ptr<XMLToolAttrib>, char*, int);
	// std::shared_ptr<XMLToolAttrib> GetNextAttribString(int, char*, int, char*, int);
	// std::shared_ptr<XMLToolAttrib> GetPrevAttribString(int, char*, int, char*, int);
	BOOL GetAttribName(std::shared_ptr<XMLToolAttrib>, char*, int);
	char* GetAttribPath(std::shared_ptr<XMLToolTag>, std::shared_ptr<XMLToolAttrib>, char*, int);
	int GetAttribValueLength(std::shared_ptr<XMLToolAttrib>);
	int GetAttribValue(std::shared_ptr<XMLToolAttrib>, void*, int);
	BOOL GetAttribValue(std::shared_ptr<XMLToolAttrib>, char*, int);
	BOOL GetAttribValue(std::shared_ptr<XMLToolAttrib>, unsigned int*, int);
	BOOL GetAttribValue(std::shared_ptr<XMLToolAttrib>, int*, int);
	BOOL GetAttribValue(std::shared_ptr<XMLToolAttrib>, float*, int);
	BOOL GetAttribValue(std::shared_ptr<XMLToolAttrib>, double*, int);
	BOOL GetAttribValue(std::shared_ptr<XMLToolAttrib>, Vector4*, int);
	BOOL GetAttribValue(std::shared_ptr<XMLToolAttrib>, Matrix4x4*, int);
	BOOL SetAttribValue(std::shared_ptr<XMLToolAttrib>, void*, int);
	BOOL SetAttribName(std::shared_ptr<XMLToolAttrib>, char*);
	BOOL SetAttribValue(std::shared_ptr<XMLToolAttrib>, char*);
	BOOL SetAttribValue(std::shared_ptr<XMLToolAttrib>, unsigned int*, int);
	BOOL SetAttribValue(std::shared_ptr<XMLToolAttrib>, int*, int);
	BOOL SetAttribValue(std::shared_ptr<XMLToolAttrib>, float*, int);
	BOOL SetAttribValue(std::shared_ptr<XMLToolAttrib>, double*, int);
	BOOL SetAttribValue(std::shared_ptr<XMLToolAttrib>, Vector4*, int);
	BOOL SetAttribValue(std::shared_ptr<XMLToolAttrib>, Matrix4x4*, int);
	BOOL SetAttribValueBOOL(std::shared_ptr<XMLToolAttrib>, unsigned int*, int);
	void RecursiveProcessTagItem(std::shared_ptr<XMLToolTag>, int, const std::vector<BXMLToolTagItem>&, int, const std::vector<BXMLToolAttribItem>&, int, const std::vector<std::string>&, int, char*, int);
	void RecursiveBuildBinaryValueHeap(std::shared_ptr<XMLToolTag>, FILE*, int*);
	void RecursiveGetTotalBinarySize(std::shared_ptr<XMLToolTag>, int*);
	void RecursiveGetTotals(std::shared_ptr<XMLToolTag>, int*, int*);
	void RecursiveBuildStringTable(UniqueStringTable*, std::shared_ptr<XMLToolTag>);
	void RecursiveAddToGlobalLists(std::shared_ptr<XMLToolTag>, std::vector<std::shared_ptr<XMLToolTag>>&, int*, std::vector<std::shared_ptr<XMLToolAttrib>>&, int*);
};