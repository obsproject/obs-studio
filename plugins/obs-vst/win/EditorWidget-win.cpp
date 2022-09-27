/*****************************************************************************
Copyright (C) 2016-2017 by Colin Edwards.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*****************************************************************************/

#include "../headers/EditorWidget.h"
#include "../VstWinDefs.h"

#include <functional>
#include <string>

using namespace std;

EditorWidget::~EditorWidget()
{

}

void EditorWidget::send_show()
{
	std::lock_guard<std::recursive_mutex> grd(m_mutex);

	if (!m_windowCreated && m_plugin->getEffect() != nullptr)
	{
		afx_sendHwndMsg(m_plugin->getEffect(), VstProxy::WM_USER_MSG::WM_USER_CREATE_WINDOW);
		m_windowCreated = true;
	}

	if (m_plugin->getEffect() != nullptr)
		afx_sendHwndMsg(m_plugin->getEffect(), VstProxy::WM_USER_MSG::WM_USER_SHOW);

	m_windowOpen = true;
}

void EditorWidget::send_hide()
{
	std::lock_guard<std::recursive_mutex> grd(m_mutex);

	if (m_windowCreated && m_plugin->getEffect() != nullptr)
		afx_sendHwndMsg(m_plugin->getEffect(), VstProxy::WM_USER_MSG::WM_USER_HIDE);

	m_windowOpen = false;
}

void EditorWidget::send_close()
{
	std::lock_guard<std::recursive_mutex> grd(m_mutex);

	m_plugin->chunkDataBank = m_plugin->getChunk(ChunkType::Bank);
	m_plugin->chunkDataProgram = m_plugin->getChunk(ChunkType::Program);
	m_plugin->chunkDataParameter = m_plugin->getChunk(ChunkType::Parameter);

	if (m_windowCreated && m_plugin->getEffect() != nullptr)
	{
		afx_sendHwndMsg(m_plugin->getEffect(), VstProxy::WM_USER_MSG::WM_USER_CLOSE);
		m_windowCreated = false;
	}

	m_windowOpen = false;
}

void EditorWidget::send_setChunk()
{
	std::lock_guard<std::recursive_mutex> grd(m_mutex);

	m_plugin->setChunk(ChunkType::Parameter, m_plugin->chunkDataParameter);
	m_plugin->setChunk(ChunkType::Program, m_plugin->chunkDataProgram);
	m_plugin->setChunk(ChunkType::Bank, m_plugin->chunkDataBank);
}

void EditorWidget::send_loadEffectFromPath(string path)
{
	//;
}

void EditorWidget::buildEffectContainer()
{
	//;
}

void EditorWidget::createWindow()
{
	//;
}

void EditorWidget::buildEffectContainer_worker()
{
	//;
}

void EditorWidget::dispatcherClose()
{
	//;
}

void EditorWidget::setWindowTitle(const char *title)
{
	//;
}

void EditorWidget::close()
{
	//;
}

void EditorWidget::show()
{
	//;
}

void EditorWidget::send_setWindowTitle(const char *title)
{
	//;
}
