#pragma once

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>

class MonitorWidget : public QWidget
{
    Q_OBJECT
      public:
    explicit MonitorWidget(const QString &title, QWidget *parent = nullptr);
    void setStatus(const QString &status);

      protected:
    void paintEvent(QPaintEvent *event) override;

      private:
    QString m_title;
    QLabel *m_statusLabel;
};
