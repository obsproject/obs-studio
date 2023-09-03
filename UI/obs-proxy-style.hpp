#pragma once

#include <QProxyStyle>

#if defined(_WIN32) && QT_VERSION == QT_VERSION_CHECK(6, 5, 2)
#define QT_TOOLTIP_WORKAROUND_NEEDED
#endif

class OBSProxyStyle : public QProxyStyle {
public:
	int styleHint(StyleHint hint, const QStyleOption *option,
		      const QWidget *widget,
		      QStyleHintReturn *returnData) const override;

#ifdef QT_TOOLTIP_WORKAROUND_NEEDED
	void polish(QWidget *widget) override;
#endif
};

class OBSContextBarProxyStyle : public OBSProxyStyle {
public:
	QPixmap generatedIconPixmap(QIcon::Mode iconMode, const QPixmap &pixmap,
				    const QStyleOption *option) const override;
};
