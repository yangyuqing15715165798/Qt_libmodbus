#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>
#include <QTextBrowser>
#include <QPushButton>
#include <libmodbus/modbus.h>
#include <QtCharts>

using namespace QtCharts;
QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class ModbusThread : public QThread {
    Q_OBJECT
public:
    ModbusThread(QObject *parent = nullptr);
    ~ModbusThread();

    void run() override;
    void stopModbusOperation();

signals:
    void newDataReceived(QString data);
    void modbusError(QString error);

private:
    modbus_t *modbusCtx;
    volatile bool stopped;
};

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void onStartButtonClicked();
    void onStopButtonClicked();
    void onNewDataReceived(const QString &data);
    void onModbusError(QString error);

private:
    Ui::MainWindow *ui;
    ModbusThread *modbusWorkerThread;
    QChart *chart;
    QLineSeries *series;
    QValueAxis *axisX;  // 添加 X 轴
    QValueAxis *axisY;  // 添加 Y 轴
};

#endif // MAINWINDOW_H
