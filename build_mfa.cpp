//---------------------------------------------------------------------------

#pragma hdrstop

#include "main.h"
#include "build_mfa.h"
#include "mfa_reader.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)

extern bool BUILD_MFA;

void MFA_BUILDER::BuildMFA(TFileStream *old_mfa, TFileStream *new_mfa, TStringList *filelist, String &path_to_new_MFA)
{
	//lê o número de blocos de arquivos
	String blocks_num = filelist->Strings[filelist->Count - 1];

	/*aloca um array para guardar infos como lba
	e tamanho em bytes dos arquivos que serão
	inseridos no novo container MFA, para no
	final atualizar o toc de cada bloco no
	novo MFA */
	INFOS *FILEINFOS;
	FILEINFOS = new INFOS[StrToInt(blocks_num)];
	if (FILEINFOS == nullptr)
	{
		ShowMessage("Out of memory!");
		return;
	}
	int old_mfa_length = old_mfa->Size;
	int new_mfa_length = old_mfa_length;

	MFA_Reader reader;
	reader.mfa = old_mfa;
	reader.FindFirstTOCofMFA();

	int header_length = old_mfa->Position;
	old_mfa->Seek(0, soFromBeginning);

	unsigned char header_block[header_length];
	old_mfa->Read(header_block, header_length);
	new_mfa->Write(header_block, header_length);

	int i = 0, j, f = 0, block_start_lba = 0;
	reader.FILE_MODE = WRITE_FILE;
	BUILD_MFA = true;

	while (BUILD_MFA)
	{
		Form1->block_num->Caption = "BLOCK: ";
		Form1->block_num->Caption += IntToStr(i + 1);

		unsigned char data[8];
		old_mfa->Read(data, 8);
		new_mfa->Write(data, 8);
		int block_files_num = (data[3] << 24) | (data[2] << 16) | (data[1] << 8) | (data[0]);
		int block_files_len = (data[7] << 24) | (data[6] << 16) | (data[5] << 8) | (data[4]);

		int next_block_files_lba;
		next_block_files_lba = block_files_len;
		next_block_files_lba += block_start_lba;
		next_block_files_lba += SECTOR_LEN;
		int toc_length = block_files_num * 16;
		unsigned char *toc_data;

		try
		{
			FILEINFOS[i].lba = new int [block_files_num];
			FILEINFOS[i].filesize = new int [block_files_num];
			toc_data = new unsigned char[toc_length];
		}
		catch(std::bad_alloc())
		{
			ShowMessage("Out of memory!");
			DeleteFILEINFOS(FILEINFOS, i);
			BUILD_MFA= false;
			delete new_mfa;
			DeleteFile(path_to_new_MFA);
			return;
		}
		FILEINFOS[i].block_start_lba = new_mfa->Position - 16;
		FILEINFOS[i].block_files_num = block_files_num;
		FILEINFOS[i].block_length = 0;
		if (i == 0)
			FILEINFOS[i].block_start_lba = 0;

		old_mfa->Read(toc_data, toc_length);
		new_mfa->Write(toc_data, toc_length);

		int first_file_lba = (toc_data[7] << 24) | (toc_data[6] << 16) | (toc_data[5] << 8) | (toc_data[4]);
		first_file_lba = (first_file_lba + block_start_lba) + SECTOR_LEN;
		delete[] toc_data;

		while (old_mfa->Position != first_file_lba)
		{ //extrai a tabela com os nomes e caminhos dos arquivos do MFA antigo para o novo
			Application->ProcessMessages();
			unsigned char b;
			old_mfa->Read(&b, 1);
			new_mfa->Write(&b, 1);
		}

		f += 2;
		/* f += 2 ignora os dois indíces de
		filelist que contêm a lba de início
		do bloco no mfa antigo e também o
		número de arquivos. No arquivo txt
		gerado na extração dos arquivos, essas
		duas informações são usadas apenas para
		separar uma lista de arquivos da outra
		e não são necessárias aqui.*/
		j = 0;

		while (j < block_files_num)
		{ //insere os arquivos em cada bloco no novo container MFA
			Application->ProcessMessages();

			TFileStream *file = new TFileStream(filelist->Strings[f], fmOpenRead);
			if (file == nullptr)
			{
				delete new_mfa;
				DeleteFile(path_to_new_MFA);
				DeleteFILEINFOS(FILEINFOS, i + 1);
				ShowMessage("The " + filelist->Strings[f] + " file is missing or is being used by another application!");
				Form1->ProgressBar->Position = 0;
				BUILD_MFA = false;
				return;
			}
			Form1->filepath->Update();
			Form1->filepath->Caption = filelist->Strings[f];

			FILEINFOS[i].lba[j] = new_mfa->Position;
			FILEINFOS[i].lba[j] =
			(FILEINFOS[i].lba[j] -
			FILEINFOS[i].block_start_lba) - SECTOR_LEN;

			int st = reader.SaveToFile(file, new_mfa, file->Size, new_mfa_length);
			if (st != SUCCESS) ShowMessage("There was an error when to insert the file at " + path_to_new_MFA);

			FILEINFOS[i].filesize[j] = file->Size;
			FILEINFOS[i].block_length += file->Size;
			delete file;
			CreatePadding(new_mfa, FILEINFOS, i);
			j++;
			f++;
		}
		CreatePadding(new_mfa, FILEINFOS, i);

		if ((next_block_files_lba) >= old_mfa_length)
			BUILD_MFA = false;
		else
		{
			old_mfa->Seek(next_block_files_lba, soFromBeginning);
			old_mfa->Read(data, 8);
			new_mfa->Write(data, 8);
			block_start_lba = next_block_files_lba;
		}
		i++;
		Form1->block_num->Update();
	}
	delete new_mfa;

	TFileStream *update_mfa = new TFileStream(path_to_new_MFA, fmOpenReadWrite);
	UpdateMFA_TOC(update_mfa, i, header_length, FILEINFOS);
	delete update_mfa;
	DeleteFILEINFOS(FILEINFOS, i);
	Form1->ProgressBar->Position = 100;
	ShowMessage("Build Complete!");
	Form1->ProgressBar->Position = 0;
}

