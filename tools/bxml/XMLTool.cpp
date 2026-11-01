#include <Windows.h>
#include "XMLTool.h"
#include "malloc.h"
#include "string.h"
#include "Base64.h"
#include <algorithm>
// #include "VectorFloat.h"

/*
   BINARY XML FILE FORMAT
   Start of write:
      -BXMLFileHeader
      -Variable length null term. string * BXMLFileHeader::NumStrings (size in bytes BXMLFileHeader::StringSize)
      -byte * BXMLFileHeader::BinarySize
      -BXMLToolAttribItem * BXMLFileHeader::NumAttribs
      -BXMLToolTagItem * BXMLFileHeader::NumTags
*/

#define BXML_VERSION    (1002)
#define BXML_FILEID     (0x4C4D5842)

#define XMLVAL_NONE     (0)
#define XMLVAL_STRING   (1)
#define XMLVAL_BINARY   (2)
#define XMLVAL_INT      (3)
#define XMLVAL_UINT     (4)
#define XMLVAL_FLOAT    (5)
#define XMLVAL_DOUBLE   (6)
#define XMLVAL_VECTOR4  (7)
#define XMLVAL_MATRIX4X4 (8)
#define XMLVAL_BOOL      (11)

#define ATTRIB_SEARCH_ID_KEY  (0x80000000)
#define DECODE_ATTRIB_SEARCHID(id) (ATTRIB_SEARCH_ID_KEY ^ (id))
#define ENCODE_ATTRIB_SEARCHID(id) (ATTRIB_SEARCH_ID_KEY | (id))

#define MAX_STRING 260

//tells how many character an int takes as string
inline int CountIntChars(int number);
//counts occurances of 'countChar' in 'pString'
inline int CountChars(char *pString, char countChar);

inline BOOL ReadIntsFromString(int *pInts, int num, char *pString);
inline BOOL ReadUintsFromString(unsigned int *pUints, int num, char *pString);
inline BOOL ReadFloatsFromString(float *pFloats, int num, char *pString);
inline BOOL ReadDoublesFromString(double *pDoubles, int num, char *pString);
inline BOOL ReadVector4sFromString(Vector4 *pVector4s, int num, char *pString);
inline BOOL ReadMatrix44sFromString(Matrix4x4 *pMatrix44s, int num, char *pString);

inline BOOL WriteIntsToString(int *pInts, int num, char *pString, int stringLen);
inline BOOL WriteUintsToString(unsigned int *pUints, int num, char *pString, int stringLen);
inline BOOL WriteFloatsToString(float *pFloats, int num, char *pString, int stringLen);
inline BOOL WriteDoublesToString(double *pDoubles, int num, char *pString, int stringLen);
inline BOOL WriteVector4sToString(Vector4 *pVector4s, int num, char *pString, int stringLen);
inline BOOL WriteMatrix44sToString(Matrix4x4 *pMatrix44s, int num, char *pString, int stringLen);

inline BOOL WriteIntsToStream(int *pInts, int num, FILE *pStream);
inline BOOL WriteUintsToStream(unsigned int *pUints, int num, FILE *pStream);
inline BOOL WriteFloatsToStream(float *pFloats, int num, FILE *pStream);
inline BOOL WriteDoublesToStream(double *pDoubles, int num, FILE *pStream);
inline BOOL WriteVector4sToStream(Vector4 *pVector4s, int num, FILE *pStream);
inline BOOL WriteMatrix44sToStream(Matrix4x4 *pMatrix44s, int num, FILE *pStream);

XMLToolTag::XMLToolTag()
{
   mNumChildren = 0;
   mNumAttribs = 0;

   mName = "";
   mStringValue = "";

   mValueType = XMLVAL_NONE;
   mpValue = NULL;
   mNumValueElements = 0;

   mChildStartIndex = 0;
   mAttribStartIndex = 0;
   mBinaryValueIndex = 0;
}

XMLToolTag::~XMLToolTag()
{

   free(mpValue);
}

XMLToolAttrib::XMLToolAttrib()
{
    mName = "";
    mStringValue = "";
    mBinaryValueIndex = 0;
    mNumValueElements = 0;
    mpValue = nullptr;
    mValueType = XMLVAL_NONE;
}

XMLTool::XMLTool()
{
    m_loadedFile = nullptr;
   mpRootTag = NULL;

   for (int i = 0; i < MAX_ATTRIB_SEARCHES; i++) {
      mAvailableAttribSearches[i] = i;
      mAttribSearches[i].mpParentTag = NULL;
   }
   mNumAvailableAttribSearches = MAX_ATTRIB_SEARCHES;

   for (int i = 0; i < MAX_CHILD_SEARCHES; i++) {
      mAvailableChildSearches[i] = i;
      mChildSearches[i].mpParentTag = NULL;
   }
   mNumAvailableChildSearches = MAX_CHILD_SEARCHES;
}

XMLTool::~XMLTool()
{
}

////////////////////////////////////////////////////////////
//IO functions
////////////////////////////////////////////////////////////

BOOL XMLTool::LoadFromXML(
   char *pPath
)
{
   HRESULT hr;
   IXMLDOMDocument *pDoc = NULL;
   IXMLDOMNode *pNode;
   XMLTool outXML;

   //parse using ms
   hr = CoInitialize(NULL);
   hr = CoCreateInstance(CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, IID_IXMLDOMDocument, (void**)&pDoc);

   // strcpy(m_loadedFile, pPath);

   _variant_t names(pPath);
   // names.SetString(pPath);
   VARIANT_BOOL result;

   pDoc->load(names, &result);
   pDoc->get_firstChild(&pNode);

   //convert to XMLTool
   if (pNode != NULL) {   
      AddTag(NULL, pNode, true);
      pNode->Release();
   }

   pDoc->Release();
   CoUninitialize();

   return pNode != NULL;
}

BOOL XMLTool::LoadFromBXML(
   char *pPath
)
{
   BOOL ret;
   FILE *pInFile = fopen(pPath, "rb");
   if (pInFile == NULL) {
      return FALSE;
   }
      
   ret = LoadFromBXML(pInFile);
   fclose(pInFile);
   return ret;
}

int XMLTool::LoadFromBXML(
   FILE *pFile
) 
{
   BXMLFileHeader fileHeader;
   // BXMLToolTagItem *pTagItems = NULL;
   std::vector<BXMLToolAttribItem> pAttribItems;
   std::vector<std::string> stringPool;
   char *pStringHeap = NULL, *pBinaryHeap = NULL;
   int i, ret, numStrings;

   Unload();

   ret = (int)fread(&fileHeader, sizeof(BXMLFileHeader), 1, pFile);
   
   if (!ret) {
      return FALSE;
   }
   else if (fileHeader.BXML != BXML_FILEID || fileHeader.Version != BXML_VERSION) {
      return FALSE;
   }

   if (fileHeader.StringSize) {
      pStringHeap = (char *) malloc(fileHeader.StringSize);
      stringPool.reserve(fileHeader.NumStrings);

      ret = (int)fread(pStringHeap, fileHeader.StringSize, 1, pFile);
      
      if (!ret) {
         free(pStringHeap);
         // free(pStringTable);
         return FALSE;
      }

      //process strings
      stringPool.push_back(pStringHeap);
      numStrings = 1;

      for (i = 1; numStrings < fileHeader.NumStrings; i++) {
         if (pStringHeap[i] == NULL) {
            stringPool.push_back(&pStringHeap[i + 1]);
            numStrings++;
         }
      }

      if (numStrings != fileHeader.NumStrings) {
         free(pStringHeap);
         return FALSE;
      }
   }

   //load binary value heap
   if (fileHeader.BinarySize) {
      pBinaryHeap = (char*) malloc(fileHeader.BinarySize);
      ret = (int)fread(pBinaryHeap, fileHeader.BinarySize, 1, pFile);
      if (!ret) {
         free(pBinaryHeap);
         free(pStringHeap);
         return FALSE;
      }
   }
   else {
      pBinaryHeap = NULL;
   }

   //load attribs
   if (fileHeader.NumAttribs) {
      pAttribItems.resize(fileHeader.NumAttribs);
      ret = (int)fread(pAttribItems.data(), sizeof(BXMLToolAttribItem) * fileHeader.NumAttribs, 1, pFile);
      if (!ret) {
         free(pBinaryHeap);
         free(pStringHeap);
         return FALSE;
      }
   }

   std::cout << ftell(pFile) <<std::endl;
   //fseek(pFile, -2, SEEK_CUR);

   //load tags
   if (fileHeader.NumTags) {
      std::vector<BXMLToolTagItem> pTagItems(fileHeader.NumTags);

      ret = (int)fread(pTagItems.data(), sizeof(BXMLToolTagItem) * fileHeader.NumTags, 1, pFile);
      std::cout << ftell(pFile) << std::endl;
      if (!ret) {
         free(pStringHeap);
         return FALSE;
      }

      //process tags
      std::shared_ptr<XMLToolTag> pTag = AddTag(NULL, (char*)stringPool[pTagItems[0].NameStringIndex].c_str());
      RecursiveProcessTagItem(pTag, 0, pTagItems, fileHeader.NumTags, pAttribItems, fileHeader.NumAttribs, stringPool, fileHeader.NumStrings, pBinaryHeap, fileHeader.BinarySize);
   }

   printf("%d", ftell(pFile));

   free(pBinaryHeap);
   free(pStringHeap);
   return TRUE;
}

BOOL XMLTool::WriteXML(
   char *pFile
)
{
   FILE *pOutFile;
   BOOL ret;
   pOutFile = fopen(pFile, "wt");
   if (pOutFile == NULL) {
      return FALSE;
   }

   ret = WriteXML(pOutFile);
   fclose(pOutFile);
   return ret;
}

BOOL XMLTool::WriteXML(
   FILE * pOutFile
)
{
   if (mpRootTag != NULL) {
      RecursivePrintTag(mpRootTag, pOutFile, 0);
   }
   return TRUE;
}

BOOL XMLTool::WriteBinaryXML(
   char *pFile
)
{
   FILE *pOutFile;
   BOOL ret;
   pOutFile = fopen(pFile, "wb");
   if (pOutFile == NULL) {
       return FALSE;
   }

   ret = WriteBinaryXML(pOutFile);
   fclose(pOutFile);
   return ret;
}

BOOL XMLTool::WriteBinaryXML(
   FILE * pOutFile
)
{
   BXMLFileHeader fileHeader;
   BXMLToolTagItem tagItem;
   BXMLToolAttribItem attribItem;

   long topOfFile;

   
   int globalTagListNum;
   int globalAttribListNum;

   topOfFile = ftell(pOutFile);

   fileHeader.BXML = BXML_FILEID;//"BXML"
   fileHeader.Version = BXML_VERSION;

   //build string list
   fileHeader.NumStrings = mUniqueStringTable.GetStringNum();
   //get string heap size
   fileHeader.StringSize = 0;
   for (int i = 0; i < fileHeader.NumStrings; i++) {
      fileHeader.StringSize += mUniqueStringTable.GetString(i).size() + 1;
   }
   
   //get binary size
   fileHeader.BinarySize = 0;
   RecursiveGetTotalBinarySize(mpRootTag, &fileHeader.BinarySize);

   //build global lists
   fileHeader.NumAttribs = 0;
   fileHeader.NumTags = 0;
   RecursiveGetTotals(mpRootTag, &fileHeader.NumTags, &fileHeader.NumAttribs);
   
   //build end of file offset
   fileHeader.EndOfFileOffset = sizeof(BXMLFileHeader) + fileHeader.StringSize;//pass string
   fileHeader.EndOfFileOffset += fileHeader.BinarySize;//pass binary
   fileHeader.EndOfFileOffset += (fileHeader.NumAttribs * sizeof(BXMLToolAttribItem));//pass attribs
   fileHeader.EndOfFileOffset += sizeof(BXMLToolTagItem) * fileHeader.NumTags;//pass tags

   std::vector<std::shared_ptr<XMLToolTag>> pGlobalTagList(fileHeader.NumTags);
   globalTagListNum = 0;
   std::vector<std::shared_ptr<XMLToolAttrib>> pGlobalAttribList(fileHeader.NumAttribs);
   globalAttribListNum = 0;
   
   pGlobalTagList[0] = mpRootTag;
   globalTagListNum = 1;
   this->RecursiveAddToGlobalLists(mpRootTag, pGlobalTagList, &globalTagListNum, pGlobalAttribList, &globalAttribListNum);

   //start writing file
   fwrite((void*) &fileHeader,sizeof(BXMLFileHeader), 1, pOutFile);

   //write strings
   for (int i = 0; i < fileHeader.NumStrings; i++) {
       std::uint8_t nul = 0;
       fwrite((void*)mUniqueStringTable.GetString(i).c_str(), mUniqueStringTable.GetString(i).size(), 1, pOutFile);
       fwrite(reinterpret_cast<void*>(&nul), 1, 1, pOutFile);
   }
   
   //write binary value heap
   int tmp = 0;
   RecursiveBuildBinaryValueHeap(mpRootTag, pOutFile, &tmp);

   //write attribs
   for (int i = 0; i < fileHeader.NumAttribs; i++) {
      attribItem.NameStringIndex = mUniqueStringTable.LookUpString(pGlobalAttribList[i]->mName);
      attribItem.ValueType = pGlobalAttribList[i]->mValueType;

      if (pGlobalAttribList[i]->mValueType == XMLVAL_STRING) {
          attribItem.ValueIndex = mUniqueStringTable.LookUpString(pGlobalAttribList[i]->mStringValue);
          attribItem.ValueLen = 0;
      }
      else if (pGlobalAttribList[i]->mValueType == XMLVAL_NONE) {
          attribItem.ValueIndex = -1;
          attribItem.ValueLen = 0;
      }
      else {
          //all other types, bin, int, uint, float, double, vector4, matrix4x4
          attribItem.ValueIndex = pGlobalAttribList[i]->mBinaryValueIndex;
          attribItem.ValueLen = pGlobalAttribList[i]->mNumValueElements;
      }

      fwrite((void*)&attribItem, sizeof(BXMLToolAttribItem), 1, pOutFile);
   }

   //write tags
   for (int i = 0; i < fileHeader.NumTags; i++) {
      tagItem.NameStringIndex = mUniqueStringTable.LookUpString(pGlobalTagList[i]->mName);

      tagItem.ValueType = pGlobalTagList[i]->mValueType;
      tagItem.unk = 0;
      if(pGlobalTagList[i]->mValueType == XMLVAL_STRING) {
         tagItem.ValueIndex = mUniqueStringTable.LookUpString(pGlobalTagList[i]->mStringValue);
         tagItem.ValueLen = 0;
      }
      else if (pGlobalTagList[i]->mValueType == XMLVAL_NONE) {
         tagItem.ValueIndex = -1;
         tagItem.ValueLen = 0;
      }
      else {
         //all other types, bin, int, uint, float, double, vector4, matrix4x4
         tagItem.ValueIndex = pGlobalTagList[i]->mBinaryValueIndex;
         tagItem.ValueLen = pGlobalTagList[i]->mNumValueElements;
      }

      tagItem.NumChildren = pGlobalTagList[i]->mNumChildren;
      tagItem.FirstChildIndex = pGlobalTagList[i]->mChildStartIndex;
      tagItem.NumAttribs = pGlobalTagList[i]->mNumAttribs;
      tagItem.FirstAttribIndex = pGlobalTagList[i]->mAttribStartIndex;

      fwrite((void*) &tagItem, sizeof(BXMLToolTagItem), 1, pOutFile);
   }
   
   return TRUE;//(int)(ftell(pOutFile) - topOfFile);//return bytes written
}

inline void PrintTabs(FILE *pOut, int tabNum)
{
   for (; tabNum >0; tabNum--) {
      fprintf(pOut,"   ");
   }
}

