#pragma once

#include <QWidget>
#include <QSlider>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>

class AudioMixer : public QWidget
{
    Q_OBJECT
      public:
    explicit AudioMixer(QWidget *parent = nullptr);
    void addChannel(const QString &name, int volume);

      private:
    QVBoxLayout *m_layout;
};

class AudioChannelWidget : public QWidget
{
    Q_OBJECT
      public:
    AudioChannelWidget(const QString &name, int volume, QWidget *parent = nullptr);

      private:
    QSlider *m_slider;
    QLabel *m_label;
    QLabel *m_valLabel;
};
