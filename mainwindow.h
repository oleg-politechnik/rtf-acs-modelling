#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
class QGraphicsScene;

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

    typedef struct {
        int taskID;
        int arrivalTime;        //a moment of appearing of the task in a queue (model time units)
        int procBegin;          //a moment of the task processing start in a queue (model time units)
        int procEnd;            //a moment of the task processing end in a queue (model time units)
    } Task;

    typedef struct {
        int nodeID;
        QList<Task*> pList;  //list of processed tasks by a computer + current task
        Task* currProc;      //current task being processed by a computer
    } Node;

private slots:
    void simulate();

private:
    void readSettings();
    void writeSettings();

    Ui::MainWindow *ui;
    QList<Node*> dc;
    QGraphicsScene *m_gload;

    int random(int from, int to);
    void print(const QString &s);
};

#endif // MAINWINDOW_H
