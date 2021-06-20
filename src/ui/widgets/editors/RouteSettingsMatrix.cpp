#include "RouteSettingsMatrix.hpp"

#include "Common/Utils.hpp"
#include "GeositeReader/GeositeReader.hpp"
#include "ui/WidgetUIBase.hpp"

#include <QFileDialog>
#include <QInputDialog>

#define QV_MODULE_NAME "RouteSettingsMatrix"

RouteSettingsMatrixWidget::RouteSettingsMatrixWidget(QWidget *parent) : QWidget(parent)
{
    setupUi(this);
    //
    builtInSchemesMenu = new QMenu(this);
    builtInSchemesMenu->addActions(this->getBuiltInSchemes());
    builtInSchemeBtn->setMenu(builtInSchemesMenu);
    //
    auto sourceStringsDomain = GeositeReader::ReadGeoSiteFromFile(GlobalConfig->behaviorConfig->GeoSitePath);
    directDomainTxt = new AutoCompleteTextEdit("geosite", sourceStringsDomain, this);
    proxyDomainTxt = new AutoCompleteTextEdit("geosite", sourceStringsDomain, this);
    blockDomainTxt = new AutoCompleteTextEdit("geosite", sourceStringsDomain, this);
    //
    auto sourceStringsIP = GeositeReader::ReadGeoSiteFromFile(GlobalConfig->behaviorConfig->GeoIPPath);
    directIPTxt = new AutoCompleteTextEdit("geoip", sourceStringsIP, this);
    proxyIPTxt = new AutoCompleteTextEdit("geoip", sourceStringsIP, this);
    blockIPTxt = new AutoCompleteTextEdit("geoip", sourceStringsIP, this);
    //
    directTxtLayout->addWidget(directDomainTxt, 0, 0);
    proxyTxtLayout->addWidget(proxyDomainTxt, 0, 0);
    blockTxtLayout->addWidget(blockDomainTxt, 0, 0);
    //
    directIPLayout->addWidget(directIPTxt, 0, 0);
    proxyIPLayout->addWidget(proxyIPTxt, 0, 0);
    blockIPLayout->addWidget(blockIPTxt, 0, 0);
}

/**
 * @brief RouteSettingsMatrixWidget::getBuiltInSchemes
 * @return
 */
QList<QAction *> RouteSettingsMatrixWidget::getBuiltInSchemes()
{
    QList<QAction *> list;
    list.append(this->schemeToAction(tr("empty scheme"), RouteSchemeIO::emptyScheme));
    list.append(this->schemeToAction(tr("empty scheme (no ads)"), RouteSchemeIO::noAdsScheme));
    return list;
}

QAction *RouteSettingsMatrixWidget::schemeToAction(const QString &name, const Qv2ray::Models::RouteMatrixConfig &scheme)
{
    QAction *action = new QAction(this);
    action->setText(name);
    connect(action, &QAction::triggered, [this, &scheme] { this->SetRouteConfig(scheme); });
    return action;
}

void RouteSettingsMatrixWidget::SetRouteConfig(const Qv2ray::Models::RouteMatrixConfig &conf)
{
    domainStrategyCombo->setCurrentText(conf.domainStrategy);
    domainMatcherCombo->setCurrentIndex(conf.domainMatcher == QStringLiteral("mph") ? 1 : 0);
    //
    directDomainTxt->setPlainText(conf.domains->direct->join(NEWLINE));
    proxyDomainTxt->setPlainText(conf.domains->proxy->join(NEWLINE));
    blockDomainTxt->setPlainText(conf.domains->block->join(NEWLINE));
    //
    blockIPTxt->setPlainText(conf.ips->block->join(NEWLINE));
    directIPTxt->setPlainText(conf.ips->direct->join(NEWLINE));
    proxyIPTxt->setPlainText(conf.ips->proxy->join(NEWLINE));
}

Qv2ray::Models::RouteMatrixConfig RouteSettingsMatrixWidget::GetRouteConfig() const
{
    Qv2ray::Models::RouteMatrixConfig conf;
    // Workaround for translation
    const auto index = domainMatcherCombo->currentIndex();
    conf.domainMatcher = index == 0 ? "" : "mph";
    conf.domainStrategy = domainStrategyCombo->currentText();
    conf.domains->block = SplitLines(blockDomainTxt->toPlainText().replace(" ", ""));
    conf.domains->direct = SplitLines(directDomainTxt->toPlainText().replace(" ", ""));
    conf.domains->proxy = SplitLines(proxyDomainTxt->toPlainText().replace(" ", ""));
    //
    conf.ips->block = SplitLines(blockIPTxt->toPlainText().replace(" ", ""));
    conf.ips->direct = SplitLines(directIPTxt->toPlainText().replace(" ", ""));
    conf.ips->proxy = SplitLines(proxyIPTxt->toPlainText().replace(" ", ""));
    return conf;
}

