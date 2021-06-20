#pragma once

#include "MessageBus/MessageBus.hpp"
#include "QJsonModel/QJsonModel.hpp"
#include "ui_w_JsonEditor.h"

#include <QDialog>

class JsonEditor
    : public QDialog
    , private Ui::JsonEditor
{
    Q_OBJECT

  public:
    explicit JsonEditor(QJsonObject rootObject = {}, QWidget *parent = nullptr);
    ~JsonEditor();
    QJsonObject OpenEditor();

  private:
    QvMessageBusSlotDecl;

  private slots:
    void on_jsonEditor_textChanged();

    void on_formatJsonBtn_clicked();

    void on_removeCommentsBtn_clicked();

  private:
    QJsonModel model;
    QJsonObject original;
    QJsonObject final;
};
