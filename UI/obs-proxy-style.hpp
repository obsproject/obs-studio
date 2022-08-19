#pragma once

#include <QProxyStyle>

class OBSIgnoreWheelProxyStyle : public QProxyStyle {
public:
	int styleHint(StyleHint hint, const QStyleOption *option,
		      const QWidget *widget,
		      QStyleHintReturn *returnData) const override;
};

class OBSContextBarProxyStyle : public OBSIgnoreWheelProxyStyle {
public:
	QPixmap generatedIconPixmap(QIcon::Mode iconMode, const QPixmap &pixmap,
				    const QStyleOption *option) const override;
};
