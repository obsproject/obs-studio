#pragma once

#include <QDockWidget>
#include <QList>
#include <QMainWindow>

#include <algorithm>

/* Screen-independent sizing for the main window preview.
 *
 * QMainWindow::saveState stores dock sizes in absolute pixels. When that state
 * is restored on a smaller screen (or the window is shrunk), Qt keeps the docks
 * at their saved size and the central preview absorbs the whole difference,
 * so the video ends up squeezed. These helpers describe the preview as a share
 * of the window instead, and resize the docks around it to reach that share. */
namespace PreviewLayout {

constexpr double kMinShare = 0.30;
constexpr double kMaxShare = 0.90;
constexpr double kStep = 0.06;

/* Presets exposed in the View menu. */
constexpr double kCompactHeightShare = 0.44;
constexpr double kCompactWidthShare = 0.46;
constexpr double kBalancedHeightShare = 0.58;
constexpr double kBalancedWidthShare = 0.56;
constexpr double kLargeHeightShare = 0.72;
constexpr double kLargeWidthShare = 0.70;

/* Below these shares the preview counts as squeezed and docks are trimmed. */
constexpr double kFloorHeightShare = 0.36;
constexpr double kFloorWidthShare = 0.40;

inline double Clamp(double share)
{
	return std::clamp(share, kMinShare, kMaxShare);
}

/* Fraction of `windowExtent` used by `centralExtent`, or 0 when unknown. */
inline double Share(int centralExtent, int windowExtent)
{
	if (windowExtent <= 0 || centralExtent <= 0) {
		return 0.0;
	}
	return std::min(1.0, double(centralExtent) / double(windowExtent));
}

/* Extent the docks along one axis should occupy so the central area reaches
 * `share` of `windowExtent`, given the docks currently use `dockExtent` next
 * to a central area of `centralExtent`. Never returns less than `minimum`. */
inline int DockExtentForShare(double share, int windowExtent, int centralExtent, int dockExtent, int minimum)
{
	const int available = std::max(centralExtent, 0) + std::max(dockExtent, 0);
	const int targetCentral = int(Clamp(share) * double(windowExtent) + 0.5);
	const int result = available - targetCentral;
	return std::clamp(result, std::max(minimum, 0), std::max(available, std::max(minimum, 0)));
}

/* Splits `total` across groups in proportion to their current extents while
 * honouring each group's minimum. Groups with no current extent get nothing. */
inline QList<int> Distribute(int total, const QList<int> &current, const QList<int> &minimums)
{
	QList<int> result;
	int sum = 0;
	for (int extent : current) {
		sum += std::max(extent, 0);
	}

	for (int i = 0; i < current.size(); i++) {
		const int minimum = i < minimums.size() ? std::max(minimums[i], 0) : 0;
		int extent = 0;
		if (sum > 0 && current[i] > 0) {
			extent = int(double(total) * double(current[i]) / double(sum) + 0.5);
		}
		result.append(std::max(extent, minimum));
	}
	return result;
}

/* Docked, visible dock widgets currently living in `area`. */
inline QList<QDockWidget *> DocksInArea(const QMainWindow *window, Qt::DockWidgetArea area)
{
	QList<QDockWidget *> docks;
	for (QDockWidget *dock : window->findChildren<QDockWidget *>(QString(), Qt::FindDirectChildrenOnly)) {
		if (dock->isFloating() || !dock->isVisible()) {
			continue;
		}
		if (window->dockWidgetArea(dock) == area) {
			docks.append(dock);
		}
	}
	return docks;
}

inline int GroupExtent(const QList<QDockWidget *> &docks, Qt::Orientation orientation)
{
	int extent = 0;
	for (QDockWidget *dock : docks) {
		extent = std::max(extent, orientation == Qt::Vertical ? dock->height() : dock->width());
	}
	return extent;
}

inline int GroupMinimum(const QList<QDockWidget *> &docks, Qt::Orientation orientation)
{
	int minimum = 0;
	for (QDockWidget *dock : docks) {
		const QSize hint = dock->minimumSizeHint();
		const int extent = orientation == Qt::Vertical ? std::max(hint.height(), dock->minimumHeight())
							       : std::max(hint.width(), dock->minimumWidth());
		minimum = std::max(minimum, extent);
	}
	return minimum;
}

inline double HeightShare(const QMainWindow *window)
{
	const QWidget *central = window->centralWidget();
	return central ? Share(central->height(), window->height()) : 0.0;
}

inline double WidthShare(const QMainWindow *window)
{
	const QWidget *central = window->centralWidget();
	return central ? Share(central->width(), window->width()) : 0.0;
}

/* Resizes the bottom dock row so the central area takes `share` of the window
 * height. Returns false when there is nothing docked below the preview. */
inline bool ApplyHeightShare(QMainWindow *window, double share)
{
	const QWidget *central = window->centralWidget();
	const QList<QDockWidget *> bottom = DocksInArea(window, Qt::BottomDockWidgetArea);
	if (!central || bottom.isEmpty()) {
		return false;
	}

	const int target = DockExtentForShare(share, window->height(), central->height(),
					      GroupExtent(bottom, Qt::Vertical), GroupMinimum(bottom, Qt::Vertical));
	QList<int> sizes;
	for (int i = 0; i < bottom.size(); i++) {
		sizes.append(target);
	}
	window->resizeDocks(bottom, sizes, Qt::Vertical);
	return true;
}

/* Resizes the left and right dock columns so the central area takes `share`
 * of the window width. The reduction is split in proportion to the current
 * column widths. Returns false when nothing is docked beside the preview. */
inline bool ApplyWidthShare(QMainWindow *window, double share)
{
	const QWidget *central = window->centralWidget();
	const QList<QDockWidget *> left = DocksInArea(window, Qt::LeftDockWidgetArea);
	const QList<QDockWidget *> right = DocksInArea(window, Qt::RightDockWidgetArea);
	if (!central || (left.isEmpty() && right.isEmpty())) {
		return false;
	}

	const int leftWidth = GroupExtent(left, Qt::Horizontal);
	const int rightWidth = GroupExtent(right, Qt::Horizontal);
	const int leftMin = GroupMinimum(left, Qt::Horizontal);
	const int rightMin = GroupMinimum(right, Qt::Horizontal);
	const int target = DockExtentForShare(share, window->width(), central->width(), leftWidth + rightWidth,
					      leftMin + rightMin);
	const QList<int> split = Distribute(target, {leftWidth, rightWidth}, {leftMin, rightMin});

	if (!left.isEmpty()) {
		QList<int> sizes;
		for (int i = 0; i < left.size(); i++) {
			sizes.append(split[0]);
		}
		window->resizeDocks(left, sizes, Qt::Horizontal);
	}
	if (!right.isEmpty()) {
		QList<int> sizes;
		for (int i = 0; i < right.size(); i++) {
			sizes.append(split[1]);
		}
		window->resizeDocks(right, sizes, Qt::Horizontal);
	}
	return true;
}

inline void ApplyShare(QMainWindow *window, double heightShare, double widthShare)
{
	ApplyHeightShare(window, heightShare);
	ApplyWidthShare(window, widthShare);
}

/* Trims docks only when the preview has fallen below the floor on an axis;
 * a preview the user made deliberately small on purpose is left alone. */
inline bool EnsureMinimumShare(QMainWindow *window, double floorHeightShare = kFloorHeightShare,
			       double floorWidthShare = kFloorWidthShare)
{
	bool changed = false;
	if (HeightShare(window) < floorHeightShare) {
		changed = ApplyHeightShare(window, floorHeightShare) || changed;
	}
	if (WidthShare(window) < floorWidthShare) {
		changed = ApplyWidthShare(window, floorWidthShare) || changed;
	}
	return changed;
}

} // namespace PreviewLayout
