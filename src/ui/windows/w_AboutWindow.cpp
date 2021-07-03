#include "w_AboutWindow.hpp"

#include "Qv2rayBase/Common/Utils.hpp"
#include "Qv2rayBase/Interfaces/IStorageProvider.hpp"
#include "Qv2rayBase/Qv2rayBaseLibrary.hpp"
#include "QvPlugin/Common/QvPluginBase.hpp"
#include "ui_w_AboutWindow.h"

AboutWindow::AboutWindow(QWidget *parent) : QDialog(parent), ui(new Ui::w_AboutWindow)
{
    ui->setupUi(this);
    ui->configdirLabel->setText(QvBaselib->StorageProvider()->StorageLocation());
    ui->qvVersion->setText(QStringLiteral(QV2RAY_VERSION_STRING));
    ui->qvBuildInfo->setText(QStringLiteral(QV2RAY_BUILD_INFO));
    ui->qvBuildExInfo->setText(QStringLiteral(QV2RAY_BUILD_EXTRA_INFO));
    ui->qvPluginInterfaceVersionLabel->setText(QString::number(Qv2rayPlugin::QV2RAY_PLUGIN_INTERFACE_VERSION));
}

AboutWindow::~AboutWindow()
{
    delete ui;
}

void AboutWindow::on_openConfigDirCB_clicked()
{
    QvBaselib->OpenURL(QvBaselib->StorageProvider()->StorageLocation());
}