void XMLTool::RecursivePrintTag(
   std::shared_ptr<XMLToolTag>pTag,
   FILE * pFileOut,
   int tabDepth
)
{
   PrintTabs(pFileOut, tabDepth);

   fprintf(pFileOut, "<%s", pTag->mName.c_str());

   for (int i = 0; i < pTag->mNumAttribs; i++) {
       char maxAttribValue[260];
       GetAttribValueAsString(pTag->mpAttribs[i], maxAttribValue, 260);
       fprintf(pFileOut, " %s=\"%s\"", pTag->mpAttribs[i]->mName.c_str(), maxAttribValue);

       int vType = pTag->mpAttribs[i]->mValueType;
       if (vType != XMLVAL_STRING && vType != XMLVAL_NONE) {
           const char* tName = "";
           if (vType == XMLVAL_BINARY) tName = "binary";
           else if (vType == XMLVAL_INT) tName = "int";
           else if (vType == XMLVAL_UINT) tName = "uint";
           else if (vType == XMLVAL_FLOAT) tName = "float";
           else if (vType == XMLVAL_DOUBLE) tName = "double";
           else if (vType == XMLVAL_VECTOR4) tName = "vector4";
           else if (vType == XMLVAL_MATRIX4X4) tName = "matrix4x4";
           else if (vType == XMLVAL_BOOL) tName = "bool";

           if (tName[0] != '\0') {
               fprintf(pFileOut, " _attrtype_%s=\"%s\"", pTag->mpAttribs[i]->mName.c_str(), tName);
           }
       }
   }

   if (pTag->mNumChildren || pTag->mValueType != XMLVAL_NONE) {
      switch (pTag->mValueType) {
      case XMLVAL_BINARY:
         //add an extra attrib to identify binary data
         fprintf(pFileOut, " _valuetype=\"binary\"");
         break;
      case XMLVAL_INT:
         //add an extra attrib to identify binary data
         fprintf(pFileOut, " _valuetype=\"int\"");
         break;
      case XMLVAL_UINT:
         //add an extra attrib to identify binary data
         fprintf(pFileOut, " _valuetype=\"uint\"");
         break;
      case XMLVAL_FLOAT:
         //add an extra attrib to identify binary data
         fprintf(pFileOut, " _valuetype=\"float\"");
         break;
      case XMLVAL_DOUBLE:
         //add an extra attrib to identify binary data
         fprintf(pFileOut, " _valuetype=\"double\"");
         break;
      case XMLVAL_VECTOR4:
         //add an extra attrib to identify binary data
         fprintf(pFileOut, " _valuetype=\"vector4\"");
         break;
      case XMLVAL_MATRIX4X4:
         //add an extra attrib to identify binary data
         fprintf(pFileOut, " _valuetype=\"matrix4x4\"");
         break;
      case XMLVAL_BOOL:
          //add an extra attrib to identify binary data
          fprintf(pFileOut, " _valuetype=\"bool\"");
          break;
      }
      fprintf(pFileOut, ">\n");

      switch (pTag->mValueType) {
      case XMLVAL_STRING:
         PrintTabs(pFileOut, tabDepth+1);
         fprintf(pFileOut, "%s\n", pTag->mStringValue.c_str());
         break;
      case XMLVAL_BINARY:
         {
            char *pTmp;
            //account for NULL
            int size = 1 + GETCONVSIZEFROM256(pTag->mNumValueElements);
            pTmp = (char*)malloc(size);
            ConvertBinToBase64ASCII(pTag->mpValue, pTag->mNumValueElements, pTmp, size);
    
            PrintTabs(pFileOut, tabDepth+1);
            fprintf(pFileOut, "%s\n", pTmp);
            free(pTmp);
         }
         break;
      case XMLVAL_INT:
         PrintTabs(pFileOut, tabDepth+1);
         WriteIntsToStream((int*)pTag->mpValue, pTag->mNumValueElements, pFileOut);
         fprintf(pFileOut, "\n");
         break;
      case XMLVAL_UINT:
         PrintTabs(pFileOut, tabDepth+1);
         WriteUintsToStream((unsigned int*)pTag->mpValue, pTag->mNumValueElements, pFileOut);
         fprintf(pFileOut, "\n");
         break;
      case XMLVAL_FLOAT:
         PrintTabs(pFileOut, tabDepth+1);
         WriteFloatsToStream((float*)pTag->mpValue, pTag->mNumValueElements, pFileOut);
         fprintf(pFileOut, "\n");
         break;
      case XMLVAL_DOUBLE:
         PrintTabs(pFileOut, tabDepth+1);
         WriteDoublesToStream((double*)pTag->mpValue, pTag->mNumValueElements, pFileOut);
         fprintf(pFileOut, "\n");
         break;
      case XMLVAL_VECTOR4:
         PrintTabs(pFileOut, tabDepth+1);
         WriteVector4sToStream((Vector4*)pTag->mpValue, pTag->mNumValueElements, pFileOut);
         fprintf(pFileOut, "\n");
         break;
      case XMLVAL_MATRIX4X4:
         PrintTabs(pFileOut, tabDepth+1);
         WriteMatrix44sToStream((Matrix4x4*)pTag->mpValue, pTag->mNumValueElements, pFileOut);
         fprintf(pFileOut, "\n");
         break;
      case XMLVAL_BOOL:
          PrintTabs(pFileOut, tabDepth + 1);
          WriteUintsToStream((unsigned int*)pTag->mpValue, pTag->mNumValueElements, pFileOut);
          fprintf(pFileOut, "\n");
          break;
      }

      for (int i = 0; i < pTag->mNumChildren; i++) {
         RecursivePrintTag(pTag->mpChildren[i], pFileOut, tabDepth+1);
      }
      
      PrintTabs(pFileOut, tabDepth);
      fprintf(pFileOut, "</%s>\n", pTag->mName.c_str());
   }
   else {
      fprintf(pFileOut, "/>\n");
   }
}

void XMLTool::Unload()
{
   if (mpRootTag) {
      RemoveTag(mpRootTag);
      mpRootTag = NULL;
   }
}

////////////////////////////////////////////////////////////
//XML creation functions
////////////////////////////////////////////////////////////

std::shared_ptr<XMLToolTag>XMLTool::AddTag(
   std::shared_ptr<XMLToolTag>pParent,
   IXMLDOMNode *pDOMNode,
   bool recursive /*= true*/
   )
{
   BSTR tmpStr;
   VARIANT value;
   VariantInit(&value);
   DOMNodeType type;
   IXMLDOMNode *pDOMChild;
   IXMLDOMNamedNodeMap *pAttribMap;
   int valueType = XMLVAL_STRING;
   char nameStr[MAX_STRING], valueStr[MAX_STRING];
   std::shared_ptr<XMLToolTag> pChild;

   //already has a root node
   if (pParent == NULL && mpRootTag != NULL) {
      return NULL;
   }

   //get new tag's name
   pDOMNode->get_baseName(&tmpStr);
   _snprintf(nameStr, MAX_STRING, "%S", tmpStr);//convert from wide character set
   
   pChild = AddTag(pParent, nameStr);

   //get tag's attribs
   //get tag's attribs
   pDOMNode->get_attributes(&pAttribMap);
   if (pAttribMap != NULL) {
       long attribNum, i;
       IXMLDOMNode* pAttribNode;
       pAttribMap->get_length(&attribNum);

       std::vector<std::pair<std::string, std::string>> pendingAttribs;
       std::vector<std::pair<std::string, std::string>> pendingAttribTypes;

       for (int i = 0; i < attribNum; i++) {
           pAttribMap->get_item(i, &pAttribNode);
           if (pAttribNode != NULL) {
               pAttribNode->get_baseName(&tmpStr);
               VariantClear(&value);
               pAttribNode->get_nodeTypedValue(&value);

               _snprintf(nameStr, MAX_STRING, "%S", tmpStr);
               _snprintf(valueStr, MAX_STRING, "%S", value.bstrVal);

               //check for system attrib
               if (nameStr[0] != '_') {
                   pendingAttribs.push_back({ nameStr, valueStr });
               }
               //now handle system attrib
               else if (!stricmp(nameStr, "_valuetype")) {
                   if (!stricmp(valueStr, "binary")) valueType = XMLVAL_BINARY;
                   else if (!stricmp(valueStr, "int")) valueType = XMLVAL_INT;
                   else if (!stricmp(valueStr, "uint")) valueType = XMLVAL_UINT;
                   else if (!stricmp(valueStr, "float")) valueType = XMLVAL_FLOAT;
                   else if (!stricmp(valueStr, "double")) valueType = XMLVAL_DOUBLE;
                   else if (!stricmp(valueStr, "vector4")) valueType = XMLVAL_VECTOR4;
                   else if (!stricmp(valueStr, "matrix4x4")) valueType = XMLVAL_MATRIX4X4;
                   else if (!stricmp(valueStr, "bool")) valueType = XMLVAL_BOOL;
               }
               // collect attribute type metadata
               else if (!strncmp(nameStr, "_attrtype_", 10)) {
                   pendingAttribTypes.push_back({ nameStr + 10, valueStr });
               }

               pAttribNode->Release();
           }
           else {//stop attrib list and correct attrib num
               break;
           }
       }

       // Process and restore attributes to binary types
       for (const auto& attr : pendingAttribs) {
           auto pAttrib = AddAttrib(pChild, (char*)attr.first.c_str(), (char*)attr.second.c_str());

           for (const auto& typeAttr : pendingAttribTypes) {
               if (typeAttr.first == attr.first) {
                   const char* typeStr = typeAttr.second.c_str();
                   char* pValueStr = (char*)attr.second.c_str();

                   int attrValueType = XMLVAL_STRING;
                   if (!stricmp(typeStr, "binary")) attrValueType = XMLVAL_BINARY;
                   else if (!stricmp(typeStr, "int")) attrValueType = XMLVAL_INT;
                   else if (!stricmp(typeStr, "uint")) attrValueType = XMLVAL_UINT;
                   else if (!stricmp(typeStr, "float")) attrValueType = XMLVAL_FLOAT;
                   else if (!stricmp(typeStr, "double")) attrValueType = XMLVAL_DOUBLE;
                   else if (!stricmp(typeStr, "vector4")) attrValueType = XMLVAL_VECTOR4;
                   else if (!stricmp(typeStr, "matrix4x4")) attrValueType = XMLVAL_MATRIX4X4;
                   else if (!stricmp(typeStr, "bool")) attrValueType = XMLVAL_BOOL;

                   if (attrValueType != XMLVAL_STRING) {
                       int numElements = CountChars(pValueStr, ',') + 1;
                       if (attrValueType == XMLVAL_INT) {
                           int* pData = (int*)malloc(sizeof(int) * numElements);
                           ReadIntsFromString(pData, numElements, pValueStr);
                           SetAttribValue(pAttrib, pData, numElements);
                           free(pData);
                       }
                       else if (attrValueType == XMLVAL_UINT) {
                           unsigned int* pData = (unsigned int*)malloc(sizeof(unsigned int) * numElements);
                           ReadUintsFromString(pData, numElements, pValueStr);
                           SetAttribValue(pAttrib, pData, numElements);
                           free(pData);
                       }
                       else if (attrValueType == XMLVAL_FLOAT) {
                           float* pData = (float*)malloc(sizeof(float) * numElements);
                           ReadFloatsFromString(pData, numElements, pValueStr);
                           SetAttribValue(pAttrib, pData, numElements);
                           free(pData);
                       }
                       else if (attrValueType == XMLVAL_DOUBLE) {
                           double* pData = (double*)malloc(sizeof(double) * numElements);
                           ReadDoublesFromString(pData, numElements, pValueStr);
                           SetAttribValue(pAttrib, pData, numElements);
                           free(pData);
                       }
                       else if (attrValueType == XMLVAL_VECTOR4) {
                           numElements = numElements / 4;
                           if (numElements > 0) {
                               Vector4* pData = (Vector4*)malloc(sizeof(Vector4) * numElements);
                               ReadVector4sFromString(pData, numElements, pValueStr);
                               SetAttribValue(pAttrib, pData, numElements);
                               free(pData);
                           }
                       }
                       else if (attrValueType == XMLVAL_MATRIX4X4) {
                           numElements = numElements / 16;
                           if (numElements > 0) {
                               Matrix4x4* pData = (Matrix4x4*)malloc(sizeof(Matrix4x4) * numElements);
                               ReadMatrix44sFromString(pData, numElements, pValueStr);
                               SetAttribValue(pAttrib, pData, numElements);
                               free(pData);
                           }
                       }
                       else if (attrValueType == XMLVAL_BOOL) {
                           unsigned int* pData = (unsigned int*)malloc(sizeof(unsigned int) * numElements);
                           ReadUintsFromString(pData, numElements, pValueStr);
                           SetAttribValueBOOL(pAttrib, pData, numElements);
                           free(pData);
                       }
                       else if (attrValueType == XMLVAL_BINARY) {
                           int outSize = GETCONVSIZEFROM64((int)strlen(pValueStr));
                           void* pData = malloc(outSize);
                           ConvertBase64ASCIIToBin(pValueStr, (int)strlen(pValueStr), pData, outSize);
                           SetAttribValue(pAttrib, pData, outSize);
                           free(pData);
                       }
                   }
                   break; // Move on to next attribute
               }
           }
       }
       pAttribMap->Release();
   }

   //check if first child is a value node
   pDOMNode->get_firstChild(&pDOMChild);
   if (pDOMChild != NULL) {
      pDOMChild->get_nodeType(&type);
      //if it's the tag's value
      if (type != NODE_ELEMENT) {
         VariantClear(&value);
         pDOMChild->get_nodeTypedValue(&value);
         //string value
         
         char *pValueStr;
         char *pFullValueStr = new char[wcslen(value.bstrVal)+1];

         sprintf(pFullValueStr, "%S", value.bstrVal);//convert from wide character set
         //remove leading whitespace
         for (pValueStr = pFullValueStr; (*pValueStr) == '\n' || (*pValueStr) == '\r' || (*pValueStr) == '\t' || (*pValueStr) == ' '; pValueStr++);
         
         if ((*pValueStr) != NULL) {
            //remove trailing whitespace             
            char *pEnd, *pLast;
            //keep track of last known non-whitespace character in pLast
            pLast = pValueStr;
            for (pEnd = pValueStr; (*pEnd) != NULL; pEnd++) {
               if ((*pEnd) != '\n' && (*pEnd) != '\r' && (*pEnd) != '\t' && (*pEnd) != ' ') {
                  pLast = pEnd;
               }
            }

            //now terminate after last
            pLast++;
            (*pLast) = NULL;
            
            //store value now
            switch (valueType) {
            case XMLVAL_STRING:
               SetTagValue(pChild, pValueStr);
               break;
            case XMLVAL_BINARY:
               {
                  pChild->mValueType = valueType;
                  pChild->mNumValueElements = GETCONVSIZEFROM64((int)strlen(pValueStr));
                  pChild->mpValue = malloc(pChild->mNumValueElements);
                  ConvertBase64ASCIIToBin(pValueStr, (int)strlen(pValueStr), pChild->mpValue, pChild->mNumValueElements);
               }
               break;
            case XMLVAL_INT:
               {
                  pChild->mValueType = valueType;
                  //get number of elements
                  pChild->mNumValueElements = CountChars(pValueStr, ',') + 1;
                  pChild->mpValue = malloc(sizeof(int) * pChild->mNumValueElements);
                  ReadIntsFromString((int*) pChild->mpValue, pChild->mNumValueElements, pValueStr);
               }
               break;
            case XMLVAL_UINT:
               {
                  pChild->mValueType = valueType;
                  //get number of elements
                  pChild->mNumValueElements = CountChars(pValueStr, ',') + 1;
                  pChild->mpValue = malloc(sizeof(unsigned int) * pChild->mNumValueElements);
                  ReadUintsFromString((unsigned int*) pChild->mpValue, pChild->mNumValueElements, pValueStr);
               }
               break;
            case XMLVAL_FLOAT:
               {
                  pChild->mValueType = valueType;
                  //get number of elements
                  pChild->mNumValueElements = CountChars(pValueStr, ',') + 1;
                  pChild->mpValue = malloc(sizeof(float) * pChild->mNumValueElements);
                  ReadFloatsFromString((float*) pChild->mpValue, pChild->mNumValueElements, pValueStr);               
               }
               break;
            case XMLVAL_DOUBLE:
               {
                  pChild->mValueType = valueType;
                  //get number of elements
                  pChild->mNumValueElements = CountChars(pValueStr, ',') + 1;
                  pChild->mpValue = malloc(sizeof(double) * pChild->mNumValueElements);
                  ReadDoublesFromString((double*) pChild->mpValue, pChild->mNumValueElements, pValueStr);
               }
               break;
            case XMLVAL_VECTOR4:
               {
                  pChild->mValueType = valueType;
                  //get number of elements
                  pChild->mNumValueElements = (CountChars(pValueStr, ',') + 1) /4;
                  pChild->mpValue = malloc(sizeof(Vector4) * pChild->mNumValueElements);
                  ReadVector4sFromString((Vector4*) pChild->mpValue, pChild->mNumValueElements, pValueStr);
               }
               break;
            case XMLVAL_MATRIX4X4:
               {
                  pChild->mValueType = valueType;
                  //get number of elements
                  pChild->mNumValueElements = (CountChars(pValueStr, ',') + 1) /16;
                  pChild->mpValue = malloc(sizeof(Matrix4x4) * pChild->mNumValueElements);
                  ReadMatrix44sFromString((Matrix4x4*) pChild->mpValue, pChild->mNumValueElements, pValueStr);
               }
               break;
            case XMLVAL_BOOL:
            {
                pChild->mValueType = valueType;
                //get number of elements
                pChild->mNumValueElements = CountChars(pValueStr, ',') + 1;
                pChild->mpValue = malloc(sizeof(unsigned int) * pChild->mNumValueElements);
                ReadUintsFromString((unsigned int*)pChild->mpValue, pChild->mNumValueElements, pValueStr);
            }
            break;
            }

         }
      
         delete [] pFullValueStr;
      }
      pDOMChild->Release();
   }

   //get all sub tags if requested
   if (recursive) {
      pDOMNode->get_firstChild(&pDOMChild);
      while (pDOMChild != NULL) {
         IXMLDOMNode * pOld;
         pDOMChild->get_nodeType(&type);

         if (type == NODE_ELEMENT) {
            AddTag(pChild, pDOMChild, true);
         }

         pOld = pDOMChild;
         pOld->get_nextSibling(&pDOMChild);
         pOld->Release();
      }
   }
   return pChild;   
}

