#include "CMyPack.h"
#include <DbgHelp.h>
#include <stdio.h>
#include <time.h>
#include "lz4.h"
#pragma comment(lib, "DbgHelp.lib")

struct TypeOffset
{
	WORD Offset : 12;
	WORD Type : 4;
};

CMyPack::CMyPack(LPCSTR SectionName)
{
	memcpy(m_SectionName, SectionName, strlen(SectionName) <= 8 ? strlen(SectionName) : 8);
}

//******************************************************************************
// ��������: GetDosHeader
// ����˵��: ��ȡ��Dosͷ
// ��    ��: lracker
// ʱ    ��: 2019/12/07
// ��    ��: DWORD PeBase
// �� �� ֵ: PIMAGE_DOS_HEADER
//******************************************************************************
PIMAGE_DOS_HEADER CMyPack::GetDosHeader(DWORD* PeBase)
{
	return (PIMAGE_DOS_HEADER)PeBase;
}

//******************************************************************************
// ��������: GetNtHeader
// ����˵��: ��ȡ��NTͷ
// ��    ��: lracker
// ʱ    ��: 2019/12/07
// ��    ��: DWORD PeBase
// �� �� ֵ: PIMAGE_NT_HEADERS
//******************************************************************************
PIMAGE_NT_HEADERS CMyPack::GetNtHeader(DWORD* PeBase)
{
	return (PIMAGE_NT_HEADERS)(GetDosHeader(PeBase)->e_lfanew + (DWORD)PeBase);
}

//******************************************************************************
// ��������: GetFileHeader
// ����˵��: ��ȡ���ļ�ͷ
// ��    ��: lracker
// ʱ    ��: 2019/12/07
// ��    ��: DWORD PeBase
// �� �� ֵ: PIMAGE_FILE_HEADER
//******************************************************************************
PIMAGE_FILE_HEADER CMyPack::GetFileHeader(DWORD* PeBase)
{
	return (PIMAGE_FILE_HEADER)&GetNtHeader(PeBase)->FileHeader;
}

//******************************************************************************
// ��������: GetOpt
// ����˵��: ��ȡ������ͷ
// ��    ��: lracker
// ʱ    ��: 2019/12/07
// ��    ��: DWORD PeBase
// �� �� ֵ: PIMAGE_OPTIONAL_HEADER
//******************************************************************************
PIMAGE_OPTIONAL_HEADER CMyPack::GetOptHeader(DWORD* PeBase)
{
	return (PIMAGE_OPTIONAL_HEADER)&GetNtHeader(PeBase)->OptionalHeader;
}

//******************************************************************************
// ��������: SetAlignment
// ����˵��: ���ڰ���ָ���ֽڽ��ж���ĺ���
// ��    ��: lracker
// ʱ    ��: 2019/12/08
// ��    ��: DWORD Num
// ��    ��: DWORD Alignment
// �� �� ֵ: DWORD
//******************************************************************************
DWORD CMyPack::SetAlignment(DWORD Num, DWORD Alignment)
{
	return Num % Alignment == 0 ? Num : (Num / Alignment + 1) * Alignment;
}

//******************************************************************************
// ��������: GetSection
// ����˵��: ��ȡָ���ε���Ϣ
// ��    ��: lracker
// ʱ    ��: 2019/12/08
// ��    ��: DWORD * DllBase
// ��    ��: LPCSTR SectionName
// �� �� ֵ: PIMAGE_SECTION_HEADER
//******************************************************************************
PIMAGE_SECTION_HEADER CMyPack::GetSection(DWORD* Base, LPCSTR SectionName)
{
	PIMAGE_SECTION_HEADER SectionTable = IMAGE_FIRST_SECTION(GetNtHeader(Base));
	for (int i = 0; i < GetFileHeader(Base)->NumberOfSections; ++i)
	{
		if (!memcmp(SectionTable[i].Name, SectionName, strlen(SectionName) + 1))
			return &SectionTable[i];
	}
	return nullptr;
}

