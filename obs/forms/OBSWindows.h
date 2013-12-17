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
#define ID_FPSPANEL_COMMON 1035
#define ID_FPS_COMMON 1036
#define ID_FPSPANEL_INTEGER 1037
#define ID_FPS_INTEGER 1038
#define ID_FPSPANEL_FRACTION 1039
#define ID_FPS_NUMERATOR 1040
#define ID_FPS_DENOMINATOR 1041
#define ID_FPSPANEL_NANOSECONDS 1042
#define ID_FPS_NANOSECONDS 1043
#define ID_SETTINGS_AUDIO 1044
#define ID_DESKTOP_AUDIO_DEVICE 1045
#define ID_AUX_AUDIO_DEVICE1 1046
#define ID_AUX_AUDIO_DEVICE2 1047
#define ID_AUX_AUDIO_DEVICE3 1048
#define ID_AUX_AUDIO_DEVICE4 1049
#define ID_OK 1050
#define ID_CANCEL 1051
#define ID_APPLY 1052

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
		wxButton* toggleStreamButton;
		wxButton* ToggleRecordButton;
		wxButton* settingsButton;
		wxButton* exitButton;
		wxStatusBar* statusBar;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnMinimize( wxIconizeEvent& event ) { event.Skip(); }
		virtual void OnSize( wxSizeEvent& event ) { event.Skip(); }
		virtual void fileNewClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void fileOpenClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void fileSaveClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void fileExitClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void scenesRDown( wxMouseEvent& event ) { event.Skip(); }
		virtual void sceneAddClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void sceneRemoveClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void scenePropertiesClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void sceneUpClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void sceneDownClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void sourcesRDown( wxMouseEvent& event ) { event.Skip(); }
		virtual void sourceAddClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void sourceRemoveClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void sourcePropertiesClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void sourceUpClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void sourceDownClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void toggleStreamClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void toggleRecordClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void settingsClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void exitClicked( wxCommandEvent& event ) { event.Skip(); }
		
	
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
		wxStaticText* m_staticText27;
		wxPanel* videoPanel;
		wxStaticText* m_staticText211;
		wxStaticText* m_staticText6;
		wxStaticText* m_staticText8;
		wxStaticText* m_staticText10;
		wxStaticText* m_staticText11;
		wxStaticLine* m_staticline1;
		wxStaticText* m_staticText22;
		wxPanel* m_panel13;
		wxPanel* m_panel14;
		wxPanel* m_panel15;
		wxStaticText* m_staticText20;
		wxStaticText* m_staticText21;
		wxPanel* m_panel16;
		wxPanel* audioPanel;
		wxStaticText* m_staticText23;
		wxStaticText* m_staticText24;
		wxStaticText* m_staticText241;
		wxStaticText* m_staticText242;
		wxStaticText* m_staticText243;
		wxButton* okButton;
		wxButton* cancelButton;
		wxButton* applyButton;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void PageChanged( wxListbookEvent& event ) { event.Skip(); }
		virtual void PageChanging( wxListbookEvent& event ) { event.Skip(); }
		
	
	public:
		wxListbook* settingsList;
		wxPanel* generalPanel;
		wxComboBox* languageList;
		wxStaticText* generalChangedText;
		wxPanel* outputsPanel;
		wxComboBox* rendererList;
		wxComboBox* videoAdapterList;
		wxComboBox* baseResList;
		wxComboBox* outputResList;
		wxComboBox* filterList;
		wxCheckBox* disableAeroCheckbox;
		wxChoicebook* fpsTypeList;
		wxComboBox* fpsCommonList;
		wxSpinCtrl* fpsIntegerScroller;
		wxSpinCtrl* fpsNumeratorScroller;
		wxSpinCtrl* fpsDenominatorScroller;
		wxSpinCtrl* fpsNanosecondsScroller;
		wxStaticText* videoChangedText;
		wxComboBox* desktopAudioDeviceList;
		wxComboBox* auxAudioDeviceList1;
		wxComboBox* auxAudioDeviceList2;
		wxComboBox* auxAudioDeviceList3;
		wxComboBox* auxAudioDeviceList4;
		
		OBSBasicSettingsBase( wxWindow* parent, wxWindowID id = ID_OBS_BASIC_SETTINGS, const wxString& title = _("Settings"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 872,686 ), long style = wxDEFAULT_DIALOG_STYLE ); 
		~OBSBasicSettingsBase();
	
};

#endif //__OBSWINDOWS_H__