std::shared_ptr<XMLToolTag>XMLTool::AddTag(
   std::shared_ptr<XMLToolTag>pParent,
   char *pName
)
{   
   if (pParent == NULL) {
      //already a root tag
      if (mpRootTag != NULL) {
         return NULL;
      }

      mpRootTag = std::make_shared<XMLToolTag>();
      
      mpRootTag->mValueType = XMLVAL_NONE;
      SetTagName(mpRootTag, pName);
      return mpRootTag;
   }

   pParent->mpChildren.emplace_back(std::make_shared<XMLToolTag>());
   pParent->mNumChildren++;
   
   //strncpy(pParent->mpChildren[pParent->mNumChildren-1]->mName, pName, MAX_STRING);
   pParent->mpChildren[pParent->mNumChildren-1]->mValueType = XMLVAL_NONE;
   pParent->mpChildren[pParent->mNumChildren-1]->mpParent = pParent;
   SetTagName(pParent->mpChildren[pParent->mNumChildren-1], pName);

   return pParent->mpChildren[pParent->mNumChildren-1];
}

std::shared_ptr<XMLToolAttrib>XMLTool::AddAttrib(
   std::shared_ptr<XMLToolTag>pParent,
   char *pName,
   char *pValue
)
{
   pParent->mNumAttribs++;
   pParent->mpAttribs.emplace_back(std::make_shared<XMLToolAttrib>());

   //strncpy(pParent->mpChildren[pParent->mNumChildren-1]->mName, pName, MAX_STRING);
   pParent->mpAttribs[pParent->mNumAttribs-1]->mName = "";
   pParent->mpAttribs[pParent->mNumAttribs - 1]->mStringValue = "";

   SetAttribName(pParent->mpAttribs[pParent->mNumAttribs-1], pName);
   SetAttribValue(pParent->mpAttribs[pParent->mNumAttribs-1], pValue);

   return pParent->mpAttribs[pParent->mNumAttribs-1];
}

//NOTE: this function will reset the current child filter to NULL on pTag's parent
BOOL XMLTool::RemoveTag(
   std::shared_ptr<XMLToolTag> pTag
)
{
   std::shared_ptr<XMLToolTag>pParent = pTag->mpParent;

   if (!pParent) {
      mpRootTag = NULL;
      pTag.~shared_ptr();
      return TRUE;
   }

   int foundIndex = pParent->mNumChildren;

   for (int i = 0; i < pParent->mNumChildren; i++) {
      if (pParent->mpChildren[i] == pTag) {
          foundIndex = i;
          break;
      }
   }

   //pTag not found
   if (foundIndex == pParent->mNumChildren) {
      return FALSE;
   }

   // to detach pTag from pParent swap pTag in pParent's child list with the last entry
   pParent->mpChildren[foundIndex] = pParent->mpChildren[pParent->mNumChildren-1];
   pParent->mNumChildren--;

   //reset any search for the parent's children
   for (int i = 0; i < MAX_CHILD_SEARCHES; i++) {
      if (mChildSearches[i].mpParentTag == pParent) {
         mChildSearches[i].mCurrentChildIndex = -1;
      }
   }

   pTag.~shared_ptr();
   return TRUE;
}

//NOTE: this function will reset the current attrib filter to NULL on pAttrib's parent
BOOL XMLTool::RemoveAttrib(
   std::shared_ptr<XMLToolTag>pTag,
   std::shared_ptr<XMLToolAttrib>pAttrib
)
{
    int foundIndex = pTag->mNumAttribs;

   for (int i = 0; i < pTag->mNumAttribs; i++) {
      if (pTag->mpAttribs[i] == pAttrib) {
          foundIndex = i;
          break;
      }
   }

   //pAttrib not found
   if (foundIndex == pTag->mNumAttribs) {
      return FALSE;
   }

   //to detach pAttrib from pTag swap pAttrib in pParent's attrib list with the last entry
   pTag->mpAttribs[foundIndex] = pTag->mpAttribs[pTag->mNumAttribs-1];
   pTag->mNumAttribs--;

   //reset any search for this tag's attribs
   for (int i = 0; i < MAX_ATTRIB_SEARCHES; i++) {
      if (mAttribSearches[i].mpParentTag == pTag) {
         mAttribSearches[i].mCurrentAttribIndex = -1;
      }
   }

   pAttrib.~shared_ptr();
   return TRUE;
}

////////////////////////////////////////////////////////////
//Tag enumeration functions
////////////////////////////////////////////////////////////

std::shared_ptr<XMLToolTag>XMLTool::GetRootTag()
{
   return mpRootTag;
}

int XMLTool::GetChildNum(
   std::shared_ptr<XMLToolTag>pTag
)
{
   return pTag->mNumChildren;
}

std::shared_ptr<XMLToolTag>XMLTool::FindFirstChild(
   std::shared_ptr<XMLToolTag>pParent,
   char *pName
)
{
   if(!pParent)
      return NULL;
   
   if (!pParent->mNumChildren) {
      return NULL;
   }

   if (pName != NULL) {
      for (int i = 0; i < pParent->mNumChildren; i++) {
         if (pParent->mpChildren[i]->mName == pName) {
            return pParent->mpChildren[i];
         }
      }
      return NULL;
   }
   else {
      return pParent->mpChildren[0];
   }
}

std::shared_ptr<XMLToolTag> XMLTool::OpenChildSearch(
   std::shared_ptr<XMLToolTag>pTag,
   char *pString,
   int *pOutChildSearchID
)
{
   int searchID;
   std::shared_ptr<XMLToolTag>pChildTag;
   DEBUG_ASSERT(mNumAvailableChildSearches > 0, ("XMLTool::OpenChildSearch() - No more available child searches."), FALSE);
   
   //get an unused child search
   searchID = mAvailableChildSearches[mNumAvailableChildSearches-1];

   mChildSearches[searchID].mCurrentChildIndex = -1;
   mChildSearches[searchID].mpParentTag = pTag;

   if (pString != NULL) {
      strcpy(mChildSearches[searchID].mSearchString, pString);
   }
   else {
      mChildSearches[searchID].mSearchString[0] = NULL;
   }

   if ((pChildTag = GetNextChild(searchID)) == NULL) {
      mChildSearches[searchID].mpParentTag = NULL;
      (*pOutChildSearchID) = -1;
      return NULL;
   }
   
   //remove the child search from the available list
   mNumAvailableChildSearches--;
   (*pOutChildSearchID) = searchID;
   return pChildTag;
}

void XMLTool::CloseChildSearch(
   int childSearchID
)
{
   if (childSearchID == -1) {
      return;
   }
   DEBUG_ASSERT(childSearchID >= 0 && childSearchID < MAX_CHILD_SEARCHES, ("XMLTool::GetNextChild() - Invalid searchID."), FALSE);
   DEBUG_ASSERT(mNumAvailableChildSearches < MAX_CHILD_SEARCHES, ("XMLTool::GetNextChild() - All searches are already available.", childSearchID), FALSE);   
   DEBUG_ASSERT(mChildSearches[childSearchID].mpParentTag != NULL, ("XMLTool::GetNextChild() - Child search at index %i is already closed.", childSearchID), FALSE);

   //return this index to the list of available
   mChildSearches[childSearchID].mpParentTag = NULL;
   mAvailableChildSearches[mNumAvailableChildSearches] = childSearchID;
   mNumAvailableChildSearches++;
}

std::shared_ptr<XMLToolTag> XMLTool::GetNextChild(
   int childSearchID
)
{
   DEBUG_ASSERT(childSearchID >= 0 && childSearchID < MAX_CHILD_SEARCHES, ("XMLTool::GetNextChild() - Invalid searchID."), FALSE);

   if (mChildSearches[childSearchID].mCurrentChildIndex >= mChildSearches[childSearchID].mpParentTag->mNumChildren - 1) {
      mChildSearches[childSearchID].mCurrentChildIndex = mChildSearches[childSearchID].mpParentTag->mNumChildren;
      //at the end of the list
      return NULL;
   }
   else if (mChildSearches[childSearchID].mSearchString[0] == NULL) {
      return mChildSearches[childSearchID].mpParentTag->mpChildren[++mChildSearches[childSearchID].mCurrentChildIndex];
   }

   int i, num;
   char *pString;
   pString = mChildSearches[childSearchID].mSearchString;
   num = mChildSearches[childSearchID].mpParentTag->mNumChildren;

   for (i = mChildSearches[childSearchID].mCurrentChildIndex+1; i < num; i++) {
      if (pString == mChildSearches[childSearchID].mpParentTag->mpChildren[i]->mName) {
         mChildSearches[childSearchID].mCurrentChildIndex = i;
         return mChildSearches[childSearchID].mpParentTag->mpChildren[i];
      }
   }

   mChildSearches[childSearchID].mCurrentChildIndex = i;
   return NULL;
}

std::shared_ptr<XMLToolTag> XMLTool::GetPrevChild(
   int childSearchID
)
{
   DEBUG_ASSERT(childSearchID >= 0 && childSearchID < MAX_CHILD_SEARCHES, ("XMLTool::GetNextChild() - Invalid searchID."), FALSE);

   if (mChildSearches[childSearchID].mCurrentChildIndex <= 0) {
      mChildSearches[childSearchID].mCurrentChildIndex = -1;
      //at the front of the list
      return NULL;
   }
   else if (mChildSearches[childSearchID].mSearchString[0] == NULL) {
      return mChildSearches[childSearchID].mpParentTag->mpChildren[--mChildSearches[childSearchID].mCurrentChildIndex];
   }

   char *pString;
   pString = mChildSearches[childSearchID].mSearchString;

   for (int i = mChildSearches[childSearchID].mCurrentChildIndex-1; i >= 0; i--) {
      if (pString == mChildSearches[childSearchID].mpParentTag->mpChildren[i]->mName) {
         mChildSearches[childSearchID].mCurrentChildIndex = i;
         return mChildSearches[childSearchID].mpParentTag->mpChildren[i];
      }
   }

   mChildSearches[childSearchID].mCurrentChildIndex = -1;
   return NULL;
}


/*std::shared_ptr<XMLToolTag>XMLTool::SetChildFilter(
   std::shared_ptr<XMLToolTag>pTag,
   char *pString
)
{
   if (pString) {
      strcpy(pTag->mChildFilter, pString);
   }
   else {
      pTag->mChildFilter[0] = NULL;
   }

   pTag->mCurrentChild = -1;
   return GetNextChild(pTag);
}

std::shared_ptr<XMLToolTag>XMLTool::GetNextChild(
   std::shared_ptr<XMLToolTag>pParentTag
   )
{
   if (pParentTag->mCurrentChild >= pParentTag->mNumChildren-1) {
      return NULL;
   }
   
   pParentTag->mCurrentChild++;
   if (pParentTag->mChildFilter[0]) {
      while (stricmp(pParentTag->mChildFilter, pParentTag->mpChildren[pParentTag->mCurrentChild]->mName)) {
         if (pParentTag->mCurrentChild >= pParentTag->mNumChildren-1) {
            return NULL;
         }
         pParentTag->mCurrentChild++;
      }
   }

   return pParentTag->mpChildren[pParentTag->mCurrentChild];
}

std::shared_ptr<XMLToolTag>XMLTool::GetPrevChild(
   std::shared_ptr<XMLToolTag>pParentTag
   )
{
   if (pParentTag->mCurrentChild <= 0) {
      return NULL;
   }
   
   pParentTag->mCurrentChild--;
   if (pParentTag->mChildFilter[0]) {
      while (stricmp(pParentTag->mChildFilter, pParentTag->mpChildren[pParentTag->mCurrentChild]->mName)) {
         if (pParentTag->mCurrentChild <= 0) {
            return NULL;
         }
         pParentTag->mCurrentChild--;
      }
   }

   return pParentTag->mpChildren[pParentTag->mCurrentChild];
}*/

////////////////////////////////////////////////////////////
// Functions for getting and setting a tag's name and value
////////////////////////////////////////////////////////////

BOOL XMLTool::IsIntValueType(
   std::shared_ptr<XMLToolTag>pTag
)
{
   if (pTag->mValueType == XMLVAL_INT) {
      return TRUE;
   }
   else {
      return FALSE;
   }
}

BOOL XMLTool::IsUnsignedIntValueType(
   std::shared_ptr<XMLToolTag>pTag
)
{
   if (pTag->mValueType == XMLVAL_UINT) {
      return TRUE;
   }
   else {
      return FALSE;
   }
}

BOOL XMLTool::IsFloatValueType(
   std::shared_ptr<XMLToolTag>pTag
)
{
   if (pTag->mValueType == XMLVAL_FLOAT) {
      return TRUE;
   }
   else {
      return FALSE;
   }
}

BOOL XMLTool::IsDoubleValueType(
   std::shared_ptr<XMLToolTag>pTag
)
{
   if (pTag->mValueType == XMLVAL_DOUBLE) {
      return TRUE;
   }
   else {
      return FALSE;
   }
}

BOOL XMLTool::IsVector4ValueType(
   std::shared_ptr<XMLToolTag>pTag
)
{
   if (pTag->mValueType == XMLVAL_VECTOR4) {
      return TRUE;
   }
   else {
      return FALSE;
   }
}


BOOL XMLTool::IsMatrix44ValueType(
   std::shared_ptr<XMLToolTag>pTag
)
{
   if (pTag->mValueType == XMLVAL_MATRIX4X4) {
      return TRUE;
   }
   else {
      return FALSE;
   }
}

BOOL XMLTool::IsBinaryValueType(
   std::shared_ptr<XMLToolTag>pTag
)
{
   if (pTag->mValueType == XMLVAL_BINARY) {
      return TRUE;
   }
   else {
      return FALSE;
   }
}

BOOL XMLTool::IsStringValueType(
   std::shared_ptr<XMLToolTag>pTag
)
{
   if (pTag->mValueType == XMLVAL_STRING) {
      return TRUE;
   }
   else {
      return FALSE;
   }
}

BOOL XMLTool::IsBOOLValueType(
    std::shared_ptr<XMLToolTag>pTag
)
{
    if (pTag->mValueType == XMLVAL_BOOL) {
        return TRUE;
    }
    else {
        return FALSE;
    }
}

int XMLTool::GetTagNameLength(
   std::shared_ptr<XMLToolTag>pTag
)
{
   return pTag->mName.size();
}

BOOL XMLTool::GetTagName(
   std::shared_ptr<XMLToolTag>pTag,
   char *pNameOut, 
   int nameLen
)
{
   strncpy(pNameOut, pTag->mName.c_str(), nameLen);
   return TRUE;
}

