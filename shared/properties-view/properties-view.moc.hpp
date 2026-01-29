#pragma once

#include <QComboBox>
#include <QLabel>
#include <QSpinBox>
#include <QStackedWidget>
#include <QWidget>

#include <obs.h>
#include <media-io/frame-rate.h>

#include <vector>

#ifdef _MSC_VER
#pragma warning(disable : 4505)
#endif

static bool operator!=(const media_frames_per_second &a, const media_frames_per_second &b)
{
	// Compare fractions via cross multiplication, to avoid rounding or division
	uint64_t lhs = (uint64_t)a.numerator * (uint64_t)b.denominator;
	uint64_t rhs = (uint64_t)b.numerator * (uint64_t)a.denominator;
	return lhs != rhs;
}

static bool operator==(const media_frames_per_second &a, const media_frames_per_second &b)
{
	return !(a != b);
}

using frame_rate_range_t = std::pair<media_frames_per_second, media_frames_per_second>;
using frame_rate_ranges_t = std::vector<frame_rate_range_t>;

class OBSFrameRatePropertyWidget : public QWidget {
	Q_OBJECT

public:
	frame_rate_ranges_t fps_ranges;

	QComboBox *modeSelect = nullptr;
	QStackedWidget *modeDisplay = nullptr;

	QWidget *labels = nullptr;
	QLabel *currentFPS = nullptr;
	QLabel *timePerFrame = nullptr;
	QLabel *minLabel = nullptr;
	QLabel *maxLabel = nullptr;

	QComboBox *simpleFPS = nullptr;

	QComboBox *fpsRange = nullptr;
	QSpinBox *numEdit = nullptr;
	QSpinBox *denEdit = nullptr;

	bool updating = false;

	const char *name = nullptr;
	obs_data_t *settings = nullptr;

	QLabel *warningLabel = nullptr;

	OBSFrameRatePropertyWidget() = default;
};
