///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Nov  6 2013)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "OBSWindows.h"

#include "res/delete.ico.h"
#include "res/down.ico.h"
#include "res/list_add.png.h"
#include "res/properties.ico.h"
#include "res/up.ico.h"

///////////////////////////////////////////////////////////////////////////

OBSBasicBase::OBSBasicBase( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : WindowSubclass( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	mainMenu = new wxMenuBar( 0 );
	fileMenu = new wxMenu();
	wxMenuItem* fileNewMenu;
	fileNewMenu = new wxMenuItem( fileMenu, ID_FILE_NEW, wxString( _("MainMenu.File.New") ) , wxEmptyString, wxITEM_NORMAL );
	fileMenu->Append( fileNewMenu );
	
	wxMenuItem* fileOpenMenu;
	fileOpenMenu = new wxMenuItem( fileMenu, IF_FILE_OPEN, wxString( _("MainMenu.File.Open") ) , wxEmptyString, wxITEM_NORMAL );
	fileMenu->Append( fileOpenMenu );
	
	wxMenuItem* fileSaveMenu;
	fileSaveMenu = new wxMenuItem( fileMenu, IF_FILE_SAVE, wxString( _("MainMenu.File.Save") ) , wxEmptyString, wxITEM_NORMAL );
	fileMenu->Append( fileSaveMenu );
	
	fileMenu->AppendSeparator();
	
	wxMenuItem* fileExitMenu;
	fileExitMenu = new wxMenuItem( fileMenu, ID_FILE_EXIT, wxString( _("MainWindow.Exit") ) , wxEmptyString, wxITEM_NORMAL );
	fileMenu->Append( fileExitMenu );
	
	mainMenu->Append( fileMenu, _("MainMenu.File") ); 
	
	this->SetMenuBar( mainMenu );
	
	wxBoxSizer* mainContainer;
	mainContainer = new wxBoxSizer( wxVERTICAL );
	
	mainPanel = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* panelContainer;
	panelContainer = new wxBoxSizer( wxVERTICAL );
	
	previewContainer = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* previewVertical;
	previewVertical = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizer35;
	bSizer35 = new wxBoxSizer( wxVERTICAL );
	
	previewPanel = new wxPanel( mainPanel, ID_PROGRAM, wxDefaultPosition, wxSize( -1,-1 ), wxTAB_TRAVERSAL );
	previewPanel->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_APPWORKSPACE ) );
	
	bSizer35->Add( previewPanel, 0, wxALIGN_CENTER|wxALL, 5 );
	
	
	previewVertical->Add( bSizer35, 1, wxEXPAND, 5 );
	
	
	previewContainer->Add( previewVertical, 1, wxALIGN_CENTER, 5 );
	
	
	panelContainer->Add( previewContainer, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bottomContainer;
	bottomContainer = new wxBoxSizer( wxVERTICAL );
	
	bottomContainer->SetMinSize( wxSize( -1,155 ) ); 
	wxBoxSizer* bSizer36;
	bSizer36 = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bottomCenterContainer;
	bottomCenterContainer = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* scenesContainer;
	scenesContainer = new wxBoxSizer( wxVERTICAL );
	
	scenesLabel = new wxStaticText( mainPanel, wxID_ANY, _("MainWindow.Scenes"), wxDefaultPosition, wxDefaultSize, 0 );
	scenesLabel->Wrap( -1 );
	scenesContainer->Add( scenesLabel, 0, wxALL|wxEXPAND, 2 );
	
	scenesPanel = new wxPanel( mainPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSIMPLE_BORDER );
	wxBoxSizer* bSizer16;
	bSizer16 = new wxBoxSizer( wxVERTICAL );
	
	scenes = new wxListBox( scenesPanel, ID_SCENES, wxDefaultPosition, wxDefaultSize, 0, NULL, 0|wxNO_BORDER ); 
	bSizer16->Add( scenes, 1, wxALL|wxEXPAND, 0 );
	
	scenesToolbar = new wxToolBar( scenesPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTB_HORIZONTAL ); 
	scenesToolbar->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );
	
	scenesToolbar->AddTool( ID_SCENE_ADD, _("tool"), list_add_png_to_wx_bitmap(), wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString, NULL ); 
	
	scenesToolbar->AddTool( ID_SCENE_DELETE, _("tool"), delete_ico_to_wx_bitmap(), wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString, NULL ); 
	
	scenesToolbar->AddTool( ID_SCENE_PROPERTIES, _("tool"), properties_ico_to_wx_bitmap(), wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString, NULL ); 
	
	scenesToolbar->AddSeparator(); 
	
	scenesToolbar->AddTool( ID_SCENE_MOVEUP, _("tool"), up_ico_to_wx_bitmap(), wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString, NULL ); 
	
	scenesToolbar->AddTool( ID_SCENE_MOVEDOWN, _("tool"), down_ico_to_wx_bitmap(), wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString, NULL ); 
	
	scenesToolbar->Realize(); 
	
	bSizer16->Add( scenesToolbar, 0, wxEXPAND, 5 );
	
	
	scenesPanel->SetSizer( bSizer16 );
	scenesPanel->Layout();
	bSizer16->Fit( scenesPanel );
	scenesContainer->Add( scenesPanel, 1, wxEXPAND | wxALL, 2 );
	
	
	bottomCenterContainer->Add( scenesContainer, 1, wxEXPAND, 5 );
	
	wxBoxSizer* sourcesContainer;
	sourcesContainer = new wxBoxSizer( wxVERTICAL );
	
	sourcesLabel = new wxStaticText( mainPanel, wxID_ANY, _("MainWindow.Sources"), wxDefaultPosition, wxDefaultSize, 0 );
	sourcesLabel->Wrap( -1 );
	sourcesContainer->Add( sourcesLabel, 0, wxALL|wxEXPAND, 2 );
	
	sourcesPanel = new wxPanel( mainPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSIMPLE_BORDER );
	wxBoxSizer* bSizer17;
	bSizer17 = new wxBoxSizer( wxVERTICAL );
	
	wxArrayString sourcesChoices;
	sources = new wxCheckListBox( sourcesPanel, ID_SOURCES, wxDefaultPosition, wxDefaultSize, sourcesChoices, 0|wxNO_BORDER );
	bSizer17->Add( sources, 1, wxALL, 0 );
	
	sourcesToolbar = new wxToolBar( sourcesPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTB_HORIZONTAL ); 
	sourcesToolbar->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );
	
	sourcesToolbar->AddTool( ID_SOURCE_ADD, _("tool"), list_add_png_to_wx_bitmap(), wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString, NULL ); 
	
	sourcesToolbar->AddTool( ID_SOURCE_DELETE, _("tool"), delete_ico_to_wx_bitmap(), wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString, NULL ); 
	
	sourcesToolbar->AddTool( ID_SOURCE_PROPERTIES, _("tool"), properties_ico_to_wx_bitmap(), wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString, NULL ); 
	
	sourcesToolbar->AddSeparator(); 
	
	sourcesToolbar->AddTool( ID_SOURCE_MOVEUP, _("tool"), up_ico_to_wx_bitmap(), wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString, NULL ); 
	
	sourcesToolbar->AddTool( ID_SOURCE_MOVEDOWN, _("tool"), down_ico_to_wx_bitmap(), wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString, NULL ); 
	
	sourcesToolbar->Realize(); 
	
	bSizer17->Add( sourcesToolbar, 0, wxEXPAND, 5 );
	
	
	sourcesPanel->SetSizer( bSizer17 );
	sourcesPanel->Layout();
	bSizer17->Fit( sourcesPanel );
	sourcesContainer->Add( sourcesPanel, 1, wxEXPAND | wxALL, 2 );
	
	
	bottomCenterContainer->Add( sourcesContainer, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer42;
	bSizer42 = new wxBoxSizer( wxVERTICAL );
	
	sourcesLabel1 = new wxStaticText( mainPanel, wxID_ANY, _("MainWindow.Volume"), wxDefaultPosition, wxDefaultSize, 0 );
	sourcesLabel1->Wrap( -1 );
	bSizer42->Add( sourcesLabel1, 0, wxALL, 2 );
	
	m_scrolledWindow1 = new wxScrolledWindow( mainPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHSCROLL|wxSIMPLE_BORDER|wxVSCROLL );
	m_scrolledWindow1->SetScrollRate( 5, 5 );
	wxBoxSizer* bSizer44;
	bSizer44 = new wxBoxSizer( wxVERTICAL );
	
	
	m_scrolledWindow1->SetSizer( bSizer44 );
	m_scrolledWindow1->Layout();
	bSizer44->Fit( m_scrolledWindow1 );
	bSizer42->Add( m_scrolledWindow1, 1, wxEXPAND | wxALL, 2 );
	
	
	bottomCenterContainer->Add( bSizer42, 1, wxEXPAND, 5 );
	
	wxBoxSizer* rightButtonsContainer;
	rightButtonsContainer = new wxBoxSizer( wxVERTICAL );
	
	m_staticText6 = new wxStaticText( mainPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText6->Wrap( -1 );
	rightButtonsContainer->Add( m_staticText6, 0, wxALL, 2 );
	
	toggleStreamButton = new wxButton( mainPanel, ID_STARTSTREAM, _("MainWindow.StartStream"), wxDefaultPosition, wxSize( -1,-1 ), 0 );
	rightButtonsContainer->Add( toggleStreamButton, 0, wxALL|wxEXPAND, 2 );
	
	ToggleRecordButton = new wxButton( mainPanel, ID_RECORD, _("MainWindow.StartRecording"), wxDefaultPosition, wxSize( -1,-1 ), 0 );
	rightButtonsContainer->Add( ToggleRecordButton, 0, wxALL|wxEXPAND, 2 );
	
	settingsButton = new wxButton( mainPanel, ID_SETTINGS, _("MainWindow.Settings"), wxDefaultPosition, wxSize( -1,-1 ), 0 );
	rightButtonsContainer->Add( settingsButton, 0, wxALL|wxEXPAND, 2 );
	
	exitButton = new wxButton( mainPanel, ID_EXIT, _("MainWindow.Exit"), wxDefaultPosition, wxSize( -1,-1 ), 0 );
	rightButtonsContainer->Add( exitButton, 0, wxALL|wxEXPAND, 2 );
	
	
	bottomCenterContainer->Add( rightButtonsContainer, 1, wxEXPAND, 5 );
	
	
	bSizer36->Add( bottomCenterContainer, 1, wxALIGN_CENTER, 5 );
	
	
	bottomContainer->Add( bSizer36, 1, wxALIGN_CENTER|wxEXPAND, 5 );
	
	
	panelContainer->Add( bottomContainer, 0, wxEXPAND, 5 );
	
	
	mainPanel->SetSizer( panelContainer );
	mainPanel->Layout();
	panelContainer->Fit( mainPanel );
	mainContainer->Add( mainPanel, 1, wxEXPAND | wxALL, 0 );
	
	
	this->SetSizer( mainContainer );
	this->Layout();
	statusBar = this->CreateStatusBar( 1, wxST_SIZEGRIP, wxID_ANY );
	
	this->Centre( wxBOTH );
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( OBSBasicBase::OnClose ) );
	this->Connect( wxEVT_ICONIZE, wxIconizeEventHandler( OBSBasicBase::OnMinimize ) );
	this->Connect( wxEVT_SIZE, wxSizeEventHandler( OBSBasicBase::OnSize ) );
	this->Connect( fileNewMenu->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( OBSBasicBase::fileNewClicked ) );
	this->Connect( fileOpenMenu->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( OBSBasicBase::fileOpenClicked ) );
	this->Connect( fileSaveMenu->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( OBSBasicBase::fileSaveClicked ) );
	this->Connect( fileExitMenu->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( OBSBasicBase::fileExitClicked ) );
	scenes->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( OBSBasicBase::scenesRDown ), NULL, this );
	this->Connect( ID_SCENE_ADD, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( OBSBasicBase::sceneAddClicked ) );
	this->Connect( ID_SCENE_DELETE, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( OBSBasicBase::sceneRemoveClicked ) );
	this->Connect( ID_SCENE_PROPERTIES, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( OBSBasicBase::scenePropertiesClicked ) );
	this->Connect( ID_SCENE_MOVEUP, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( OBSBasicBase::sceneUpClicked ) );
	this->Connect( ID_SCENE_MOVEDOWN, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( OBSBasicBase::sceneDownClicked ) );
	sources->Connect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( OBSBasicBase::sourcesRDown ), NULL, this );
	this->Connect( ID_SOURCE_ADD, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( OBSBasicBase::sourceAddClicked ) );
	this->Connect( ID_SOURCE_DELETE, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( OBSBasicBase::sourceRemoveClicked ) );
	this->Connect( ID_SOURCE_PROPERTIES, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( OBSBasicBase::sourcePropertiesClicked ) );
	this->Connect( ID_SOURCE_MOVEUP, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( OBSBasicBase::sourceUpClicked ) );
	this->Connect( ID_SOURCE_MOVEDOWN, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( OBSBasicBase::sourceDownClicked ) );
	toggleStreamButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( OBSBasicBase::toggleStreamClicked ), NULL, this );
	ToggleRecordButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( OBSBasicBase::toggleRecordClicked ), NULL, this );
	settingsButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( OBSBasicBase::settingsClicked ), NULL, this );
	exitButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( OBSBasicBase::exitClicked ), NULL, this );
}

