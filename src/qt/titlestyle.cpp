#include "titlestyle.h"
void setTitleStyle(QWidget* titlewidget,QLabel*titlelabel,QPushButton*backbutton )
{
    QStackedLayout* titlelayout=new QStackedLayout();
    if(backbutton)
    {
        titlelayout->addWidget(backbutton);
        backbutton->setFlat(true);
    }
    titlelayout->addWidget(titlelabel);

    titlelabel->setFixedHeight(49);
    titlelabel->setAlignment(Qt::AlignCenter);
    titlewidget->setFixedHeight(49);
    QFont font ("Microsoft YaHei",16, QFont::Normal );
    titlelabel->setFont(font);
    titlelabel->setAttribute(Qt::WA_TranslucentBackground);
    //titlelabel->setStyleSheet("QLabel{background-color:rgb(44,185,126)}");
    titlewidget->setStyleSheet("QWidget{background-color:rgb(44,185,126)}");
    if(backbutton)
    {
        backbutton->setText("");
        backbutton->setStyleSheet("QPushButton{border:none;background-image: url(:/res/png/back.png);border-image: url(:/res/png/back.png);}");
        backbutton->setFixedSize(49,49);
        backbutton->setFocusPolicy(Qt::NoFocus);
    }
    titlelayout->setStackingMode(QStackedLayout::StackAll);
    titlewidget->setLayout(titlelayout);

}
