#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QQueue>
#include <QSettings>
#include <QTime>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    readSettings();

    m_gload = new QGraphicsScene;
    ui->graphicsView->setScene(m_gload);
    ui->graphicsView->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);

    //feed random seed with a volatile data source
    qsrand(QTime(0,0,0).secsTo(QTime::currentTime()));

    connect(ui->pushButton, SIGNAL(clicked()), this, SLOT(simulate()));
}

MainWindow::~MainWindow()
{
    this->writeSettings();
    delete ui;
}

void MainWindow::print(const QString &s)
{
    ui->plainTextEdit->appendPlainText(s);
}

int MainWindow::random(int from, int to)
{
    return qAbs(qrand() % (to - from + 1) + from);
}

void MainWindow::readSettings()
{
    QSettings settings(this);
    ui->tasksCountBox->setValue(settings.value(QString("Modelling/tasksCount"), 100).toInt());
    ui->spinBox_2->setValue(settings.value(QString("Modelling/taskFrom"), 1).toInt());
    ui->spinBox_3->setValue(settings.value(QString("Modelling/taskTo"), 9).toInt());
    ui->tasksCountBox_2->setValue(settings.value(QString("Modelling/nodesCount"), 2).toInt());
    ui->spinBox_4->setValue(settings.value(QString("Modelling/procFrom"), 1).toInt());
    ui->spinBox_5->setValue(settings.value(QString("Modelling/procTo"), 5).toInt());
    ui->checkBox->setChecked(settings.value(QString("Modelling/waitForAll"), true).toBool());
    ui->randomLoadCheckBox->setChecked(settings.value(QString("Modelling/randomLoad"), false).toBool());
}