void MFA_BUILDER::CreatePadding(TFileStream *new_mfa, INFOS *FILEINFOS, int i)
{
	int offset = new_mfa->Position;
	int mod = offset / SECTOR_LEN;
	if (mod * SECTOR_LEN != offset)
	{
		while (1)
		{
			FILEINFOS[i].block_length++;
			unsigned char zero = 0;
			new_mfa->Write(&zero, 1);
			mod = FILEINFOS[i].block_length / SECTOR_LEN;
			if (mod * SECTOR_LEN == FILEINFOS[i].block_length)
				break;
		}
        WriteBlockEndMarker(new_mfa);
	}
}

void MFA_BUILDER::WriteBlockEndMarker(TFileStream *file)
{
	int offset = file->Position - 1;
	file->Seek(offset, soFromBeginning);
	file->Write(&BLOCK_END_MARKER, 1);
}

void MFA_BUILDER::UpdateMFA_TOC(TFileStream *new_mfa, int blocks_num, int header_length, INFOS *FILEINFOS)
{
	int i = 0;
	while (i < blocks_num)
	{
		int off1, off2;
		if (i == 0)
		{
			off1 = header_length + 4;
			off2 = header_length + 12;
		}
		else
		{
			off1 = FILEINFOS[i].block_start_lba + 12;
			off2 = FILEINFOS[i].block_start_lba + 20;
		}
		new_mfa->Seek(off1, soFromBeginning);
		new_mfa->WriteBuffer((&FILEINFOS[i].block_length), 4);
		new_mfa->Seek(off2, soFromBeginning);

		int j = 0;
		while (j < FILEINFOS[i].block_files_num)
		{
			new_mfa->WriteBuffer((&FILEINFOS[i].lba[j]), 4);
			off1 = new_mfa->Position + 4;
			new_mfa->Seek(off1, soFromBeginning);
			new_mfa->WriteBuffer((&FILEINFOS[i].filesize[j]), 4);
			off1 = new_mfa->Position + 4;
			new_mfa->Seek(off1, soFromBeginning);
			j++;
		}
		i++;
	}
}

void MFA_BUILDER::DeleteFILEINFOS(INFOS *FILEINFOS, int blocks_num)
{
	int i = 0;
	while (i < blocks_num)
	{
		delete[] FILEINFOS[i].lba;
		delete[] FILEINFOS[i++].filesize;
	}
	delete[] FILEINFOS;
}