OBSBasicBase::~OBSBasicBase()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( OBSBasicBase::OnClose ) );
	this->Disconnect( wxEVT_ICONIZE, wxIconizeEventHandler( OBSBasicBase::OnMinimize ) );
	this->Disconnect( wxEVT_SIZE, wxSizeEventHandler( OBSBasicBase::OnSize ) );
	this->Disconnect( ID_FILE_NEW, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( OBSBasicBase::fileNewClicked ) );
	this->Disconnect( IF_FILE_OPEN, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( OBSBasicBase::fileOpenClicked ) );
	this->Disconnect( IF_FILE_SAVE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( OBSBasicBase::fileSaveClicked ) );
	this->Disconnect( ID_FILE_EXIT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( OBSBasicBase::fileExitClicked ) );
	scenes->Disconnect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( OBSBasicBase::scenesRDown ), NULL, this );
	this->Disconnect( ID_SCENE_ADD, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( OBSBasicBase::sceneAddClicked ) );
	this->Disconnect( ID_SCENE_DELETE, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( OBSBasicBase::sceneRemoveClicked ) );
	this->Disconnect( ID_SCENE_PROPERTIES, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( OBSBasicBase::scenePropertiesClicked ) );
	this->Disconnect( ID_SCENE_MOVEUP, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( OBSBasicBase::sceneUpClicked ) );
	this->Disconnect( ID_SCENE_MOVEDOWN, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( OBSBasicBase::sceneDownClicked ) );
	sources->Disconnect( wxEVT_RIGHT_DOWN, wxMouseEventHandler( OBSBasicBase::sourcesRDown ), NULL, this );
	this->Disconnect( ID_SOURCE_ADD, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( OBSBasicBase::sourceAddClicked ) );
	this->Disconnect( ID_SOURCE_DELETE, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( OBSBasicBase::sourceRemoveClicked ) );
	this->Disconnect( ID_SOURCE_PROPERTIES, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( OBSBasicBase::sourcePropertiesClicked ) );
	this->Disconnect( ID_SOURCE_MOVEUP, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( OBSBasicBase::sourceUpClicked ) );
	this->Disconnect( ID_SOURCE_MOVEDOWN, wxEVT_COMMAND_TOOL_CLICKED, wxCommandEventHandler( OBSBasicBase::sourceDownClicked ) );
	toggleStreamButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( OBSBasicBase::toggleStreamClicked ), NULL, this );
	ToggleRecordButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( OBSBasicBase::toggleRecordClicked ), NULL, this );
	settingsButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( OBSBasicBase::settingsClicked ), NULL, this );
	exitButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( OBSBasicBase::exitClicked ), NULL, this );
	
}

