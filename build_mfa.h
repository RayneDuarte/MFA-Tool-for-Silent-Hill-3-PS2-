//---------------------------------------------------------------------------

#ifndef build_mfaH
#define build_mfaH
//---------------------------------------------------------------------------
#endif

#pragma once
typedef struct
{
	int block_start_lba;
	int block_length;
	int block_files_num;
	int *lba;
	int *filesize;
}
INFOS;

#pragma once
class MFA_BUILDER
{
    public: unsigned char BLOCK_END_MARKER = 0x0A;
	public: void BuildMFA(TFileStream *old_mfa, TFileStream *new_mfa, TStringList *filelist, String &path_to_new_MFA);
	private: void UpdateMFA_TOC(TFileStream *new_mfa, int blocks_num, int header_length, INFOS *FILEINFOS);
	private: void DeleteFILEINFOS(INFOS *FILEINFOS, int blocks_num);
	private: void CreatePadding(TFileStream *new_mfa, INFOS *FILEINFOS, int i);
    private: void WriteBlockEndMarker(TFileStream *file);
};

