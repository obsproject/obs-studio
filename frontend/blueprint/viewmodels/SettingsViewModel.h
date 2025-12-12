#pragma once

#include <QObject>
#include <QVariantMap>

class SettingsViewModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantMap generalSettings READ generalSettings NOTIFY generalSettingsChanged)
    Q_PROPERTY(QVariantMap streamSettings READ streamSettings NOTIFY streamSettingsChanged)
    Q_PROPERTY(QVariantMap outputSettings READ outputSettings NOTIFY outputSettingsChanged)
    Q_PROPERTY(QVariantMap audioSettings READ audioSettings NOTIFY audioSettingsChanged)
    Q_PROPERTY(QVariantMap videoSettings READ videoSettings NOTIFY videoSettingsChanged)

      public:
    explicit SettingsViewModel(QObject *parent = nullptr);
    ~SettingsViewModel();

    QVariantMap generalSettings() const
    {
        return m_general;
    }
    QVariantMap streamSettings() const
    {
        return m_stream;
    }
    QVariantMap outputSettings() const
    {
        return m_output;
    }
    QVariantMap audioSettings() const
    {
        return m_audio;
    }
    QVariantMap videoSettings() const
    {
        return m_video;
    }

    Q_INVOKABLE void updateSetting(const QString &category, const QString &key, const QVariant &value);
    Q_INVOKABLE void applyChanges();
    Q_INVOKABLE void reset();

      signals:
    void generalSettingsChanged();
    void streamSettingsChanged();
    void outputSettingsChanged();
    void audioSettingsChanged();
    void videoSettingsChanged();

      private:
    void loadDefaults();

    QVariantMap m_general;
    QVariantMap m_stream;
    QVariantMap m_output;
    QVariantMap m_audio;
    QVariantMap m_video;
};