//******************************************************************************
// ��������: LoadFile
// ����˵��: ���ļ����Ҽ����ļ����ڴ���
// ��    ��: lracker
// ʱ    ��: 2019/12/07
// ��    ��: LPCTSTR FileName
// �� �� ֵ: VOID
//******************************************************************************
VOID CMyPack::LoadFile(LPCSTR FileName)
{
	// ��ȡ���ļ����
	HANDLE hFile = CreateFile(FileName, GENERIC_READ, NULL, NULL, OPEN_EXISTING, NULL, NULL);
	// ��ȡ���ļ��Ĵ�С
	m_dwFileSize = GetFileSize(hFile, NULL);
	// ����ռ�������ļ�������
	m_dwpFileBase = (DWORD*)malloc(m_dwFileSize * sizeof(BYTE));
	memset(m_dwpFileBase, 0, m_dwFileSize);
	// ��ȡʵ�ʶ�ȡ�Ĵ�С
	DWORD dwRead = 0;
	// ��ȡ�ļ�
	ReadFile(hFile, m_dwpFileBase, m_dwFileSize, &dwRead, NULL);
	// ��ֹ���й¶���رվ��
	CloseHandle(hFile);
	// ѹ�������
	char* NewPe = PackSection(".text", m_dwpFileBase);
	// �ͷŵ�֮ǰ�Ŀռ�
	free(m_dwpFileBase);
	m_dwpFileBase = (DWORD*)NewPe;
}

//******************************************************************************
// ��������: AddSection
// ����˵��: Ϊ�ļ�����µ�����
// ��    ��: lracker
// ʱ    ��: 2019/12/07
// ��    ��: LPCTSTR SectionName
// �� �� ֵ: VOID
//******************************************************************************
VOID CMyPack::CopySectionInfo(LPCSTR NewSectionName, LPCSTR SectionName)
{
	// ��ȡ�����һ�����ε�����
	PIMAGE_SECTION_HEADER LastSection = &IMAGE_FIRST_SECTION(GetNtHeader(m_dwpFileBase))[GetFileHeader(m_dwpFileBase)->NumberOfSections - 1];
	// ����µ����α���Ϣ�ṹ��
	// �ļ�ͷ�����������+1
	GetFileHeader(m_dwpFileBase)->NumberOfSections++;
	// ͨ�����һ�������ҵ��µ�����
	PIMAGE_SECTION_HEADER NewSection = LastSection + 1;
	memset(NewSection, 0, sizeof(IMAGE_SECTION_HEADER));
	// ��dll���ҵ�����Ҫ���Ƶ�������
	PIMAGE_SECTION_HEADER  SrcSection = GetSection(m_dwpDllBase, SectionName);
	// ��Դ�����и��ƽṹ����Ϣ��Ŀ������
	memcpy(NewSection, SrcSection, sizeof(IMAGE_SECTION_HEADER));
	// �������ε�����
	memcpy(NewSection->Name, NewSectionName, strlen(NewSectionName) <= 8 ? strlen(NewSectionName) : 8);
	// �������εĴ�С���ֱ�ΪMisc.VirtualSize �� SizeOfRawData
//	NewSection->Misc.VirtualSize = NewSection->SizeOfRawData = SrcSection->SizeOfRawData;
	// �������ε�RVA = ��һ�����ε�RVA + ��һ�����ζ������ڴ��С
	NewSection->VirtualAddress = LastSection->VirtualAddress + SetAlignment(LastSection->Misc.VirtualSize, GetOptHeader(m_dwpFileBase)->SectionAlignment);
	// �������ε�FOA = ��һ�����ε�FOA + ��һ�����ζ������ڴ��С
	NewSection->PointerToRawData = LastSection->PointerToRawData + SetAlignment(LastSection->SizeOfRawData, GetOptHeader(m_dwpFileBase)->FileAlignment);
	// �����µ����α��е�����: ��������
	NewSection->Characteristics = SrcSection->Characteristics;
	// ��PE�ļ�������µ�����
	m_dwFileSize = NewSection->PointerToRawData + NewSection->SizeOfRawData;
	DWORD SizeOfImage = NewSection->VirtualAddress + NewSection->Misc.VirtualSize;
	// ��������һƬ�ڴ�ռ䣬��Ϊ���Գ�ʼ��Ϊ0.
	DWORD* TempBase = (DWORD*)malloc(m_dwFileSize * sizeof(BYTE));
	memset(TempBase, 0, m_dwFileSize);
	// NewSection->PointerToRawData��ʵ֮ǰ�Ŀռ��Сһ��
	memcpy(TempBase, m_dwpFileBase, NewSection->PointerToRawData);
	free(m_dwpFileBase);
	m_dwpFileBase = TempBase;
	// �޸�SizeOfImage�Ĵ�С 
	GetOptHeader(m_dwpFileBase)->SizeOfImage = SizeOfImage;
}

