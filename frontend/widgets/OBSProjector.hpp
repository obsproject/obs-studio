#pragma once

#include "OBSQTDisplay.hpp"

class Multiview;

enum class ProjectorType {
	Source,
	Scene,
	Preview,
	StudioProgram,
	Multiview,
};

class QMouseEvent;

class OBSProjector : public QWidget {
	Q_OBJECT

private:
	OBSQTDisplay display;
	OBSWeakSourceAutoRelease weakSource;
	std::vector<OBSSignal> sigs;
	bool qtFullscreenNativeWorks;

	static void OBSRenderMultiview(void *data, uint32_t cx, uint32_t cy);
	static void OBSRender(void *data, uint32_t cx, uint32_t cy);
	static void OBSSourceRenamed(void *data, calldata_t *params);
	static void OBSSourceDestroyed(void *data, calldata_t *params);

	void mousePressEvent(QMouseEvent *event) override;
	void mouseDoubleClickEvent(QMouseEvent *event) override;
	void closeEvent(QCloseEvent *event) override;
	virtual bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;

	bool isAlwaysOnTop;
	bool isAlwaysOnTopOverridden = false;
	int savedMonitor = -1;
	ProjectorType type = ProjectorType::Source;

	Multiview *multiview = nullptr;

	bool ready = false;

	void UpdateMultiview();
	void UpdateProjectorTitle(QString name);

	QRect prevGeometry;
	void SetMonitor(int monitor);

private slots:
	void EscapeTriggered();
	void OpenFullScreenProjector();
	void ResizeToContent();
	void OpenWindowedProjector();
	void AlwaysOnTopToggled(bool alwaysOnTop);
	void ScreenRemoved(QScreen *screen);
	void RenameProjector(QString oldName, QString newName);

public:
	OBSProjector(QWidget *widget, obs_source_t *source_, int monitor, ProjectorType type_);
	~OBSProjector();

	OBSSource GetSource();
	ProjectorType GetProjectorType();
	int GetMonitor();
	static void UpdateMultiviewProjectors();
	void SetHideCursor();

	bool IsAlwaysOnTop() const;
	bool IsAlwaysOnTopOverridden() const;
	void SetIsAlwaysOnTop(bool isAlwaysOnTop, bool isOverridden);
};
