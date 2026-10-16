#ifndef CATABLE_RESIZE_RUNTIME_MIXIN_H
#define CATABLE_RESIZE_RUNTIME_MIXIN_H

#include <QFont>
#include <QHeaderView>
#include <QTableWidget>

class caTableResizeRuntime
{
public:
    virtual ~caTableResizeRuntime() = default;
    virtual void resizeTablePresentation(qreal fontSize, int rowHeight) = 0;
};

template <typename Widget>
class caTableResizeRuntimeMixin : public caTableResizeRuntime
{
public:
    void resizeTablePresentation(qreal fontSize, int rowHeight) override
    {
        Widget *table = static_cast<Widget *>(this);
        QFont font = table->font();
        font.setPointSize(qRound(fontSize));

        table->setUpdatesEnabled(false);
        for (int i = 0; i < table->rowCount(); ++i) {
            for (int j = 0; j < table->columnCount(); ++j) {
                QTableWidgetItem *selectedItem = table->item(i, j);
                if (selectedItem != Q_NULLPTR) selectedItem->setFont(font);
            }
        }
        table->setValueFont(font);
        table->verticalHeader()->setDefaultSectionSize(rowHeight);

        QString styleSheet("QHeaderView { font-size: %1pt; }");
        styleSheet = styleSheet.arg(qRound(fontSize));
        table->horizontalHeader()->setStyleSheet(styleSheet);
        table->verticalHeader()->setStyleSheet(styleSheet);
        table->setUpdatesEnabled(true);
    }
};

#endif // CATABLE_RESIZE_RUNTIME_MIXIN_H
