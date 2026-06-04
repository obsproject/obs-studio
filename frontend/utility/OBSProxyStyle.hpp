#pragma once

#include <QProxyStyle>

class OBSProxyStyle : public QProxyStyle {
	Q_OBJECT

public:
	OBSProxyStyle() : QProxyStyle() {}

	OBSProxyStyle(const QString &key) : QProxyStyle(key) {}

	int styleHint(StyleHint hint, const QStyleOption *option, const QWidget *widget,
		      QStyleHintReturn *returnData) const override;
};

class OBSInvisibleCursorProxyStyle : public OBSProxyStyle {
	Q_OBJECT

public:
	OBSInvisibleCursorProxyStyle() : OBSProxyStyle() {}

	int pixelMetric(PixelMetric pm, const QStyleOption *option, const QWidget *widget) const override;
};
