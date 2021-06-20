#include "w_ImportConfig.hpp"

#include "Common/ProfileHelpers.hpp"
#include "Common/Utils.hpp"
#include "Plugin/PluginAPIHost.hpp"
#include "Profile/ProfileManager.hpp"
#include "ui/windows/editors/w_JsonEditor.hpp"

#include <QFileDialog>

#define QV_MODULE_NAME "ImportWindow"

constexpr auto LINK_PAGE = 0;
constexpr auto MANUAL_PAGE = 1;

ImportConfigWindow::ImportConfigWindow(QWidget *parent) : QvDialog("ImportWindow", parent)
{
    setupUi(this);
    QvMessageBusConnect();
    auto defaultItemIndex = 0;
    for (const auto &gid : QvBaselib->ProfileManager()->GetGroups())
    {
        groupCombo->addItem(GetDisplayName(gid), gid.toString());
        if (gid == DefaultGroupId)
            defaultItemIndex = groupCombo->count() - 1;
    }
    groupCombo->setCurrentIndex(defaultItemIndex);
    qrCodeTab->setVisible(false);
    tabWidget->removeTab(1);
}

void ImportConfigWindow::processCommands(QString command, QStringList commands, QMap<QString, QString> args)
{
    const static QMap<QString, int> indexMap{ { "link", 0 }, { "advanced", 1 } };
    nameTxt->setText(args["name"]);
    if (commands.isEmpty())
        return;
    if (command == "open")
    {
        const auto c = commands.takeFirst();
        tabWidget->setCurrentIndex(indexMap[c]);
    }
}

QvMessageBusSlotImpl(ImportConfigWindow)
{
    switch (msg)
    {
        MBShowDefaultImpl;
        MBHideDefaultImpl;
        MBRetranslateDefaultImpl;
        MBUpdateColorSchemeDefaultImpl;
    }
}

ImportConfigWindow::~ImportConfigWindow()
{
}

std::pair<GroupId, QMultiMap<QString, ProfileContent>> ImportConfigWindow::DoImportConnections()
{
    // partial import means only import as an outbound, will set outboundsOnly to
    // false and disable the checkbox
    this->exec();
    return result() == Accepted ? std::pair{ selectedGroup, connections } : std::pair{ NullGroupId, QMultiMap<QString, ProfileContent>{} };
}

void ImportConfigWindow::on_beginImportBtn_clicked()
{
    const auto aliasPrefix = nameTxt->text();

    switch (tabWidget->currentIndex())
    {
        case LINK_PAGE:
        {
            auto linkList = SplitLines(linkTxt->toPlainText());
            linkTxt->clear();
            QvLog() << linkList.count() << "entries found.";

            while (!linkList.isEmpty())
            {
                const auto link = linkList.takeFirst().trimmed();
                if (link.isEmpty() || link.startsWith("#") || link.startsWith("//"))
                    continue;

                QString errMessage;

                const auto optConf = QvBaselib->PluginAPIHost()->Outbound_Deserialize(link);

                if (!optConf)
                {
                    linkTxt->appendPlainText(link + NEWLINE);
                    continue;
                }

                auto name = aliasPrefix + optConf->first;
                const auto outbound = optConf->second;

                if (name.isEmpty())
                {
                    auto [protocol, host, port] = GetOutboundInfoTuple(outbound);
                    name = protocol + "/" + host + ":" + QString::number(port) + "-" + GenerateRandomString(5);
                }

                connections.insert(name, ProfileContent{ outbound });
            }
            break;
        }
        case MANUAL_PAGE:
        {
            const auto path = fileLineTxt->text();
            const auto config = ProfileContent::fromJson(JsonFromString(ReadFile(path)));
            connections.insert(aliasPrefix + QFileInfo(path).fileName(), config);
            break;
        }
    }

    accept();
}

void ImportConfigWindow::on_cancelImportBtn_clicked()
{
    reject();
}

void ImportConfigWindow::on_jsonEditBtn_clicked()
{
    JsonEditor w({}, this);
    const auto result = w.OpenEditor();
    const auto alias = nameTxt->text();
    const auto isChanged = w.result() == QDialog::Accepted;

    if (isChanged)
    {
        connections.insert(alias, ProfileContent::fromJson(result));
        accept();
    }
}
