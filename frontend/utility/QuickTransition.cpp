#include "QuickTransition.hpp"

#include <OBSApp.hpp>
#include <qt-wrappers.hpp>

static inline QString MakeQuickTransitionText(QuickTransition *qt)
{
	QString name;

	if (!qt->fadeToBlack)
		name = QT_UTF8(obs_source_get_name(qt->source));
	else
		name = QTStr("FadeToBlack");

	if (!obs_transition_fixed(qt->source))
		name += QString(" (%1ms)").arg(QString::number(qt->duration));
	return name;
}

void QuickTransition::SourceRenamed(void *param, calldata_t *)
{
	QuickTransition *qt = reinterpret_cast<QuickTransition *>(param);

	QString hotkeyName = QTStr("QuickTransitions.HotkeyName").arg(MakeQuickTransitionText(qt));

	obs_hotkey_set_description(qt->hotkey, QT_TO_UTF8(hotkeyName));
}
