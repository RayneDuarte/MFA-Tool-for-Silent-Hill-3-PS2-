#include <vcl.h>
#pragma hdrstop
#include "main.h"
#include "mfa_reader.h"
#include "build_mfa.h"
#include "infos_screen.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"

bool READ_MFA, BUILD_MFA;
TForm1 *Form1;

//---------------------------------------------------------------------------
__fastcall TForm1::TForm1(TComponent* Owner)
	: TForm(Owner)
{
}
//---------------------------------------------------------------------------

void __fastcall TForm1::open_mfaClick(TObject *Sender)
{
	if (READ_MFA)
	{
		ShowMessage("Wait for the current operation to end");
		return;
	}
	OpenMFA->FileName = "";
	OpenMFA->Execute();
	if (OpenMFA->FileName == "") return;
	String filename, path;
	path = OpenMFA->FileName;

	FRONT Front;
	Front.GetFileNameFromPath(path, filename);
	std::unique_ptr<MFA_Reader> MFA(new MFA_Reader);
	MFA->ExtractFiles(path, filename);
}
//---------------------------------------------------------------------------

void FRONT::GetFileNameFromPath(String &src, String &dst)
{
	wchar_t *aux = src.c_str();
	while (*aux != L'\0')
		aux++;

	wchar_t *s = aux;
	while (*s != L'\\')
		s--;
	s++;
	dst = *s++;

	while (s != aux)
		dst += *s++;
}

void FRONT::GetOnlyFileName(String &str, String &name)
{
	wchar_t *s = str.c_str();
	name = *s++;
	while (*s != L'.')
		name += *s++;
}

void __fastcall TForm1::FormActivate(TObject *Sender)
{
	READ_MFA = false;
	BUILD_MFA = false;
}

//---------------------------------------------------------------------------
void __fastcall TForm1::build_mfaClick(TObject *Sender)
{
	if (BUILD_MFA)
	{
		ShowMessage("Wait for the current rebuild operation to end");
		return;
	}
	BuildMFA->FileName = "";
	BuildMFA->Execute();
	if (BuildMFA->FileName == "") return;

	String path, mfa_filename, txt_filename;
	path = BuildMFA->FileName;
	FRONT front;
	front.GetFileNameFromPath(path, mfa_filename);
	front.GetOnlyFileName(mfa_filename, txt_filename);
	txt_filename += ".txt";

	String path_to_new_MFA = "NEW_MFA\\";
	path_to_new_MFA += mfa_filename;
	MFA_Reader mfa_r;
	mfa_r.CreateSubDir(path_to_new_MFA);

	TFileStream *old_mfa, *new_mfa;
	TStringList *filelist;
	try
	{
		old_mfa = new TFileStream(BuildMFA->FileName, fmOpenRead);
		new_mfa = new TFileStream(path_to_new_MFA, fmCreate);
		filelist = new TStringList();
	}
	catch(std::bad_alloc())
	{
		ShowMessage("There was an error when was trying to read or create some files");
		return;
	}
	if (!FileExists(txt_filename))
	{
		ShowMessage("The " + txt_filename + " was not found!");
		delete new_mfa;
	}
	else
	{
		filelist->LoadFromFile(txt_filename);
		MFA_BUILDER builder;
		builder.BuildMFA(old_mfa, new_mfa, filelist, path_to_new_MFA);
	}
	delete old_mfa;
	delete filelist;
}
//---------------------------------------------------------------------------

void __fastcall TForm1::FormClose(TObject *Sender, TCloseAction &Action)
{
	if ((BUILD_MFA) || (READ_MFA))
	{
		ShowMessage("Wait for the current operation to end");
		return;
	}
}
//---------------------------------------------------------------------------

void __fastcall TForm1::aboutClick(TObject *Sender)
{
	Form2->ShowModal();
}
//---------------------------------------------------------------------------

