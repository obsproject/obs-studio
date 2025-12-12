#pragma once
#include <QWidget>

class ScriptModule : public QWidget
{
    Q_OBJECT
      public:
    explicit ScriptModule(QWidget *parent = nullptr);
};

class LayerMixerModule : public QWidget
{
    Q_OBJECT
      public:
    explicit LayerMixerModule(QWidget *parent = nullptr);
};

class MacroModule : public QWidget
{
    Q_OBJECT
      public:
    explicit MacroModule(QWidget *parent = nullptr);
};