//******************************************************************************
// ��������: CopySectionContent
// ����˵��: ������������
// ��    ��: lracker
// ʱ    ��: 2019/12/09
// �� �� ֵ: VOID
//******************************************************************************
VOID CMyPack::CopySectionContent(LPCSTR DestSectionName, LPCSTR SrcSectionName)
{
	// ���ƶ�������
	BYTE* SrcData = (BYTE*)(GetSection(m_dwpDllBase, SrcSectionName)->VirtualAddress + (DWORD)m_dwpDllBase);
	BYTE* DstData = (BYTE*)(GetSection(m_dwpFileBase, DestSectionName)->PointerToRawData + (DWORD)m_dwpFileBase);
	memcpy(DstData, SrcData, GetSection(m_dwpDllBase, SrcSectionName)->SizeOfRawData);
}

//******************************************************************************
// ��������: SaveFile
// ����˵��: ���浽�ļ���
// ��    ��: lracker
// ʱ    ��: 2019/12/08
// ��    ��: LPCTSTR FileName
// �� �� ֵ: VOID
//******************************************************************************
VOID CMyPack::SaveFile(LPCSTR FileName)
{
	// ��ȡ�����ļ��ľ��
	HANDLE hFile = CreateFile(FileName, GENERIC_WRITE, NULL, NULL, CREATE_ALWAYS, NULL, NULL);
	// ������д���ļ���
	DWORD dwWrite = NULL;
	WriteFile(hFile, m_dwpFileBase, m_dwFileSize, &dwWrite, NULL);
	// д���Ժ�رվ�������ͷſռ�
	CloseHandle(hFile);
	free(m_dwpFileBase);
	m_dwpFileBase = NULL;
} 

//******************************************************************************
// ��������: LoadStub
// ����˵��: ��ȡ�Ǵ���dll���ڴ���
// ��    ��: lracker
// ʱ    ��: 2019/12/08
// ��    ��: LPCSTR FileName
// �� �� ֵ: VOID
//******************************************************************************
VOID CMyPack::LoadStub(LPCSTR FileName)
{
	// ��ִ��DLL��ʼ���ķ�ʽ����DLL
	m_dwpDllBase = (DWORD*)LoadLibraryExA(FileName, NULL, DONT_RESOLVE_DLL_REFERENCES);
	// ��dll��ȡ��start���������Ҽ�������Ķ���ƫ��
	DWORD Start = (DWORD)GetProcAddress((HMODULE)m_dwpDllBase, "start");
	m_dwStartOffset = Start - (DWORD)m_dwpDllBase - GetSection(m_dwpDllBase,".text")->VirtualAddress;
	m_pShareData = (PSHAREDATA)GetProcAddress((HMODULE)m_dwpDllBase, "ShareData");
	// ��������䵽����������
	m_pShareData->nSrcSize = m_nSrcSize;
	m_pShareData->nDestSize = m_nDestSize;
	m_pShareData->dwSectionRVA = m_dwSectionRVA;
}

//******************************************************************************
// ��������: SetOEP
// ����˵��: ����OEP
// ��    ��: lracker
// ʱ    ��: 2019/12/09
// �� �� ֵ: VOID
//******************************************************************************
VOID CMyPack::SetOEP()
{
	// ����ɵ�OEP��RVA
	m_pShareData->OldOep = GetOptHeader(m_dwpFileBase)->AddressOfEntryPoint;
	GetOptHeader(m_dwpFileBase)->AddressOfEntryPoint = GetSection(m_dwpFileBase, m_SectionName)->VirtualAddress + m_dwStartOffset;
}

