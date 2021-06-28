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
    builtInSchemesMenu = new QMenu(this);

    {
        const auto act = new QAction(tr("Empty scheme"), builtInSchemesMenu);
        connect(act, &QAction::triggered, [this] { this->SetRouteConfig(RouteSchemeIO::EmptyScheme); });
        builtInSchemesMenu->addAction(act);
    }
    {
        const auto act = new QAction(tr("Empty scheme (no ads)"), builtInSchemesMenu);
        connect(act, &QAction::triggered, [this] { this->SetRouteConfig(RouteSchemeIO::NoAdsScheme); });
        builtInSchemesMenu->addAction(act);
    }

    builtInSchemeBtn->setMenu(builtInSchemesMenu);

    const auto sourceStringsDomain = GeositeReader::ReadGeoSiteFromFile(GlobalConfig->behaviorConfig->GeoSitePath);
    directDomainTxt = new AutoCompleteTextEdit(QStringLiteral("geosite"), sourceStringsDomain, this);
    proxyDomainTxt = new AutoCompleteTextEdit(QStringLiteral("geosite"), sourceStringsDomain, this);
    blockDomainTxt = new AutoCompleteTextEdit(QStringLiteral("geosite"), sourceStringsDomain, this);

    const auto sourceStringsIP = GeositeReader::ReadGeoSiteFromFile(GlobalConfig->behaviorConfig->GeoIPPath);
    directIPTxt = new AutoCompleteTextEdit(QStringLiteral("geoip"), sourceStringsIP, this);
    proxyIPTxt = new AutoCompleteTextEdit(QStringLiteral("geoip"), sourceStringsIP, this);
    blockIPTxt = new AutoCompleteTextEdit(QStringLiteral("geoip"), sourceStringsIP, this);

    directTxtLayout->addWidget(directDomainTxt, 0, 0);
    proxyTxtLayout->addWidget(proxyDomainTxt, 0, 0);
    blockTxtLayout->addWidget(blockDomainTxt, 0, 0);

    directIPLayout->addWidget(directIPTxt, 0, 0);
    proxyIPLayout->addWidget(proxyIPTxt, 0, 0);
    blockIPLayout->addWidget(blockIPTxt, 0, 0);
}

void RouteSettingsMatrixWidget::SetRouteConfig(const Qv2ray::Models::RouteMatrixConfig &conf)
{
    domainStrategyCombo->setCurrentText(conf.domainStrategy);
    domainMatcherCombo->setCurrentIndex(conf.domainMatcher == QStringLiteral("mph") ? 1 : 0);
    //
    directDomainTxt->setPlainText(conf.domains->direct->join('\n'));
    proxyDomainTxt->setPlainText(conf.domains->proxy->join('\n'));
    blockDomainTxt->setPlainText(conf.domains->block->join('\n'));
    //
    blockIPTxt->setPlainText(conf.ips->block->join('\n'));
    directIPTxt->setPlainText(conf.ips->direct->join('\n'));
    proxyIPTxt->setPlainText(conf.ips->proxy->join('\n'));
}

Qv2ray::Models::RouteMatrixConfig RouteSettingsMatrixWidget::GetRouteConfig() const
{
    Qv2ray::Models::RouteMatrixConfig conf;
    // Workaround for translation
    const auto index = domainMatcherCombo->currentIndex();
    conf.domainMatcher->clear();
    if (index != 0)
        conf.domainMatcher = QStringLiteral("mph");
    conf.domainStrategy = domainStrategyCombo->currentText();
    conf.domains->block = SplitLines(blockDomainTxt->toPlainText());
    conf.domains->direct = SplitLines(directDomainTxt->toPlainText());
    conf.domains->proxy = SplitLines(proxyDomainTxt->toPlainText());
    conf.ips->block = SplitLines(blockIPTxt->toPlainText());
    conf.ips->direct = SplitLines(directIPTxt->toPlainText());
    conf.ips->proxy = SplitLines(proxyIPTxt->toPlainText());
    return conf;
}

RouteSettingsMatrixWidget::~RouteSettingsMatrixWidget()
{
}

void RouteSettingsMatrixWidget::on_importSchemeBtn_clicked()
{
    const auto filePath = this->openFileDialog();
    if (!filePath)
        return;

    RouteSchemeIO::Qv2rayRouteScheme scheme;
    scheme.loadJson(JsonFromString(ReadFile(*filePath)));

    const auto strPrompt = tr("Import scheme '%1' made by '%2'? \r\n Description: %3").arg(scheme.name, scheme.author, scheme.description);
    const auto decision = QvBaselib->Ask(tr("Importing Scheme"), strPrompt);
    if (decision != Qv2rayBase::MessageOpt::Yes)
        return;

    QvLog() << "Imported route config:" << scheme.name << "by:" << scheme.author;
    this->SetRouteConfig(scheme);
}

/**
 * @brief RouteSettingsMatrixWidget::on_exportSchemeBtn_clicked
 * @author DuckSoft <realducksoft@gmail.com>
 */
void RouteSettingsMatrixWidget::on_exportSchemeBtn_clicked()
{
    const auto config = this->GetRouteConfig();

    const auto dialogTitle = tr("Exporting Scheme");

    bool ok = false;
    const auto schemeName = QInputDialog::getText(this, dialogTitle, tr("Scheme name:"), QLineEdit::Normal, tr("Unnamed Scheme"), &ok);
    if (!ok)
        return;

    const auto schemeAuthor = QInputDialog::getText(this, dialogTitle, tr("Author:"), QLineEdit::Normal, QStringLiteral("Anonymous <mystery@example.com>"), &ok);
    if (!ok)
        return;

    const auto schemeDescription = QInputDialog::getText(this, dialogTitle, tr("Description:"), QLineEdit::Normal, tr("The author is too lazy to leave a comment"));
    if (!ok)
        return;

    const auto savePath = this->saveFileDialog();
    if (!savePath)
        return;

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
    return dialog.selectedFiles().value(0);
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
    return dialog.selectedFiles().value(0);
}