OBSStudioBase::OBSStudioBase( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : WindowSubclass( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( 900,400 ), wxDefaultSize );
	
	wxBoxSizer* windowSizer;
	windowSizer = new wxBoxSizer( wxVERTICAL );
	
	mainPanel = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* clientSizer;
	clientSizer = new wxBoxSizer( wxHORIZONTAL );
	
	leftSizer = new wxBoxSizer( wxVERTICAL );
	
	
	clientSizer->Add( leftSizer, 0, wxEXPAND, 5 );
	
	wxBoxSizer* centerSizer;
	centerSizer = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* topSizer;
	topSizer = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizer5;
	bSizer5 = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizer8;
	bSizer8 = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer33;
	bSizer33 = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer191;
	bSizer191 = new wxBoxSizer( wxVERTICAL );
	
	m_staticText6 = new wxStaticText( mainPanel, wxID_ANY, _("MainWindow.Preview"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText6->Wrap( -1 );
	bSizer191->Add( m_staticText6, 0, wxALL, 3 );
	
	m_panel2 = new wxPanel( mainPanel, wxID_ANY, wxDefaultPosition, wxSize( 480,270 ), wxTAB_TRAVERSAL );
	m_panel2->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_APPWORKSPACE ) );
	
	bSizer191->Add( m_panel2, 0, wxALIGN_CENTER|wxALL, 3 );
	
	
	bSizer33->Add( bSizer191, 0, wxALIGN_CENTER, 0 );
	
	
	bSizer8->Add( bSizer33, 0, wxALIGN_CENTER|wxEXPAND, 5 );
	
	
	bSizer5->Add( bSizer8, 1, wxALIGN_CENTER, 5 );
	
	
	topSizer->Add( bSizer5, 1, wxEXPAND, 5 );
	
	transitionSizer = new wxBoxSizer( wxVERTICAL );
	
	m_button7 = new wxButton( mainPanel, wxID_ANY, _("MainWindow.Cut"), wxDefaultPosition, wxDefaultSize, 0 );
	transitionSizer->Add( m_button7, 0, wxALL, 5 );
	
	
	topSizer->Add( transitionSizer, 0, wxALIGN_CENTER, 5 );
	
	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer( wxHORIZONTAL );
	
	wxBoxSizer* bSizer13;
	bSizer13 = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer34;
	bSizer34 = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer201;
	bSizer201 = new wxBoxSizer( wxVERTICAL );
	
	m_staticText7 = new wxStaticText( mainPanel, wxID_ANY, _("MainWindow.Program"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText7->Wrap( -1 );
	bSizer201->Add( m_staticText7, 0, wxALL, 3 );
	
	m_panel3 = new wxPanel( mainPanel, wxID_ANY, wxDefaultPosition, wxSize( 480,270 ), wxTAB_TRAVERSAL );
	m_panel3->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_APPWORKSPACE ) );
	
	bSizer201->Add( m_panel3, 0, wxALIGN_CENTER|wxALL, 3 );
	
	
	bSizer34->Add( bSizer201, 0, wxALIGN_CENTER, 0 );
	
	
	bSizer13->Add( bSizer34, 0, wxALIGN_CENTER|wxEXPAND, 5 );
	
	
	bSizer6->Add( bSizer13, 1, wxALIGN_CENTER, 5 );
	
	
	topSizer->Add( bSizer6, 1, wxEXPAND, 5 );
	
	
	centerSizer->Add( topSizer, 1, wxEXPAND, 5 );
	
	bottomSizer = new wxBoxSizer( wxVERTICAL );
	
	
	centerSizer->Add( bottomSizer, 0, wxEXPAND, 5 );
	
	
	clientSizer->Add( centerSizer, 1, wxEXPAND, 5 );
	
	rightSizer = new wxBoxSizer( wxVERTICAL );
	
	
	clientSizer->Add( rightSizer, 0, wxEXPAND, 5 );
	
	
	mainPanel->SetSizer( clientSizer );
	mainPanel->Layout();
	clientSizer->Fit( mainPanel );
	windowSizer->Add( mainPanel, 1, wxEXPAND, 0 );
	
	
	this->SetSizer( windowSizer );
	this->Layout();
	m_menubar1 = new wxMenuBar( 0 );
	m_menu1 = new wxMenu();
	wxMenuItem* m_menuItem1;
	m_menuItem1 = new wxMenuItem( m_menu1, wxID_ANY, wxString( _("MyMenuItem") ) , wxEmptyString, wxITEM_NORMAL );
	m_menu1->Append( m_menuItem1 );
	
	m_menubar1->Append( m_menu1, _("MainMenu.File") ); 
	
	this->SetMenuBar( m_menubar1 );
	
	m_statusBar1 = this->CreateStatusBar( 1, wxST_SIZEGRIP, wxID_ANY );
	
	this->Centre( wxBOTH );
}

