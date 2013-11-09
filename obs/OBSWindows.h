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
class ListCtrlFixed;
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
#include <wx/listctrl.h>

///////////////////////////////////////////////////////////////////////////

#define ID_FILE_NEW 1000
#define IF_FILE_OPEN 1001
#define IF_FILE_SAVE 1002
#define ID_FILE_EXIT 1003
#define ID_PROGRAM 1004
#define ID_SCENES 1005
#define ID_SCENE_ADD 1006
#define ID_SCENE_DELETE 1007
#define ID_SCENE_PROPERTIES 1008
#define ID_SCENE_MOVEUP 1009
#define ID_SCENE_MOVEDOWN 1010
#define ID_SOURCES 1011
#define ID_SOURCE_ADD 1012
#define ID_SOURCE_DELETE 1013
#define ID_SOURCE_PROPERTIES 1014
#define ID_SOURCE_MOVEUP 1015
#define ID_SOURCE_MOVEDOWN 1016
#define ID_SETPOSSIZE 1017
#define ID_CROP 1018
#define ID_LOCK 1019
#define ID_PREVIEW 1020
#define ID_STARTSTREAM 1021
#define ID_RECORD 1022
#define ID_SETTINGS 1023
#define ID_EXIT 1024

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
		
	
	public:
		
		OBSBasicBase( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _(".mainwindow"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 923,677 ), long style = wxDEFAULT_FRAME_STYLE|wxTAB_TRAVERSAL );
		
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
		wxStaticText* m_staticText6;
		wxPanel* m_panel2;
		wxBoxSizer* transitionContainer;
		wxButton* m_button7;
		wxStaticText* m_staticText7;
		wxPanel* m_panel3;
		wxStaticText* m_staticText1;
		wxPanel* m_panel13;
		wxListBox* m_listBox1;
		wxToolBar* m_toolBar1;
		wxStaticText* m_staticText2;
		wxPanel* m_panel12;
		wxListBox* m_listBox2;
		wxToolBar* m_toolBar11;
		wxStaticText* m_staticText3;
		wxPanel* m_panel14;
		wxCheckListBox* m_checkList1;
		wxToolBar* m_toolBar12;
		wxStaticText* m_staticText4;
		wxPanel* m_panel15;
		ListCtrlFixed* m_listCtrl1;
		wxToolBar* m_toolBar13;
		wxMenuBar* m_menubar1;
		wxMenu* m_menu1;
		wxStatusBar* m_statusBar1;
	
	public:
		
		OBSStudioBase( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _(".openBroadcastStudio"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 1240,825 ), long style = wxDEFAULT_FRAME_STYLE|wxTAB_TRAVERSAL );
		
		~OBSStudioBase();
	
};

#endif //__OBSWINDOWS_H__
