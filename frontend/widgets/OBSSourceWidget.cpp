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

#include "OBSSourceWidget.hpp"

#include <widgets/OBSSourceWidgetView.hpp>

#include <QScrollArea>
#include <QVBoxLayout>
#include <QWindow>

#include "moc_OBSSourceWidget.cpp"

constexpr int kMinWidth = 48;
constexpr int kMinHeight = 27;

OBSSourceWidget::OBSSourceWidget(QWidget *parent) : QFrame(parent), fixedAspectRatio(0.0)
{
	layout = new QVBoxLayout();
	setLayout(layout);

	layout->setContentsMargins(0, 0, 0, 0);

	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	if (window()) {
		window()->installEventFilter(this);
	}

	if (parent) {
		parent->installEventFilter(this);
	}

	QObject *checkParent = parent;

	while (checkParent) {
		QScrollArea *scrollParent = qobject_cast<QScrollArea *>(checkParent);
		if (scrollParent && scrollParent->widget()) {
			scrollParent->widget()->installEventFilter(this);
		}

		if (!checkParent->parent() || checkParent->parent() == checkParent) {
			break;
		}

		checkParent = checkParent->parent();
	}
}

OBSSourceWidget::OBSSourceWidget(QWidget *parent, obs_source_t *source) : OBSSourceWidget(parent)
{
	setSource(source);
}

void OBSSourceWidget::setFixedAspectRatio(double ratio)
{
	if (ratio > 0.0) {
		fixedAspectRatio = ratio;
	} else {
		fixedAspectRatio = 0;
	}
}

void OBSSourceWidget::setSource(obs_source_t *source)
{
	if (!sourceView) {
		sourceView = new OBSSourceWidgetView(this, source);
		layout->addWidget(sourceView);

		connect(sourceView, &OBSSourceWidgetView::viewReady, this, [this]() {
			updateGeometry();
			resizeSourceView();
		});
	}

	sourceView->setSource(source);
}

void OBSSourceWidget::setForceLinearSRGB(bool enable)
{
	if (!sourceView) {
		return;
	}

	sourceView->setForceLinearSRGB(enable);
}

void OBSSourceWidget::resizeSourceView()
{
	if (!sourceView) {
		return;
	}

	if (sourceView->sourceWidth() <= 0 || sourceView->sourceHeight() <= 0) {
		return;
	}

	const double aspectRatio = fixedAspectRatio > 0
					   ? fixedAspectRatio
					   : (double)sourceView->sourceWidth() / (double)sourceView->sourceHeight();

	// Widget only expands in one direction
	const bool singleExpandDirection = (sizePolicy().horizontalPolicy() & QSizePolicy::ExpandFlag) !=
					   (sizePolicy().verticalPolicy() & QSizePolicy::ExpandFlag);

	const int scaledWidth = std::floor(height() / aspectRatio);
	const int scaledHeight = std::floor(width() / aspectRatio);

	if (fixedAspectRatio) {
		setMaximumWidth(QWIDGETSIZE_MAX);
		setMaximumHeight(scaledHeight);
	} else if (singleExpandDirection) {
		setMaximumWidth(QWIDGETSIZE_MAX);
		setMaximumHeight(QWIDGETSIZE_MAX);

		if ((sizePolicy().horizontalPolicy() & QSizePolicy::ExpandFlag) == QSizePolicy::ExpandFlag) {
			setMaximumHeight(scaledHeight);
		} else {
			setMaximumWidth(scaledWidth);
		}
	}

	QWindow *nativeWindow = sourceView->windowHandle();
	QRegion visible = sourceView->visibleRegion();
	if (nativeWindow) {
		QPoint position = sourceView->mapTo(sourceView->nativeParentWidget(), QPoint());
		nativeWindow->setGeometry(QRect(position, sourceView->geometry().size()));

		if (!visible.isNull()) {
			if (visible.boundingRect().width() > 0 && visible.boundingRect().height() > 0) {
				nativeWindow->setMask(visible.boundingRect());
			}
		} else {
			nativeWindow->setMask(QRegion(0, 0, 1, 1));
		}
	}
}

QSize OBSSourceWidget::minimumSizeHint() const
{
	if (fixedAspectRatio > 0.0) {
		const int width = int(kMinHeight * fixedAspectRatio);

		return QSize(width, kMinHeight);
	}

	return QSize(kMinWidth, kMinHeight);
}

QSize OBSSourceWidget::sizeHint() const
{
	if (sourceView) {
		const int width = sourceView->sourceWidth();
		const int height = sourceView->sourceHeight();

		if (width > 0 && height > 0) {
			return QSize(width, height);
		}
	}

	return QSize(kMinWidth, kMinHeight);
}

bool OBSSourceWidget::hasHeightForWidth() const
{
	return true;
}

int OBSSourceWidget::heightForWidth(int width) const
{
	double aspectRatio = fixedAspectRatio;

	if (aspectRatio <= 0.0 && sourceView && sourceView->sourceWidth() > 0 && sourceView->sourceHeight() > 0) {
		aspectRatio = double(sourceView->sourceWidth()) / double(sourceView->sourceHeight());
	}

	if (aspectRatio <= 0.0) {
		aspectRatio = 16.0 / 9.0;
	}

	return int(width / aspectRatio);
}

bool OBSSourceWidget::eventFilter(QObject *, QEvent *event)
{
	if (event->type() == QEvent::Resize) {
		resizeSourceView();
	} else if (event->type() == QEvent::Move) {
		resizeSourceView();
	}

	return false;
}

void OBSSourceWidget::moveEvent(QMoveEvent *event)
{
	QFrame::moveEvent(event);
	resizeSourceView();
}

void OBSSourceWidget::resizeEvent(QResizeEvent *event)
{
	QFrame::resizeEvent(event);
	resizeSourceView();
}

OBSSourceWidget::~OBSSourceWidget() {}
