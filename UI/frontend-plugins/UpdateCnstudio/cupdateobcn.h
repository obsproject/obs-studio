#ifndef CUPDATEOBCN_H
#define CUPDATEOBCN_H

#include <QDialog>
#include <memory>
#include <vector>
#include <string>
#include "ui_cupdateobcn.h"

class CUpdateOBCN : public QDialog
{
    Q_OBJECT

public:
	std::unique_ptr<Ui_CUpdateOBCN> ui;
    explicit CUpdateOBCN(QWidget *parent = 0);
    ~CUpdateOBCN();

private:
 
};

#endif // CUPDATEOBCN_H
