#pragma once

#include <QMainWindow>
#include <QCheckBox>
#include <QSlider>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include "realtime.h"
#include "utils/aspectratiowidget/aspectratiowidget.hpp"

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    void initialize();
    void finish();

private:
    void connectUIElements();
    void connectBloomControls();
    void connectScrollControls();
    void connectNear();
    void connectFar();

    // From old Project 6
    // void connectPerPixelFilter();
    // void connectKernelBasedFilter();

    void connectUploadFile();
    void connectSaveImage();
    Realtime *realtime;
    AspectRatioWidget *aspectRatioWidget;

    // From old Project 6
    // QCheckBox *filter1;
    // QCheckBox *filter2;

    QPushButton *uploadFile;
    QPushButton *saveImage;
    QSlider *bloomSlider;
    QSlider *scrollSlider;
    QDoubleSpinBox *bloomBox;
    QDoubleSpinBox *scrollBox;
    QSlider *nearSlider;
    QSlider *farSlider;
    QDoubleSpinBox *nearBox;
    QDoubleSpinBox *farBox;

private slots:
    // From old Project 6
    // void onPerPixelFilter();
    // void onKernelBasedFilter();

    void onUploadFile();
    void onSaveImage();
    void onBloomSliderChanged(int value);
    void onBloomBoxChanged(double value);
    void onScrollSliderChanged(int value);
    void onScrollBoxChanged(double value);
    void onValChangeNearSlider(int newValue);
    void onValChangeFarSlider(int newValue);
    void onValChangeNearBox(double newValue);
    void onValChangeFarBox(double newValue);

};
