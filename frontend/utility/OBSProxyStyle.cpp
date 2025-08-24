#include "OBSProxyStyle.hpp"
#include "moc_OBSProxyStyle.cpp"

int OBSProxyStyle::styleHint(StyleHint hint, const QStyleOption *option, const QWidget *widget,
			     QStyleHintReturn *returnData) const
{
	if (hint == SH_ComboBox_AllowWheelScrolling)
		return 0;
#ifdef __APPLE__
	if (hint == SH_ComboBox_UseNativePopup)
		return 1;
#endif

	return QProxyStyle::styleHint(hint, option, widget, returnData);
}

int OBSInvisibleCursorProxyStyle::pixelMetric(PixelMetric metric, const QStyleOption *option,
					      const QWidget *widget) const
{

	if (metric == PM_TextCursorWidth)
		return 0;

	return QProxyStyle::pixelMetric(metric, option, widget);
}
