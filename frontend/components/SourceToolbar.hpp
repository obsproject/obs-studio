#pragma once

#include <obs.hpp>

#include <QWidget>

class SourceToolbar : public QWidget {
	Q_OBJECT

	OBSWeakSource weakSource;

protected:
	using properties_delete_t = decltype(&obs_properties_destroy);
	using properties_t = std::unique_ptr<obs_properties_t, properties_delete_t>;

	properties_t props;
	OBSDataAutoRelease oldData;

	void SaveOldProperties(obs_source_t *source);
	void SetUndoProperties(obs_source_t *source, bool repeatable = false);

public:
	SourceToolbar(QWidget *parent, OBSSource source);

	OBSSource GetSource() { return OBSGetStrongRef(weakSource); }

public slots:
	virtual void Update() {}
};
