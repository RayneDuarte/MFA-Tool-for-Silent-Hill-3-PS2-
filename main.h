//---------------------------------------------------------------------------

#ifndef mainH
#define mainH
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.StdCtrls.hpp>
#include <Vcl.Forms.hpp>
#include <Vcl.ExtCtrls.hpp>
#include <Vcl.Imaging.pngimage.hpp>
#include <Vcl.Dialogs.hpp>
#include <Vcl.ComCtrls.hpp>
//---------------------------------------------------------------------------

#pragma once
class FRONT
{
    public: void GetFileNameFromPath(String &src, String &dst);
    public: void GetOnlyFileName(String &str, String &name);
};

class TForm1 : public TForm
{
__published:	// IDE-managed Components
	TImage *background;
	TButton *open_mfa;
	TProgressBar *ProgressBar;
	TLabel *filepath;
	TLabel *block_num;
	TButton *build_mfa;
	TImage *about;
	TOpenDialog *OpenMFA;
	TOpenDialog *BuildMFA;
	void __fastcall open_mfaClick(TObject *Sender);
	void __fastcall FormActivate(TObject *Sender);
	void __fastcall build_mfaClick(TObject *Sender);
	void __fastcall FormClose(TObject *Sender, TCloseAction &Action);
	void __fastcall aboutClick(TObject *Sender);
private:	// User declarations
public:		// User declarations
	__fastcall TForm1(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TForm1 *Form1;
//---------------------------------------------------------------------------
#endif
