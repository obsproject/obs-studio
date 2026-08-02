#pragma once

#include <obs.hpp>

#include <QLabel>
#include <QWidget>

// Live gain-reduction meter shown above compressor filter properties.
// Polls the filter's get_gain_reduction proc handler on a timer.
class GainReductionMeter : public QWidget {
	Q_OBJECT

public:
	// source is the compressor filter instance to meter
	explicit GainReductionMeter(QWidget *parent = nullptr, obs_source_t *source = nullptr);
	~GainReductionMeter() override;

	QSize minimumSizeHint() const override;
	QSize sizeHint() const override;

protected:
	// Draws the amber GR bar and peak-hold tick at the bottom of the widget
	void paintEvent(QPaintEvent *event) override;

private slots:
	// UI-thread cleanup when the filter source is destroyed
	void onSourceDestroyed();
	// Timer callback: poll GR, update peak hold + label, repaint
	void tick();

private:
	// OBS signal callback (may run off the UI thread) -> queues onSourceDestroyed
	static void obsSourceDestroyed(void *data, calldata_t *);

	// Reads current GR dB from the filter via proc_handler
	void pollGainReduction();
	// Keeps the most-reduced (most negative) value for kPeakHoldDurationSec
	void updatePeakHold(float gainReductionDb, uint64_t ts);
	// Maps GR dB (0 .. kMinimumDb) to a horizontal pixel width
	float dbToBarWidth(float gainReductionDb, int width) const;

	OBSWeakSource weakSource;
	OBSSignal destroyedSignal;

	QLabel *valueLabel = nullptr; // live numeric readout, e.g. "-6.2 dB"

	float currentGainReductionDb = 0.0f; // live value (bar fill + label)
	float peakHoldDb = 0.0f;             // peak marker only (not shown in label)
	uint64_t peakHoldTimeNs = 0;         // when peakHoldDb was last set

	static constexpr float kMinimumDb = -60.0f;         // right edge of the bar scale
	static constexpr float kPeakHoldDurationSec = 1.5f; // how long the peak tick sticks
};
