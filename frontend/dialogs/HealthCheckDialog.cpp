/******************************************************************************
    Copyright (C) 2026 by Taylor Giampaolo <warchamp7@obsproject.com>

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

#include "HealthCheckDialog.hpp"

#include <OBSApp.hpp>

#include <utility/HealthCheckService.hpp>
#include <utility/HealthCheckItem.hpp>
#include <widgets/HealthCheckWidget.hpp>

#include <QDialogButtonBox>
#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>

HealthCheckDialog::HealthCheckDialog(QWidget *parent) : QWidget(parent)
{
	setWindowFlag(Qt::Window, true);
	setWindowTitle("Detected Issues");
	resize(600, 320);

	auto *buttons = new QDialogButtonBox({QDialogButtonBox::Close}, this);
	connect(buttons, &QDialogButtonBox::rejected, this, &QWidget::close);

	QVBoxLayout *windowLayout = new QVBoxLayout(this);
	setLayout(windowLayout);

	auto *healthScroll = new QScrollArea(this);

	auto *healthScrollContents = new QFrame(healthScroll);
	auto *scrollLayout = new QVBoxLayout(healthScrollContents);
	scrollLayout->setContentsMargins(0, 0, 0, 0);

	healthScrollContents->setLayout(scrollLayout);
	healthScrollContents->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
	healthScroll->setFrameShape(QFrame::NoFrame);
	healthScroll->setLineWidth(0);

	healthScroll->setWidget(healthScrollContents);
	healthScroll->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
	healthScroll->setWidgetResizable(true);
	healthScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	healthScroll->viewport()->setAutoFillBackground(false);
	healthScrollContents->setAutoFillBackground(false);

	auto healthIssues = App()->healthService()->getInvalidItems();

	auto criticalLayout = new QVBoxLayout();
	auto warningLayout = new QVBoxLayout();

	scrollLayout->addLayout(criticalLayout);
	scrollLayout->addLayout(warningLayout);

	auto addWidgetToStatusLayout = [criticalLayout, warningLayout](HealthCheckWidget *widget) {
		if (widget->item()->status() == OBS::HealthStatus::Critical) {
			criticalLayout->addWidget(widget);
		} else if (widget->item()->status() == OBS::HealthStatus::Warning) {
			warningLayout->addWidget(widget);
		}
	};

	for (const auto &item : healthIssues) {
		auto *healthCheckWidget = new HealthCheckWidget(healthScrollContents, item);

		addWidgetToStatusLayout(healthCheckWidget);

		connect(item, &OBS::HealthCheckItem::statusChanged, healthCheckWidget, [healthCheckWidget, item]() {
			if (item->status() == OBS::HealthStatus::Valid) {
				healthCheckWidget->hide();
			}
		});
	}

	connect(App()->healthService(), &OBS::HealthCheckService::globalStatusChanged, this, [this]() {
		if (App()->healthService()->getInvalidCount() == 0) {
			close();
		}
	});

	scrollLayout->addStretch(1);

	windowLayout->addWidget(healthScroll);
	windowLayout->addWidget(buttons);
}
