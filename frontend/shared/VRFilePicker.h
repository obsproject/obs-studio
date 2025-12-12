#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QHBoxLayout>

class VRFilePicker : public QWidget
{
    Q_OBJECT
      public:
    explicit VRFilePicker(const QString &placeholder, QWidget *parent = nullptr);
    QString filePath() const;

      signals:
    void fileSelected(const QString &path);

      private:
    QLineEdit *m_lineEdit;
    QPushButton *m_browseBtn;
};
