///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Nov  6 2013)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __OBSWINDOWS_H__
#define __OBSWINDOWS_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class DialogSubclass;
class WindowSubclass;

#include "../wx-subclass.hpp"
#include <wx/string.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/menu.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/listbox.h>
#include <wx/toolbar.h>
#include <wx/checklst.h>
#include <wx/button.h>
#include <wx/statusbr.h>
#include <wx/frame.h>
#include <wx/combobox.h>
#include <wx/checkbox.h>
#include <wx/statline.h>
#include <wx/spinctrl.h>
#include <wx/choicebk.h>
#include <wx/listbook.h>
#include <wx/listctrl.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

#define ID_OBS_BASIC 1000
#define ID_FILE_NEW 1001
#define IF_FILE_OPEN 1002
#define IF_FILE_SAVE 1003
#define ID_FILE_EXIT 1004
#define ID_PROGRAM 1005
#define ID_SCENES 1006
#define ID_SCENE_ADD 1007
#define ID_SCENE_DELETE 1008
#define ID_SCENE_PROPERTIES 1009
#define ID_SCENE_MOVEUP 1010
#define ID_SCENE_MOVEDOWN 1011
#define ID_SOURCES 1012
#define ID_SOURCE_ADD 1013
#define ID_SOURCE_DELETE 1014
#define ID_SOURCE_PROPERTIES 1015
#define ID_SOURCE_MOVEUP 1016
#define ID_SOURCE_MOVEDOWN 1017
#define ID_STARTSTREAM 1018
#define ID_RECORD 1019
#define ID_SETTINGS 1020
#define ID_EXIT 1021
#define ID_OBS_STUDIO 1022
#define ID_OBS_BASIC_SETTINGS 1023
#define ID_SETTINGS_LIST 1024
#define ID_SETTINGS_GENERAL 1025
#define ID_LANGUAGE 1026
#define ID_SETTINGS_OUTPUTS 1027
#define ID_SETTINGS_VIDEO 1028
#define ID_ADAPTER 1029
#define ID_BASE_RES 1030
#define ID_DOWNSCALE_RES 1031
#define ID_DOWNSCALE_FILTER 1032
#define ID_DISABLEAERO 1033
#define ID_FPS_TYPE 1034
#define ID_FPS_COMMON 1035
#define ID_FPS_INTEGER 1036
#define ID_FPS_NUMERATOR 1037
#define ID_FPS_DENOMINATOR 1038
#define ID_FPS_NANOSECONDS 1039
#define ID_SETTINGS_AUDIO 1040
#define ID_DESKTOP_AUDIO_DEVICE 1041
#define ID_AUX_AUDIO_DEVICE1 1042
#define ID_AUX_AUDIO_DEVICE2 1043
#define ID_AUX_AUDIO_DEVICE3 1044
#define ID_AUX_AUDIO_DEVICE4 1045
#define ID_OK 1046
#define ID_CANCEL 1047
#define ID_APPLY 1048

///////////////////////////////////////////////////////////////////////////////
/// Class OBSBasicBase
///////////////////////////////////////////////////////////////////////////////
class OBSBasicBase : public WindowSubclass
{
	private:
	
	protected:
		wxMenuBar* mainMenu;
		wxMenu* fileMenu;
		wxPanel* mainPanel;
		wxBoxSizer* previewContainer;
		wxPanel* previewPanel;
		wxStaticText* scenesLabel;
		wxPanel* scenesPanel;
		wxListBox* scenes;
		wxToolBar* scenesToolbar;
		wxStaticText* sourcesLabel;
		wxPanel* sourcesPanel;
		wxCheckListBox* sources;
		wxToolBar* sourcesToolbar;
		wxStaticText* m_staticText6;
		wxButton* toggleStream;
		wxButton* TogglePreview;
		wxButton* settingsButton;
		wxButton* exitButton;
		wxStatusBar* statusBar;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnMinimize( wxIconizeEvent& event ) { event.Skip(); }
		virtual void OnSize( wxSizeEvent& event ) { event.Skip(); }
		virtual void file_newOnMenuSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void file_openOnMenuSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void file_saveOnMenuSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void file_exitOnMenuSelection( wxCommandEvent& event ) { event.Skip(); }
		virtual void scenesOnRightDown( wxMouseEvent& event ) { event.Skip(); }
		virtual void sceneAddOnToolClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void sceneRemoveOnToolClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void scenePropertiesOnToolClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void sceneUpOnToolClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void sceneDownOnToolClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void sourcesOnRightDown( wxMouseEvent& event ) { event.Skip(); }
		virtual void sourceAddOnToolClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void sourceRemoveOnToolClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void sourcePropertiesOnToolClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void sourceUpOnToolClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void sourceDownOnToolClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void exitButtonOnButtonClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		OBSBasicBase( wxWindow* parent, wxWindowID id = ID_OBS_BASIC, const wxString& title = _(".mainwindow"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 1117,783 ), long style = wxDEFAULT_FRAME_STYLE|wxTAB_TRAVERSAL );
		