char *XMLTool::GetTagPath(
   std::shared_ptr<XMLToolTag>pTag, 
   char *pPathOut, 
   int pathLen
)
{
   int index;
   int occurance = 0;
   if (pTag->mpParent) {
      //find this tag's index in it's parent's list
      for (index = 0; index < pTag->mpParent->mNumChildren; index++) {
         if (pTag->mpParent->mpChildren[index] == pTag) {
            break;
         }
         //since strings are consolidated, then we can compare their pointers
         if (pTag->mpParent->mpChildren[index]->mName == pTag->mName) {
            occurance++;
         }
      }

      //index = strlen("\\<name>[occurance]") +1
      index = pTag->mName.size() + CountIntChars(occurance) + 6;

      //if the string is not large enough for "\\<name>[occurance]\0"
      if (pathLen < index) {
         return FALSE;
      }

      if (!GetTagPath(pTag->mpParent, pPathOut, pathLen - index)) {
         //this is the first tag
         if (pathLen - index < 4) {
            //not enough room for this tag and "...\"
            pPathOut[0] = NULL;
            return FALSE;
         }
         
         strcpy(pPathOut, "...\\");
         pathLen -= 4;
      }
      else {
         strcat(pPathOut, "\\");
         pathLen--;
      }
   }
   else {
      //this is the root node, so terminate the string's begining so it can be strcat'd
      pPathOut[0] = NULL;
   }

   //index = strlen("<name>[occurance]") + 1
   index = pTag->mName.size() + CountIntChars(occurance) + 5;

   //if the string is not large enough for "<name>[occurance]\0"
   if (pathLen < index) {
      return FALSE;
   }
   char *pTmpStr = new char[pTag->mName.size() + CountIntChars(occurance) + 5];
   sprintf(pTmpStr, "<%s>[%i]", pTag->mName.c_str(), occurance);
   strcat(pPathOut, pTmpStr);
   delete [] pTmpStr;
   return pPathOut;
}

int XMLTool::GetTagValueSize(
   std::shared_ptr<XMLToolTag>pTag
)
{
   switch (pTag->mValueType) {
   case XMLVAL_STRING:
      return pTag->mStringValue.size() + 1;
      break;
   case XMLVAL_BINARY:
      return pTag->mNumValueElements;
      break;
   case XMLVAL_INT:
      return sizeof(int) * pTag->mNumValueElements;
      break;
   case XMLVAL_UINT:
      return sizeof(unsigned int) * pTag->mNumValueElements;
      break;
   case XMLVAL_FLOAT:
      return sizeof(float) * pTag->mNumValueElements;
      break;
   case XMLVAL_DOUBLE:
      return sizeof(double) * pTag->mNumValueElements;
      break;
   case XMLVAL_VECTOR4:
      return sizeof(Vector4) * pTag->mNumValueElements;
      break;
   case XMLVAL_MATRIX4X4:
      return sizeof(Matrix4x4) * pTag->mNumValueElements;
      break;
   case XMLVAL_BOOL:
       return sizeof(unsigned int) * pTag->mNumValueElements;
       break;
   default:
      return 0;
   }
}

int XMLTool::GetTagValue(
   std::shared_ptr<XMLToolTag>pTag,
   void *pValueOut,
   int valueLen
)
{
   if (pTag->mValueType != XMLVAL_BINARY) {
      return 0;
   }

   if (valueLen < pTag->mNumValueElements) {
      return 0;
   }

   memcpy(pValueOut, pTag->mpValue, pTag->mNumValueElements);
   return pTag->mNumValueElements;
}

BOOL XMLTool::GetTagValue(
   std::shared_ptr<XMLToolTag>pTag, 
   char *pValueOut, 
   int nameLen
)
{
   strncpy(pValueOut, pTag->mStringValue.c_str(), nameLen);
   if(pTag->mValueType == XMLVAL_STRING) {
      return TRUE;
   }
   return FALSE;
}

BOOL XMLTool::GetTagValue(
   std::shared_ptr<XMLToolTag>pTag,
   unsigned int *pValue,
   int numValues /*=1*/
)
{
   switch(pTag->mValueType) {
   case XMLVAL_UINT:
      if (pTag->mNumValueElements < numValues) {
         return FALSE;
      }
      memcpy(pValue, pTag->mpValue, sizeof(unsigned int) * numValues);
      break;
   case XMLVAL_STRING:
      return ReadUintsFromString(pValue, numValues, (char*)pTag->mStringValue.c_str());
      break;
   case XMLVAL_BINARY:
      return FALSE;//for now don't support binary type conversion
      //DEBUG_ASSERT(pTag->mNumValueElements >= (signed)(sizeof(unsigned int)*numValues), ("XMLTool::GetTagValue() - Binary value is not long enough."), FALSE);
      if (pTag->mNumValueElements < (signed)(sizeof(unsigned int)*numValues)) {
         return FALSE;
      }

      memcpy(pValue, pTag->mpValue, sizeof(unsigned int)*numValues);
      break;
   default:
      //DEBUG_ASSERT(0, ("XMLTool::GetTagValue() - No value."), FALSE);
      return FALSE;
   }
   return TRUE;
}

BOOL XMLTool::GetTagValue(
   std::shared_ptr<XMLToolTag>pTag,
   int *pValue,
   int numValues /*=1*/
)
{
   switch(pTag->mValueType) {
   case XMLVAL_INT:
      if (pTag->mNumValueElements < numValues) {
         return FALSE;
      }
      memcpy(pValue, pTag->mpValue, sizeof(int) * numValues);
      break;
   case XMLVAL_STRING:
      return ReadIntsFromString(pValue, numValues, (char*)pTag->mStringValue.c_str());
   case XMLVAL_BINARY:
      return FALSE;//for now don't support binary type conversion
      //DEBUG_ASSERT(pTag->mNumValueElements >= (signed)(sizeof(int)*numValues), ("XMLTool::GetTagValue() - Binary value is not long enough."), FALSE);
      if (pTag->mNumValueElements < (signed)(sizeof(int)*numValues)) {
         return FALSE;
      }

      memcpy(pValue, pTag->mpValue, sizeof(int)*numValues);
      break;
   default:
      //DEBUG_ASSERT(0, ("XMLTool::GetTagValue() - No value."), FALSE);
      return FALSE;
   }
   return TRUE;
}

BOOL XMLTool::GetTagValue(
   std::shared_ptr<XMLToolTag>pTag,
   float *pValue,
   int numValues /*=1*/
)
{
   switch(pTag->mValueType) {
   case XMLVAL_FLOAT:
      if (pTag->mNumValueElements < numValues) {
         return FALSE;
      }
      memcpy(pValue, pTag->mpValue, sizeof(float) * numValues);
      break;
   case XMLVAL_STRING:
      return ReadFloatsFromString(pValue, numValues, (char*)pTag->mStringValue.c_str());
      break;
   case XMLVAL_BINARY:
      return FALSE;//for now don't support binary type conversion
      //DEBUG_ASSERT(pTag->mBinaryValueSize >= (signed)(sizeof(float)*numValues), ("XMLTool::GetTagValue() - Binary value is not long enough."), FALSE);
      if (pTag->mNumValueElements < (signed)(sizeof(float)*numValues)) {
         return FALSE;
      }

      memcpy(pValue, pTag->mpValue, sizeof(float)*numValues);
      break;
   default:
      //DEBUG_ASSERT(0, ("XMLTool::GetTagValue() - No value."), FALSE);
      return FALSE;
   }
   return TRUE;
}

BOOL XMLTool::GetTagValue(
   std::shared_ptr<XMLToolTag>pTag,
   double *pValue,
   int numValues /*=1*/
)
{
   switch(pTag->mValueType) {
   case XMLVAL_DOUBLE:
      if (pTag->mNumValueElements < numValues) {
         return FALSE;
      }
      memcpy(pValue, pTag->mpValue, sizeof(double) * numValues);
      break;
   case XMLVAL_STRING:
      return ReadDoublesFromString(pValue, numValues, (char*)pTag->mStringValue.c_str());
   case XMLVAL_BINARY:
      return FALSE;//for now don't support binary type conversion
      //DEBUG_ASSERT(pTag->mBinaryValueSize >= (signed)(sizeof(double)*numValues), ("XMLTool::GetTagValue() - Binary value is not long enough."), FALSE);
      if (pTag->mNumValueElements < (signed)(sizeof(double)*numValues)) {
         return FALSE;
      }

      memcpy(pValue, pTag->mpValue, sizeof(double)*numValues);
      break;
   default:
      //DEBUG_ASSERT(0, ("XMLTool::GetTagValue() - No value."), FALSE);
      return FALSE;
   }
   return TRUE;
}

BOOL XMLTool::GetTagValue(
   std::shared_ptr<XMLToolTag>pTag,
   Vector4 *pValue,
   int numValues /*=1*/
)
{
   switch(pTag->mValueType) {
   case XMLVAL_VECTOR4:
      if (pTag->mNumValueElements < numValues) {
         return FALSE;
      }
      memcpy(pValue, pTag->mpValue, sizeof(Vector4) * numValues);
      break;
   case XMLVAL_STRING:
      return ReadVector4sFromString(pValue, numValues, (char*)pTag->mStringValue.c_str());
   case XMLVAL_BINARY:
      return FALSE;//for now don't support binary type conversion
      //DEBUG_ASSERT(pTag->mBinaryValueSize >= (signed)(sizeof(float)*numValues * 4), ("XMLTool::GetTagValue() - Binary value is not long enough."), FALSE);
      if (pTag->mNumValueElements < (signed)(sizeof(Vector4)*numValues)) {
         return FALSE;
      }

      memcpy(pValue, pTag->mpValue, sizeof(Vector4) * numValues);
      break;
   default:
      //DEBUG_ASSERT(0, ("XMLTool::GetTagValue() - No value."), FALSE);
      return FALSE;
   }
   return TRUE;
}

BOOL XMLTool::GetTagValue(
   std::shared_ptr<XMLToolTag>pTag,
   Matrix4x4 *pValue,
   int numValues /*=1*/
)
{
   switch(pTag->mValueType) {
   case XMLVAL_MATRIX4X4:
      if (pTag->mNumValueElements < numValues) {
         return FALSE;
      }
      memcpy(pValue, pTag->mpValue, sizeof(Matrix4x4) * numValues);
      break;
   case XMLVAL_STRING:
      return ReadMatrix44sFromString(pValue, numValues, (char*)pTag->mStringValue.c_str());
   case XMLVAL_BINARY:
      return FALSE;//for now don't support binary type conversion
      //DEBUG_ASSERT(pTag->mBinaryValueSize >= (signed)(sizeof(float)*numValues * 4), ("XMLTool::GetTagValue() - Binary value is not long enough."), FALSE);
      if (pTag->mNumValueElements < (signed)(sizeof(Matrix4x4)*numValues)) {
         return FALSE;
      }

      memcpy(pValue, pTag->mpValue, sizeof(Matrix4x4) * numValues);
      break;
   default:
      //DEBUG_ASSERT(0, ("XMLTool::GetTagValue() - No value."), FALSE);
      return FALSE;
   }
   return TRUE;
}

BOOL XMLTool::SetTagName(
   std::shared_ptr<XMLToolTag>pTag,
   char *pName
)
{
   if (strlen(pName) >= MAX_STRING) {
      return FALSE;
   }

   if (pTag->mName != "") {
      mUniqueStringTable.RemoveString(pTag->mName);
   }

   mUniqueStringTable.AddString(pName);

   pTag->mName = pName;

   return TRUE;
}

BOOL XMLTool::SetTagValue(
   std::shared_ptr<XMLToolTag>pTag, 
   void *pValue,
   int bufferLen
)
{
   void *ptr = realloc(pTag->mpValue, bufferLen);
   if (ptr == NULL) {
      DEBUG_ASSERT(0, ("XMLTool::SetTagValue() - Out of memory."), FALSE);
      return FALSE;
   }
   pTag->mpValue = ptr;
   
   //if the old value was a string, then clear the string
   if (pTag->mValueType == XMLVAL_STRING) {
      mUniqueStringTable.RemoveString(pTag->mStringValue);
      pTag->mStringValue = "";
   }
   
   pTag->mValueType = XMLVAL_BINARY;
   pTag->mNumValueElements = bufferLen;
   memcpy(pTag->mpValue, pValue, bufferLen);
   return TRUE;
}

BOOL XMLTool::SetTagValue(
   std::shared_ptr<XMLToolTag>pTag,
   char *pValue
)
{
   if (strlen(pValue) >= MAX_STRING) {
      return FALSE;
   }

   //if old value was not a string then free the data
   if (pTag->mValueType != XMLVAL_STRING) {
      free(pTag->mpValue);
      pTag->mpValue = NULL;
      pTag->mNumValueElements = 0;
   }
   //if the last value was a string then remove the string
   if (pTag->mStringValue != "") {
      mUniqueStringTable.RemoveString(pTag->mStringValue);
   }

   pTag->mValueType = XMLVAL_STRING;
   pTag->mStringValue = mUniqueStringTable.GetString(mUniqueStringTable.AddString(pValue));
   return TRUE;
}

BOOL XMLTool::SetTagValue(
   std::shared_ptr<XMLToolTag>pTag,
   unsigned int *pValue,
   int numValues /*= 1*/
)
{
   void *ptr = realloc(pTag->mpValue, numValues * sizeof(unsigned int));
   if (ptr == NULL) {
      DEBUG_ASSERT(0, ("XMLTool::SetTagValue() - Out of memory."), FALSE);
      return FALSE;
   }
   pTag->mpValue = ptr;
   
   //if the old value was a string, then clear the string
   if (pTag->mValueType == XMLVAL_STRING) {
      mUniqueStringTable.RemoveString(pTag->mStringValue);
      pTag->mStringValue = "";
   }
   
   pTag->mValueType = XMLVAL_UINT;
   pTag->mNumValueElements = numValues;
   memcpy(pTag->mpValue, pValue, numValues * sizeof(unsigned int));
   return TRUE;
}

BOOL XMLTool::SetTagValue(
   std::shared_ptr<XMLToolTag>pTag,
   int *pValue,
   int numValues /*= 1*/
)
{
   void *ptr = realloc(pTag->mpValue, numValues * sizeof(int));
   if (ptr == NULL) {
      DEBUG_ASSERT(0, ("XMLTool::SetTagValue() - Out of memory."), FALSE);
      return FALSE;
   }
   pTag->mpValue = ptr;
   
   //if the old value was a string, then clear the string
   if (pTag->mValueType == XMLVAL_STRING) {
      mUniqueStringTable.RemoveString(pTag->mStringValue);
      pTag->mStringValue = "";
   }
   
   pTag->mValueType = XMLVAL_INT;
   pTag->mNumValueElements = numValues;
   memcpy(pTag->mpValue, pValue, numValues * sizeof(int));
   return TRUE;
}

BOOL XMLTool::SetTagValue(
   std::shared_ptr<XMLToolTag>pTag,
   float *pValue,
   int numValues /*= 1*/
)
{
   void *ptr = realloc(pTag->mpValue, numValues * sizeof(float));
   if (ptr == NULL) {
      DEBUG_ASSERT(0, ("XMLTool::SetTagValue() - Out of memory."), FALSE);
      return FALSE;
   }
   pTag->mpValue = ptr;
   
   //if the old value was a string, then clear the string
   if (pTag->mValueType == XMLVAL_STRING) {
      mUniqueStringTable.RemoveString(pTag->mStringValue);
      pTag->mStringValue = "";
   }
   
   pTag->mValueType = XMLVAL_FLOAT;
   pTag->mNumValueElements = numValues;
   memcpy(pTag->mpValue, pValue, numValues * sizeof(float));
   return TRUE;
}

BOOL XMLTool::SetTagValue(
   std::shared_ptr<XMLToolTag>pTag,
   double *pValue,
   int numValues /*= 1*/
)
{
   void *ptr = realloc(pTag->mpValue, numValues * sizeof(double));
   if (ptr == NULL) {
      DEBUG_ASSERT(0, ("XMLTool::SetTagValue() - Out of memory."), FALSE);
      return FALSE;
   }
   pTag->mpValue = ptr;
   
   //if the old value was a string, then clear the string
   if (pTag->mValueType == XMLVAL_STRING) {
      mUniqueStringTable.RemoveString(pTag->mStringValue);
      pTag->mStringValue = "";
   }
   
   pTag->mValueType = XMLVAL_DOUBLE;
   pTag->mNumValueElements = numValues;
   memcpy(pTag->mpValue, pValue, numValues * sizeof(double));
   return TRUE;
}

BOOL XMLTool::SetTagValue(
   std::shared_ptr<XMLToolTag>pTag,
   Vector4 *pValue,
   int numValues /*= 1*/
)
{
   void *ptr = realloc(pTag->mpValue, numValues * sizeof(Vector4));
   if (ptr == NULL) {
      DEBUG_ASSERT(0, ("XMLTool::SetTagValue() - Out of memory."), FALSE);
      return FALSE;
   }
   pTag->mpValue = ptr;
   
   //if the old value was a string, then clear the string
   if (pTag->mValueType == XMLVAL_STRING) {
      mUniqueStringTable.RemoveString(pTag->mStringValue);
      pTag->mStringValue = "";
   }
   
   pTag->mValueType = XMLVAL_VECTOR4;
   pTag->mNumValueElements = numValues;
   memcpy(pTag->mpValue, pValue, numValues * sizeof(Vector4));
   return TRUE;
}

