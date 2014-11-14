#ifndef RAW_DATA_H
#define RAW_DATA_H

/************************************************************
  Description:    ��װ��ƽ�����ݽṹ�洢����      
  Version:         TTC 3.0
  Function List:   
    1.  Init()			��ʼ������
   2.  InsertRow()		����һ������
   3. DecodeRow() 	��ȡһ������
   4. ReplaceCurRow()	�滻��ǰ��
   5. DeleteCurRow()	ɾ����ǰ��
   6. CopyRow()		��Refrence��copy��ǰ��
   7. CopyAll()		��refrence��copy��������
   8. AppendNRecords()	���N���Ѿ���ʽ���õ�����
  History:        
      Paul    2008.07.01     3.0         build this moudle  
***********************************************************/

#include "bin_malloc.h"
#include "global.h"
#include "field.h"

#define PRE_DECODE_ROW 1

typedef enum _CDataType{
	DATA_TYPE_RAW,			// ƽ�����ݽṹ
	DATA_TYPE_TREE_ROOT,	// ���ĸ��ڵ�
	DATA_TYPE_TREE_NODE		// ���Ľڵ�
}CDataType;

typedef enum _enum_oper_type_
{
    OPER_DIRTY	    = 0x02,	// cover INSERT, DELETE, UPDATE
    OPER_SELECT     = 0x30,
    OPER_INSERT_OLD = 0x31,	// old stuff, same as SELECT aka useless
    OPER_UPDATE     = 0x32,
    OPER_DELETE_NA  = 0x33,	// async DELETE require quite a lot change
    OPER_FLUSH      = 0x34,	// useless too, same as SELECT
    OPER_RESV1      = 0x35,
    OPER_INSERT     = 0x36,
    OPER_RESV2      = 0x37,
} TOperType;



struct CRawFormat{
	unsigned char	m_uchDataType;	// ��������CDataType
	uint32_t m_uiDataSize;			// �����ܴ�С
	uint32_t m_uiRowCnt;			// ����
	uint8_t m_uchGetCount;			// get����
	uint16_t m_LastAccessHour;			// �������ʱ��
	uint16_t m_LastUpdateHour;			// �������ʱ��
	uint16_t m_CreateHour;			// ����ʱ��
	char	m_achKey[0];			// key
	char	m_achRows[0];			// ������
}__attribute__((packed));


// ע�⣺�޸Ĳ������ܻᵼ��handle�ı䣬�����Ҫ������±���
class CRawData
{
private:
	char*	m_pchContent; // ע�⣺��ַ���ܻ���Ϊrealloc���ı�
	uint32_t	m_uiDataSize; // ����data_type,data_size,rowcnt,key,rows�������ݴ�С
	uint32_t	m_uiRowCnt;
	uint8_t		m_uchKeyIdx;
	int		m_iKeySize;
	int		m_iLAId;
    int     m_iLCmodId;
	
	ALLOC_SIZE_T	m_uiKeyStart;
	ALLOC_SIZE_T	m_uiDataStart;
	ALLOC_SIZE_T	m_uiRowOffset;
	ALLOC_SIZE_T	m_uiOffset;
	ALLOC_SIZE_T	m_uiLAOffset;
	int m_uiGetCountOffset;
	int m_uiTimeStampOffSet;
	uint8_t m_uchGetCount;	
	uint16_t m_LastAccessHour;
	uint16_t m_LastUpdateHour;
	uint16_t m_CreateHour;
	ALLOC_SIZE_T	m_uiNeedSize; // ���һ�η����ڴ�ʧ����Ҫ�Ĵ�С
	
	MEM_HANDLE_T	_handle;
	uint64_t    	_size;
	CMallocator*	_mallocator;
	int		_autodestroy;
	
	CRawData* m_pstRef;
	char	m_szErr[200];

protected:
	template <class T>
	T* Pointer(void) const {return reinterpret_cast<T *>(_mallocator->Handle2Ptr(_handle));}
	
