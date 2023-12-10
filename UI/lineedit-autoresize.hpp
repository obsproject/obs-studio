#pragma once

#include <QTextEdit>
#include <QAbstractTextDocumentLayout>
#include <QKeyEvent>

class LineEditAutoResize : public QTextEdit {
	Q_OBJECT

	Q_PROPERTY(int maxLength READ maxLength WRITE setMaxLength)

public:
	LineEditAutoResize();
	int maxLength();
	void setMaxLength(int length);
	QString text();
	void setText(const QString &text);

private:
	int m_maxLength;

signals:
	void returnPressed();

private slots:
	void checkTextLength();
	void resizeVertically(const QSizeF &newSize);
	void SetPlainText(const QString &text);

protected:
	virtual void keyPressEvent(QKeyEvent *event) override;
};
