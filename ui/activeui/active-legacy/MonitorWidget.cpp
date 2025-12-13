#include "MonitorWidget.h"
#include <QPainter>
#include <QStyleOption>

MonitorWidget::MonitorWidget(const QString &title, QWidget *parent) : QWidget(parent), m_title(title)
{
	// Styling
	setAutoFillBackground(true);
	QVBoxLayout *layout = new QVBoxLayout(this);

	// Title Label
	QLabel *titleLabel = new QLabel(m_title, this);
	titleLabel->setStyleSheet("color: #AAAAAA; font-weight: bold;");
	titleLabel->setAlignment(Qt::AlignCenter);
	layout->addWidget(titleLabel);

	// Video Placeholder (Status)
	m_statusLabel = new QLabel("Waiting for Source...", this);
	m_statusLabel->setStyleSheet("color: #666666; font-size: 10px;");
	m_statusLabel->setAlignment(Qt::AlignCenter);
	layout->addWidget(m_statusLabel);

	layout->addStretch();
}

void MonitorWidget::setStatus(const QString &status)
{
	m_statusLabel->setText(status);
}

void MonitorWidget::paintEvent(QPaintEvent *)
{
	QStyleOption opt;
	opt.initFrom(this);
	QPainter p(this);

	// Background
	p.fillRect(rect(), QColor(20, 20, 20));

	// Border
	p.setPen(QPen(QColor(50, 50, 50), 1));
	p.drawRect(rect().adjusted(0, 0, -1, -1));
}

// TODO: Implement resizeEvent to send CMD_RESIZE_PREVIEW to libvr
// This ensures the backend renders the shared texture at the correct resolution
// for the dynamic tile size.
// void MonitorWidget::resizeEvent(QResizeEvent *event) {
//     if (m_previewHandle) {
//         VRClient::get()->sendResize(m_previewHandle, width(), height());
//     }
//     QWidget::resizeEvent(event);
// }
