#include "mainwindow.h"
#include "ui_mainwindow.h"


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    QStringList sl;
    
    sl << "手机号" << "MsgID" << "消息类型"<< "数据包";
    ui->tableWidget->setColumnCount(4);
    ui->tableWidget->setRowCount(100);
    ui->tableWidget->setHorizontalHeaderLabels(sl);
    ui->tableWidget->setColumnWidth(3,1600);


}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::addrow(string s1, string s2,string s3)
{
    static int t=0;
    ui->tableWidget->setItem(t,0,new QTableWidgetItem(s1.c_str()));
    ui->tableWidget->setItem(t,1,new QTableWidgetItem(s2.c_str()));
    ui->tableWidget->setItem(t,3,new QTableWidgetItem(s3.c_str()));
    t++;
}

void MainWindow::on_action_2_triggered()
{

}
