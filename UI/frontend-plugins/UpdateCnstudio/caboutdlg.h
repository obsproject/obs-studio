#ifndef CABOUTDLG_H
#define CABOUTDLG_H

#include <QDialog>
#include <memory>
#include <vector>
#include <string>
#include"ui_caboutdlg.h"

class CAboutDlg : public QDialog
{
    Q_OBJECT
	
public:
	std::unique_ptr<Ui_CAboutDlg> ui;
    explicit CAboutDlg(QWidget *parent = 0);
    ~CAboutDlg();
	void closeEvent(QCloseEvent *event) override;
private slots:
    void on_pushButton_clicked();

private:
 
};

#endif // CABOUTDLG_H