//******************************************************************************
// ��������: FixReloc
// ����˵��: �޸��Ǵ�����ض�λ
// ��    ��: lracker
// ʱ    ��: 2019/12/09
// �� �� ֵ: VOID
//******************************************************************************
VOID CMyPack::FixReloc()
{
	// �޸�DLL���.reloc���ݣ��޸����������ЩRVAΪ�Ƕ�.text��RVA
	PIMAGE_SECTION_HEADER pSection = GetSection(m_dwpFileBase, ".MyPack");
	DWORD dwSize = 0, dwOldProtect = 0;
	// ��ȡ��������ض�λ��
	PIMAGE_BASE_RELOCATION pRelocTable = (PIMAGE_BASE_RELOCATION)ImageDirectoryEntryToData(m_dwpDllBase, TRUE, 5, &dwSize);
	while (pRelocTable->SizeOfBlock)
	{
		VirtualProtect((LPVOID)pRelocTable, 0x8, PAGE_READWRITE, &dwOldProtect);
		// ÿһҳ��VirutalAddress����������Ƕε�RVA
		pRelocTable->VirtualAddress += pSection->VirtualAddress - GetSection(m_dwpDllBase,".text")->VirtualAddress;
		VirtualProtect((LPVOID)pRelocTable, 0x8, dwOldProtect, &dwOldProtect);

		// ����ض�λ�������ڴ���Σ�����Ҫ�޸ķ�������
		VirtualProtect((LPVOID)(pRelocTable->VirtualAddress + (DWORD)m_dwpFileBase),
			0x1000, PAGE_READWRITE, &dwOldProtect);

		// ��ȡ�ض�λ��������׵�ַ���ض�λ�������
		int count = (pRelocTable->SizeOfBlock - 8) / 2;
		TypeOffset* to = (TypeOffset*)(pRelocTable + 1);

		// ����ÿһ���ض�λ��������
		for (int i = 0; i < count; ++i)
		{
			// ��� type ��ֵΪ 3 ���ǲ���Ҫ��ע
			if (to[i].Type == 3)
			{
				DWORD Temp = RvaToFoa(m_dwpFileBase, pRelocTable->VirtualAddress);
				// ��ȡ����Ҫ�ض�λ�ĵ�ַ���ڵ�λ��
				DWORD* addr = (DWORD*)((DWORD)m_dwpFileBase + Temp + to[i].Offset);
				DWORD Item = *addr - (DWORD)m_dwpDllBase - GetSection(m_dwpDllBase, ".text")->VirtualAddress;
				// ���������Ķ���ƫ�� = *addr - imagebase - .text va
				*addr = Item + GetSection(m_dwpFileBase, ".MyPack")->VirtualAddress + 0x400000;
			}
		}

		// ��ԭԭ���εĵı�������
		VirtualProtect((LPVOID)(pRelocTable->VirtualAddress + (DWORD)m_dwpFileBase),
			0x1000, dwOldProtect, &dwOldProtect);
		// �ҵ���һ���ض�λ��
		pRelocTable = (PIMAGE_BASE_RELOCATION)((DWORD)pRelocTable + pRelocTable->SizeOfBlock);
	}
	// ����.reloc����
	CopySectionContent(".nreloc", ".reloc");

}

//******************************************************************************
// ��������: EncrySection
// ����˵��: ��������
// ��    ��: lracker
// ʱ    ��: 2019/12/09
// �� �� ֵ: VOID
//******************************************************************************
VOID CMyPack::EncrySection()
{
	// ���ܴ����
	PIMAGE_SECTION_HEADER pSection = GetSection(m_dwpFileBase, ".text");
	m_pShareData->dwRVA = pSection->VirtualAddress;
	m_pShareData->dwSize = pSection->SizeOfRawData;
	// pDataָ������
	BYTE* pData = (BYTE*)(pSection->PointerToRawData + (DWORD)m_dwpFileBase);
	srand(time(NULL));
	// m_pShareData->bKey = rand() % 0xFF;
	m_pShareData->bKey = 0x11;
	for (int i = 0; i < pSection->SizeOfRawData; ++i)
		pData[i] ^= m_pShareData->bKey;
}

