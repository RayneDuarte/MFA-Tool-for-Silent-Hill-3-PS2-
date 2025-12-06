#pragma hdrstop
#include "main.h"
#include "mfa_reader.h"
#include <System.IOUtils.hpp>
//---------------------------------------------------------------------------
#pragma package(smart_init)

int BUFFER_LEN = 8388608; //8MB
extern bool READ_MFA;

void MFA_Reader::ExtractFiles(String &path, String &filename)
{
	mfa = new TFileStream(path, fmOpenRead);
	size_t mfa_length = mfa->Size;

	FindFirstTOCofMFA();
	int block_start_lba = 0, block_n = 0;

	wchar_t *s = filename.c_str();
	String files_dir = *s++;
	while (*s != L'.')
		files_dir += *s++;

	String logfile = files_dir + ".txt";

	TFileStream *list = new TFileStream(logfile, fmCreate);
	if (list == nullptr)
	{
		ShowMessage("Failed to create the log file for the extracted files");
		return;
	}
	TDirectory::CreateDirectory(files_dir);

	FILE_MODE = READ_FILE;
	READ_MFA = true;

	while (READ_MFA)
	{
		Form1->block_num->Caption = "BLOCK: ";
		Form1->block_num->Caption += IntToStr(block_n + 1);
		int block_files_num, block_files_len;
		mfa->Read(&block_files_num, 4);
		mfa->Read(&block_files_len, 4);

		int next_block_files_lba;
		next_block_files_lba = block_files_len;
		next_block_files_lba += block_start_lba;
		next_block_files_lba += SECTOR_LEN;

		FILELIST *LIST;
		try
		{
			LIST = new FILELIST[block_files_num];
		}
		catch(std::bad_alloc())
		{
			ShowMessage("Out of memory!");
			delete mfa;
			delete list;
			READ_MFA = false;
			return;
		}

		//faz a extração dos arquivos do bloco
		int i = 0;
		while (i < block_files_num)
		{
			ENTRIES Entry;
			mfa->ReadBuffer(&Entry, sizeof(ENTRIES));
			int next_entry = mfa->Position;
			int lba = (Entry.lba + block_start_lba) + SECTOR_LEN;
			int filename_off = Entry.filename_offset + block_start_lba;

			mfa->Seek(filename_off, soFromBeginning);
			LIST[i].path = files_dir + "\\";
			while (1)
			{
				unsigned char c;
				mfa->Read(&c, 1);
				if (c == '/') LIST[i].path += "\\";
				else if (c == ' ') LIST[i].path += "_";
				else if (c >= 0xA1 && c <= 0xD4) LIST[i].path += "_";
				else if (c == 0) break;
				else LIST[i].path += (wchar_t)c;
			}
			Form1->filepath->Update();
			Form1->filepath->Caption = LIST[i].path;
			CreateSubDir(LIST[i].path);
			TFileStream *save = new TFileStream(LIST[i].path, fmCreate);
			Application->ProcessMessages();

			mfa->Seek(lba, soFromBeginning);
			int st = SaveToFile(mfa, save, Entry.filesize, mfa_length);
			if (st != SUCCESS) ShowMessage("There was an error when to save the file at " + LIST[i].path);
			mfa->Seek(next_entry, soFromBeginning);
            delete save;
			i++;
		}
		SaveToString(list, (String)block_start_lba);
		SaveToString(list, (String)block_files_num);

		i = 0;
		while (i < block_files_num)
			SaveToString(list, LIST[i++].path);
		delete[] LIST;

		if ((next_block_files_lba + 8) >= mfa_length)
			READ_MFA = false;
		else
		{
			mfa->Seek(next_block_files_lba + 8, soFromBeginning);
			block_start_lba = next_block_files_lba;
		}
		block_n++;
		Form1->block_num->Update();
	}
	SaveToString(list, (String)block_n);
	delete list;
	delete mfa;
	Form1->ProgressBar->Position = 100;
	ShowMessage("Finished Extraction!");
	Form1->ProgressBar->Position = 0;
    READ_MFA = false;
}

void MFA_Reader::SaveToString(TFileStream *file, String s)
{
	s += L'\n';
	TBytes bytes = TEncoding::UTF8->GetBytes(s);
	file->WriteBuffer(bytes, bytes.Length);
}

void MFA_Reader::CreateSubDir(String &src)
{
	char dir[MAX_PATH], path[MAX_PATH];
	int k = 0, m = 0, n = 0, directory = 0;
	wchar_t *s = src.c_str();
	while (*s != L'\0')
		dir[k++] = (char)*s++;
	dir[k] = 0;
	k = 0;

	while (dir[k] != '\0')
	{
		if (dir[k] == 0x5C) directory++;
		k++;
	}
	k = 0;
	while (k < directory)
	{
		if (k > 0)
		{
			path[n] = 0x5C;
			n++;
		}
		while (dir[m] != 0x5C)
		{
			path[n] = dir[m];
			m++;
			n++;
		}
		path[n] = '\0';
		TDirectory::CreateDirectory(path);
		m++;
		k++;
	}
}

int MFA_Reader::SaveToFile(TFileStream *src, TFileStream *dst, size_t length, size_t mfa_length)
{
	if (length <= BUFFER_LEN)
	{
		byte *buff = alloc_mem(length);
		if (!buff) return OUT_OF_MEMORY;
		src->ReadBuffer(buff, length);
		dst->WriteBuffer(buff, length);
		UpdatePROGRESSBAR(src, dst, mfa_length);
		delete[] buff;
	}
	else
	{
		byte *buff = alloc_mem(BUFFER_LEN);
		if (!buff) return OUT_OF_MEMORY;
		while (1)
		{
			if (length >= BUFFER_LEN)
			{
				Application->ProcessMessages();
				src->ReadBuffer(buff, BUFFER_LEN);
				dst->WriteBuffer(buff, BUFFER_LEN);
				length -= BUFFER_LEN;
				if (length == 0) break;
			}
			else
			{
				delete[] buff;
				buff = alloc_mem(length);
				if (!buff) return OUT_OF_MEMORY;
				src->ReadBuffer(buff, length);
				dst->WriteBuffer(buff, length);
				break;
			}
			UpdatePROGRESSBAR(src, dst, mfa_length);
		}
		delete[] buff;
	}
	return SUCCESS;
}

void MFA_Reader::UpdatePROGRESSBAR(TFileStream *src, TFileStream *dst, float mfa_length)
{
	float percent;
	if (FILE_MODE == READ_FILE) percent = src->Position;
	else percent = dst->Position;
	Form1->ProgressBar->Position = (percent / mfa_length) * 100.0;
}

unsigned char *MFA_Reader::alloc_mem(size_t size)
{
	return (new byte[size]);
}

void MFA_Reader::FindFirstTOCofMFA()
{
	char test[20];
	mfa->Seek(0xA0, soFromBeginning);
	mfa->Read(test, 16);
	test[15] = 0;

	if (strstr(test, "/mv0.mfa.new") != NULL)
	{
		mfa->Seek(0xD8, soFromBeginning);
		return;
	}

	unsigned char byte;
	mfa->Read(&byte, 1);
	if (byte == 0x0A || byte == 0x20 || byte == 0x0C)
		mfa->Seek(0xB8, soFromBeginning);
	else
		mfa->Seek(0xD8, soFromBeginning);
}