BOOL XMLTool::SetTagValue(
   std::shared_ptr<XMLToolTag>pTag,
   Matrix4x4 *pValue,
   int numValues /*= 1*/
)
{
   void *ptr = realloc(pTag->mpValue, numValues * sizeof(Matrix4x4));
   if (ptr == NULL) {
      DEBUG_ASSERT(0, ("XMLTool::SetTagValue() - Out of memory."), FALSE);
      return FALSE;
   }
   pTag->mpValue = ptr;
   
   //if the old value was a string, then clear the string
   if (pTag->mValueType == XMLVAL_STRING) {
      mUniqueStringTable.RemoveString(pTag->mStringValue);
      pTag->mStringValue = "";
   }
   
   pTag->mValueType = XMLVAL_MATRIX4X4;
   pTag->mNumValueElements = numValues;
   memcpy(pTag->mpValue, pValue, numValues * sizeof(Matrix4x4));
   return TRUE;
}

BOOL XMLTool::SetTagValueBOOL(
    std::shared_ptr<XMLToolTag> pTag,
    unsigned int* pValue,
    int numValues /*= 1*/
)
{
    void* ptr = realloc(pTag->mpValue, numValues * sizeof(unsigned int));
    if (ptr == NULL) {
        DEBUG_ASSERT(0, ("XMLTool::SetTagValue() - Out of memory."), FALSE);
        return FALSE;
    }
    pTag->mpValue = ptr;

    //if the old value was a string, then clear the string
    if (pTag->mValueType == XMLVAL_STRING) {
        mUniqueStringTable.RemoveString(pTag->mStringValue);
        pTag->mStringValue = "";
    }

    pTag->mValueType = XMLVAL_BOOL;
    pTag->mNumValueElements = numValues;
    memcpy(pTag->mpValue, pValue, numValues * sizeof(unsigned int));
    return TRUE;
}

////////////////////////////////////////////////////////////
// Functions for searching tag attributes
////////////////////////////////////////////////////////////

int XMLTool::GetAttribNum(
   std::shared_ptr<XMLToolTag>pTag
)
{
   return pTag->mNumAttribs;
}

std::shared_ptr<XMLToolAttrib>XMLTool::FindFirstAttrib(
   std::shared_ptr<XMLToolTag>pParent,
   char *pName
)
{
   if (!pParent->mNumAttribs) {
      return NULL;
   }

   if (pName != NULL) {
      for (int i = 0; i < pParent->mNumAttribs; i++) {
         if (pParent->mpAttribs[i]->mName == pName) {
            return pParent->mpAttribs[i];
         }
      }
      return NULL;
   }
   else {
      return pParent->mpAttribs[0];
   }
}

std::shared_ptr<XMLToolAttrib>XMLTool::OpenAttribSearch(
   std::shared_ptr<XMLToolTag>pTag,
   char *pString,
   int *pOutAttribSearchID
)
{
   int searchID;
   std::shared_ptr<XMLToolAttrib>pAttrib;
   DEBUG_ASSERT(mNumAvailableAttribSearches > 0, ("XMLTool::OpenAttribSearch() - No more available attrib searches."), FALSE);
   
   //get an unused attrib search
   searchID = mAvailableAttribSearches[mNumAvailableAttribSearches-1];

   mAttribSearches[searchID].mCurrentAttribIndex = -1;
   mAttribSearches[searchID].mpParentTag = pTag;

   if (pString != NULL) {
      strcpy(mAttribSearches[searchID].mSearchString, pString);
   }
   else {
      mAttribSearches[searchID].mSearchString[0] = NULL;
   }

   searchID = ENCODE_ATTRIB_SEARCHID(searchID);

   if ((pAttrib = GetNextAttrib(searchID)) == NULL) {
      mAttribSearches[searchID].mpParentTag = NULL;
      (*pOutAttribSearchID) = -1;
      return NULL;
   }
   
   //remove the attrib search from the available list
   mNumAvailableAttribSearches--;
   //encode search id so that attrib search id's and child search id's can't mix
   (*pOutAttribSearchID) = searchID;
   return pAttrib;
}

void XMLTool::CloseAttribSearch(
   int attribSearchID
)
{
   if (attribSearchID == -1) {
      return;
   }
   
   //decode search id so that attrib search id's and child search id's can't mix
   attribSearchID = DECODE_ATTRIB_SEARCHID(attribSearchID);
   
   DEBUG_ASSERT(attribSearchID >= 0 && attribSearchID < MAX_ATTRIB_SEARCHES, ("XMLTool::GetNextAttrib() - Invalid searchID."), FALSE);
   DEBUG_ASSERT(mNumAvailableAttribSearches < MAX_ATTRIB_SEARCHES, ("XMLTool::GetNextAttrib() - All searches are already available.", attribSearchID), FALSE);   
   DEBUG_ASSERT(mAttribSearches[attribSearchID].mpParentTag != NULL, ("XMLTool::GetNextAttrib() - Attrib search at index %i is already closed.", attribSearchID), FALSE);

   //return this index to the list of available
   mAttribSearches[attribSearchID].mpParentTag = NULL;
   mAvailableAttribSearches[mNumAvailableAttribSearches] = attribSearchID;
   mNumAvailableAttribSearches++;
}

std::shared_ptr<XMLToolAttrib>XMLTool::GetNextAttrib(
   int attribSearchID
)
{
   //decode search id so that attrib search id's and child search id's can't mix
   attribSearchID = DECODE_ATTRIB_SEARCHID(attribSearchID);

   DEBUG_ASSERT(attribSearchID >= 0 && attribSearchID < MAX_ATTRIB_SEARCHES, ("XMLTool::GetNextAttrib() - Invalid searchID."), FALSE);

   if (mAttribSearches[attribSearchID].mCurrentAttribIndex >= mAttribSearches[attribSearchID].mpParentTag->mNumAttribs - 1) {
      mAttribSearches[attribSearchID].mCurrentAttribIndex = mAttribSearches[attribSearchID].mpParentTag->mNumAttribs;
      //at the end of the list
      return NULL;
   }
   else if (mAttribSearches[attribSearchID].mSearchString[0] == NULL) {
      return mAttribSearches[attribSearchID].mpParentTag->mpAttribs[++mAttribSearches[attribSearchID].mCurrentAttribIndex];
   }

   int i, num;
   char *pString;
   pString = mAttribSearches[attribSearchID].mSearchString;
   num = mAttribSearches[attribSearchID].mpParentTag->mNumAttribs;

   for (i = mAttribSearches[attribSearchID].mCurrentAttribIndex+1; i < num; i++) {
      if (pString == mAttribSearches[attribSearchID].mpParentTag->mpAttribs[i]->mStringValue) {
         mAttribSearches[attribSearchID].mCurrentAttribIndex = i;
         return mAttribSearches[attribSearchID].mpParentTag->mpAttribs[i];
      }
   }

   mAttribSearches[attribSearchID].mCurrentAttribIndex = i;
   return NULL;
}

std::shared_ptr<XMLToolAttrib>XMLTool::GetPrevAttrib(
   int attribSearchID
)
{
   //decode search id so that attrib search id's and child search id's can't mix
   attribSearchID = DECODE_ATTRIB_SEARCHID(attribSearchID);

   DEBUG_ASSERT(attribSearchID >= 0 && attribSearchID < MAX_ATTRIB_SEARCHES, ("XMLTool::GetNextAttrib() - Invalid searchID."), FALSE);

   if (mAttribSearches[attribSearchID].mCurrentAttribIndex <= 0) {
      mAttribSearches[attribSearchID].mCurrentAttribIndex = -1;
      //at the front of the list
      return NULL;
   }
   else if (mAttribSearches[attribSearchID].mSearchString[0] == NULL) {
      return mAttribSearches[attribSearchID].mpParentTag->mpAttribs[--mAttribSearches[attribSearchID].mCurrentAttribIndex];
   }

   char *pString;
   pString = mAttribSearches[attribSearchID].mSearchString;

   for (int i = mAttribSearches[attribSearchID].mCurrentAttribIndex-1; i >= 0; i--) {
      if (pString == mAttribSearches[attribSearchID].mpParentTag->mpAttribs[i]->mStringValue) {
         mAttribSearches[attribSearchID].mCurrentAttribIndex = i;
         return mAttribSearches[attribSearchID].mpParentTag->mpAttribs[i];
      }
   }

   mAttribSearches[attribSearchID].mCurrentAttribIndex = -1;
   return NULL;
}


/*std::shared_ptr<XMLToolAttrib>XMLTool::SetAttribFilter(
   std::shared_ptr<XMLToolTag>pTag,
   char *pString
)
{
   if (pString) {
      strcpy(pTag->mAttribFilter, pString);
   }
   else {
      pTag->mAttribFilter[0] = NULL;
   }

   pTag->mCurrentAttrib = -1;
   return GetNextAttrib(pTag);
}

std::shared_ptr<XMLToolAttrib>XMLTool::GetNextAttrib(
   std::shared_ptr<XMLToolTag>pTag
   )
{
   if (pTag->mCurrentAttrib >= pTag->mNumAttribs-1) {
      return NULL;
   }
   
   pTag->mCurrentAttrib++;
   if (pTag->mAttribFilter[0]) {
      while (stricmp(pTag->mAttribFilter, pTag->mpAttribs[pTag->mCurrentAttrib].mName)) {
         if (pTag->mCurrentAttrib >= pTag->mNumAttribs-1) {
            return NULL;
         }
         pTag->mCurrentAttrib++;
      }
   }

   return &pTag->mpAttribs[pTag->mCurrentAttrib];
}

std::shared_ptr<XMLToolAttrib>XMLTool::GetPrevAttrib(
   std::shared_ptr<XMLToolTag>pTag
   )
{
   if (pTag->mCurrentAttrib <= 0) {
      return NULL;
   }
   
   pTag->mCurrentAttrib--;
   if (pTag->mAttribFilter[0]) {
      while (stricmp(pTag->mAttribFilter, pTag->mpAttribs[pTag->mCurrentAttrib].mName)) {
         if (pTag->mCurrentAttrib <= 0) {
            return NULL;
         }
         pTag->mCurrentAttrib--;
      }
   }

   return &pTag->mpAttribs[pTag->mCurrentAttrib];
}*/

////////////////////////////////////////////////////////////
// Functions for getting/setting an attrib's name and value
////////////////////////////////////////////////////////////
/*
std::shared_ptr<XMLToolAttrib>XMLTool::GetNextAttribString(
   int attribSearchID,
   char *pName, 
   int nameLen, 
   char *pValue, 
   int valueLen
)
{
   std::shared_ptr<XMLToolAttrib>pAttrib = GetNextAttrib(attribSearchID);
   strncpy(pName, pAttrib->mpName, nameLen);
   strncpy(pValue, pAttrib->mpStringValue, valueLen);

   return pAttrib;
}

std::shared_ptr<XMLToolAttrib>XMLTool::GetPrevAttribString(
   int attribSearchID, 
   char *pName, 
   int nameLen, 
   char *pValue, 
   int valueLen
)
{
   std::shared_ptr<XMLToolAttrib>pAttrib = GetPrevAttrib(attribSearchID);
   strncpy(pName, pAttrib->mpName, nameLen);
   strncpy(pValue, pAttrib->mpStringValue, valueLen);
   return pAttrib;
}
*/

BOOL XMLTool::GetAttribValueAsString(std::shared_ptr<XMLToolAttrib> pAttrib, char* pOutName, int Len) {
    switch (pAttrib->mValueType) {
    case XMLVAL_NONE:
        break;
    case XMLVAL_STRING:
        strncpy(pOutName, pAttrib->mStringValue.c_str(), Len);
        break;
    case XMLVAL_BINARY:
        {
            int reqSize = 1 + GETCONVSIZEFROM256(pAttrib->mNumValueElements);
            if (Len >= reqSize) {
                ConvertBinToBase64ASCII(pAttrib->mpValue, pAttrib->mNumValueElements, pOutName, reqSize);
            }
            else {
                if (Len > 0) pOutName[0] = '\0';
            }
        }
        break;
    case XMLVAL_INT:
        WriteIntsToString((int*)pAttrib->mpValue, pAttrib->mNumValueElements, pOutName, Len);
        break;
    case XMLVAL_UINT:
        WriteUintsToString((unsigned int*)pAttrib->mpValue, pAttrib->mNumValueElements, pOutName, Len);
        break;
    case XMLVAL_FLOAT:
        WriteFloatsToString((float*)pAttrib->mpValue, pAttrib->mNumValueElements, pOutName, Len);
        break;
    case XMLVAL_DOUBLE:
        WriteDoublesToString((double*)pAttrib->mpValue, pAttrib->mNumValueElements, pOutName, Len);
        break;
    case XMLVAL_VECTOR4:
        WriteVector4sToString((Vector4*)pAttrib->mpValue, pAttrib->mNumValueElements, pOutName, Len);
        break;
    case XMLVAL_MATRIX4X4:
        WriteMatrix44sToString((Matrix4x4*)pAttrib->mpValue, pAttrib->mNumValueElements, pOutName, Len);
        break;
    case XMLVAL_BOOL:
        WriteUintsToString((unsigned int*)pAttrib->mpValue, pAttrib->mNumValueElements, pOutName, Len);
        break;
    }
    return 0;
}

BOOL XMLTool::GetAttribName(
   std::shared_ptr<XMLToolAttrib>pAttrib,
   char *pNameOut, 
   int nameLen
)
{
   strncpy(pNameOut, pAttrib->mName.c_str(), nameLen);
   return 0;
}

char *XMLTool::GetAttribPath(
   std::shared_ptr<XMLToolTag>pParentTag,
   std::shared_ptr<XMLToolAttrib>pAttrib,
   char *pPathOut,
   int pathLen
)
{
   int index;
   int occurance = 0;
   //find this attrib's index in it's parent's list
   for (index = 0; index < pParentTag->mNumAttribs; index++) {
      if (pParentTag->mpAttribs[index] == pAttrib) {
         break;
      }

      if (pParentTag->mpAttribs[index]->mName == pAttrib->mName) {
         occurance++;
      }
   }

   //index = strlen(".name[occurance]") + 1
   index = (pAttrib->mName.size() + CountIntChars(occurance) + 4);

   //if the string is not large enough for 
   if (pathLen < index) {
      return FALSE;
   }

   if (!GetTagPath(pParentTag, pPathOut, pathLen - index)) {
      //must show atleast first level parent tag
      return FALSE;
   }
   
   char *pTmpStr = new char[index];
   sprintf(pTmpStr, ".%s[%i]", pAttrib->mName.c_str(), occurance);
   strcat(pPathOut, pTmpStr);
   delete [] pTmpStr;
   return pPathOut;
}

int XMLTool::GetAttribValueLength(
   std::shared_ptr<XMLToolAttrib>pAttrib
)
{
   return pAttrib->mStringValue.size() + 1;
}

int XMLTool::GetAttribValue(std::shared_ptr<XMLToolAttrib> pAttrib, void* pValueOut, int valueLen)
{
    if (pAttrib->mValueType != XMLVAL_BINARY) {
        return 0;
    }

    if (valueLen < pAttrib->mNumValueElements) {
        return 0;
    }

    memcpy(pValueOut, pAttrib->mpValue, pAttrib->mNumValueElements);
    return pAttrib->mNumValueElements;
}

BOOL XMLTool::GetAttribValue(
   std::shared_ptr<XMLToolAttrib>pAttrib, 
   char *pValueOut, 
   int nameLen
   )
{
    if (pAttrib->mValueType == XMLVAL_STRING) {
        strncpy(pValueOut, pAttrib->mStringValue.c_str(), nameLen);
    }
   return TRUE;
}

BOOL XMLTool::GetAttribValue(
   std::shared_ptr<XMLToolAttrib>pAttrib,
   unsigned int *pValue,
   int numValues /*=1*/
)
{
    if (pAttrib->mValueType == XMLVAL_STRING) {
        return ReadUintsFromString(pValue, numValues, (char*)pAttrib->mStringValue.c_str());
    }
    else if (pAttrib->mValueType == XMLVAL_UINT) {
        memcpy(pValue, pAttrib->mpValue, numValues * sizeof(unsigned int));
    }
    return TRUE;
}

