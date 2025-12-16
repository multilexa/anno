#pragma once
#include "LabelDefinition.h"
#include "ui_LabelDefinitionPropertiesWidget.h"
#include <QWidget>

class LabelDefinitionPropertiesWidget : public QWidget
{
    Q_OBJECT

public:
    LabelDefinitionPropertiesWidget(QWidget *parent=nullptr);
    ~LabelDefinitionPropertiesWidget();

    std::shared_ptr<LabelDefinition> GetSelectedDefinition() { return definition_; }
    std::shared_ptr<LabelCategory> GetSelectedCategory() { return category_; }

public slots:
    void Select(std::shared_ptr<LabelDefinition>, std::shared_ptr<LabelCategory>);

private slots:
    void OnDescriptionTextChanged();
    void OnRenderingScriptTextChanged();
    void OnChangeCategoryValue();
    void OnChangeCategoryColor();
    void ShowAddCodeLineMenu();
    void AddCodeLine();

private:
    void UpdateCategoryData();

private:
    Ui::LabelDefinitionPropertiesWidget ui;
    std::shared_ptr<LabelDefinition> definition_;
    std::shared_ptr<LabelCategory> category_;
};