	int SetDataSize();
	int SetRowCount();
	int ExpandChunk(ALLOC_SIZE_T tExpSize);
	int ReAllocChunk(ALLOC_SIZE_T tSize);
	int SkipRow(const CRowValue &stRow);
	int EncodeRow(const CRowValue &stRow, unsigned char uchOp, bool expendBuf=true);
	
public:

	/*************************************************
	  Description:    ���캯��
	  Input:          pstMalloc	�ڴ������
	                     iAutoDestroy	������ʱ���Ƿ��Զ��ͷ��ڴ�
	  Output:         
	  Return:         
	*************************************************/
	CRawData(CMallocator* pstMalloc, int iAutoDestroy=0);
	
	~CRawData();
	
	const char* GetErrMsg(){ return m_szErr; }
	
	/*************************************************
	  Description:	�·���һ���ڴ棬����ʼ��
	  Input:		 uchKeyIdx	��Ϊkey���ֶ���table����±�
				iKeySize	key�ĸ�ʽ��0Ϊ�䳤����0Ϊ��������
				pchKey	Ϊ��ʽ�����key���䳤key�ĵ�0�ֽ�Ϊ����
				uiDataSize	Ϊ���ݵĴ�С������һ�η����㹻���chunk���������Ϊ0����insert row��ʱ����realloc����
	  Output:		
	  Return:		0Ϊ�ɹ�����0ʧ��
	*************************************************/
	int Init(uint8_t uchKeyIdx, int iKeySize, const char* pchKey, ALLOC_SIZE_T uiDataSize=0, int laid=-1);
	
	/*************************************************
	  Description:	attachһ���Ѿ���ʽ���õ��ڴ�
	  Input:		hHandle	�ڴ�ľ��
				uchKeyIdx	��Ϊkey���ֶ���table����±�
				iKeySize	key�ĸ�ʽ��0Ϊ�䳤����0Ϊ��������
	  Output:		
	  Return:		0Ϊ�ɹ�����0ʧ��
	*************************************************/
	int Attach(MEM_HANDLE_T hHandle, uint8_t uchKeyIdx, int iKeySize, int laid=-1,int lastcmod = -1);
	
	/*************************************************
	  Description:	��ȡ�ڴ��ľ��
	  Input:		
	  Output:		
	  Return:		����� ע�⣺�κ��޸Ĳ������ܻᵼ��handle�ı䣬�����Ҫ������±���
	*************************************************/
	MEM_HANDLE_T GetHandle(){ return _handle; }
	
	const char* GetAddr() const { return m_pchContent; }
	
	/*************************************************
	  Description:	����һ��refrence���ڵ���CopyRow()����CopyAll()��ʱ��ʹ��
	  Input:		pstRef	refrenceָ��
	  Output:		
	  Return:		
	*************************************************/
	void SetRefrence(CRawData* pstRef){ m_pstRef = pstRef; }
	
	/*************************************************
	  Description:	����key��rows�������ڴ�Ĵ�С
	  Input:		
	  Output:		
	  Return:		�����ڴ�Ĵ�С
	*************************************************/
	uint32_t DataSize() const { return m_uiDataSize; }
	
	/*************************************************
	  Description:	rows�Ŀ�ʼƫ����
	  Input:		
	  Output:		
	  Return:		rows�Ŀ�ʼƫ����
	*************************************************/
	uint32_t DataStart() const { return m_uiDataStart; }
	
	/*************************************************
	  Description:	�ڴ����ʧ��ʱ����������Ҫ���ڴ��С
	  Input:		
	  Output:		
	  Return:		��������Ҫ���ڴ��С
	*************************************************/
	ALLOC_SIZE_T NeedSize() { return m_uiNeedSize; }
	
	/*************************************************
	  Description:	��������������Ҫ���ڴ��С
	  Input:		stRow	������
	  Output:		
	  Return:		��������Ҫ���ڴ��С
	*************************************************/
	ALLOC_SIZE_T CalcRowSize(const CRowValue &stRow, int keyIndex);
	
