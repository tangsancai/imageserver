/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 5.2.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralWidget;
    QPushButton *searchid;
    QLineEdit *pathid;
    QLineEdit *serverid;
    QWidget *widget;
    QLabel *imagelabel;
    QWidget *widget_2;
    QLineEdit *imagename;
    QPushButton *pre;
    QPushButton *post;
    QMenuBar *menuBar;
    QToolBar *mainToolBar;
    QStatusBar *statusBar;
    QToolBar *toolBar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QStringLiteral("MainWindow"));
        MainWindow->resize(645, 518);
        centralWidget = new QWidget(MainWindow);
        centralWidget->setObjectName(QStringLiteral("centralWidget"));
        searchid = new QPushButton(centralWidget);
        searchid->setObjectName(QStringLiteral("searchid"));
        searchid->setGeometry(QRect(390, 20, 99, 27));
        searchid->setContextMenuPolicy(Qt::DefaultContextMenu);
        pathid = new QLineEdit(centralWidget);
        pathid->setObjectName(QStringLiteral("pathid"));
        pathid->setGeometry(QRect(10, 20, 371, 27));
        serverid = new QLineEdit(centralWidget);
        serverid->setObjectName(QStringLiteral("serverid"));
        serverid->setGeometry(QRect(550, 20, 81, 27));
        widget = new QWidget(centralWidget);
        widget->setObjectName(QStringLiteral("widget"));
        widget->setGeometry(QRect(90, 70, 461, 341));
        imagelabel = new QLabel(widget);
        imagelabel->setObjectName(QStringLiteral("imagelabel"));
        imagelabel->setGeometry(QRect(10, 10, 441, 321));
        imagelabel->setTextFormat(Qt::AutoText);
        imagelabel->setScaledContents(false);
        widget_2 = new QWidget(centralWidget);
        widget_2->setObjectName(QStringLiteral("widget_2"));
        widget_2->setGeometry(QRect(90, 420, 461, 31));
        imagename = new QLineEdit(widget_2);
        imagename->setObjectName(QStringLiteral("imagename"));
        imagename->setGeometry(QRect(100, 0, 261, 27));
        imagename->setAlignment(Qt::AlignCenter);
        pre = new QPushButton(centralWidget);
        pre->setObjectName(QStringLiteral("pre"));
        pre->setGeometry(QRect(20, 220, 51, 51));
        post = new QPushButton(centralWidget);
        post->setObjectName(QStringLiteral("post"));
        post->setGeometry(QRect(570, 220, 51, 51));
        MainWindow->setCentralWidget(centralWidget);
        menuBar = new QMenuBar(MainWindow);
        menuBar->setObjectName(QStringLiteral("menuBar"));
        menuBar->setGeometry(QRect(0, 0, 645, 25));
        MainWindow->setMenuBar(menuBar);
        mainToolBar = new QToolBar(MainWindow);
        mainToolBar->setObjectName(QStringLiteral("mainToolBar"));
        MainWindow->addToolBar(Qt::TopToolBarArea, mainToolBar);
        statusBar = new QStatusBar(MainWindow);
        statusBar->setObjectName(QStringLiteral("statusBar"));
        MainWindow->setStatusBar(statusBar);
        toolBar = new QToolBar(MainWindow);
        toolBar->setObjectName(QStringLiteral("toolBar"));
        MainWindow->addToolBar(Qt::TopToolBarArea, toolBar);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QApplication::translate("MainWindow", "MainWindow", 0));
        searchid->setText(QApplication::translate("MainWindow", "Search", 0));
        pathid->setText(QApplication::translate("MainWindow", "input picture path here ...", 0));
        serverid->setText(QApplication::translate("MainWindow", "address", 0));
        imagelabel->setText(QApplication::translate("MainWindow", "similar pictures will input here. press left to pre & right to poster", 0));
        imagename->setText(QApplication::translate("MainWindow", "similar pictures' name will input here", 0));
        pre->setText(QApplication::translate("MainWindow", "pre", 0));
        post->setText(QApplication::translate("MainWindow", "post", 0));
        toolBar->setWindowTitle(QApplication::translate("MainWindow", "toolBar", 0));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