RouteSettingsMatrixWidget::~RouteSettingsMatrixWidget()
{
}

/**
 * @brief RouteSettingsMatrixWidget::on_importSchemeBtn_clicked
 * @author DuckSoft <realducksoft@gmail.com>
 * @todo add some debug output
 */
void RouteSettingsMatrixWidget::on_importSchemeBtn_clicked()
{
    // open up the file dialog and choose a file.
    auto filePath = this->openFileDialog();
    if (!filePath)
        return;

    // read the file and parse back to struct.
    // if error occurred on parsing, an exception will be thrown.
    auto content = ReadFile(*filePath);
    RouteSchemeIO::Qv2rayRouteScheme scheme;
    scheme.loadJson(JsonFromString(content));

    // show the information of this scheme to user,
    // and ask user if he/she wants to import and apply this.
    auto strPrompt = tr("Import scheme '%1' made by '%2'? \r\n Description: %3").arg(scheme.name, scheme.author, scheme.description);
    auto decision = QvBaselib->Ask(tr("Importing Scheme"), strPrompt);

    // if user don't want to import, just leave.
    if (decision != Qv2rayBase::MessageOpt::Yes)
        return;

    // write the scheme onto the window
    this->SetRouteConfig(scheme);

    // done
    QvLog() << "Imported route config:" << scheme.name << "by:" << scheme.author;
}

/**
 * @brief RouteSettingsMatrixWidget::on_exportSchemeBtn_clicked
 * @author DuckSoft <realducksoft@gmail.com>
 */
void RouteSettingsMatrixWidget::on_exportSchemeBtn_clicked()
{
    // parse the config back from the window components
    auto config = this->GetRouteConfig();

    // init some constants
    const auto dialogTitle = tr("Exporting Scheme");

    // scheme name?
    bool ok = false;
    auto schemeName = QInputDialog::getText(this, dialogTitle, tr("Scheme name:"), QLineEdit::Normal, tr("Unnamed Scheme"), &ok);
    if (!ok)
        return;

    // scheme author?
    auto schemeAuthor = QInputDialog::getText(this, dialogTitle, tr("Author:"), QLineEdit::Normal, "Anonymous <mystery@example.com>", &ok);
    if (!ok)
        return;

    // scheme description?
    auto schemeDescription = QInputDialog::getText(this, dialogTitle, tr("Description:"), QLineEdit::Normal, tr("The author is too lazy to leave a comment"));
    if (!ok)
        return;

    // where to save?
    auto savePath = this->saveFileDialog();
    if (!savePath)
        return;

    // construct the data structure
    RouteSchemeIO::Qv2rayRouteScheme scheme;
    scheme.name = schemeName;
    scheme.author = schemeAuthor;
    scheme.description = schemeDescription;
    scheme.domainStrategy = config.domainStrategy;
    scheme.ips = config.ips;
    scheme.domains = config.domains;

    // serialize and write out
    auto content = JsonToString(scheme.toJson());
    WriteFile(content.toUtf8(), *savePath);

    // done
    // TODO: Give some success as Notification
    QvBaselib->Info(dialogTitle, tr("Your route scheme has been successfully exported!"));
}

/**
 * @brief opens a save dialog and asks user to specify the save path.
 * @return the selected file path, if any
 * @author DuckSoft <realducksoft@gmail.com>
 */
std::optional<QString> RouteSettingsMatrixWidget::saveFileDialog()
{
    QFileDialog dialog;
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setOption(QFileDialog::Option::DontConfirmOverwrite, false);
    dialog.setNameFilter(tr("QvRoute Schemes(*.json)"));
    dialog.setAcceptMode(QFileDialog::AcceptMode::AcceptSave);
    if (!dialog.exec() || dialog.selectedFiles().length() != 1)
    {
        return std::nullopt;
    }
    return dialog.selectedFiles().first();
}

/**
 * @brief opens up a dialog and asks user to choose a scheme file.
 * @return the selected file path, if any
 * @author DuckSoft <realducksoft@gmail.com>
 */
std::optional<QString> RouteSettingsMatrixWidget::openFileDialog()
{
    QFileDialog dialog;
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setNameFilter(tr("QvRoute Schemes(*.json)"));
    if (!dialog.exec() || dialog.selectedFiles().length() != 1)
    {
        return std::nullopt;
    }
    return dialog.selectedFiles().first();
}