BOOL XMLTool::GetAttribValue(
   std::shared_ptr<XMLToolAttrib>pAttrib,
   int *pValue,
   int numValues /*=1*/
)
{
    if (pAttrib->mValueType == XMLVAL_STRING) {
        return ReadIntsFromString(pValue, numValues, (char*)pAttrib->mStringValue.c_str());
    }
    else if (pAttrib->mValueType == XMLVAL_INT) {
        memcpy(pValue, pAttrib->mpValue, numValues * sizeof(int));
    }
    return TRUE;
}

BOOL XMLTool::GetAttribValue(
   std::shared_ptr<XMLToolAttrib>pAttrib,
   float *pValue,
   int numValues /*=1*/
)
{
    if (pAttrib->mValueType == XMLVAL_STRING) {
        return ReadFloatsFromString(pValue, numValues, (char*)pAttrib->mStringValue.c_str());
    }
    else if (pAttrib->mValueType == XMLVAL_FLOAT) {
        memcpy(pValue, pAttrib->mpValue, numValues * sizeof(float));
    }
    return TRUE;
}

BOOL XMLTool::GetAttribValue(
   std::shared_ptr<XMLToolAttrib>pAttrib,
   double *pValue,
   int numValues /*=1*/
)
{
    if (pAttrib->mValueType == XMLVAL_STRING) {
        return ReadDoublesFromString(pValue, numValues, (char*)pAttrib->mStringValue.c_str());
    }
    else if (pAttrib->mValueType == XMLVAL_DOUBLE) {
        memcpy(pValue, pAttrib->mpValue, numValues * sizeof(double));
    }
    return TRUE;
}

BOOL XMLTool::GetAttribValue(
   std::shared_ptr<XMLToolAttrib>pAttrib,
   Vector4 *pValue,
   int numValues /*=1*/
)
{
    if (pAttrib->mValueType == XMLVAL_STRING) {
        return ReadVector4sFromString(pValue, numValues, (char*)pAttrib->mStringValue.c_str());
    }
    else if (pAttrib->mValueType == XMLVAL_VECTOR4) {
        memcpy(pValue, pAttrib->mpValue, numValues * sizeof(Vector4));
    }
    return TRUE;
}

BOOL XMLTool::GetAttribValue(
   std::shared_ptr<XMLToolAttrib>pAttrib,
   Matrix4x4 *pValue,
   int numValues /*=1*/
)
{
    if (pAttrib->mValueType == XMLVAL_STRING) {
        return ReadMatrix44sFromString(pValue, numValues, (char*)pAttrib->mStringValue.c_str());
    }
    else if (pAttrib->mValueType == XMLVAL_MATRIX4X4) {
        memcpy(pValue, pAttrib->mpValue, numValues * sizeof(Matrix4x4));
    }
    return TRUE;
}

BOOL XMLTool::SetAttribValue(std::shared_ptr<XMLToolAttrib> pAttrib, void* pValue, int bufferLen)
{
    void* ptr = realloc(pAttrib->mpValue, bufferLen);
    if (ptr == NULL) {
        DEBUG_ASSERT(0, ("XMLTool::SetTagValue() - Out of memory."), FALSE);
        return FALSE;
    }
    pAttrib->mpValue = ptr;

    //if the old value was a string, then clear the string
    if (pAttrib->mValueType == XMLVAL_STRING) {
        mUniqueStringTable.RemoveString(pAttrib->mStringValue);
        pAttrib->mStringValue = "";
    }

    pAttrib->mValueType = XMLVAL_BINARY;
    pAttrib->mNumValueElements = bufferLen;
    memcpy(pAttrib->mpValue, pValue, bufferLen);
    return TRUE;
}

BOOL XMLTool::SetAttribName(
   std::shared_ptr<XMLToolAttrib>pAttrib,
   char *pName
)
{
   if (pName == nullptr || strlen(pName) >= MAX_STRING) {
      return FALSE;
   }

   if (pAttrib->mName != "") {
      mUniqueStringTable.RemoveString(pAttrib->mName);
   }

   pAttrib->mName = mUniqueStringTable.GetString(mUniqueStringTable.AddString(pName));

   return TRUE;
}

BOOL XMLTool::SetAttribValue(
   std::shared_ptr<XMLToolAttrib>pAttrib,
   char *pValue
)
{
    if (pAttrib->mValueType != XMLVAL_STRING && pAttrib->mpValue != nullptr) {
        free(pAttrib->mpValue);
        pAttrib->mValueType = XMLVAL_NONE;
    }

    if (pAttrib->mValueType == XMLVAL_STRING && pAttrib->mStringValue != "") {
        mUniqueStringTable.RemoveString(pAttrib->mStringValue);
    }
    pAttrib->mValueType = XMLVAL_STRING;

   if (pValue == NULL) {
      pAttrib->mStringValue = "";
      return TRUE;
   }

   if (strlen(pValue) >= MAX_STRING) {
      return FALSE;
   }

   pAttrib->mStringValue = mUniqueStringTable.GetString(mUniqueStringTable.AddString(pValue));
   return TRUE;
}

BOOL XMLTool::SetAttribValue(
   std::shared_ptr<XMLToolAttrib>pAttrib,
   unsigned int *pValue,
   int numValues /*= 1*/
)
{
    //if the old value was a string, then clear the string
    if (pAttrib->mValueType == XMLVAL_STRING && pAttrib->mStringValue != "") {
        mUniqueStringTable.RemoveString(pAttrib->mStringValue);
        pAttrib->mStringValue = "";
    }

    void* ptr = realloc(pAttrib->mpValue, numValues * sizeof(unsigned int));
    if (ptr == NULL) {
        DEBUG_ASSERT(0, ("XMLTool::SetAttribValue() - Out of memory."), FALSE);
        return FALSE;
    }
    pAttrib->mpValue = ptr;

    pAttrib->mValueType = XMLVAL_UINT;
    pAttrib->mNumValueElements = numValues;
    memcpy(pAttrib->mpValue, pValue, numValues * sizeof(unsigned int));
    return TRUE;
}

BOOL XMLTool::SetAttribValue(
   std::shared_ptr<XMLToolAttrib>pAttrib,
   int *pValue,
   int numValues /*= 1*/
)
{
    //if the old value was a string, then clear the string
    if (pAttrib->mValueType == XMLVAL_STRING && pAttrib->mStringValue != "") {
        mUniqueStringTable.RemoveString(pAttrib->mStringValue);
        pAttrib->mStringValue = "";
    }

    void* ptr = realloc(pAttrib->mpValue, numValues * sizeof(int));
    if (ptr == NULL) {
        DEBUG_ASSERT(0, ("XMLTool::SetAttribValue() - Out of memory."), FALSE);
        return FALSE;
    }
    pAttrib->mpValue = ptr;

    pAttrib->mValueType = XMLVAL_INT;
    pAttrib->mNumValueElements = numValues;
    memcpy(pAttrib->mpValue, pValue, numValues * sizeof(int));
    return TRUE;
}

BOOL XMLTool::SetAttribValue(
   std::shared_ptr<XMLToolAttrib>pAttrib,
   float *pValue,
   int numValues /*= 1*/
)
{
    //if the old value was a string, then clear the string
    if (pAttrib->mValueType == XMLVAL_STRING && pAttrib->mStringValue != "") {
        mUniqueStringTable.RemoveString(pAttrib->mStringValue);
        pAttrib->mStringValue = "";
    }

    void* ptr = realloc(pAttrib->mpValue, numValues * sizeof(float));
    if (ptr == NULL) {
        DEBUG_ASSERT(0, ("XMLTool::SetAttribValue() - Out of memory."), FALSE);
        return FALSE;
    }
    pAttrib->mpValue = ptr;

    pAttrib->mValueType = XMLVAL_FLOAT;
    pAttrib->mNumValueElements = numValues;
    memcpy(pAttrib->mpValue, pValue, numValues * sizeof(float));
    return TRUE;
}

BOOL XMLTool::SetAttribValue(
   std::shared_ptr<XMLToolAttrib>pAttrib,
   double *pValue,
   int numValues /*= 1*/
)
{
    //if the old value was a string, then clear the string
    if (pAttrib->mValueType == XMLVAL_STRING && pAttrib->mStringValue != "") {
        mUniqueStringTable.RemoveString(pAttrib->mStringValue);
        pAttrib->mStringValue = "";
    }

    void* ptr = realloc(pAttrib->mpValue, numValues * sizeof(double));
    if (ptr == NULL) {
        DEBUG_ASSERT(0, ("XMLTool::SetAttribValue() - Out of memory."), FALSE);
        return FALSE;
    }
    pAttrib->mpValue = ptr;

    pAttrib->mValueType = XMLVAL_DOUBLE;
    pAttrib->mNumValueElements = numValues;
    memcpy(pAttrib->mpValue, pValue, numValues * sizeof(double));
    return TRUE;
}

BOOL XMLTool::SetAttribValue(
   std::shared_ptr<XMLToolAttrib>pAttrib,
   Vector4 *pValue,
   int numValues /*= 1*/
)
{
    //if the old value was a string, then clear the string
    if (pAttrib->mValueType == XMLVAL_STRING && pAttrib->mStringValue != "") {
        mUniqueStringTable.RemoveString(pAttrib->mStringValue);
        pAttrib->mStringValue = "";
    }

    void* ptr = realloc(pAttrib->mpValue, numValues * sizeof(Vector4));
    if (ptr == NULL) {
        DEBUG_ASSERT(0, ("XMLTool::SetAttribValue() - Out of memory."), FALSE);
        return FALSE;
    }
    pAttrib->mpValue = ptr;

    pAttrib->mValueType = XMLVAL_VECTOR4;
    pAttrib->mNumValueElements = numValues;
    memcpy(pAttrib->mpValue, pValue, numValues * sizeof(Vector4));
    return TRUE;
}

BOOL XMLTool::SetAttribValue(
   std::shared_ptr<XMLToolAttrib>pAttrib,
   Matrix4x4 *pValue,
   int numValues /*= 1*/
)
{
    //if the old value was a string, then clear the string
    if (pAttrib->mValueType == XMLVAL_STRING && pAttrib->mStringValue != "") {
        mUniqueStringTable.RemoveString(pAttrib->mStringValue);
        pAttrib->mStringValue = "";
    }

    void* ptr = realloc(pAttrib->mpValue, numValues * sizeof(Matrix4x4));
    if (ptr == NULL) {
        DEBUG_ASSERT(0, ("XMLTool::SetAttribValue() - Out of memory."), FALSE);
        return FALSE;
    }
    pAttrib->mpValue = ptr;

    pAttrib->mValueType = XMLVAL_MATRIX4X4;
    pAttrib->mNumValueElements = numValues;
    memcpy(pAttrib->mpValue, pValue, numValues * sizeof(Matrix4x4));
    return TRUE;
}

BOOL XMLTool::SetAttribValueBOOL(
    std::shared_ptr<XMLToolAttrib> pAttrib,
    unsigned int* pValue,
    int numValues /*= 1*/
)
{
    //if the old value was a string, then clear the string
    if (pAttrib->mValueType == XMLVAL_STRING && pAttrib->mStringValue != "") {
        mUniqueStringTable.RemoveString(pAttrib->mStringValue);
        pAttrib->mStringValue = "";
    }

    void* ptr = realloc(pAttrib->mpValue, numValues * sizeof(unsigned int));
    if (ptr == NULL) {
        DEBUG_ASSERT(0, ("XMLTool::SetAttribValue() - Out of memory."), FALSE);
        return FALSE;
    }
    pAttrib->mpValue = ptr;

    pAttrib->mValueType = XMLVAL_BOOL;
    pAttrib->mNumValueElements = numValues;
    memcpy(pAttrib->mpValue, pValue, numValues * sizeof(unsigned int));
    return TRUE;
}

////////////////////////////////////////////////////////////
//Internal Functions
////////////////////////////////////////////////////////////

void XMLTool::RecursiveProcessTagItem(
   std::shared_ptr<XMLToolTag>pTag,
   int index,
   const std::vector<BXMLToolTagItem>& pTagItems,
   int numTagItems,
   const std::vector<BXMLToolAttribItem>& pAttribItems,
   int numAttribItems,
   const std::vector<std::string>& pStringTable,
   int numStrings,
   char *pBinaryHeap,
   int heapSize
)
{
   std::shared_ptr<XMLToolTag>pChild;
   const BXMLToolTagItem *pChildItem;

   switch(pTagItems[index].ValueType) {
   case XMLVAL_STRING:
      SetTagValue(pTag, (char*)pStringTable[pTagItems[index].ValueIndex].c_str());
      break;
   case XMLVAL_BINARY:
      SetTagValue(pTag, (void*) &pBinaryHeap[pTagItems[index].ValueIndex], pTagItems[index].ValueLen);
      break;
   case XMLVAL_INT:
      SetTagValue(pTag, (int*) &pBinaryHeap[pTagItems[index].ValueIndex], pTagItems[index].ValueLen);
      break;
   case XMLVAL_UINT:
      SetTagValue(pTag, (unsigned int*) &pBinaryHeap[pTagItems[index].ValueIndex], pTagItems[index].ValueLen);
      break;
   case XMLVAL_FLOAT:
      SetTagValue(pTag, (float*) &pBinaryHeap[pTagItems[index].ValueIndex], pTagItems[index].ValueLen);
      break;
   case XMLVAL_DOUBLE:
      SetTagValue(pTag, (double*) &pBinaryHeap[pTagItems[index].ValueIndex], pTagItems[index].ValueLen);
      break;
   case XMLVAL_VECTOR4:
      SetTagValue(pTag, (Vector4*) &pBinaryHeap[pTagItems[index].ValueIndex], pTagItems[index].ValueLen);
      break;
   case XMLVAL_MATRIX4X4:
      SetTagValue(pTag, (Matrix4x4*) &pBinaryHeap[pTagItems[index].ValueIndex], pTagItems[index].ValueLen);
      break;
   case XMLVAL_BOOL:
       SetTagValueBOOL(pTag, (unsigned int*)&pBinaryHeap[pTagItems[index].ValueIndex], pTagItems[index].ValueLen);
       break;
   }

   //add all attribs
   for (int i = 0; i < pTagItems[index].NumAttribs; i++) {

       // itsmeft24 here again! the original version never had binary attrib values; it forced strings on everything.
       // between the development of this version of XMLTool and the release of Mater-National, the devs changed
       // the BXML format to allow for binary values. so instead of calling AddAttrib, we instead call SetAttribValue.
       const BXMLToolAttribItem* attribItem = &pAttribItems[pTagItems[index].FirstAttribIndex + i];

       char* pName = (char*)pStringTable[attribItem->NameStringIndex].c_str();

       pTag->mNumAttribs++;
       pTag->mpAttribs.emplace_back(std::make_shared<XMLToolAttrib>());

       auto& addedAttrib = pTag->mpAttribs[pTag->mNumAttribs-1];
       // std::shared_ptr<XMLToolAttrib> addedAttrib = &pTag->mpAttribs[pTag->mNumAttribs - 1];
       // std::memset(addedAttrib, 0, sizeof(XMLToolAttrib));

       addedAttrib->mName = "";
       addedAttrib->mStringValue = "";

       SetAttribName(pTag->mpAttribs[pTag->mNumAttribs - 1], pName);

       std::cout << attribItem->ValueType << std::endl;
       std::cout << attribItem->ValueLen << std::endl;

       switch (attribItem->ValueType) {
       case XMLVAL_STRING:
           SetAttribValue(addedAttrib, (char*)pStringTable[attribItem->ValueIndex].c_str());
           break;
       case XMLVAL_BINARY:
           SetAttribValue(addedAttrib, (void*)&pBinaryHeap[attribItem->ValueIndex], attribItem->ValueLen);
           break;
       case XMLVAL_INT:
           SetAttribValue(addedAttrib, (int*)&pBinaryHeap[attribItem->ValueIndex], attribItem->ValueLen);
           break;
       case XMLVAL_UINT:
           SetAttribValue(addedAttrib, (unsigned int*)&pBinaryHeap[attribItem->ValueIndex], attribItem->ValueLen);
           break;
       case XMLVAL_FLOAT:
           SetAttribValue(addedAttrib, (float*)&pBinaryHeap[attribItem->ValueIndex], attribItem->ValueLen);
           break;
       case XMLVAL_DOUBLE:
           SetAttribValue(addedAttrib, (double*)&pBinaryHeap[attribItem->ValueIndex], attribItem->ValueLen);
           break;
       case XMLVAL_VECTOR4:
           SetAttribValue(addedAttrib, (Vector4*)&pBinaryHeap[attribItem->ValueIndex], attribItem->ValueLen);
           break;
       case XMLVAL_MATRIX4X4:
           SetAttribValue(addedAttrib, (Matrix4x4*)&pBinaryHeap[attribItem->ValueIndex], attribItem->ValueLen);
           break;
       case XMLVAL_BOOL:
           SetAttribValueBOOL(addedAttrib, (unsigned int*)&pBinaryHeap[attribItem->ValueIndex], attribItem->ValueLen);
           break;
       }
      // AddAttrib(pTag, pStringTable[pAttribItems[pTagItems[index].FirstAttribIndex + i].NameStringIndex], pStringTable[pAttribItems[pTagItems[index].FirstAttribIndex + i].ValueStringIndex]);
   }

   //add and process children

   if (pTagItems[index].NumChildren >= 65535) {
       std::cout << "What the fuck is this" << std::endl;
   }
   else {
       for (int i = 0; i < pTagItems[index].NumChildren; i++) {
           pChildItem = &pTagItems[pTagItems[index].FirstChildIndex + i];
           pChild = AddTag(pTag, (char*)pStringTable[pChildItem->NameStringIndex].c_str());
           RecursiveProcessTagItem(pChild, pTagItems[index].FirstChildIndex + i, pTagItems, numTagItems, pAttribItems, numAttribItems, pStringTable, numStrings, pBinaryHeap, heapSize);
       }
   }
}

