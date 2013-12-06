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
class WindowSubclass;

#include "window-subclass.hpp"
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
#include <wx/checkbox.h>
#include <wx/statusbr.h>
#include <wx/frame.h>

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
#define ID_SETPOSSIZE 1018
#define ID_CROP 1019
#define ID_LOCK 1020
#define ID_PREVIEW 1021
#define ID_STARTSTREAM 1022
#define ID_RECORD 1023
#define ID_SETTINGS 1024
#define ID_EXIT 1025

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
		wxButton* positionSizeButton;
		wxButton* cropButton;
		wxCheckBox* lockPreview;
		wxCheckBox* enablePreview;
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
		virtual void whatever( wxSizeEvent& event ) { event.Skip(); }
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
		
		OBSStudioBase( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _(".openBroadcastStudio"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 1240,825 ), long style = wxDEFAULT_FRAME_STYLE|wxTAB_TRAVERSAL );
		
		~OBSStudioBase();
	
};

#endif //__OBSWINDOWS_H__