		~OBSBasicBase();
	
};

///////////////////////////////////////////////////////////////////////////////
/// Class OBSStudioBase
///////////////////////////////////////////////////////////////////////////////
class OBSStudioBase : public WindowSubclass
{
	private:
	
	protected:
		wxPanel* mainPanel;
		wxBoxSizer* leftSizer;
		wxStaticText* m_staticText6;
		wxPanel* m_panel2;
		wxBoxSizer* transitionSizer;
		wxButton* m_button7;
		wxStaticText* m_staticText7;
		wxPanel* m_panel3;
		wxBoxSizer* bottomSizer;
		wxBoxSizer* rightSizer;
		wxMenuBar* m_menubar1;
		wxMenu* m_menu1;
		wxStatusBar* m_statusBar1;
	
	public:
		
		OBSStudioBase( wxWindow* parent, wxWindowID id = ID_OBS_STUDIO, const wxString& title = _(".openBroadcastStudio"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 1240,825 ), long style = wxDEFAULT_FRAME_STYLE|wxTAB_TRAVERSAL );
		
		~OBSStudioBase();
	
};

///////////////////////////////////////////////////////////////////////////////
/// Class OBSBasicSettingsBase
///////////////////////////////////////////////////////////////////////////////
class OBSBasicSettingsBase : public DialogSubclass
{
	private:
	
	protected:
		wxListbook* m_listbook1;
		wxPanel* m_panel8;
		wxStaticText* m_staticText27;
		wxComboBox* languageList;
		wxPanel* m_panel9;
		wxPanel* m_panel10;
		wxStaticText* m_staticText6;
		wxComboBox* videoAdapterList;
		wxStaticText* m_staticText8;
		wxComboBox* baseResList;
		wxStaticText* m_staticText10;
		wxComboBox* downscaleResList;
		wxStaticText* m_staticText11;
		wxComboBox* filterList;
		wxCheckBox* disableAeroCheckbox;
		wxStaticLine* m_staticline1;
		wxStaticText* m_staticText22;
		wxChoicebook* fpsTypeList;
		wxPanel* m_panel13;
		wxComboBox* fpsCommonList;
		wxPanel* m_panel14;
		wxSpinCtrl* fpsIntegerScroller;
		wxPanel* m_panel15;
		wxStaticText* m_staticText20;
		wxSpinCtrl* fpsNumeratorScroller;
		wxStaticText* m_staticText21;
		wxSpinCtrl* fpsDenominatorScroller;
		wxPanel* m_panel16;
		wxSpinCtrl* fpsNanosecondsScroller;
		wxPanel* m_panel11;
		wxStaticText* m_staticText23;
		wxComboBox* desktopAudioDeviceList;
		wxStaticText* m_staticText24;
		wxComboBox* auxAudioDeviceList1;
		wxStaticText* m_staticText241;
		wxComboBox* auxAudioDeviceList2;
		wxStaticText* m_staticText242;
		wxComboBox* auxAudioDeviceList3;
		wxStaticText* m_staticText243;
		wxComboBox* auxAudioDeviceList4;
		wxButton* okButton;
		wxButton* cancelButton;
		wxButton* applyButton;
	
	public:
		
		OBSBasicSettingsBase( wxWindow* parent, wxWindowID id = ID_OBS_BASIC_SETTINGS, const wxString& title = _("Settings"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 872,686 ), long style = wxDEFAULT_DIALOG_STYLE ); 
		~OBSBasicSettingsBase();
	
};

#endif //__OBSWINDOWS_H__
