//Added by qt3to4:
#include <Q3VBoxLayout>
#include <Q3GridLayout>
#include <Q3HBoxLayout>
#include <QDialog>
#include <QLabel>
/*
For general Scribus (>=1.3.2) copyright and licensing information please refer
to the COPYING file provided with the program. Following this notice may exist
a copyright and/or license notice that predates the release of Scribus 1.3.2
for which a new license (GPL+exception) is in place.
*/
#ifndef DELCOLOR_H
#define DELCOLOR_H

class QLayout;
class QLabel;
class QPushbutton;
class ColorCombo;

#include <QDialog>
#include "scribusapi.h"
#include "sccolor.h"

class SCRIBUS_API DelColor : public QDialog
{ 
    Q_OBJECT

public:
    DelColor(QWidget* parent, ColorList colorList, QString colorName, bool haveDoc );
    ~DelColor() {};
	const QString getReplacementColor();

private:
    QLabel* deleteLabel;
    QLabel* colorToDelLabel;
    QLabel* replaceLabel;
    QPushButton* okButton;
    QPushButton* cancelButton;
    ColorCombo* replacementColData;
    Q3VBoxLayout* dialogLayout;
    Q3GridLayout* delColorLayout;
    Q3HBoxLayout* okCancelLayout;

    QString replacementColor;

private slots:
    virtual void ReplaceColor(int);
};

#endif // DELCOLOR_H
