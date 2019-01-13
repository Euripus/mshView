#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "gl2widget.h"
#include <QSizePolicy>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(std::make_unique<Ui::MainWindow>())
{
    ui->setupUi(this);

    connect(ui->exitButton, &QPushButton::clicked, QApplication::instance(), &QApplication::quit);

    GL2Widget * glWindow = new GL2Widget();
    glWindow->setSizePolicy(QSizePolicy::Policy::MinimumExpanding, QSizePolicy::Policy::MinimumExpanding);
    ui->glWidgetLayout->addWidget(glWindow);

    connect(ui->loadMdlButton, &QPushButton::clicked, glWindow, &GL2Widget::loadMesh);
    connect(glWindow, &GL2Widget::numTriChanged, this, &MainWindow::updateMeshTriangles);
    connect(glWindow, &GL2Widget::anmPresent, this, &MainWindow::animPresent);

    connect(ui->loadAnmButton, &QPushButton::clicked, glWindow, &GL2Widget::loadAnimation);
    connect(glWindow, &GL2Widget::anmLoaded, this, &MainWindow::updateFrames);

    connect(ui->loadTextureButton, &QPushButton::clicked, glWindow, &GL2Widget::loadTexture);
    connect(ui->checkDrawBBox, &QCheckBox::stateChanged, glWindow, &GL2Widget::drawBBox);
    connect(glWindow, &GL2Widget::stateBBoxCheck, this, &MainWindow::stateBBoxCheck);
}

MainWindow::~MainWindow()
{
}

void MainWindow::updateMeshTriangles(int numTri)
{
    ui->triLabel->setText(QString("Triangles: %1")
                          .arg(numTri));
    ui->framesLabel->setText(QString("Frames: 0"));
}

void MainWindow::animPresent(bool val)
{
    ui->loadAnmButton->setEnabled(val);
}

void MainWindow::updateFrames(int numFrames)
{
    ui->framesLabel->setText(QString("Frames: %1")
                             .arg(numFrames));
}

void MainWindow::stateBBoxCheck(bool val)
{
    Qt::CheckState state = val ? Qt::Checked : Qt::Unchecked;
    ui->checkDrawBBox->setCheckState(state);
}
