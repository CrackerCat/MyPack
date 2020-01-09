#include <windows.h>

typedef struct _SHAREDATA
{
	// ԭʼOEP
	LONG OldOep = 0;
	// ���εĴ�С
	DWORD dwSize = 0;
	// ���ε�RVA
	DWORD dwRVA = 0;
	// ��Կ
	BYTE bKey = 0;
	// ԭʼ���ݴ�С
	INT nSrcSize = 0;
	// ѹ�������ݴ�С
	INT nDestSize = 0;
	// ѹ�����ݵ�FOA
	DWORD dwSectionRVA = 0;
	// �����ļ��ض�λ���RVA
	DWORD dwRelocRVA = 0;
	// �����ļ���ImageBase
	DWORD dwFileImageBase = 0;
	// ����ԭ��������RVA
	DWORD dwOldImportRVA = 0;
	// TLS�Ƿ����
	BOOL bTLSEnable = TRUE;
	// Indexһ��Ϊ0
	DWORD TlsIndex = 0;
	// ����TLS�����Ϣ
	IMAGE_TLS_DIRECTORY pOldTls;
}SHAREDATA, * PSHAREDATA;

class CMyPack
{
private:
	// ������PE�ļ��ļ��ػ�ַ
	DWORD* m_dwpFileBase;
	// ������DLL�ļ��ļ��ػ�ַ
	DWORD* m_dwpDllBase;
	// �����ļ��Ĵ�С
	DWORD m_dwFileSize;
	// start�Ķ���ƫ��
	DWORD m_dwStartOffset;
	// ���湲����Ϣ
	PSHAREDATA m_pShareData = nullptr;
	// �������
	CHAR m_SectionName[8] = {};
	// ����ѹ����Ĵ�С
	INT m_nCompressSize;
	// ����ѹ���ε�FOA
	DWORD m_dwSectionRVA = 0;
	// ����ѹ����ѹ�����С
	INT m_nDestSize = 0;
	// ����ѹ����ԭʼ��С
	INT m_nSrcSize = 0;
private:
	// ��ȡ��DOSͷ
	PIMAGE_DOS_HEADER GetDosHeader(DWORD* PeBase);
	// ��ȡ��NTͷ
	PIMAGE_NT_HEADERS GetNtHeader(DWORD* PeBase);
	// ��ȡ���ļ�ͷ
	PIMAGE_FILE_HEADER GetFileHeader(DWORD* PeBase);
	// ��ȡ����չͷ
	PIMAGE_OPTIONAL_HEADER GetOptHeader(DWORD* PeBase);
	// ���ڰ���ָ���ֽڽ��ж���ĺ���
	DWORD SetAlignment(DWORD Num, DWORD Alignment);
	// ��ȡ����Ϣ
	PIMAGE_SECTION_HEADER GetSection(DWORD* DllBase, LPCSTR);
	// ѹ�������
	CHAR* PackSection(LPCSTR SectionName, DWORD* DllBase);
	// RVAתFOA
	DWORD RvaToFoa(DWORD* PeBase, DWORD dwRVA);
public:
	// �������
	CMyPack(LPCSTR SectionName);
	// ���ļ����Ҽ����ļ����ڴ���
	VOID LoadFile(LPCSTR FileName);
	// Ϊ�ļ�����µ�����
	VOID CopySectionInfo(LPCSTR NewSectionName, LPCSTR SectionName);
	// ������������
	VOID CopySectionContent(LPCSTR DestSectionName, LPCSTR SrcSectionName);
	// ���浽�ļ���
	VOID SaveFile(LPCSTR FileName);
	// ��ȡ�Ǵ���dll���ڴ���
	VOID LoadStub(LPCSTR FileName);
	// ����OEP
	VOID SetOEP();
	// �޸��Ǵ�����ض�λ
	VOID FixReloc();
	// ��������
	VOID EncrySection();
	// �޸ĺ������ε�FOA
	VOID ChangeSectionRVA(LPCSTR SectionName, INT nOffset, DWORD* DllBase);
	// �����ɵ��ض�λ����Ϣ
	VOID KeepReloc();
	// ��ձ�������Ŀ¼���[1]��[12]�������
	VOID SetImport();
	// ����TLS����Ϣ
	VOID SaveTLS();
	// ����TLS
	VOID SetTLS();
};

