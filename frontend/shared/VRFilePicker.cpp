#include "VRFilePicker.h"
#include <QFileDialog>
#include "VRCommon.h"

VRFilePicker::VRFilePicker(const QString &placeholder, QWidget *parent) : QWidget(parent)
{
	QHBoxLayout *layout = new QHBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);

	m_lineEdit = new QLineEdit(this);
	m_lineEdit->setPlaceholderText(placeholder);
	m_lineEdit->setStyleSheet("background: " + VRTheme::color("bg_light") +
				  "; color: white; border: 1px solid #555; padding: 5px;");

	m_browseBtn = new QPushButton("...", this);
	m_browseBtn->setFixedWidth(30);
	m_browseBtn->setStyleSheet(VRTheme::buttonStyle());

	layout->addWidget(m_lineEdit);
	layout->addWidget(m_browseBtn);

	connect(m_browseBtn, &QPushButton::clicked, this, [this]() {
		QString path = QFileDialog::getOpenFileName(this, "Select File");
		if (!path.isEmpty()) {
			m_lineEdit->setText(path);
			emit fileSelected(path);
		}
	});
}

QString VRFilePicker::filePath() const
{
	return m_lineEdit->text();
}