void XMLTool::RecursiveBuildBinaryValueHeap(
   std::shared_ptr<XMLToolTag>pTag,
   FILE *pFileOut,
   int *pTotalSize
)
{
   int ret;
   switch (pTag->mValueType) {
   case XMLVAL_BINARY:
      pTag->mBinaryValueIndex = (*pTotalSize);//get the offset we're about to write to   
      ret = (int)fwrite(pTag->mpValue, pTag->mNumValueElements, 1, pFileOut);
      DEBUG_ASSERT(pTag->mNumValueElements == 0 || ret != 0, ("XMLTool::RecursiveBuildBinaryValueHeap() - Failed to write binary data to file."), FALSE);
      (*pTotalSize) += pTag->mNumValueElements;
      break;
   case XMLVAL_INT:
      pTag->mBinaryValueIndex = (*pTotalSize);//get the offset we're about to write to   
      ret = (int)fwrite(pTag->mpValue, pTag->mNumValueElements * sizeof(int), 1, pFileOut);
      DEBUG_ASSERT(pTag->mNumValueElements == 0 || ret != 0, ("XMLTool::RecursiveBuildBinaryValueHeap() - Failed to write binary data to file."), FALSE);
      (*pTotalSize) += pTag->mNumValueElements * sizeof(int);
      break;
   case XMLVAL_UINT:
      pTag->mBinaryValueIndex = (*pTotalSize);//get the offset we're about to write to   
      ret = (int)fwrite(pTag->mpValue, pTag->mNumValueElements * sizeof(unsigned int), 1, pFileOut);
      DEBUG_ASSERT(pTag->mNumValueElements == 0 || ret != 0, ("XMLTool::RecursiveBuildBinaryValueHeap() - Failed to write binary data to file."), FALSE);
      (*pTotalSize) += pTag->mNumValueElements * sizeof(unsigned int);
      break;
   case XMLVAL_FLOAT:
      pTag->mBinaryValueIndex = (*pTotalSize);//get the offset we're about to write to   
      ret = (int)fwrite(pTag->mpValue, pTag->mNumValueElements * sizeof(float), 1, pFileOut);
      DEBUG_ASSERT(pTag->mNumValueElements == 0 || ret != 0, ("XMLTool::RecursiveBuildBinaryValueHeap() - Failed to write binary data to file."), FALSE);
      (*pTotalSize) += pTag->mNumValueElements * sizeof(float);
      break;
   case XMLVAL_DOUBLE:
      pTag->mBinaryValueIndex = (*pTotalSize);//get the offset we're about to write to   
      ret = (int)fwrite(pTag->mpValue, pTag->mNumValueElements * sizeof(double), 1, pFileOut);
      DEBUG_ASSERT(pTag->mNumValueElements == 0 || ret != 0, ("XMLTool::RecursiveBuildBinaryValueHeap() - Failed to write binary data to file."), FALSE);
      (*pTotalSize) += pTag->mNumValueElements * sizeof(double);
      break;
   case XMLVAL_VECTOR4:
      pTag->mBinaryValueIndex = (*pTotalSize);//get the offset we're about to write to   
      ret = (int)fwrite(pTag->mpValue, pTag->mNumValueElements * sizeof(Vector4), 1, pFileOut);
      DEBUG_ASSERT(pTag->mNumValueElements == 0 || ret != 0, ("XMLTool::RecursiveBuildBinaryValueHeap() - Failed to write binary data to file."), FALSE);
      (*pTotalSize) += pTag->mNumValueElements * sizeof(Vector4);
      break;
   case XMLVAL_MATRIX4X4:
      pTag->mBinaryValueIndex = (*pTotalSize);//get the offset we're about to write to   
      ret = (int)fwrite(pTag->mpValue, pTag->mNumValueElements * sizeof(Matrix4x4), 1, pFileOut);
      DEBUG_ASSERT(pTag->mNumValueElements == 0 || ret != 0, ("XMLTool::RecursiveBuildBinaryValueHeap() - Failed to write binary data to file."), FALSE);
      (*pTotalSize) += pTag->mNumValueElements * sizeof(Matrix4x4);
      break;
   case XMLVAL_BOOL:
       pTag->mBinaryValueIndex = (*pTotalSize);//get the offset we're about to write to   
       ret = (int)fwrite(pTag->mpValue, pTag->mNumValueElements * sizeof(unsigned int), 1, pFileOut);
       DEBUG_ASSERT(pTag->mNumValueElements == 0 || ret != 0, ("XMLTool::RecursiveBuildBinaryValueHeap() - Failed to write binary data to file."), FALSE);
       (*pTotalSize) += pTag->mNumValueElements * sizeof(unsigned int);
       break;
   }

   //add all attribs
   for (int i = 0; i < pTag->mNumAttribs; i++) {

       // itsmeft24 here again!

       // std::shared_ptr<XMLToolAttrib> attribItem = &pTag->mpAttribs[pTag->mAttribStartIndex + i];
       std::shared_ptr<XMLToolAttrib> attribItem = pTag->mpAttribs[i];

       switch (attribItem->mValueType) {
       case XMLVAL_BINARY:
           attribItem->mBinaryValueIndex = (*pTotalSize);//get the offset we're about to write to   
           ret = (int)fwrite(attribItem->mpValue, attribItem->mNumValueElements, 1, pFileOut);
           DEBUG_ASSERT(attribItem->mNumValueElements == 0 || ret != 0, ("XMLTool::RecursiveBuildBinaryValueHeap() - Failed to write binary data to file."), FALSE);
           (*pTotalSize) += attribItem->mNumValueElements;
           break;
       case XMLVAL_INT:
           attribItem->mBinaryValueIndex = (*pTotalSize);//get the offset we're about to write to   
           ret = (int)fwrite(attribItem->mpValue, attribItem->mNumValueElements * sizeof(int), 1, pFileOut);
           DEBUG_ASSERT(attribItem->mNumValueElements == 0 || ret != 0, ("XMLTool::RecursiveBuildBinaryValueHeap() - Failed to write binary data to file."), FALSE);
           (*pTotalSize) += attribItem->mNumValueElements * sizeof(int);
           break;
       case XMLVAL_UINT:
           attribItem->mBinaryValueIndex = (*pTotalSize);//get the offset we're about to write to   
           ret = (int)fwrite(attribItem->mpValue, attribItem->mNumValueElements * sizeof(unsigned int), 1, pFileOut);
           DEBUG_ASSERT(attribItem->mNumValueElements == 0 || ret != 0, ("XMLTool::RecursiveBuildBinaryValueHeap() - Failed to write binary data to file."), FALSE);
           (*pTotalSize) += attribItem->mNumValueElements * sizeof(unsigned int);
           break;
       case XMLVAL_FLOAT:
           attribItem->mBinaryValueIndex = (*pTotalSize);//get the offset we're about to write to   
           ret = (int)fwrite(attribItem->mpValue, attribItem->mNumValueElements * sizeof(float), 1, pFileOut);
           DEBUG_ASSERT(attribItem->mNumValueElements == 0 || ret != 0, ("XMLTool::RecursiveBuildBinaryValueHeap() - Failed to write binary data to file."), FALSE);
           (*pTotalSize) += attribItem->mNumValueElements * sizeof(float);
           break;
       case XMLVAL_DOUBLE:
           attribItem->mBinaryValueIndex = (*pTotalSize);//get the offset we're about to write to   
           ret = (int)fwrite(attribItem->mpValue, attribItem->mNumValueElements * sizeof(double), 1, pFileOut);
           DEBUG_ASSERT(attribItem->mNumValueElements == 0 || ret != 0, ("XMLTool::RecursiveBuildBinaryValueHeap() - Failed to write binary data to file."), FALSE);
           (*pTotalSize) += attribItem->mNumValueElements * sizeof(double);
           break;
       case XMLVAL_VECTOR4:
           attribItem->mBinaryValueIndex = (*pTotalSize);//get the offset we're about to write to   
           ret = (int)fwrite(attribItem->mpValue, attribItem->mNumValueElements * sizeof(Vector4), 1, pFileOut);
           DEBUG_ASSERT(attribItem->mNumValueElements == 0 || ret != 0, ("XMLTool::RecursiveBuildBinaryValueHeap() - Failed to write binary data to file."), FALSE);
           (*pTotalSize) += attribItem->mNumValueElements * sizeof(Vector4);
           break;
       case XMLVAL_MATRIX4X4:
           attribItem->mBinaryValueIndex = (*pTotalSize);//get the offset we're about to write to   
           ret = (int)fwrite(attribItem->mpValue, attribItem->mNumValueElements * sizeof(Matrix4x4), 1, pFileOut);
           DEBUG_ASSERT(attribItem->mNumValueElements == 0 || ret != 0, ("XMLTool::RecursiveBuildBinaryValueHeap() - Failed to write binary data to file."), FALSE);
           (*pTotalSize) += attribItem->mNumValueElements * sizeof(Matrix4x4);
           break;
       case XMLVAL_BOOL:
           attribItem->mBinaryValueIndex = (*pTotalSize);//get the offset we're about to write to   
           ret = (int)fwrite(attribItem->mpValue, attribItem->mNumValueElements * sizeof(unsigned int), 1, pFileOut);
           DEBUG_ASSERT(attribItem->mNumValueElements == 0 || ret != 0, ("XMLTool::RecursiveBuildBinaryValueHeap() - Failed to write binary data to file."), FALSE);
           (*pTotalSize) += attribItem->mNumValueElements * sizeof(unsigned int);
           break;
       }
   }

   for (int i = 0; i < pTag->mNumChildren; i++) {
      RecursiveBuildBinaryValueHeap(pTag->mpChildren[i], pFileOut, pTotalSize);
   }
}

void XMLTool::RecursiveGetTotalBinarySize(
   std::shared_ptr<XMLToolTag>pTag,
   int *pTotalSize
)
{
   switch (pTag->mValueType) {
   case XMLVAL_BINARY:
      (*pTotalSize) += pTag->mNumValueElements;
      break;
   case XMLVAL_INT:
      (*pTotalSize) += pTag->mNumValueElements * sizeof(int);
      break;
   case XMLVAL_UINT:
      (*pTotalSize) += pTag->mNumValueElements * sizeof(unsigned int);
      break;
   case XMLVAL_FLOAT:
      (*pTotalSize) += pTag->mNumValueElements * sizeof(float);
      break;
   case XMLVAL_DOUBLE:
      (*pTotalSize) += pTag->mNumValueElements * sizeof(double);
      break;
   case XMLVAL_VECTOR4:
      (*pTotalSize) += pTag->mNumValueElements * sizeof(Vector4);
      break;
   case XMLVAL_MATRIX4X4:
      (*pTotalSize) += pTag->mNumValueElements * sizeof(Matrix4x4);
      break;
   case XMLVAL_BOOL:
       (*pTotalSize) += pTag->mNumValueElements * sizeof(unsigned int);
       break;
   }

   //add all attribs
   for (int i = 0; i < pTag->mNumAttribs; i++) {

       // itsmeft24 here again!
       std::shared_ptr<XMLToolAttrib> attribItem = pTag->mpAttribs[i];

       switch (attribItem->mValueType) {
       case XMLVAL_BINARY:
           (*pTotalSize) += attribItem->mNumValueElements;
           break;
       case XMLVAL_INT:
           (*pTotalSize) += attribItem->mNumValueElements * sizeof(int);
           break;
       case XMLVAL_UINT:
           (*pTotalSize) += attribItem->mNumValueElements * sizeof(unsigned int);
           break;
       case XMLVAL_FLOAT:
           (*pTotalSize) += attribItem->mNumValueElements * sizeof(float);
           break;
       case XMLVAL_DOUBLE:
           (*pTotalSize) += attribItem->mNumValueElements * sizeof(double);
           break;
       case XMLVAL_VECTOR4:
           (*pTotalSize) += attribItem->mNumValueElements * sizeof(Vector4);
           break;
       case XMLVAL_MATRIX4X4:
           (*pTotalSize) += attribItem->mNumValueElements * sizeof(Matrix4x4);
           break;
       case XMLVAL_BOOL:
           (*pTotalSize) += attribItem->mNumValueElements * sizeof(unsigned int);
           break;
       }
   }

   for (int i = 0; i < pTag->mNumChildren; i++) {
      RecursiveGetTotalBinarySize(pTag->mpChildren[i], pTotalSize);
   }
}

void XMLTool::RecursiveGetTotals(
   std::shared_ptr<XMLToolTag>pTag, 
   int *pTotalTagCount, 
   int *pTotalAttribCount
   )
{
   (*pTotalTagCount)++;//account for this tag
   (*pTotalAttribCount) += pTag->mNumAttribs;

   for (int i = 0; i < pTag->mNumChildren; i++) {
      RecursiveGetTotals(pTag->mpChildren[i], pTotalTagCount, pTotalAttribCount);
   }
}

void XMLTool::RecursiveBuildStringTable(
   UniqueStringTable *pStringTable,
   std::shared_ptr<XMLToolTag>pTag
)
{
   pStringTable->AddString(pTag->mName);
   if (pTag->mValueType == XMLVAL_STRING) {
      pStringTable->AddString(pTag->mStringValue);
   }

   for (int i = 0; i < pTag->mNumAttribs; i++) {
      pStringTable->AddString(pTag->mpAttribs[i]->mName);
      if (pTag->mpAttribs[i]->mValueType == XMLVAL_STRING) {
          if (pTag->mpAttribs[i]->mStringValue[0] != NULL) {
              pStringTable->AddString(pTag->mpAttribs[i]->mStringValue);
          }
      }
   }

   for (int i = 0; i < pTag->mNumChildren; i++) {
      RecursiveBuildStringTable(pStringTable, pTag->mpChildren[i]);
   }
}

void XMLTool::RecursiveAddToGlobalLists(
   std::shared_ptr<XMLToolTag> pRoot,
   std::vector<std::shared_ptr<XMLToolTag>>& pTagList,
   int *pTagListNum,
   std::vector<std::shared_ptr<XMLToolAttrib>>& pAttribList,
   int *pAttribListNum
   )
{
   
   pRoot->mChildStartIndex = (*pTagListNum);
   pRoot->mAttribStartIndex = (*pAttribListNum);

   for (int i = 0; i < pRoot->mNumAttribs; i++) {
      pAttribList[(*pAttribListNum)] = pRoot->mpAttribs[i];
      (*pAttribListNum)++;
   }

   for (int i = 0; i < pRoot->mNumChildren; i++) {
      pTagList[(*pTagListNum)] = pRoot->mpChildren[i];
      (*pTagListNum)++;
   }

   for (int i = 0; i < pRoot->mNumChildren; i++) {
      RecursiveAddToGlobalLists(pRoot->mpChildren[i], pTagList, pTagListNum, pAttribList, pAttribListNum);
   }
}

inline int CountChars(char *pString, char countChar)
{
   int count = 0;
   for (; *pString != NULL; pString++) {
      if (*pString == countChar) {
         count++;
      }
   }
   return count;
}

inline char *PassWhiteSpace(char *pString)
{
   while (*pString == ' ' || *pString == '\n' || *pString == '\r' || *pString == '\t') {
      pString++;
   }
   return pString;
}