void MainWindow::simulate()
{
    int tasksCount = ui->tasksCountBox->value();   //input task number
    int taskFrom = ui->spinBox_2->value();         //minimal interval of task appearance
    int taskTo = ui->spinBox_3->value();           //maximal interval of task appearance
    int nodesCount = ui->tasksCountBox_2->value(); //computer number
    int procFrom = ui->spinBox_4->value();         //minimal time of processing a task by a computer
    int procTo = ui->spinBox_5->value();           //maximal time of processing a task by a computer
    bool waitForAll = ui->checkBox->isChecked();   //wait for all the computers to finish
    bool randomLoad = ui->randomLoadCheckBox->isChecked();

    if (taskFrom > taskTo) {
        print(tr("Invalid arrival interval"));
        return;
    }

    if (procFrom > procTo) {
        print(tr("Invalid processing interval"));
        return;
    }

    dc.clear();             //just in case
    for (int i = 0; i < nodesCount; ++i) {
        //initialize computer pool
        Node *n = new Node;
        n->nodeID = i;
        n->currProc = 0;
        n->pList.clear();
        dc.append(n);
    }

    QQueue<Task*> q;        //task queue is shared
    q.clear();
    int maxQueueLen = 0;

    int time = 0;                   //modelling time
    int tasksRemain = tasksCount;
    int nextArrivalTime = 0;

    int aveWait = 0;

    QList<Task*> serviced;
    serviced.clear();

    forever {
        if (tasksRemain > 0) {
            if (nextArrivalTime == time) {
                Task *t = new Task;
                t->taskID = tasksCount - tasksRemain;
                t->arrivalTime = time;
                q.append(t);

                --tasksRemain;
                nextArrivalTime += random(taskFrom, taskTo);
            }
        } else {
            if (!waitForAll) {
                --time;
                break;
            }
        }

        bool allFree = true;

        QList<Node*> free;
        free.clear();

        foreach (Node *n, dc) {
            if (n->currProc && (n->pList.last()->procEnd <= time)) {
                //look for the next free computer
                n->currProc = 0;
            }

            if (!n->currProc)
                free.append(n);
            else
                allFree = false;
        }

        while (!free.isEmpty() && !q.isEmpty()) {
            Node *n;
            if (randomLoad) {
                n = free.takeAt(this->random(0, free.length() - 1));
            } else {
                //load computers from "top" to "bottom"
                n = free.takeFirst();
            }

            q.head()->procBegin = time;
            q.head()->procEnd = time + this->random(procFrom, procTo);
            n->pList.append(q.head());
            n->currProc = q.head();

            aveWait += q.head()->procBegin - q.head()->arrivalTime;

            serviced.append(q.head());
            q.dequeue();

            allFree = false;
        }

        if ((tasksRemain <= 0) && q.isEmpty() && allFree) {
            break;
        }

        maxQueueLen = qMax(maxQueueLen, q.length());

        ++time;
    }

    //================================== REPORT =================================

    ui->plainTextEdit->clear();

    this->print(tr("Amount of computers: %1").arg(dc.length()));
    this->print(tr("Amount of tasks: %1").arg(tasksCount));
    this->print(tr(""));

    int totalLoadTime = 0;

    foreach (Node *n, dc) {
        this->print(tr("==== Computer %1 ====").arg(n->nodeID));
        if (n->currProc) {
            this->print(tr("Busy"));
        }

        int currLoadTime = 0;
        foreach (Task *p, n->pList) {
            currLoadTime += qMin(p->procEnd, time) - p->procBegin;
        }
        if (time != 0)
            this->print(tr("Load: %1\%").arg((float) qRound(currLoadTime*10000 / time)/100));
        totalLoadTime += currLoadTime;

        this->print("");
    }

    this->print(tr("Maximal queue length: %1").arg(maxQueueLen));
    this->print(tr("Current queue length: %1").arg(q.length()));
    this->print(tr("Total modelling time: %1").arg(time));
    this->print("");

    if (dc.length()*time != 0) {
        float f = 1 - (float) totalLoadTime / (dc.length()*time);
        f = (float) qRound(f*100)/100;
        this->print(tr("Idle probability: %1")
                    .arg(f));
    }

    foreach (Task *t, q) {
        aveWait += time - t->arrivalTime;
    }
    if (tasksCount > 0)
        this->print(tr("Average time spent by a task in a queue: %1").arg((float) qRound(aveWait*100 / tasksCount)/100));

    //================================= GRAPH =================================

    m_gload->clear();

    QFont font;
    QFontMetrics nfm(font);
    QFont fontb;
    fontb.setBold(true);
    QFontMetrics bfm(fontb);

    int x0 = nfm.width(tr("Computer #%1 ").arg(dc.length()));
    int fHeight = nfm.height();

    int dy = fHeight + 2;
    int dx = bfm.width(tr("  %1  ").arg(time));

    int maxx = 0;

    int y = 0;
    foreach (Node *n, dc) {
        m_gload->addSimpleText(tr("Computer #%1").arg(n->nodeID))->setPos(0, y);

        m_gload->addRect(
                QRectF(x0, y, (time)*dx, fHeight),
                QPen(Qt::DotLine), QBrush(QColor("White")));

        foreach (Task *p, n->pList) {
            QGraphicsRectItem *procRect = m_gload->addRect(QRectF(p->procBegin*dx + x0, y, (p->procEnd - p->procBegin)*dx, fHeight));
            procRect->setPen(QPen(Qt::SolidLine));

            QGraphicsSimpleTextItem *taskId = m_gload->addSimpleText(tr(" %1").arg(p->taskID));
            taskId->setPos(p->procBegin*dx + x0, y);

            if (p == n->currProc) { //(p->procEnd > time)
                procRect->setBrush(QBrush((n->nodeID%2) ? QColor("Yellow") : QColor("Gold")));
            } else {
                procRect->setBrush(QBrush((n->nodeID%2) ? QColor("Green") : QColor("Dark green")));
                taskId->setBrush(QBrush(QColor("White")));
            }
        }

        if (n->currProc)
            maxx = qMax(maxx, n->pList.last()->procEnd*dx + x0); //achtung code

        y += dy;
    }

    maxx = qMax(maxx, (time)*dx + x0); //achtung code

    for (int i = 0; i <= time; ++i) {
        QGraphicsSimpleTextItem *tick = m_gload->addSimpleText(tr(" %1 ").arg(i), fontb);
        tick->setPos(x0 + i*dx + (- nfm.width(tick->text()))/2, y);
    }
    y += dy;

    //render queue
    int ys = y;
    if (maxQueueLen > 0) {
        for (int r = time - 1*(waitForAll); r >= 0; --r) {
            //render one queue tick
            y = ys;
            m_gload->addRect(QRectF(x0 + r*dx + 1, y, dx - 2, dy*maxQueueLen),
                             QPen(Qt::DotLine), QBrush(QColor("Light gray")));

            foreach (Task *t, q) {
                m_gload->addRect(QRectF(x0 + r*dx + 1, y, dx - 2, dy),
                                 QPen(Qt::SolidLine),
                                 QBrush((t->arrivalTime == r) ? QColor("Red") : QColor("Dark red")));

                QGraphicsSimpleTextItem *taskId = m_gload->addSimpleText(tr("%1").arg(t->taskID));
                taskId->setPos(x0 + r*dx + (dx - nfm.width(taskId->text()))/2, y);
                taskId->setBrush(QBrush(QColor("White")));

                y += dy;
            }

            //event handling
            while (!serviced.isEmpty() && (serviced.last()->procBegin == r)) {
                //move a task back to queue
                Task *t = serviced.takeLast();
                q.prepend(t);
            }

            if (!q.isEmpty() && (q.last()->arrivalTime == r)) {
                //remove tasks that have not arrived yet
                q.removeLast();
            }
        }
    }

    m_gload->setSceneRect(QRect(-dy, -dy, maxx + 2*dy, ys + dy*maxQueueLen + 2*dy));
}

void MainWindow::writeSettings()
{
    QSettings settings(this);
    settings.setValue(QString("Modelling/tasksCount"), ui->tasksCountBox->value());
    settings.setValue(QString("Modelling/taskFrom"), ui->spinBox_2->value());
    settings.setValue(QString("Modelling/taskTo"), ui->spinBox_3->value());
    settings.setValue(QString("Modelling/nodesCount"), ui->tasksCountBox_2->value());
    settings.setValue(QString("Modelling/procFrom"), ui->spinBox_4->value());
    settings.setValue(QString("Modelling/procTo"), ui->spinBox_5->value());
    settings.setValue(QString("Modelling/waitForAll"), ui->checkBox->isChecked());
    settings.setValue(QString("Modelling/randomLoad"), ui->randomLoadCheckBox->isChecked());
}
