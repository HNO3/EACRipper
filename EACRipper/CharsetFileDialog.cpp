#include "Defaults.h"

#include "MainController.h"
#include "CharsetFileDialog.h"
#include "ComponentServiceImpl.h"

using namespace std;

namespace EACRipper
{
	using namespace ServiceImpl;

	// TODO: Implementation of to load/save the last-selected charset.

	CharsetFileDialog::CharsetFileDialog(bool isOpen, Window *owner, const std::wstring &title, const FileDialogFilter &filter, const std::wstring &defExt)
		: FileDialog(isOpen, owner, title, filter, defExt), charsetIdx(0)
	{
		HRESULT hr;
		IFileDialogCustomize *cust = getCustomize();

		if(cust != nullptr)
		{
			int idx = 0;

			hr = cust->StartVisualGroup(0, L"&Encoding:");
			if(FAILED(hr))
				return;
			
			hr = cust->AddComboBox(IDC_ENCODING);
			if(FAILED(hr))
				return;

			vector<wstring> list = StringCharsetConverter::getCharsetList();
			sort(list.begin(), list.end());

			auto addCharsetText = [&cust, &idx, this](const wstring &charset)->void
			{
				HRESULT hr = cust->AddControlItem(IDC_ENCODING, idx ++, charset.c_str());
				if(SUCCEEDED(hr))
					charsetList.push_back(charset);
			};

			addCharsetText(L"Auto-detect");
			addCharsetText(L"UTF-8");
			addCharsetText(L"Shift_JIS");
			addCharsetText(L"ISO-8859-1");
			addCharsetText(L"UTF-16");
			addCharsetText(L"UTF-16LE");
			addCharsetText(L"UTF-16BE");

			for_each(list.begin(), list.end(), [&cust, &idx, addCharsetText](const wstring &name)
			{
				if(name != L"UTF-8" && name != L"UTF-16" && name != L"UTF-16LE" && name != L"UTF-16BE" && name != L"Shift_JIS" && name != L"ISO-8859-1")
					addCharsetText(name);
			});
			cust->SetSelectedControlItem(IDC_ENCODING, 0);

			cust->EndVisualGroup();
		}
		else
		{
			OPENFILENAMEW &ofn = getOpenFileName();

			ofn.hInstance = MainController::instance().getInstance();
			ofn.lpTemplateName = MAKEINTRESOURCEW(IDD_OPEN_DIALOG);
			ofn.lpfnHook = procCharsetFileDialog;
			ofn.lCustData = reinterpret_cast<LPARAM>(this);
			ofn.Flags |= OFN_ENABLETEMPLATE | OFN_ENABLEHOOK;
		}
	}

	CharsetFileDialog::~CharsetFileDialog()
	{
	}

