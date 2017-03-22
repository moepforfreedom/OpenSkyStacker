#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "processingdialog.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>
#include <QGraphicsPixmapItem>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/video/video.hpp>

using namespace cv;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // cv::Mat can't be passed through a signal without this declaration
    qRegisterMetaType<cv::Mat>("cv::Mat");

    stacker = new ImageStacker();
    workerThread = new QThread();

    stacker->moveToThread(workerThread);
    workerThread->start();

    connect(ui->buttonSelectRefImage, SIGNAL (released()), this, SLOT (handleButtonRefImage()));
    connect(ui->buttonSelectTargetImages, SIGNAL (released()), this, SLOT (handleButtonTargetImages()));
    connect(ui->buttonStack, SIGNAL (released()), this, SLOT (handleButtonStack()));
    connect(this, SIGNAL (stackImages()), stacker, SLOT(process()));
    connect(stacker, SIGNAL(finished(cv::Mat)), this, SLOT(finishedStacking(cv::Mat)));
}

void MainWindow::finishedStacking(Mat image) {
    QString path = stacker->getSaveFilePath();
    imwrite(path.toUtf8().constData(), image);
    setMemImage(Mat2QImage(image));
    qDebug() << "Done stacking";
}

void MainWindow::handleButtonStack() {

    QString saveFilePath = QFileDialog::getSaveFileName(
                this, "Select Output Image", selectedDir.absolutePath(), "TIFF Image (*.tif)");

    if (saveFilePath.isEmpty()) {
        qDebug() << "No output file selected. Cancelling.";
        return;
    }

    stacker->setSaveFilePath(saveFilePath);

    // asynchronously trigger the processing
    emit stackImages();

    ProcessingDialog *dialog = new ProcessingDialog(this);
    connect(stacker, SIGNAL(updateProgress(QString,int)), dialog, SLOT(updateProgress(QString,int)));
    connect(stacker, SIGNAL(finishedDialog(QString)), dialog, SLOT(complete(QString)));

    if (!dialog->exec()) {
        qDebug() << "Cancelling...";
        stacker->cancel = true;
    }
}

void MainWindow::handleButtonRefImage() {
    QFileDialog dialog(this);
    dialog.setDirectory(QDir::homePath());
    dialog.setFileMode(QFileDialog::ExistingFile);
    QStringList filter;
    filter << "Image files (*.jpg *.jpeg *.png *.tif)" << "All files (*)";
    dialog.setNameFilters(filter);

    if (!dialog.exec()) return;

    QString refImageFileName = dialog.selectedFiles().at(0);
    stacker->setRefImageFileName(refImageFileName);

    QFileInfo info(refImageFileName);
    selectedDir = QDir(info.absoluteFilePath());
    setFileImage(refImageFileName);
    qDebug() << refImageFileName;

    ui->buttonSelectTargetImages->setEnabled(true);
}

void MainWindow::handleButtonTargetImages() {
    QFileDialog dialog(this);
    dialog.setDirectory(selectedDir);
    dialog.setFileMode(QFileDialog::ExistingFiles);
    QStringList filter;
    filter << "Image files (*.jpg *.jpeg *.png *.tif)" << "All files (*)";
    dialog.setNameFilters(filter);

    if (!dialog.exec()) return;

    QStringList targetImageFileNames = dialog.selectedFiles();
    stacker->setTargetImageFileNames(targetImageFileNames);

    for (int i = 0; i < targetImageFileNames.length(); i++) {
        qDebug() << targetImageFileNames.at(i);
    }

    ui->buttonStack->setEnabled(true);
}

void MainWindow::handleButtonDarkFrames() {
    QFileDialog dialog(this);
    dialog.setDirectory(selectedDir);
    dialog.setFileMode(QFileDialog::ExistingFiles);
    QStringList filter;
    filter << "Image files (*.jpg *.jpeg *.png *.tif)" << "All files (*)";
    dialog.setNameFilters(filter);

    if (!dialog.exec()) return;

    QStringList darkFrameFileNames = dialog.selectedFiles();
    stacker->setDarkFrameFileNames(darkFrameFileNames);

    for (int i = 0; i < darkFrameFileNames.length(); i++) {
        qDebug() << darkFrameFileNames.at(i);
    }
}

void MainWindow::handleButtonDarkFlatFrames() {
    QFileDialog dialog(this);
    dialog.setDirectory(selectedDir);
    dialog.setFileMode(QFileDialog::ExistingFiles);
    QStringList filter;
    filter << "Image files (*.jpg *.jpeg *.png *.tif)" << "All files (*)";
    dialog.setNameFilters(filter);

    if (!dialog.exec()) return;

    QStringList darkFlatFrameFileNames = dialog.selectedFiles();
    stacker->setDarkFlatFrameFileNames(darkFlatFrameFileNames);

    for (int i = 0; i < darkFlatFrameFileNames.length(); i++) {
        qDebug() << darkFlatFrameFileNames.at(i);
    }
}

void MainWindow::handleButtonFlatFrames() {
    QFileDialog dialog(this);
    dialog.setDirectory(selectedDir);
    dialog.setFileMode(QFileDialog::ExistingFiles);
    QStringList filter;
    filter << "Image files (*.jpg *.jpeg *.png *.tif)" << "All files (*)";
    dialog.setNameFilters(filter);

    if (!dialog.exec()) return;

    QStringList flatFrameFileNames = dialog.selectedFiles();
    stacker->setFlatFrameFileNames(flatFrameFileNames);

    for (int i = 0; i < flatFrameFileNames.length(); i++) {
        qDebug() << flatFrameFileNames.at(i);
    }
}

QImage MainWindow::Mat2QImage(const cv::Mat &src) {
        QImage dest(src.cols, src.rows, QImage::Format_RGB32);
        int r, g, b;
        for(int x = 0; x < src.cols; x++) {
            for(int y = 0; y < src.rows; y++) {
                Vec<unsigned short,3> pixel = src.at<Vec<unsigned short,3>>(y,x);
                b = pixel.val[0]/256;
                g = pixel.val[1]/256;
                r = pixel.val[2]/256;
                dest.setPixel(x, y, qRgb(r,g,b));
            }
       }
       return dest;
}


void MainWindow::setFileImage(QString filename) {
    QGraphicsScene* scene = new QGraphicsScene(this);
    QGraphicsPixmapItem *p = scene->addPixmap(QPixmap(filename));
    ui->imageHolder->setScene(scene);
    ui->imageHolder->fitInView(p, Qt::KeepAspectRatio);
}

void MainWindow::setMemImage(QImage image) {
    QGraphicsScene* scene = new QGraphicsScene(this);
    QGraphicsPixmapItem *p = scene->addPixmap(QPixmap::fromImage(image));
    ui->imageHolder->setScene(scene);
    ui->imageHolder->fitInView(p, Qt::KeepAspectRatio);
}

MainWindow::~MainWindow()
{
    delete ui;
    workerThread->quit();
    workerThread->wait();
    delete workerThread;
    delete stacker;
}
