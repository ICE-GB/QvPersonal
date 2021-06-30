#pragma once

#include "Qv2rayBase/Plugin/PluginManagerCore.hpp"
#include "QvPlugin/Gui/QvGUIPluginInterface.hpp"
#include "ui/WidgetUIBase.hpp"
#include "ui_w_PluginManager.h"

#include <memory>

class PluginManageWindow
    : public QvDialog
    , private Ui::w_PluginManager
{
    Q_OBJECT

  public:
    explicit PluginManageWindow(QWidget *parent = nullptr);
    ~PluginManageWindow();
    void processCommands(QString command, QStringList commands, QMap<QString, QString>) override
    {
        if (commands.isEmpty())
            return;
        if (command == "open")
        {
            const auto c = commands.takeFirst();
            if (c == "plugindir")
                on_openPluginFolder_clicked();
            if (c == "metadata")
                tabWidget->setCurrentIndex(0);
            if (c == "settings")
                tabWidget->setCurrentIndex(1);
        }
    }
    QvMessageBusSlotDecl override;

  private slots:
    void on_pluginListWidget_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous);
    void on_pluginListWidget_itemClicked(QListWidgetItem *item);
    void on_pluginListWidget_itemChanged(QListWidgetItem *item);
    void on_pluginEditSettingsJsonBtn_clicked();
    void on_pluginListWidget_itemSelectionChanged();
    void on_openPluginFolder_clicked();
    void on_toolButton_clicked();

  private:
    void updateColorScheme() override{};

    std::shared_ptr<Qv2rayPlugin::Gui::PluginSettingsWidget> currentSettingsWidget;
    const Qv2rayBase::Plugin::PluginInfo *currentPluginInfo;
    QHash<PluginId, const Qv2rayBase::Plugin::PluginInfo *> plugins;
    bool isLoading = true;
};
