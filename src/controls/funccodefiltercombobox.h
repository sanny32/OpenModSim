#ifndef FUNCCODEFILTERCOMBOBOX_H
#define FUNCCODEFILTERCOMBOBOX_H

#include <QComboBox>

///
/// \brief The FuncCodeFilterComboBox class
///
class FuncCodeFilterComboBox : public QComboBox
{
    Q_OBJECT

public:
    explicit FuncCodeFilterComboBox(QWidget* parent = nullptr);

    int currentFunctionCode() const;
    void setCurrentFunctionCode(int functionCode);

signals:
    void functionCodeChanged(int value);

protected:
    void changeEvent(QEvent* event) override;

private slots:
    void on_currentIndexChanged(int index);

private:
    void retranslateItems();
};

#endif // FUNCCODEFILTERCOMBOBOX_H
