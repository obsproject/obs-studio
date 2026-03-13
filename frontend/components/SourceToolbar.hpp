#pragma once

#include <obs.hpp>

#include <QWidget>

class SourceToolbar : public QWidget {
	Q_OBJECT

	OBSWeakSource weakSource;

protected:
	OBSProperties props;
	OBSDataAutoRelease oldData;

	void SaveOldProperties(obs_source_t *source);
	void SetUndoProperties(obs_source_t *source, bool repeatable = false);

public:
	SourceToolbar(QWidget *parent, OBSSource source);

	OBSSource GetSource() { return OBSGetStrongRef(weakSource); }

public slots:
	virtual void Update() {}
};
