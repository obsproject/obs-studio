#pragma once

#include <QString>
#include <QTranslator>

class OBSTranslator : public QTranslator {
	Q_OBJECT

public:
	virtual bool isEmpty() const override { return false; }

	virtual QString translate(const char *context, const char *sourceText, const char *disambiguation,
				  int n) const override;
};
