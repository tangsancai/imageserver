#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_searchid_clicked();

    void on_pathid_cursorPositionChanged(int arg1, int arg2);

    void on_pathid_returnPressed();

    void on_serverid_cursorPositionChanged(int arg1, int arg2);

    void on_serverid_returnPressed();

    void on_pre_clicked();

    void on_post_clicked();

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