	/*************************************************
	  Description:	��ȡ��ʽ�����key
	  Input:		
	  Output:		
	  Return:		��ʽ�����key
	*************************************************/
	const char* Key() const { return m_pchContent?(m_pchContent+m_uiKeyStart):NULL; }
	char* Key(){ return m_pchContent?(m_pchContent+m_uiKeyStart):NULL; }
	
	/*************************************************
	  Description:	��ȡkey�ĸ�ʽ
	  Input:		
	  Output:		
	  Return:		�䳤����0������key���ض����ĳ���
	*************************************************/
	int KeyFormat() const { return m_iKeySize; }
	
	/*************************************************
	  Description:	��ȡkey��ʵ�ʳ���
	  Input:		
	  Output:		
	  Return:		key��ʵ�ʳ���
	*************************************************/
	int KeySize();
	 
	unsigned int  TotalRows() const { return m_uiRowCnt; }
	void Rewind(void) { m_uiOffset = m_uiDataStart; m_uiRowOffset = m_uiDataStart;}
	
	/*************************************************
	  Description:	�����ͷ��ڴ�
	  Input:		
	  Output:		
	  Return:		0Ϊ�ɹ�����0ʧ��
	*************************************************/
	int Destroy();
	
	/*************************************************
	  Description:	�ͷŶ�����ڴ棨ͨ����deleteһЩrow�����һ�Σ�
	  Input:		
	  Output:		
	  Return:		0Ϊ�ɹ�����0ʧ��
	*************************************************/
	int StripMem();
	
	/*************************************************
	  Description:	��ȡһ������
	  Input:		
	  Output:		stRow	����������
				uchRowFlags	�������Ƿ������ݵ�flag
				iDecodeFlag	�Ƿ�ֻ��pre-read����fetch_row�ƶ�ָ��
	  Return:		0Ϊ�ɹ�����0ʧ��
	*************************************************/
	int DecodeRow(CRowValue &stRow, unsigned char& uchRowFlags, int iDecodeFlag=0);
	
	/*************************************************
	  Description:	����һ������
	  Input:		stRow	��Ҫ�����������
	  Output:		
				byFirst	�Ƿ���뵽��ǰ�棬Ĭ����ӵ������
				isDirty	�Ƿ�������
	  Return:		0Ϊ�ɹ�����0ʧ��
	*************************************************/
	int InsertRow(const CRowValue &stRow, bool byFirst, bool isDirty);
	
	/*************************************************
	  Description:	����һ������
	  Input:		stRow	��Ҫ�����������
	  Output:		
				byFirst	�Ƿ���뵽��ǰ�棬Ĭ����ӵ������
				uchOp	row�ı��
	  Return:		0Ϊ�ɹ�����0ʧ��
	*************************************************/
	int InsertRowFlag(const CRowValue &stRow, bool byFirst, unsigned char uchOp);
	
	/*************************************************
	  Description:	��������������
	  Input:		uiNRows	����
				stRow	��Ҫ�����������
	  Output:		
				byFirst	�Ƿ���뵽��ǰ�棬Ĭ����ӵ������
				isDirty	�Ƿ�������
	  Return:		0Ϊ�ɹ�����0ʧ��
	*************************************************/
	int InsertNRows(unsigned int uiNRows, const CRowValue* pstRow, bool byFirst, bool isDirty);
	
	/*************************************************
	  Description:	��ָ�������滻��ǰ��
	  Input:		stRow	�µ�������
	  Output:		
				isDirty	�Ƿ�������
	  Return:		0Ϊ�ɹ�����0ʧ��
	*************************************************/
	int ReplaceCurRow(const CRowValue &stRow, bool isDirty);
	
	/*************************************************
	  Description:	ɾ����ǰ��
	  Input:		stRow	��ʹ��row���ֶ����͵���Ϣ������Ҫʵ������
	  Output:		
	  Return:		0Ϊ�ɹ�����0ʧ��
	*************************************************/
	int DeleteCurRow(const CRowValue& stRow);
	
