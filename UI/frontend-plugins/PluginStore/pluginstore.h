#ifndef PLUGINSTORE_H
#define PLUGINSTORE_H

#include <QDialog>
#include <memory>
#include <vector>
#include <string>

#include "ui_pluginstore.h"

class PluginStore : public QDialog
{
    Q_OBJECT

public:
    std::unique_ptr<Ui_PluginStore> ui;
     bool loading = true;
    explicit PluginStore(QWidget *parent = 0);
    ~PluginStore();
	void closeEvent(QCloseEvent *event) override;
private slots:
	void on_close_clicked();
private:

};

#endif // PLUGINSTORE_H