	uintptr_t __stdcall CharsetFileDialog::procCharsetFileDialog(HWND window, unsigned message, WPARAM wParam, LPARAM lParam)
	{
		static CharsetFileDialog *inst = nullptr;

		switch(message)
		{
		case WM_INITDIALOG:
			{
				OPENFILENAMEW *ofn = reinterpret_cast<OPENFILENAMEW *>(lParam);
				inst = reinterpret_cast<CharsetFileDialog *>(ofn->lCustData);

				HWND parent, combo, label;
				HFONT font;

				parent = GetParent(window);
				combo = GetDlgItem(parent, cmb1);

				font = reinterpret_cast<HFONT>(SendMessageW(combo, WM_GETFONT, 0, 0));

				combo = GetDlgItem(window, IDC_ENCODING);
				label = GetDlgItem(window, IDC_ENCODING_LABEL);

				SendMessageW(combo, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
				SendMessageW(label, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);

				vector<wstring> list = StringCharsetConverter::getCharsetList();
				sort(list.begin(), list.end());

				vector<wstring> &charsetList = inst->charsetList;

				auto addCharsetText = [&combo, &charsetList](const wstring &charset)->void
				{
					int res = static_cast<int>(SendMessageW(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(charset.c_str())));
					if(res != CB_ERR && res != CB_ERRSPACE)
						charsetList.push_back(charset);
				};

				addCharsetText(L"Auto-detect");
				addCharsetText(L"UTF-8");
				addCharsetText(L"Shift_JIS");
				addCharsetText(L"ISO-8859-1");
				addCharsetText(L"UTF-16");
				addCharsetText(L"UTF-16LE");
				addCharsetText(L"UTF-16BE");
				for_each(list.begin(), list.end(), [&combo, addCharsetText](const wstring &name)
				{
					if(name != L"UTF-8" && name != L"UTF-16" && name != L"UTF-16LE" && name != L"UTF-16BE" && name != L"Shift_JIS" && name != L"ISO-8859-1")
						addCharsetText(name);
				});

				SendMessageW(combo, CB_SETCURSEL, 0, 0);
			}
			break;

		case WM_SIZE:
			{
				HWND parent, combo, label;
				RECT comborc, labelrc;
				int h;

				parent = GetParent(window);
				combo = GetDlgItem(parent, cmb1);
				label = GetDlgItem(parent, stc2);
				GetWindowRect(combo, &comborc);
				GetWindowRect(label, &labelrc);

				convertRectToClient(window, &comborc);
				convertRectToClient(window, &labelrc);

				h = comborc.bottom - comborc.top;
				h += 5;

				comborc.top += h;
				comborc.bottom += h;
				labelrc.top += h;
				labelrc.bottom += h;

				combo = GetDlgItem(window, IDC_ENCODING);
				label = GetDlgItem(window, IDC_ENCODING_LABEL);
				MoveWindow(combo, comborc.left, comborc.top, comborc.right - comborc.left, comborc.bottom - comborc.top, TRUE);
				MoveWindow(label, labelrc.left, labelrc.top, labelrc.right - labelrc.left, labelrc.bottom - labelrc.top, TRUE);
			}
			break;

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
			case IDC_ENCODING:
				if(HIWORD(wParam) == CBN_SELCHANGE)
				{
					HWND combo = GetDlgItem(window, IDC_ENCODING);
					inst->charsetIdx = SendMessageW(combo, CB_GETCURSEL, 0, 0);
				}
				break;
			}
			break;
		}

		return 0;
	}

	void CharsetFileDialog::convertRectToClient(HWND window, RECT *p)
	{
		POINT pt;
		pt.x = p->left;
		pt.y = p->top;
		ScreenToClient(window, &pt);
		p->left = pt.x;
		p->top = pt.y;

		pt.x = p->right;
		pt.y = p->bottom;
		ScreenToClient(window, &pt);
		p->right = pt.x;
		p->bottom = pt.y;
	}

	bool CharsetFileDialog::show()
	{
		if(!FileDialog::show())
			return false;

		if(charsetList.empty())
		{
			charsetIdx = 0;
		}
		else
		{
			IFileDialogCustomize *cust = getCustomize();
			if(cust != nullptr)
			{
				ulong32_t idx;
				cust->GetSelectedControlItem(IDC_ENCODING, &idx);
				charsetIdx = idx;
			}

			if(charsetIdx >= charsetList.size())
			{
				charsetIdx = 0;
			}
		}

		if(charsetIdx == 0)
		{
			charset = "Auto";
		}
		else
		{
			wstring &cs = charsetList[charsetIdx];

			vector<char> charsetBuf;
			StringCharsetConverter cv;

			size_t len = cv.getConvertedLengthFromUTF16(cs.c_str());
			charsetBuf.resize(len);
			cv.convertFromUTF16(&*charsetBuf.begin(), len, cs.c_str());

			charset = &*charsetBuf.begin();
		}

		return true;
	}

	const string &CharsetFileDialog::getCharset() const
	{
		return charset;
	}
}