inline BOOL ReadIntsFromString(int *pInts, int num, char *pString)
{
   char * pCurrent = pString;
   for (int i = 0; i < num; i++) {
      if (*pCurrent == NULL) {
         return FALSE;
      }

      pInts[i] = (int)strtol(pCurrent, NULL, 0);
      //find next ','
      pCurrent = strchr(pCurrent, ',');
      if (pCurrent == NULL) {
         if (i != (num-1)) {
            return FALSE;
         }
      }
      else {
         pCurrent++;//go past ','
         pCurrent = PassWhiteSpace(pCurrent);
      }
   }
   return TRUE;
}

inline BOOL ReadUintsFromString(unsigned int *pUints, int num, char *pString)
{
   char * pCurrent = pString;
   for (int i = 0; i < num; i++) {
      if (*pCurrent == NULL) {
         return FALSE;
      }

      pUints[i] = (unsigned int)strtoul(pCurrent, NULL, 0);
      //find next ','
      pCurrent = strchr(pCurrent, ',');
      if (pCurrent == NULL) {
         if (i != (num-1)) {
            return FALSE;
         }
      }
      else {
         pCurrent++;//go past ','
         pCurrent = PassWhiteSpace(pCurrent);
      }
   }
   return TRUE;
}

inline BOOL ReadFloatsFromString(float *pFloats, int num, char *pString)
{
   char * pCurrent = pString;
   for (int i = 0; i < num; i++) {
      if (*pCurrent == NULL) {
         return FALSE;
      }

      pFloats[i] = (float)atof(pCurrent);
      //find next ','
      pCurrent = strchr(pCurrent, ',');
      if (pCurrent == NULL) {
         if (i != (num-1)) {
            return FALSE;
         }
      }
      else {
         pCurrent++;//go past ','
         pCurrent = PassWhiteSpace(pCurrent);
      }

   }
   return TRUE;
}

inline BOOL ReadDoublesFromString(double *pDoubles, int num, char *pString)
{
   char * pCurrent = pString;
   for (int i = 0; i < num; i++) {
      if (*pCurrent == NULL) {
         return FALSE;
      }

      pDoubles[i] = (double)atof(pCurrent);
      //find next ','
      pCurrent = strchr(pCurrent, ',');
      if (pCurrent == NULL) {
         if (i != (num-1)) {
            return FALSE;
         }
      }
      else {
         pCurrent++;//go past ','
         pCurrent = PassWhiteSpace(pCurrent);
      }

   }
   return TRUE;
}

inline BOOL ReadVector4sFromString(Vector4 *pVector4s, int num, char *pString)
{
   char * pCurrent = pString;
   for (int i = 0; i < num; i++) {
      if (*pCurrent == NULL) {
         return FALSE;
      }
      pVector4s[i].x = (float)atof(pCurrent);
      //find next ','
      pCurrent = strchr(pCurrent, ',');
      if (pCurrent == NULL) {
         return FALSE;
      }
      pCurrent++;//go past ','
      pCurrent = PassWhiteSpace(pCurrent);

      if (*pCurrent == NULL) {
         return FALSE;
      }
      pVector4s[i].y = (float)atof(pCurrent);
      //find next ','
      pCurrent = strchr(pCurrent, ',');
      if (pCurrent == NULL) {
         return FALSE;
      }
      pCurrent++;//go past ','
      pCurrent = PassWhiteSpace(pCurrent);

      if (*pCurrent == NULL) {
         return FALSE;
      }
      pVector4s[i].z = (float)atof(pCurrent);
      //find next ','
      pCurrent = strchr(pCurrent, ',');
      if (pCurrent == NULL) {
         return FALSE;
      }
      pCurrent++;//go past ','
      pCurrent = PassWhiteSpace(pCurrent);

      if (*pCurrent == NULL) {
         return FALSE;
      }
      pVector4s[i].w = (float)atof(pCurrent);
      //find next ','
      pCurrent = strchr(pCurrent, ',');
      if (pCurrent == NULL) {
         if (i != (num-1)) {
            return FALSE;
         }
      }
      else {
         pCurrent++;//go past ','
         pCurrent = PassWhiteSpace(pCurrent);
      }
   }
   return TRUE;
}

inline BOOL ReadMatrix44sFromString(Matrix4x4 *pMatrix44s, int num, char *pString)
{
   char * pCurrent = pString;
   for (int i = 0; i < num; i++) {
      //top row
      if (*pCurrent == NULL)return FALSE;
      pMatrix44s[i].m00 = (float)atof(pCurrent);
      pCurrent = strchr(pCurrent, ',');
      if (pCurrent == NULL)return FALSE;
      pCurrent++;//go past ','
      pCurrent = PassWhiteSpace(pCurrent);

      if (*pCurrent == NULL)return FALSE;
      pMatrix44s[i].m01 = (float)atof(pCurrent);
      pCurrent = strchr(pCurrent, ',');
      if (pCurrent == NULL)return FALSE;
      pCurrent++;//go past ','
      pCurrent = PassWhiteSpace(pCurrent);

      if (*pCurrent == NULL)return FALSE;
      pMatrix44s[i].m02 = (float)atof(pCurrent);
      pCurrent = strchr(pCurrent, ',');
      if (pCurrent == NULL)return FALSE;
      pCurrent++;//go past ','
      pCurrent = PassWhiteSpace(pCurrent);

      if (*pCurrent == NULL)return FALSE;
      pMatrix44s[i].m03 = (float)atof(pCurrent);
      pCurrent = strchr(pCurrent, ',');
      if (pCurrent == NULL)return FALSE;
      pCurrent++;//go past ','
      pCurrent = PassWhiteSpace(pCurrent);
      
      //next to top row
      if (*pCurrent == NULL)return FALSE;
      pMatrix44s[i].m10 = (float)atof(pCurrent);
      pCurrent = strchr(pCurrent, ',');
      if (pCurrent == NULL)return FALSE;
      pCurrent++;//go past ','
      pCurrent = PassWhiteSpace(pCurrent);

      if (*pCurrent == NULL)return FALSE;
      pMatrix44s[i].m11 = (float)atof(pCurrent);
      pCurrent = strchr(pCurrent, ',');
      if (pCurrent == NULL)return FALSE;
      pCurrent++;//go past ','
      pCurrent = PassWhiteSpace(pCurrent);

      if (*pCurrent == NULL)return FALSE;
      pMatrix44s[i].m12 = (float)atof(pCurrent);
      pCurrent = strchr(pCurrent, ',');
      if (pCurrent == NULL)return FALSE;
      pCurrent++;//go past ','
      pCurrent = PassWhiteSpace(pCurrent);

      if (*pCurrent == NULL)return FALSE;
      pMatrix44s[i].m13 = (float)atof(pCurrent);
      pCurrent = strchr(pCurrent, ',');
      if (pCurrent == NULL)return FALSE;
      pCurrent++;//go past ','
      pCurrent = PassWhiteSpace(pCurrent);

      //next to bottom row
      if (*pCurrent == NULL)return FALSE;
      pMatrix44s[i].m20 = (float)atof(pCurrent);
      pCurrent = strchr(pCurrent, ',');
      if (pCurrent == NULL)return FALSE;
      pCurrent++;//go past ','
      pCurrent = PassWhiteSpace(pCurrent);

      if (*pCurrent == NULL)return FALSE;
      pMatrix44s[i].m21 = (float)atof(pCurrent);
      pCurrent = strchr(pCurrent, ',');
      if (pCurrent == NULL)return FALSE;
      pCurrent++;//go past ','
      pCurrent = PassWhiteSpace(pCurrent);

      if (*pCurrent == NULL)return FALSE;
      pMatrix44s[i].m22 = (float)atof(pCurrent);
      pCurrent = strchr(pCurrent, ',');
      if (pCurrent == NULL)return FALSE;
      pCurrent++;//go past ','
      pCurrent = PassWhiteSpace(pCurrent);

      if (*pCurrent == NULL)return FALSE;
      pMatrix44s[i].m23 = (float)atof(pCurrent);
      pCurrent = strchr(pCurrent, ',');
      if (pCurrent == NULL)return FALSE;
      pCurrent++;//go past ','
      pCurrent = PassWhiteSpace(pCurrent);
      
      //bottom row
      if (*pCurrent == NULL)return FALSE;
      pMatrix44s[i].m30 = (float)atof(pCurrent);
      pCurrent = strchr(pCurrent, ',');
      if (pCurrent == NULL)return FALSE;
      pCurrent++;//go past ','
      pCurrent = PassWhiteSpace(pCurrent);

      if (*pCurrent == NULL)return FALSE;
      pMatrix44s[i].m31 = (float)atof(pCurrent);
      pCurrent = strchr(pCurrent, ',');
      if (pCurrent == NULL)return FALSE;
      pCurrent++;//go past ','
      pCurrent = PassWhiteSpace(pCurrent);

      if (*pCurrent == NULL)return FALSE;
      pMatrix44s[i].m32 = (float)atof(pCurrent);
      pCurrent = strchr(pCurrent, ',');
      if (pCurrent == NULL)return FALSE;
      pCurrent++;//go past ','
      pCurrent = PassWhiteSpace(pCurrent);

      if (*pCurrent == NULL)return FALSE;
      pMatrix44s[i].m33 = (float)atof(pCurrent);
      pCurrent = strchr(pCurrent, ',');
      if (pCurrent == NULL) {
         if (i != (num-1)) {
            return FALSE;
         }
      }
      else {
         pCurrent++;//go past ','
         pCurrent = PassWhiteSpace(pCurrent);
      }
   }
   return TRUE;
}

inline BOOL WriteIntsToString(int *pInts, int num, char *pString, int stringLen)
{
   int count, written;
   count = _snprintf(pString, stringLen, "%i", pInts[0]);
   if (count < 0) {
      return FALSE;
   }
   for (int i = 1; i < num; i++) {
      written = _snprintf(&pString[count], stringLen-count, ", %i", pInts[i]);
      if (written < 0) {
         return FALSE;
      }
      count += written;
   }
   return TRUE;
}

inline BOOL WriteUintsToString(unsigned int *pUints, int num, char *pString, int stringLen)
{
   int count, written;
   count = _snprintf(pString, stringLen, "%u", pUints[0]);
   if (count < 0) {
      return FALSE;
   }
   for (int i = 1; i < num; i++) {
      written = _snprintf(&pString[count], stringLen-count, ", %u", pUints[i]);
      if (written < 0) {
         return FALSE;
      }
      count += written;
   }
   return TRUE;
}

inline BOOL WriteFloatsToString(float *pFloats, int num, char *pString, int stringLen)
{
   int count, written;
   count = _snprintf(pString, stringLen, "%f", pFloats[0]);
   if (count < 0) {
      return FALSE;
   }
   for (int i = 1; i < num; i++) {
      written = _snprintf(&pString[count], stringLen-count, ", %f", pFloats[i]);
      if (written < 0) {
         return FALSE;
      }
      count += written;
   }
   return TRUE;
}

inline BOOL WriteDoublesToString(double *pDoubles, int num, char *pString, int stringLen)
{
   int count, written;
   count = _snprintf(pString, stringLen, "%f", pDoubles[0]);
   if (count < 0) {
      return FALSE;
   }
   for (int i = 1; i < num; i++) {
      written = _snprintf(&pString[count], stringLen-count, ", %f", pDoubles[i]);
      if (written < 0) {
         return FALSE;
      }
      count += written;
   }
   return TRUE;
}

inline BOOL WriteVector4sToString(Vector4 *pVector4s, int num, char *pString, int stringLen)
{
   int count, written;
   count = _snprintf(pString, stringLen, "%f, %f, %f, %f", pVector4s[0].x, pVector4s[0].y, pVector4s[0].z, pVector4s[0].w);
   if (count < 0) {
      return FALSE;
   }
   for (int i = 1; i < num; i++) {
      written = _snprintf(&pString[count], stringLen-count, ",  %f, %f, %f, %f", pVector4s[i].x, pVector4s[i].y, pVector4s[i].z, pVector4s[i].w);
      if (written < 0) {
         return FALSE;
      }
      count += written;
   }
   return TRUE;
}

inline BOOL WriteMatrix44sToString(Matrix4x4 *pMatrix44s, int num, char *pString, int stringLen)
{
   int count, written;
   count = _snprintf(pString, stringLen, "%f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f", pMatrix44s[0].m00, pMatrix44s[0].m01, pMatrix44s[0].m02, pMatrix44s[0].m03, pMatrix44s[0].m10, pMatrix44s[0].m11, pMatrix44s[0].m12, pMatrix44s[0].m13, pMatrix44s[0].m20, pMatrix44s[0].m21, pMatrix44s[0].m22, pMatrix44s[0].m23, pMatrix44s[0].m30, pMatrix44s[0].m31, pMatrix44s[0].m32, pMatrix44s[0].m33);
   if (count < 0) {
      return FALSE;
   }
   for (int i = 1; i < num; i++) {
      written = _snprintf(&pString[count], stringLen-count, ",  %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f", pMatrix44s[i].m00, pMatrix44s[i].m01, pMatrix44s[i].m02, pMatrix44s[i].m03, pMatrix44s[i].m10, pMatrix44s[i].m11, pMatrix44s[i].m12, pMatrix44s[i].m13, pMatrix44s[i].m20, pMatrix44s[i].m21, pMatrix44s[i].m22, pMatrix44s[i].m23, pMatrix44s[i].m30, pMatrix44s[i].m31, pMatrix44s[i].m32, pMatrix44s[i].m33);
      if (written < 0) {
         return FALSE;
      }
      count += written;
   }
   return TRUE;
}

inline BOOL WriteIntsToStream(int *pInts, int num, FILE *pStream)
{
   fprintf(pStream, "%i", pInts[0]);
   for (int i = 1; i < num; i++) {
      fprintf(pStream, ", %i", pInts[i]);
   }
   return TRUE;
}

inline BOOL WriteUintsToStream(unsigned int *pUints, int num, FILE *pStream)
{
   fprintf(pStream, "%u", pUints[0]);
   for (int i = 1; i < num; i++) {
      fprintf(pStream, ", %u", pUints[i]);
   }
   return TRUE;
}

inline BOOL WriteFloatsToStream(float *pFloats, int num, FILE *pStream)
{
   fprintf(pStream, "%f", pFloats[0]);
   for (int i = 1; i < num; i++) {
      fprintf(pStream, ", %f", pFloats[i]);
   }
   return TRUE;
}

inline BOOL WriteDoublesToStream(double *pDoubles, int num, FILE *pStream)
{
   fprintf(pStream, "%f", pDoubles[0]);
   for (int i = 1; i < num; i++) {
      fprintf(pStream, ", %f", pDoubles[i]);
   }
   return TRUE;
}

inline BOOL WriteVector4sToStream(Vector4 *pVector4s, int num, FILE *pStream)
{
   fprintf(pStream, "%f, %f, %f, %f", pVector4s[0].x, pVector4s[0].y, pVector4s[0].z, pVector4s[0].w);
   for (int i = 1; i < num; i++) {
      fprintf(pStream, ",  %f, %f, %f, %f", pVector4s[i].x, pVector4s[i].y, pVector4s[i].z, pVector4s[i].w);
   }
   return TRUE;
}

inline BOOL WriteMatrix44sToStream(Matrix4x4 *pMatrix44s, int num, FILE *pStream)
{
   fprintf(pStream, "%f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f", pMatrix44s[0].m00, pMatrix44s[0].m01, pMatrix44s[0].m02, pMatrix44s[0].m03, pMatrix44s[0].m10, pMatrix44s[0].m11, pMatrix44s[0].m12, pMatrix44s[0].m13, pMatrix44s[0].m20, pMatrix44s[0].m21, pMatrix44s[0].m22, pMatrix44s[0].m23, pMatrix44s[0].m30, pMatrix44s[0].m31, pMatrix44s[0].m32, pMatrix44s[0].m33);
   for (int i = 1; i < num; i++) {
      fprintf(pStream, ",  %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f", pMatrix44s[i].m00, pMatrix44s[i].m01, pMatrix44s[i].m02, pMatrix44s[i].m03, pMatrix44s[i].m10, pMatrix44s[i].m11, pMatrix44s[i].m12, pMatrix44s[i].m13, pMatrix44s[i].m20, pMatrix44s[i].m21, pMatrix44s[i].m22, pMatrix44s[i].m23, pMatrix44s[i].m30, pMatrix44s[i].m31, pMatrix44s[i].m32, pMatrix44s[i].m33);
   }
   return TRUE;
}

inline int CountIntChars(int number)
{
   int count = 1;
   while (number) {
      number /= 10;
      count++;
   }
   return count;
}