//******************************************************************************
// ��������: PackSection
// ����˵��: ѹ�������
// ��    ��: lracker
// ʱ    ��: 2019/12/09
// ��    ��: LPCSTR SectionName
// �� �� ֵ: VOID
//******************************************************************************
CHAR* CMyPack::PackSection(LPCSTR SectionName, DWORD* DllBase)
{
	// �ҵ����text��
	PIMAGE_SECTION_HEADER pSection = GetSection(DllBase, SectionName);
	// ����ԭ���Ĵ�С
	int nSrcSize = pSection->SizeOfRawData;
	// ���浽����������
	m_nSrcSize = nSrcSize;
	// ��ȡԤ����ѹ������ֽ���(������)��
	m_nCompressSize = LZ4_compressBound(nSrcSize);
	// �����ڴ�ռ䣬���ڱ���ѹ���������
	char* pBuffer = new char[m_nCompressSize]();
	// ��ʼѹ���ļ�����(��������ѹ����Ĵ�С)
	int nDestSize = LZ4_compress((char*)(pSection->PointerToRawData + (DWORD)DllBase), pBuffer, pSection->SizeOfRawData);
	nDestSize = SetAlignment(nDestSize, GetOptHeader(DllBase)->FileAlignment);
	// ���浽����������
	m_nDestSize = nDestSize;
	// ��������RVA
	m_dwSectionRVA = pSection->VirtualAddress;
	// ����һ���µ�PE�ļ�
	CHAR* NewPe = new CHAR[m_dwFileSize - nSrcSize + nDestSize]();
	int nSize1 = pSection->PointerToRawData;
	int nSize2 = pSection->PointerToRawData + pSection->SizeOfRawData;
	// �޸�.text���εĴ�С
	pSection->SizeOfRawData = nDestSize;
	int nOffset = nSrcSize - nDestSize;
	// ��������ε�FOA��ǰ�ƶ�nOffset���ֽ�
	ChangeSectionRVA(SectionName, nOffset, DllBase);
	// �޸��ļ��ܴ�СΪѹ������ܴ�С
	GetOptHeader(DllBase)->SizeOfImage -= nOffset;
	// ����.text����֮ǰ�����ݹ�ȥ
	memcpy(NewPe, DllBase,nSize1);
	// ����ѹ�����text��
	memcpy(NewPe + nSize1, pBuffer, nDestSize);
	// ����.text����֮������ݹ�ȥ
	memcpy(NewPe + nSize1 + nDestSize, (DWORD*)((DWORD)DllBase + nSize2), m_dwFileSize - nSize2);
	return NewPe;
}

//******************************************************************************
// ��������: RvaToFoa
// ����˵��: ����RVAתFOA��
// ��    ��: lracker
// ʱ    ��: 2019/12/11
// ��    ��: DWORD * PeBase
// ��    ��: DWORD dwRVA
// �� �� ֵ: DWORD*
//******************************************************************************
DWORD CMyPack::RvaToFoa(DWORD* PeBase, DWORD dwRVA)
{
	PIMAGE_SECTION_HEADER pSection = IMAGE_FIRST_SECTION(GetNtHeader(PeBase));
	for (int i = 0; i < GetFileHeader(PeBase)->NumberOfSections; ++i)
	{
		if ((dwRVA >= pSection[i].VirtualAddress) && (dwRVA < pSection[i].VirtualAddress + pSection[i].SizeOfRawData))
			return dwRVA - pSection[i].VirtualAddress + pSection[i].PointerToRawData;
	}
}

//******************************************************************************
// ��������: ChangeSectionRVA
// ����˵��: �޸ĺ������ε�RVA
// ��    ��: lracker
// ʱ    ��: 2019/12/10
// ��    ��: LPCSTR SectionName
// ��    ��: INT nOffset
// ��    ��: DWORD * DllBase
// �� �� ֵ: VOID
//******************************************************************************
VOID CMyPack::ChangeSectionRVA(LPCSTR SectionName, INT nOffset, DWORD* DllBase)
{
	PIMAGE_SECTION_HEADER SectionTable = IMAGE_FIRST_SECTION(GetNtHeader(DllBase));
	int nIndex = 0;
	for (int i = 0; i < GetFileHeader(DllBase)->NumberOfSections; ++i)
	{
		if (!memcmp(SectionTable[i].Name, SectionName, strlen(SectionName) + 1))
		{
			nIndex = i + 1;
			break;
		}
	}
	// �����������FOA-nOffset
	for (int i = nIndex; i < GetFileHeader(DllBase)->NumberOfSections; ++i)
	{
		SectionTable[i].PointerToRawData -= nOffset;
	}
}

VOID CMyPack::KeepReloc()
{
	// �����ɵ�����Ŀ¼�����ض�λ���RVA
	m_pShareData->dwRelocRVA = GetOptHeader(m_dwpFileBase)->DataDirectory[5].VirtualAddress;
	// �޸��ļ�������Ŀ¼��Ϊ".nreloc"��RVA
	PIMAGE_SECTION_HEADER pReloc = GetSection(m_dwpFileBase, ".nreloc");
	GetOptHeader(m_dwpFileBase)->DataDirectory[5].VirtualAddress = pReloc->VirtualAddress;
	GetOptHeader(m_dwpFileBase)->DataDirectory[5].Size = GetOptHeader(m_dwpDllBase)->DataDirectory[5].Size;
	m_pShareData->dwFileImageBase = GetOptHeader(m_dwpFileBase)->ImageBase;
}

