//---------------------------------------------------------------------------

#ifndef mfa_readerH
#define mfa_readerH
//---------------------------------------------------------------------------
#endif

#define SECTOR_LEN 2048
#define OUT_OF_MEMORY 0
#define SUCCESS 1
#define READ_FILE 2
#define WRITE_FILE 3

#pragma once
class MFA_Reader
{
	public: TFileStream *mfa;
	public: int FILE_MODE;
	public: void ExtractFiles(String &path, String &filename);
	public: void CreateSubDir(String &src);
	public: int SaveToFile(TFileStream *src, TFileStream *dst, size_t length, size_t mfa_length);
	private: void SaveToString(TFileStream *file, String s);
	private: void UpdatePROGRESSBAR(TFileStream *src, TFileStream *dst, float mfa_length);
    private: unsigned char* alloc_mem(size_t size);
	public: void FindFirstTOCofMFA();
};

#pragma once
typedef struct
{
	unsigned int filename_offset;
	unsigned int lba;
	unsigned int data1;
	unsigned int filesize;
}
ENTRIES;

#pragma once
typedef struct
{
	String path;
}
FILELIST;
