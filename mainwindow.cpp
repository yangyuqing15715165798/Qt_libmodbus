#include "mainwindow.h"
#include "ui_mainwindow.h"

// 添加配置参数
const int TIMER = 1; // 读取间隔时间(秒)
const int SERVER_ADDRESS = 1;
const int START_ADDRESS = 301;
const int QUANTITY = 100;
const char *COM_PORT = "COM8";
const int BAUD_RATE = 9600;

// ModbusThread implementation
ModbusThread::ModbusThread(QObject *parent) : QThread(parent), modbusCtx(nullptr), stopped(false) {
}

ModbusThread::~ModbusThread() {
    stopModbusOperation();
}

void ModbusThread::run() {
    // 使用之前添加的配置参数
    modbusCtx = modbus_new_rtu(COM_PORT, BAUD_RATE, 'N', 8, 1);
    if (!modbusCtx || modbus_connect(modbusCtx) == -1) {
        emit modbusError("Unable to connect to modbus");
        return;
    }
    modbus_set_slave(modbusCtx, SERVER_ADDRESS);

    uint16_t tab_reg[QUANTITY];

    while (!stopped) {
        int rc = modbus_read_registers(modbusCtx, START_ADDRESS, QUANTITY, tab_reg);
        if (rc != QUANTITY) {
            emit modbusError(modbus_strerror(errno));
        } else {
            QString dataStr;
            for (int i = 0; i < rc; i++) {
                dataStr += QString::number(tab_reg[i]) + " ";
            }
            emit newDataReceived(dataStr);
        }
        QThread::sleep(TIMER);
    }

    if (modbusCtx) {
        modbus_close(modbusCtx);
        modbus_free(modbusCtx);
        modbusCtx = nullptr;
    }
}

void ModbusThread::stopModbusOperation() {
    stopped = true;
    if (isRunning()) {
        wait();
    }
}

// MainWindow implementation
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , modbusWorkerThread(nullptr) {

    ui->setupUi(this);
// 图表和序列的初始化
    chart = new QChart();
    series = new QLineSeries(this);
    chart->addSeries(series); // 将序列添加到图表中
    chart->legend()->hide();  // 如果不需要图例，可以隐藏

    // 创建用于数值显示的x轴
    axisX = new QValueAxis;
    axisX->setTitleText("Data Points");
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    // 创建用于数值显示的y轴
    axisY = new QValueAxis;
    axisY->setLabelFormat("%i");
    axisY->setTitleText("Values");
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    // 将图表设置到QChartView控件
    ui->chartView->setChart(chart);
    ui->chartView->setRenderHint(QPainter::Antialiasing);

    connect(ui->startButton, &QPushButton::clicked, this, &MainWindow::onStartButtonClicked);
    connect(ui->stopButton, &QPushButton::clicked, this, &MainWindow::onStopButtonClicked);
}

MainWindow::~MainWindow() {
    if (modbusWorkerThread && modbusWorkerThread->isRunning()) {
        modbusWorkerThread->stopModbusOperation();
        delete modbusWorkerThread;
    }
    delete ui;
}

void MainWindow::onStartButtonClicked() {
    if (!modbusWorkerThread) {
        modbusWorkerThread = new ModbusThread(this);
        connect(modbusWorkerThread, &ModbusThread::newDataReceived, this, &MainWindow::onNewDataReceived);
        connect(modbusWorkerThread, &ModbusThread::modbusError, this, &MainWindow::onModbusError);
        modbusWorkerThread->start(); // Start thread and begin modbus operations
    } else {
        // Thread is already running; maybe show an error or do anything else
    }
}

void MainWindow::onStopButtonClicked() {
    if (modbusWorkerThread) {
        modbusWorkerThread->stopModbusOperation(); // Stop modbus operations and thread
    }
}


void MainWindow::onNewDataReceived(const QString &data) {
    ui->textBrowser->append(data); // Display new data on the text browser
    QStringList values = data.split(" ", QString::SkipEmptyParts);
    if (values.size() != QUANTITY) {
        // 数据数量不正确
//        emit modbusError("Received data count does not match expected quantity.");
        return;
    }

    int count = series->count();
    for (int i = 0; i < values.size(); ++i) {
        bool ok;
        qreal yValue = values[i].toDouble(&ok);
        if(ok) {
            series->append(count + i, yValue); // 将count+i作为X轴值
        }
    }

    // 更新X轴范围，使其显示最新的100个数据点（可调整）
    int dataCount = series->count();
    if (dataCount > 100) {
        axisX->setRange(dataCount - 100, dataCount);
    } else {
        axisX->setRange(0, dataCount);
    }

    // 更新Y轴范围（可以根据实际需求调整）
    axisY->setRange(0, 500);


    // 更新图表显示
    chart->update();
    ui->chartView->repaint();
}


void MainWindow::onModbusError(QString error) {
    ui->textBrowser->append(error); // Display modbus error on the text browser
}
