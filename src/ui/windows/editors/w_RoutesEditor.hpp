#pragma once

#include "QvPlugin/Common/CommonTypes.hpp"
#include "MessageBus/MessageBus.hpp"
#include "ui/WidgetUIBase.hpp"
#include "ui_w_RoutesEditor.h"

enum class NodeItemType;

class NodeDispatcher;
class ChainEditorWidget;
class RoutingEditorWidget;
class DnsSettingsWidget;

namespace QtNodes
{
    class Node;
    class FlowView;
    class FlowScene;
} // namespace QtNodes

class RouteEditor
    : public QvDialog
    , private Ui::RouteEditor
{
    Q_OBJECT

  public:
    explicit RouteEditor(const ProfileContent &connection, QWidget *parent = nullptr);
    ~RouteEditor();
    ProfileContent OpenEditor();
    void processCommands(QString, QStringList, QMap<QString, QString>) override{};

  private:
    void updateColorScheme() override;
    QvMessageBusSlotDecl override;

  private slots:
    void on_addBalancerBtn_clicked();
    void on_addChainBtn_clicked();
    void on_addDefaultBtn_clicked();
    void on_addInboundBtn_clicked();
    void on_addOutboundBtn_clicked();
    void on_debugPainterCB_clicked(bool checked);
    void on_defaultOutboundCombo_currentTextChanged(const QString &arg1);
    void on_domainStrategyCombo_currentIndexChanged(int arg1);
    void on_importExistingBtn_clicked();
    void on_importGroupBtn_currentIndexChanged(int index);
    void on_insertBlackBtn_clicked();
    void on_insertDirectBtn_clicked();
    void on_linkExistingBtn_clicked();
    void on_importOutboundBtn_clicked();

  private slots:
    void OnDispatcherEditChainRequested(const QString &);
    void OnDispatcherOutboundDeleted(const OutboundObject &);
    void OnDispatcherOutboundCreated(std::shared_ptr<OutboundObject>, QtNodes::Node &);
    void OnDispatcherRuleCreated(std::shared_ptr<RuleObject>, QtNodes::Node &);
    void OnDispatcherRuleDeleted(const RuleObject &);
    void OnDispatcherInboundOutboundHovered(const QString &, const IOBoundData &);
    void OnDispatcherObjectTagChanged(const NodeItemType &, const QString, const QString);

  private:
    QString defaultOutboundTag;
    std::shared_ptr<NodeDispatcher> nodeDispatcher;
    ChainEditorWidget *chainWidget;
    RoutingEditorWidget *ruleWidget;
    DnsSettingsWidget *dnsWidget;
    //
    bool isLoading = false;
    QString domainStrategy;
    //
    ProfileContent root;
    ProfileContent original;
};