OBSStudioBase::~OBSStudioBase()
{
}

OBSBasicSettingsBase::OBSBasicSettingsBase( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DialogSubclass( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizer30;
	bSizer30 = new wxBoxSizer( wxVERTICAL );
	
	wxBoxSizer* bSizer31;
	bSizer31 = new wxBoxSizer( wxVERTICAL );
	
	settingsList = new wxListbook( this, ID_SETTINGS_LIST, wxDefaultPosition, wxDefaultSize, wxLB_DEFAULT );
	generalPanel = new wxPanel( settingsList, ID_SETTINGS_GENERAL, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer32;
	bSizer32 = new wxBoxSizer( wxVERTICAL );
	
	
	bSizer32->Add( 0, 20, 0, wxEXPAND, 5 );
	
	wxFlexGridSizer* fgSizer13;
	fgSizer13 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer13->SetFlexibleDirection( wxBOTH );
	fgSizer13->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticText27 = new wxStaticText( generalPanel, wxID_ANY, _("Settings.General.Language"), wxDefaultPosition, wxSize( 270,-1 ), wxALIGN_RIGHT );
	m_staticText27->Wrap( -1 );
	fgSizer13->Add( m_staticText27, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 2 );
	
	languageList = new wxComboBox( generalPanel, ID_LANGUAGE, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY ); 
	fgSizer13->Add( languageList, 0, wxALL, 2 );
	
	
	bSizer32->Add( fgSizer13, 0, wxEXPAND, 5 );
	
	
	bSizer32->Add( 0, 20, 0, wxEXPAND, 5 );
	
	generalChangedText = new wxStaticText( generalPanel, wxID_ANY, _("Settings.RestartProgram"), wxDefaultPosition, wxDefaultSize, 0 );
	generalChangedText->Wrap( -1 );
	bSizer32->Add( generalChangedText, 1, wxALL|wxEXPAND, 5 );
	
	
	generalPanel->SetSizer( bSizer32 );
	generalPanel->Layout();
	bSizer32->Fit( generalPanel );
	settingsList->AddPage( generalPanel, _("Settings.General"), true );
	outputsPanel = new wxPanel( settingsList, ID_SETTINGS_OUTPUTS, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer33;
	bSizer33 = new wxBoxSizer( wxVERTICAL );
	
	
	outputsPanel->SetSizer( bSizer33 );
	outputsPanel->Layout();
	bSizer33->Fit( outputsPanel );
	settingsList->AddPage( outputsPanel, _("Settings.Outputs"), false );
	videoPanel = new wxPanel( settingsList, ID_SETTINGS_VIDEO, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer34;
	bSizer34 = new wxBoxSizer( wxVERTICAL );
	
	
	bSizer34->Add( 0, 20, 0, wxEXPAND, 5 );
	
	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 2, 2, 0 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticText211 = new wxStaticText( videoPanel, wxID_ANY, _("Settings.Video.Renderer"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText211->Wrap( -1 );
	fgSizer1->Add( m_staticText211, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 2 );
	
	rendererList = new wxComboBox( videoPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY ); 
	fgSizer1->Add( rendererList, 0, wxALL, 2 );
	
	m_staticText6 = new wxStaticText( videoPanel, wxID_ANY, _("Settings.Video.Adapter"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT );
	m_staticText6->Wrap( -1 );
	m_staticText6->SetMinSize( wxSize( 270,-1 ) );
	
	fgSizer1->Add( m_staticText6, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 2 );
	
	videoAdapterList = new wxComboBox( videoPanel, ID_ADAPTER, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY ); 
	videoAdapterList->Enable( false );
	
	fgSizer1->Add( videoAdapterList, 0, wxALL, 2 );
	
	m_staticText8 = new wxStaticText( videoPanel, wxID_ANY, _("Settings.Video.BaseRes"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_staticText8->Wrap( -1 );
	fgSizer1->Add( m_staticText8, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 2 );
	
	baseResList = new wxComboBox( videoPanel, ID_BASE_RES, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 ); 
	fgSizer1->Add( baseResList, 0, wxALL, 2 );
	
	m_staticText10 = new wxStaticText( videoPanel, wxID_ANY, _("Settings.Video.OutputRes"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT );
	m_staticText10->Wrap( -1 );
	fgSizer1->Add( m_staticText10, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 2 );
	
	outputResList = new wxComboBox( videoPanel, ID_DOWNSCALE_RES, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 ); 
	fgSizer1->Add( outputResList, 0, wxALL, 2 );
	
	m_staticText11 = new wxStaticText( videoPanel, wxID_ANY, _("Settings.Video.DownscaleFilter"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT );
	m_staticText11->Wrap( -1 );
	fgSizer1->Add( m_staticText11, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 2 );
	
	filterList = new wxComboBox( videoPanel, ID_DOWNSCALE_FILTER, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY ); 
	filterList->Enable( false );
	
	fgSizer1->Add( filterList, 0, wxALL, 2 );
	
	
	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );
	
	disableAeroCheckbox = new wxCheckBox( videoPanel, ID_DISABLEAERO, _("Settings.DisableAeroWindows"), wxDefaultPosition, wxDefaultSize, 0 );
	disableAeroCheckbox->Enable( false );
	
	fgSizer1->Add( disableAeroCheckbox, 0, wxALIGN_CENTER_VERTICAL|wxALL, 2 );
	
	
	fgSizer1->Add( 0, 0, 1, wxEXPAND, 5 );
	
	m_staticline1 = new wxStaticLine( videoPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgSizer1->Add( m_staticline1, 0, wxEXPAND | wxALL, 5 );
	
	m_staticText22 = new wxStaticText( videoPanel, wxID_ANY, _("Settings.Video.FPS"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText22->Wrap( -1 );
	fgSizer1->Add( m_staticText22, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 2 );
	
	fpsTypeList = new wxChoicebook( videoPanel, ID_FPS_TYPE, wxDefaultPosition, wxDefaultSize, wxCHB_DEFAULT );
	m_panel13 = new wxPanel( fpsTypeList, ID_FPSPANEL_COMMON, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer45;
	bSizer45 = new wxBoxSizer( wxHORIZONTAL );
	
	fpsCommonList = new wxComboBox( m_panel13, ID_FPS_COMMON, _("30"), wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY );
	fpsCommonList->Append( _("10") );
	fpsCommonList->Append( _("20") );
	fpsCommonList->Append( _("25") );
	fpsCommonList->Append( _("29.97") );
	fpsCommonList->Append( _("30") );
	fpsCommonList->Append( _("48") );
	fpsCommonList->Append( _("59.94") );
	fpsCommonList->Append( _("60") );
	fpsCommonList->SetSelection( 4 );
	bSizer45->Add( fpsCommonList, 0, wxTOP|wxBOTTOM|wxRIGHT, 2 );
	
	
	m_panel13->SetSizer( bSizer45 );
	m_panel13->Layout();
	bSizer45->Fit( m_panel13 );
	fpsTypeList->AddPage( m_panel13, _("Settings.Video.FPS.Common"), true );
	m_panel14 = new wxPanel( fpsTypeList, ID_FPSPANEL_INTEGER, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer46;
	bSizer46 = new wxBoxSizer( wxHORIZONTAL );
	
	fpsIntegerScroller = new wxSpinCtrl( m_panel14, ID_FPS_INTEGER, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 120, 30 );
	bSizer46->Add( fpsIntegerScroller, 0, wxTOP|wxBOTTOM|wxRIGHT, 2 );
	
	
	m_panel14->SetSizer( bSizer46 );
	m_panel14->Layout();
	bSizer46->Fit( m_panel14 );
	fpsTypeList->AddPage( m_panel14, _("Settings.Video.FPS.Integer"), false );
	m_panel15 = new wxPanel( fpsTypeList, ID_FPSPANEL_FRACTION, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxFlexGridSizer* fgSizer10;
	fgSizer10 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer10->SetFlexibleDirection( wxBOTH );
	fgSizer10->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticText20 = new wxStaticText( m_panel15, wxID_ANY, _("Settings.Video.FPS.Numerator"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText20->Wrap( -1 );
	fgSizer10->Add( m_staticText20, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 2 );
	
	fpsNumeratorScroller = new wxSpinCtrl( m_panel15, ID_FPS_NUMERATOR, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 1000000000, 30 );
	fgSizer10->Add( fpsNumeratorScroller, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 2 );
	
	m_staticText21 = new wxStaticText( m_panel15, wxID_ANY, _("Settings.Video.FPS.Denominator"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText21->Wrap( -1 );
	fgSizer10->Add( m_staticText21, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 2 );
	
	fpsDenominatorScroller = new wxSpinCtrl( m_panel15, ID_FPS_DENOMINATOR, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 1000000000, 1 );
	fgSizer10->Add( fpsDenominatorScroller, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 2 );
	
	
	m_panel15->SetSizer( fgSizer10 );
	m_panel15->Layout();
	fgSizer10->Fit( m_panel15 );
	fpsTypeList->AddPage( m_panel15, _("Settings.Video.FPS.Fraction"), false );
	m_panel16 = new wxPanel( fpsTypeList, ID_FPSPANEL_NANOSECONDS, wxDefaultPosition, wxSize( -1,-1 ), wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer50;
	bSizer50 = new wxBoxSizer( wxHORIZONTAL );
	
	fpsNanosecondsScroller = new wxSpinCtrl( m_panel16, ID_FPS_NANOSECONDS, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 8333333, 1000000000, 33333333 );
	bSizer50->Add( fpsNanosecondsScroller, 0, wxTOP|wxBOTTOM|wxRIGHT, 2 );
	
	
	m_panel16->SetSizer( bSizer50 );
	m_panel16->Layout();
	bSizer50->Fit( m_panel16 );
	fpsTypeList->AddPage( m_panel16, _("Settings.Video.FPS.Nanoseconds"), false );
	fgSizer1->Add( fpsTypeList, 0, wxALL, 2 );
	
	
	bSizer34->Add( fgSizer1, 0, wxEXPAND, 5 );
	
	
	bSizer34->Add( 0, 20, 0, wxEXPAND, 5 );
	
	videoChangedText = new wxStaticText( videoPanel, wxID_ANY, _("Settings.StreamRestart"), wxDefaultPosition, wxDefaultSize, 0 );
	videoChangedText->Wrap( -1 );
	bSizer34->Add( videoChangedText, 1, wxALL|wxEXPAND, 5 );
	
	
	videoPanel->SetSizer( bSizer34 );
	videoPanel->Layout();
	bSizer34->Fit( videoPanel );
	settingsList->AddPage( videoPanel, _("Settings.Video"), false );
	audioPanel = new wxPanel( settingsList, ID_SETTINGS_AUDIO, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer36;
	bSizer36 = new wxBoxSizer( wxVERTICAL );
	
	
	bSizer36->Add( 0, 20, 0, wxEXPAND, 5 );
	
	wxFlexGridSizer* fgSizer11;
	fgSizer11 = new wxFlexGridSizer( 0, 2, 2, 0 );
	fgSizer11->SetFlexibleDirection( wxBOTH );
	fgSizer11->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	
	m_staticText23 = new wxStaticText( audioPanel, wxID_ANY, _("Settings.Audio.DesktopAudioDevice1"), wxDefaultPosition, wxSize( 270,-1 ), wxALIGN_RIGHT );
	m_staticText23->Wrap( -1 );
	fgSizer11->Add( m_staticText23, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 2 );
	
	desktopAudioDeviceList1 = new wxComboBox( audioPanel, ID_DESKTOP_AUDIO_DEVICE, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY ); 
	desktopAudioDeviceList1->Enable( false );
	
	fgSizer11->Add( desktopAudioDeviceList1, 0, wxALIGN_CENTER_VERTICAL|wxALL, 2 );
	
	m_staticText231 = new wxStaticText( audioPanel, wxID_ANY, _("Settings.Audio.DesktopAudioDevice2"), wxDefaultPosition, wxSize( 270,-1 ), wxALIGN_RIGHT );
	m_staticText231->Wrap( -1 );
	fgSizer11->Add( m_staticText231, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 2 );
	
	desktopAudioDeviceList2 = new wxComboBox( audioPanel, ID_DESKTOP_AUDIO_DEVICE, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY ); 
	desktopAudioDeviceList2->Enable( false );
	
	fgSizer11->Add( desktopAudioDeviceList2, 0, wxALIGN_CENTER_VERTICAL|wxALL, 2 );
	
	m_staticText24 = new wxStaticText( audioPanel, wxID_ANY, _("Settings.Audio.AuxAudioDevice1"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT );
	m_staticText24->Wrap( -1 );
	fgSizer11->Add( m_staticText24, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 2 );
	
	auxAudioDeviceList1 = new wxComboBox( audioPanel, ID_AUX_AUDIO_DEVICE1, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY ); 
	auxAudioDeviceList1->Enable( false );
	
	fgSizer11->Add( auxAudioDeviceList1, 0, wxALIGN_CENTER_VERTICAL|wxALL, 2 );
	
	m_staticText241 = new wxStaticText( audioPanel, wxID_ANY, _("Settings.Audio.AuxAudioDevice2"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT );
	m_staticText241->Wrap( -1 );
	fgSizer11->Add( m_staticText241, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 2 );
	
	auxAudioDeviceList2 = new wxComboBox( audioPanel, ID_AUX_AUDIO_DEVICE2, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY ); 
	auxAudioDeviceList2->Enable( false );
	
	fgSizer11->Add( auxAudioDeviceList2, 0, wxALIGN_CENTER_VERTICAL|wxALL, 2 );
	
	m_staticText242 = new wxStaticText( audioPanel, wxID_ANY, _("Settings.Audio.AuxAudioDevice3"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT );
	m_staticText242->Wrap( -1 );
	fgSizer11->Add( m_staticText242, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 2 );
	
	auxAudioDeviceList3 = new wxComboBox( audioPanel, ID_AUX_AUDIO_DEVICE3, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY ); 
	auxAudioDeviceList3->Enable( false );
	
	fgSizer11->Add( auxAudioDeviceList3, 0, wxALIGN_CENTER_VERTICAL|wxALL, 2 );
	
	m_staticText243 = new wxStaticText( audioPanel, wxID_ANY, _("Settings.Audio.AuxAudioDevice4"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT );
	m_staticText243->Wrap( -1 );
	fgSizer11->Add( m_staticText243, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 2 );
	
	auxAudioDeviceList4 = new wxComboBox( audioPanel, ID_AUX_AUDIO_DEVICE4, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY ); 
	auxAudioDeviceList4->Enable( false );
	
	fgSizer11->Add( auxAudioDeviceList4, 0, wxALIGN_CENTER_VERTICAL|wxALL, 2 );
	
	
	bSizer36->Add( fgSizer11, 0, wxEXPAND, 5 );
	
	
	audioPanel->SetSizer( bSizer36 );
	audioPanel->Layout();
	bSizer36->Fit( audioPanel );
	settingsList->AddPage( audioPanel, _("Settings.Audio"), false );
	/*#ifndef __WXGTK__ // Small icon style not supported in GTK
	wxListView* settingsListListView = settingsList->GetListView();
	long settingsListFlags = settingsListListView->GetWindowStyleFlag();
	settingsListFlags = ( settingsListFlags & ~wxLC_ICON ) | wxLC_SMALL_ICON;
	settingsListListView->SetWindowStyleFlag( settingsListFlags );
	#endif*/
	
	bSizer31->Add( settingsList, 1, wxEXPAND | wxALL, 5 );
	
	
	bSizer30->Add( bSizer31, 1, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer37;
	bSizer37 = new wxBoxSizer( wxHORIZONTAL );
	
	okButton = new wxButton( this, ID_OK, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer37->Add( okButton, 0, wxALL, 5 );
	
	cancelButton = new wxButton( this, ID_CANCEL, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer37->Add( cancelButton, 0, wxALL, 5 );
	
	applyButton = new wxButton( this, ID_APPLY, _("Apply"), wxDefaultPosition, wxDefaultSize, 0 );
	applyButton->Enable( false );
	
	bSizer37->Add( applyButton, 0, wxALL, 5 );
	
	
	bSizer30->Add( bSizer37, 0, wxALIGN_RIGHT, 5 );
	
	
	this->SetSizer( bSizer30 );
	this->Layout();
	
	this->Centre( wxBOTH );
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( OBSBasicSettingsBase::OnClose ) );
	settingsList->Connect( wxEVT_COMMAND_LISTBOOK_PAGE_CHANGED, wxListbookEventHandler( OBSBasicSettingsBase::PageChanged ), NULL, this );
	settingsList->Connect( wxEVT_COMMAND_LISTBOOK_PAGE_CHANGING, wxListbookEventHandler( OBSBasicSettingsBase::PageChanging ), NULL, this );
	okButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( OBSBasicSettingsBase::OKClicked ), NULL, this );
	cancelButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( OBSBasicSettingsBase::CancelClicked ), NULL, this );
	applyButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( OBSBasicSettingsBase::ApplyClicked ), NULL, this );
}

OBSBasicSettingsBase::~OBSBasicSettingsBase()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( OBSBasicSettingsBase::OnClose ) );
	settingsList->Disconnect( wxEVT_COMMAND_LISTBOOK_PAGE_CHANGED, wxListbookEventHandler( OBSBasicSettingsBase::PageChanged ), NULL, this );
	settingsList->Disconnect( wxEVT_COMMAND_LISTBOOK_PAGE_CHANGING, wxListbookEventHandler( OBSBasicSettingsBase::PageChanging ), NULL, this );
	okButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( OBSBasicSettingsBase::OKClicked ), NULL, this );
	cancelButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( OBSBasicSettingsBase::CancelClicked ), NULL, this );
	applyButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( OBSBasicSettingsBase::ApplyClicked ), NULL, this );
	
}

ProjectChooserBase::ProjectChooserBase( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizer40;
	bSizer40 = new wxBoxSizer( wxVERTICAL );
	
	m_staticText22 = new wxStaticText( this, wxID_ANY, _("ProjectChooser.SelectType"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText22->Wrap( -1 );
	bSizer40->Add( m_staticText22, 0, wxALL, 5 );
	
	wxBoxSizer* bSizer41;
	bSizer41 = new wxBoxSizer( wxVERTICAL );
	
	basicButton = new wxButton( this, wxID_ANY, _("ProjectChooser.Basic"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer41->Add( basicButton, 0, wxALL|wxEXPAND, 5 );
	
	studioButton = new wxButton( this, wxID_ANY, _("ProjectChooser.Studio"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer41->Add( studioButton, 0, wxALL|wxEXPAND, 5 );
	
	exitButton = new wxButton( this, wxID_ANY, _("MainWindow.Exit"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer41->Add( exitButton, 0, wxALL|wxEXPAND, 5 );
	
	
	bSizer40->Add( bSizer41, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	
	this->SetSizer( bSizer40 );
	this->Layout();
	
	this->Centre( wxBOTH );
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( ProjectChooserBase::OnClose ) );
	basicButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ProjectChooserBase::BasicClicked ), NULL, this );
	studioButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ProjectChooserBase::StudioClicked ), NULL, this );
	exitButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ProjectChooserBase::ExitClicked ), NULL, this );
}

ProjectChooserBase::~ProjectChooserBase()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( ProjectChooserBase::OnClose ) );
	basicButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ProjectChooserBase::BasicClicked ), NULL, this );
	studioButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ProjectChooserBase::StudioClicked ), NULL, this );
	exitButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( ProjectChooserBase::ExitClicked ), NULL, this );
	
}

NameDialogBase::NameDialogBase( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	wxBoxSizer* bSizer44;
	bSizer44 = new wxBoxSizer( wxVERTICAL );
	
	questionText = new wxStaticText( this, wxID_ANY, _("Please enter a name (or is it text you want to enter?):"), wxDefaultPosition, wxDefaultSize, 0 );
	questionText->Wrap( -1 );
	bSizer44->Add( questionText, 0, wxALL, 5 );
	
	wxBoxSizer* bSizer46;
	bSizer46 = new wxBoxSizer( wxHORIZONTAL );
	
	
	bSizer46->Add( 20, 0, 0, 0, 5 );
	
	nameEdit = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer46->Add( nameEdit, 1, wxALL|wxEXPAND, 5 );
	
	
	bSizer46->Add( 20, 0, 0, 0, 5 );
	
	
	bSizer44->Add( bSizer46, 0, wxEXPAND, 5 );
	
	wxBoxSizer* bSizer45;
	bSizer45 = new wxBoxSizer( wxHORIZONTAL );
	
	okButton = new wxButton( this, wxID_ANY, _("OK"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer45->Add( okButton, 0, wxALL, 5 );
	
	cancelButton = new wxButton( this, wxID_ANY, _("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer45->Add( cancelButton, 0, wxALL, 5 );
	
	
	bSizer44->Add( bSizer45, 0, wxALIGN_CENTER_HORIZONTAL, 5 );
	
	
	this->SetSizer( bSizer44 );
	this->Layout();
	
	this->Centre( wxBOTH );
}

NameDialogBase::~NameDialogBase()
{
}
