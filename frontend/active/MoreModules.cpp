#include "MoreModules.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QTextEdit>
#include <QListWidget>
#include "VRCommon.h"
#include "VRButton.h"
#include "VRSlider.h"

// --- ScriptModule ---
ScriptModule::ScriptModule(QWidget *parent) : QWidget(parent)
{
	auto layout = new QVBoxLayout(this);
	layout->addWidget(new QLabel("Script Editor (Python/Lua)", this));

	auto editor = new QTextEdit(this);
	editor->setPlaceholderText("import vr_api\n\ndef process_frame(frame):\n    pass");
	editor->setStyleSheet("background: #111; color: #0f0; font-family: Monospace;");
	layout->addWidget(editor);

	auto runBtn = new VRButton("Run Script", this);
	layout->addWidget(runBtn);
}

// --- LayerMixerModule ---
LayerMixerModule::LayerMixerModule(QWidget *parent) : QWidget(parent)
{
	auto layout = new QVBoxLayout(this);
	layout->addWidget(new QLabel("Layer Mixer", this));

	for (int i = 1; i <= 3; i++) {
		layout->addWidget(new QLabel(QString("Layer %1 Opacity").arg(i), this));
		layout->addWidget(new VRSlider(Qt::Horizontal, this));
	}

	layout->addStretch();
}

// --- MacroModule ---
MacroModule::MacroModule(QWidget *parent) : QWidget(parent)
{
	auto layout = new QVBoxLayout(this);
	layout->addWidget(new QLabel("Macros", this));

	auto list = new QListWidget(this);
	list->addItem("Scene Switch A->B");
	list->addItem("Reset All sources");
	list->addItem("Start Stream");
	list->setStyleSheet("background: #222; color: #fff;");
	layout->addWidget(list);

	layout->addWidget(new VRButton("Record New Macro", this));
}
