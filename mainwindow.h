#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <memory>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void updateMeshTriangles(int numTri);
    void updateFrames(int numFrames);
    void animPresent(bool val);
    void stateBBoxCheck(bool val);

private:
    std::unique_ptr<Ui::MainWindow> ui;
};

#endif // MAINWINDOW_H
