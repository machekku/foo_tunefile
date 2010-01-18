/*
 * Copyright (C) 2008-2010  Maciej Niedzielski
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "../SDK/foobar2000.h"
#include "../helpers/helpers.h"
#include "resource.h"

#include <windows.h>
#include <stdio.h>

#define PLUGIN_NAME "Tune File"
#define PLUGIN_VERSION "0.2-dev"

DECLARE_COMPONENT_VERSION(PLUGIN_NAME, PLUGIN_VERSION,
	"Tune File plugin\n"
	"http://machekku.uaznia.net/foobar2000/tunefile/\n\n"

	"This plugin writes information about current song "
	"to be saved in a user-specified file."
	"Information may be formatted using title formatting.\n\n\n"


	"Copyright (C) 2008-2010  Maciej Niedzielski\n\n"

	"Permission is hereby granted, free of charge, to any person obtaining a copy "
	"of this software and associated documentation files (the \"Software\"), to deal "
	"in the Software without restriction, including without limitation the rights "
	"to use, copy, modify, merge, publish, distribute, sublicense, and/or sell "
	"copies of the Software, and to permit persons to whom the Software is "
	"furnished to do so, subject to the following conditions:\n\n"

	"The above copyright notice and this permission notice shall be included in "
	"all copies or substantial portions of the Software.\n\n"

	"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR "
	"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, "
	"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE "
	"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER "
	"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, "
	"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN "
	"THE SOFTWARE."
)


//--- options -----------------------------------

static bool default_cfg_enabled = true;
static bool default_cfg_library_only = false;
static char default_cfg_filename[] = "tune.txt";
static char default_cfg_template[] = 
	"%title%$char(10)\r\n"
	"%artist%$char(10)\r\n"
	"%album%$char(10)\r\n"
	"%tracknumber%$char(10)\r\n"
	"%_length_seconds%";


// {40901B2B-3284-4d1a-8DF4-65AC9E9B202A}
static const GUID guid_cfg_enabled = 
{ 0x40901b2b, 0x3284, 0x4d1a, { 0x8d, 0xf4, 0x65, 0xac, 0x9e, 0x9b, 0x20, 0x2a } };

// {3CFAD868-89E9-4d53-B0F6-478FE000285D}
static const GUID guid_cfg_library_only = 
{ 0x3cfad868, 0x89e9, 0x4d53, { 0xb0, 0xf6, 0x47, 0x8f, 0xe0, 0x0, 0x28, 0x5d } };

// {C06324E7-975F-4969-996D-34BDA4A2B222}
static const GUID guid_cfg_filename = 
{ 0xc06324e7, 0x975f, 0x4969, { 0x99, 0x6d, 0x34, 0xbd, 0xa4, 0xa2, 0xb2, 0x22 } };

// {E6003155-F468-401a-8DC4-8933503A93DD}
static const GUID guid_cfg_template = 
{ 0xe6003155, 0xf468, 0x401a, { 0x8d, 0xc4, 0x89, 0x33, 0x50, 0x3a, 0x93, 0xdd } };

static cfg_bool cfg_enabled(guid_cfg_enabled, default_cfg_enabled);
static cfg_bool cfg_library_only(guid_cfg_library_only, default_cfg_library_only);
static cfg_string cfg_filename(guid_cfg_filename, default_cfg_filename);
static cfg_string cfg_template(guid_cfg_template, default_cfg_template);


//--- file --------------------------------------

class tunefile_file
{
public:
	tunefile_file() : file_is_empty(true) {}

	void options_updated() {
		pfc::stringcvt::convert_utf8_to_wide(filename, MAX_PATH, cfg_filename, cfg_filename.length());
		static_api_ptr_t<titleformat_compiler>()->compile(titleformat, cfg_template);
	}

	void new_track(metadb_handle_ptr info) {
		bool old_in_library = in_library;
		in_library = static_api_ptr_t<library_manager>()->is_item_in_library(info);
		//info->format_title(0, saved_info, titleformat, 0);
		static_api_ptr_t<playback_control>()->playback_format_title_ex(info, NULL, saved_info, titleformat, NULL, playback_control::display_level_all);
		if (cfg_library_only && old_in_library && !in_library) {
			stop();
		}
		else {
			play();
		}	
	}

	void dynamic_info_track(const file_info&) {
		metadb_handle_ptr track;
		if (static_api_ptr_t<playback_control>()->get_now_playing(track)) {
			new_track(track);
		}
	}

	void stop()
	{
		if (file_is_empty)
			return;
		FILE *f;
		if (_wfopen_s(&f, filename, L"w") != 0) {
			console::formatter() << "TuneFile: could not open file!";
			return;
		}		
		fclose(f);
		file_is_empty = true;
	}

	void pause(bool paused)
	{
		if (paused)
			stop();
		else
			play();
	}

protected:
	void play() {
		if (!in_library && cfg_library_only) {
			stop();
			return;
		}
		FILE *f;
		if (_wfopen_s(&f, filename, L"w") != 0) {
			console::formatter() << "TuneFile: could not open file!";
			return;
		}		
		fputs(saved_info, f);
		fclose(f);
		file_is_empty = false;
	}

private:
	bool in_library;
	bool file_is_empty;
	pfc::string8 saved_info;
	WCHAR filename[MAX_PATH];
	service_ptr_t<titleformat_object> titleformat;
};

static tunefile_file tune_file;


//--- init/quit ---------------------------------

class tunefile_initquit : public initquit {
	void on_init() {
		tune_file.options_updated();
	}

	void on_quit() {}
};

static initquit_factory_t<tunefile_initquit> initquit_;


//--- preferences -------------------------------

// {8F6EAA4C-2E0C-4833-B36A-39BDAD341AAA}
static const GUID guid_tunefile_preferences = 
{ 0x8f6eaa4c, 0x2e0c, 0x4833, { 0xb3, 0x6a, 0x39, 0xbd, 0xad, 0x34, 0x1a, 0xaa } };

class tunefile_preferences : public preferences_page
{
public:
	HWND create(HWND parent) {
		return uCreateDialog(IDD_CONFIG, parent, WndProc);
	}

	const char * get_name() {
		return PLUGIN_NAME;
	}

	GUID get_guid() {
		return guid_tunefile_preferences;
	}

	GUID get_parent_guid() {
		return guid_tools;
	}

	bool reset_query() {
		return true;
	}

	void reset() {
		cfg_enabled = default_cfg_enabled;
		cfg_library_only = default_cfg_library_only;
	}

	bool get_help_url(pfc::string_base & out) {
		out = "http://machekku.uaznia.net/foobar2000/tunefile/";
		return true;
	}

private:
	static BOOL CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
		switch (msg) {
		case WM_INITDIALOG:
			uButton_SetCheck(hwnd, IDC_ENABLE, cfg_enabled);
			uButton_SetCheck(hwnd, IDC_LIBRARY_ONLY, cfg_library_only);
			uSetDlgItemText(hwnd, IDC_FILENAME, cfg_filename);
			uSetDlgItemText(hwnd, IDC_TEMPLATE, cfg_template);
			enable_controls(hwnd, cfg_enabled);
			break;
		case WM_COMMAND:
			switch (wParam) {
			case IDC_ENABLE:
				cfg_enabled = uButton_GetCheck(hwnd, IDC_ENABLE);
				enable_controls(hwnd, cfg_enabled);
				break;
			case IDC_LIBRARY_ONLY:
				cfg_library_only = uButton_GetCheck(hwnd, IDC_LIBRARY_ONLY);
				break;
			case (EN_KILLFOCUS<<16)|IDC_FILENAME:
				cfg_filename = string_utf8_from_window((HWND)lParam);
				tune_file.options_updated();
				break;
			case IDC_BROWSE:
				if (browseForFilename(hwnd)) {
					uSetDlgItemText(hwnd, IDC_FILENAME, cfg_filename);
					tune_file.options_updated();
				}
				break;
			case (EN_KILLFOCUS<<16)|IDC_TEMPLATE:
				cfg_template = string_utf8_from_window((HWND)lParam);
				tune_file.options_updated();
				break;
			}
			break;
		}
		return 0;
	}

	static bool browseForFilename(HWND hwnd) {
		WCHAR fn[MAX_PATH];
		pfc::stringcvt::convert_utf8_to_wide(fn, MAX_PATH, cfg_filename, cfg_filename.length());

		OPENFILENAME o;
		o.lStructSize = sizeof(OPENFILENAME);
		o.hwndOwner = GetAncestor(hwnd, GA_ROOTOWNER);
		o.lpstrFilter = 0;
		o.lpstrCustomFilter = 0;
		o.lpstrFile = fn;
		o.nMaxFile = MAX_PATH;
		o.lpstrInitialDir = 0;
		o.lpstrFileTitle = 0;
		o.lpstrTitle = 0;
		o.Flags = OFN_DONTADDTORECENT | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;
		o.lpstrDefExt = 0;
		o.lCustData = 0;
		o.dwReserved = 0;

		if (GetSaveFileName(&o)) {
			pfc::stringcvt::string_utf8_from_wide s(o.lpstrFile);
			cfg_filename = s.get_ptr();
			return true;
		}
		return false;
	}

	static void enable_controls(HWND hwnd, bool enable) {
		EnableWindow(GetDlgItem(hwnd, IDC_LIBRARY_ONLY), enable);
		EnableWindow(GetDlgItem(hwnd, IDC_FILENAME), enable);
		EnableWindow(GetDlgItem(hwnd, IDC_BROWSE), enable);
		EnableWindow(GetDlgItem(hwnd, IDC_TEMPLATE), enable);
	}
};


//--- playback callback -------------------------

class tunefile_play_callback: public play_callback_static
{
public:
	void on_playback_new_track(metadb_handle_ptr track) {		
		if (cfg_enabled)
			tune_file.new_track(track);
	}

	void on_playback_stop(play_control::t_stop_reason reason) {
		if (cfg_enabled && reason != play_control::stop_reason_starting_another)
			tune_file.stop();
	}

	void on_playback_pause(bool paused) {
		if (cfg_enabled)
			tune_file.pause(paused);
	}

	void on_playback_dynamic_info_track(const file_info& fi) {
		if (cfg_enabled)
			tune_file.dynamic_info_track(fi);
	}

	unsigned get_flags() {
		return flag_on_playback_new_track | flag_on_playback_stop | flag_on_playback_pause | flag_on_playback_dynamic_info_track;
	}

	void on_playback_starting(play_control::t_track_command, bool) {}
	void on_playback_seek(double) {}
	void on_playback_edited(metadb_handle_ptr) {}
	void on_playback_dynamic_info(const file_info&) {}
	void on_playback_time(double) {}
	void on_volume_change(float) {}
};


static preferences_page_factory_t<tunefile_preferences> preferences;
static play_callback_static_factory_t<tunefile_play_callback> playback;
