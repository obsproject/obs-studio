#include "SceneButtons.h"

SceneButtons::SceneButtons(QWidget *parent) : QWidget(parent)
{
	m_layout = new QGridLayout(this);
	setLayout(m_layout);

	addScene("Scene 1");
	addScene("Scene 2");
	addScene("BRB Screen");
	addScene("Ending");
}

void SceneButtons::addScene(const QString &name)
{
	QPushButton *btn = new QPushButton(name, this);
	btn->setMinimumHeight(60);
	btn->setStyleSheet("font-size: 14px; font-weight: bold;");

	int row = m_count / m_maxCols;
	int col = m_count % m_maxCols;
	m_layout->addWidget(btn, row, col);

	// Checkable processing to show "Active" state later

	m_count++;
}
