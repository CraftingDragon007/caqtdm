#ifndef CASIMPLEFONTRESIZERUNTIME_H
#define CASIMPLEFONTRESIZERUNTIME_H

#include <QFont>
#include <QLabel>
#include <QString>
#include <QTextCursor>
#include <QTextEdit>
#include <QWidget>

// Owns simple text/control font presentation used by runtime display resizing.
// The library retains font-size calculation and resize timing.
class caSimpleFontResizeRuntime
{
public:
    static bool isLabel(const QString &className)
    {
        return className == QLatin1String("QLabel");
    }

    static bool isLabelEligible(QWidget *widget)
    {
        QLabel *label = static_cast<QLabel *>(widget);
        const QString parentClassName = label->parent()->metaObject()->className();
        return !parentClassName.contains("Numeric") &&
               !parentClassName.contains("caSpinbox");
    }

    static bool isSimpleTextControl(const QString &className)
    {
        return className == QLatin1String("caMenu") ||
               className == QLatin1String("QPlainTextEdit") ||
               className == QLatin1String("QTextEdit") ||
               className == QLatin1String("QLineEdit") ||
               className == QLatin1String("QRadioButton") ||
               className == QLatin1String("QComboBox") ||
               className == QLatin1String("QCheckBox");
    }

    static void resize(const QString &className, QWidget *widget, qreal fontSize)
    {
        QFont font = widget->font();
        font.setPointSize(qRound(fontSize));

        // Keep the original select-all, document-size, cursor-restore sequence.
        if (className == QLatin1String("QTextEdit")) {
            QTextEdit *textEdit = static_cast<QTextEdit *>(widget);
            QTextCursor cursor = textEdit->textCursor();
            textEdit->selectAll();
            textEdit->setFontPointSize(fontSize);
            textEdit->setTextCursor(cursor);
        }

        widget->setFont(font);
    }
};

#endif // CASIMPLEFONTRESIZERUNTIME_H
