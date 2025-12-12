#include "MonitorGrid.h"

MonitorGrid::MonitorGrid(QWidget *parent) : QWidget(parent)
{
	m_layout = new QGridLayout(this);
	m_layout->setSpacing(5);
	m_layout->setContentsMargins(5, 5, 5, 5);
}

void MonitorGrid::addMonitor(const QString &title)
{
	MonitorWidget *w = new MonitorWidget(title, this);
	m_monitors.push_back(w);

	int index = m_monitors.size() - 1;
	int r = index / m_cols;
	int c = index % m_cols;

	m_layout->addWidget(w, r, c);
}