	/*************************************************
	  Description:	ɾ��������
	  Input:		
	  Output:		
	  Return:		0Ϊ�ɹ�����0ʧ��
	*************************************************/
	int DeleteAllRows();
	
	/*************************************************
	  Description:	���õ�ǰ�еı��
	  Input:		uchFlag	�еı��
	  Output:		
	  Return:		0Ϊ�ɹ�����0ʧ��
	*************************************************/
	int SetCurRowFlag(unsigned char uchFlag);
	
	/*************************************************
	  Description:	��refrence copy��ǰ�е�����bufferĩβ
	  Input:		
	  Output:		
	  Return:		0Ϊ�ɹ�����0ʧ��
	*************************************************/
	int CopyRow();
	
	/*************************************************
	  Description:	��refrence�������滻��������
	  Input:		
	  Output:		
	  Return:		0Ϊ�ɹ�����0ʧ��
	*************************************************/
	int CopyAll();
	
	/*************************************************
	  Description:	���N���Ѿ���ʽ���õ����ݵ�ĩβ
	  Input:		
	  Output:		
	  Return:		0Ϊ�ɹ�����0ʧ��
	*************************************************/
	int AppendNRecords(unsigned int uiNRows, const char* pchData, const unsigned int uiLen);

	/*************************************************
	  Description:	����������ʱ���
	  Input:	ʱ���	
	  Output:		
	  Return:
	*************************************************/
	void UpdateLastacc(uint32_t now) {
		if(m_uiLAOffset > 0) *(uint32_t *)(m_pchContent + m_uiLAOffset) = now;
	}
	/*************************************************
	  Description:	��ȡ������ʱ��
	  Input:	ʱ���	
	  Output:		
	  Return:
	*************************************************/
    int GetLastcmod(CRowValue& stRow,uint32_t & lastcmod);
    int CheckSize(MEM_HANDLE_T hHandle, uint8_t uchKeyIdx, int iKeySize, int size);
	
	/*************************************************
	  Description:	��ʼ��ʱ���������������ʱ��
	  ��������ʱ�䡢����ʱ��������
	  Input:	ʱ���(��ĳ�������¼�Ϊ��ʼ��Сʱ��)
	  ��Ȼ����ΪUpdate����ʵֻ�ᱻ����һ��
	  tomchen
	*************************************************/
	void InitTimpStamp();
	/*************************************************
	  Description:	���½ڵ�������ʱ��
	  Input:	ʱ���(��ĳ�������¼�Ϊ��ʼ��Сʱ��)
	   tomchen
	*************************************************/
	void UpdateLastAccessTimeByHour();
	/*************************************************
	  Description:	���½ڵ�������ʱ��
	  Input:	ʱ���(��ĳ�������¼�Ϊ��ʼ��Сʱ��)
	   tomchen
	*************************************************/
	void UpdateLastUpdateTimeByHour();
	/*************************************************
	  Description:	���ӽڵ㱻select����Ĵ���
	 tomchen
	*************************************************/
	void IncSelectCount();
	/*************************************************
	  Description:	��ȡ�ڵ㴴��ʱ��
	 tomchen
	*************************************************/
	uint32_t GetCreateTimeByHour();
	/*************************************************
	  Description:	��ȡ�ڵ�������ʱ��
	 tomchen
	*************************************************/
	uint32_t GetLastAccessTimeByHour();
	/*************************************************
	  Description:	��ȡ�ڵ�������ʱ��
	 tomchen
	*************************************************/
	uint32_t GetLastUpdateTimeByHour();
	/*************************************************
	  Description:	��ȡ�ڵ㱻select�����Ĵ���
	 tomchen
	*************************************************/
	uint32_t GetSelectOpCount();
	/*************************************************
	  Description:	attach��ʱ���
	 tomchen
	*************************************************/
	void AttachTimeStamp();
};

inline int CRawData::KeySize()
{
	return m_iKeySize > 0 ? m_iKeySize: (sizeof(char)+*(unsigned char*)Key());
}

#endif
