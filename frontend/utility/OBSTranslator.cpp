#include "OBSTranslator.hpp"

#include <OBSApp.hpp>

#include <qt-wrappers.hpp>

#include "moc_OBSTranslator.cpp"

QString OBSTranslator::translate(const char *, const char *sourceText, const char *, int) const
{
	const char *out = nullptr;
	QString str(sourceText);
	str.replace(" ", "");
	if (!App()->TranslateString(QT_TO_UTF8(str), &out))
		return QString(sourceText);

	return QT_UTF8(out);
}
