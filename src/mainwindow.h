#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "blesimpledevice.h"
#include <QTimer>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void UpdateValues();

private:
    Ui::MainWindow *ui;
    BLESimpleDevice* glove = nullptr;
    QTimer timerUpdateValues;
};
#endif // MAINWINDOW_H