//******************************************************************************
// ��������: SetImport
// ����˵��: �������Ŀ¼���[1]��͵�[12]������
// ��    ��: lracker
// ʱ    ��: 2019/12/12
// �� �� ֵ: VOID
//******************************************************************************
VOID CMyPack::SetImport()
{
	// ����ԭ����ĵ����
	m_pShareData->dwOldImportRVA = GetOptHeader(m_dwpFileBase)->DataDirectory[1].VirtualAddress;
	// ��յ����
	GetOptHeader(m_dwpFileBase)->DataDirectory[1].VirtualAddress = 0;
	GetOptHeader(m_dwpFileBase)->DataDirectory[1].Size = 0;
	// ���IAT��
	GetOptHeader(m_dwpFileBase)->DataDirectory[12].VirtualAddress = 0;
	GetOptHeader(m_dwpFileBase)->DataDirectory[12].Size = 0;
	return;
}

//******************************************************************************
// ��������: SaveTLS
// ����˵��: ����TLS�ε���Ϣ
// ��    ��: lracker
// ʱ    ��: 2019/12/12
// �� �� ֵ: VOID
//******************************************************************************
VOID CMyPack::SaveTLS()
{
	// ����TLS������
	if (GetOptHeader(m_dwpFileBase)->DataDirectory[9].VirtualAddress == 0)
	{
		m_pShareData->bTLSEnable = FALSE;
		return;
	}
	else
	{
		m_pShareData->bTLSEnable = TRUE;
		PIMAGE_TLS_DIRECTORY32 TlsDir = (PIMAGE_TLS_DIRECTORY32)(RvaToFoa(m_dwpFileBase, GetOptHeader(m_dwpFileBase)->DataDirectory[9].VirtualAddress) + (DWORD)m_dwpFileBase);
		// ��ȡ��TlsIndex��Offset
		DWORD dwIndexFoa = RvaToFoa(m_dwpFileBase, TlsDir->AddressOfIndex - GetOptHeader(m_dwpFileBase)->ImageBase);
		m_pShareData->TlsIndex = 0;
		if (dwIndexFoa != -1)
			m_pShareData->TlsIndex = *(DWORD*)(dwIndexFoa + (DWORD)m_dwpFileBase);
		m_pShareData->pOldTls.StartAddressOfRawData = TlsDir->StartAddressOfRawData;
		m_pShareData->pOldTls.EndAddressOfRawData = TlsDir->EndAddressOfRawData;
		m_pShareData->pOldTls.AddressOfCallBacks = TlsDir->AddressOfCallBacks;
	}
}

//******************************************************************************
// ��������: SetTLS
// ����˵��: ����TLS
// ��    ��: lracker
// ʱ    ��: 2019/12/12
// �� �� ֵ: VOID
//******************************************************************************
VOID CMyPack::SetTLS()
{
	if (m_pShareData->bTLSEnable == FALSE)
		return;
	// ��ԭ���������Ŀ���ھ���ָ��ǵ�����Ŀ¼��
	GetOptHeader(m_dwpFileBase)->DataDirectory[9].VirtualAddress = GetOptHeader(m_dwpDllBase)->DataDirectory[9].VirtualAddress - 0x1000 + GetSection(m_dwpFileBase, ".MyPack")->VirtualAddress;
	GetOptHeader(m_dwpFileBase)->DataDirectory[9].Size = GetOptHeader(m_dwpDllBase)->DataDirectory[9].Size;
	PIMAGE_TLS_DIRECTORY32 pTls = (PIMAGE_TLS_DIRECTORY32)(RvaToFoa(m_dwpFileBase, GetOptHeader(m_dwpFileBase)->DataDirectory[9].VirtualAddress) + (DWORD)m_dwpFileBase);
	DWORD IndexRva = (DWORD)&(m_pShareData->TlsIndex) - (DWORD)m_dwpDllBase - 0x1000 + GetSection(m_dwpFileBase, ".MyPack")->VirtualAddress;
	pTls->AddressOfIndex = IndexRva + GetOptHeader(m_dwpFileBase)->ImageBase;
	pTls->StartAddressOfRawData = m_pShareData->pOldTls.StartAddressOfRawData;
	pTls->EndAddressOfRawData = m_pShareData->pOldTls.EndAddressOfRawData;
	// ������ȡ��TLS�Ļص�����������ṹ���д���TLS�ص�����ָ�룬�ڿ����ֶ�����TLS������TLS�ص�����ָ�����û�ȥ��
//	pTls->AddressOfCallBacks = 0;
}
