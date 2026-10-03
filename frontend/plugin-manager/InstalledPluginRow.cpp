/******************************************************************************
    Copyright (C) 2026 by Warchamp7 <warchamp7@obsproject.com>

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
******************************************************************************/

#include "InstalledPluginRow.hpp"

#include <OBSApp.hpp>
#include <components/InfoChip.hpp>

#include <Idian/InlineButton.hpp>

constexpr std::string_view kLegacyPluginInfoLink{"https://obsproject.com/go/legacy-plugin-locations"};

namespace OBS {
InstalledPluginRow::InstalledPluginRow(QWidget *parent, const PluginManagerWindow::Entry &entry) : idian::Row(parent)
{
	OBS::ModuleInfo *metadata = entry.module;

	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
	QString name = entry.name;

	QString version = metadata && !metadata->version.empty() ? metadata->version.c_str() : "";

	auto *moduleText = new QWidget{this};
	moduleText->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
	auto *infoLayout = new QVBoxLayout{moduleText};
	infoLayout->setContentsMargins(0, 0, 0, 0);
	moduleText->setLayout(infoLayout);

	auto *headerLayout = new QHBoxLayout{};
	headerLayout->setContentsMargins(0, 0, 0, 0);
	headerLayout->setAlignment(Qt::AlignLeft);
	infoLayout->addLayout(headerLayout);

	auto nameLabel = new QLabel{name, moduleText};
	nameLabel->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Minimum);
	nameLabel->setIndent(0);
	idian::Utils::addClass(nameLabel, "title");
	if (metadata && !metadata->enabledAtLaunch) {
		idian::Utils::addClass(nameLabel, "text-muted");
	}
	headerLayout->addWidget(nameLabel);

	if (!version.isEmpty()) {
		auto versionLabel = new QLabel{version, moduleText};
		versionLabel->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Minimum);
		idian::Utils::addClass(versionLabel, "description");
		headerLayout->addWidget(versionLabel);
	}

	QVBoxLayout *detailsLayout = new QVBoxLayout{};
	detailsLayout->setContentsMargins(0, 0, 0, 0);
	infoLayout->addLayout(detailsLayout);

	if (!entry.isLegacy) {
		// TODO: Awaiting further implementation of module manifest data.
	}

	this->addWidget(moduleText);

	InfoChip *statusChip{nullptr};
	if (entry.status == PluginManagerWindow::Status::Error) {
		idian::Utils::addClass(nameLabel, "text-muted");

		statusChip = new InfoChip{QTStr("PluginManager.Status.Error"), moduleText};
		idian::Utils::addClass(statusChip, "bg-warning");
		idian::Utils::addClass(statusChip, "text-warning");
	} else if (entry.status == PluginManagerWindow::Status::Missing) {
		idian::Utils::addClass(nameLabel, "text-muted");

		statusChip = new InfoChip{QTStr("PluginManager.Status.Missing"), moduleText};
		idian::Utils::addClass(statusChip, "bg-danger");
		idian::Utils::addClass(statusChip, "text-danger");
	} else {
		if (entry.isLegacy) {
			auto legacyChip = new InfoChip{QTStr("PluginManager.Status.Legacy"), moduleText};
			idian::Utils::addClass(legacyChip, "bg-primary");

			headerLayout->insertWidget(0, legacyChip);

			auto legacyNotice = new QWidget{};
			legacyNotice->setLayout(new QHBoxLayout{});
			legacyNotice->layout()->setContentsMargins(0, 0, 0, 0);

			auto warningIcon = new QLabel{moduleText};
			warningIcon->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
			warningIcon->setPixmap(getWarningIcon().pixmap(16, 16));

			auto legacyInfo =
				new QLabel{QTStr("PluginManager.Status.Legacy.Description").arg(kLegacyPluginInfoLink),
					   moduleText};
			legacyInfo->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
			legacyInfo->setOpenExternalLinks(true);
			idian::Utils::addClass(legacyInfo, "description");

			legacyNotice->layout()->addWidget(warningIcon);
			legacyNotice->layout()->addWidget(legacyInfo);

			detailsLayout->addWidget(legacyNotice);
		}

		if (metadata && !metadata->enabledAtLaunch) {
			statusChip = new InfoChip{QTStr("PluginManager.Status.Disabled"), moduleText};
			idian::Utils::addClass(statusChip, "bg-info");
			idian::Utils::addClass(statusChip, "text-muted");
		}
	}

	if (statusChip) {
		headerLayout->addWidget(statusChip);
	}

	if (metadata) {
		auto toggleSwitch = new idian::ToggleSwitch(this);
		toggleSwitch->setChecked(metadata->enabled);
		addWidget(toggleSwitch);
		toggleSwitch->setAccessibleDescription(QTStr("PluginManager.Button.Enable").arg(name));

		connect(toggleSwitch, &idian::ToggleSwitch::toggled, this, [this, metadata](bool checked) {
			metadata->enabled = checked;

			emit toggleChanged();
		});
	}

	if (entry.status == PluginManagerWindow::Status::Missing) {
		auto removeButton = new idian::InlineButton(this);
		removeButton->setAccessibleName(QTStr("Remove"));
		removeButton->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
		removeButton->setIcon(getTrashIcon());
		idian::Utils::addClass(removeButton, "icon-trash");
		addWidget(removeButton);

		connect(removeButton, &QAbstractButton::clicked, this, [this]() {
			setEnabled(false);

			emit trashClicked();
		});
	}
}
const QIcon &InstalledPluginRow::getWarningIcon()
{
	static const QIcon &icon = *new QIcon(":/res/images/warning.svg");
	return icon;
}
const QIcon &InstalledPluginRow::getTrashIcon()
{
	static const QIcon &icon = *new QIcon(":/res/images/trash.svg");
	return icon;
}
} // namespace OBS
