#include "mainwindow.h"
#include "settings.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QSettings>
#include <QLabel>
#include <QGroupBox>
#include <QSignalBlocker>
#include <iostream>
#include <cmath>

void MainWindow::initialize() {
    realtime = new Realtime;
    aspectRatioWidget = new AspectRatioWidget(this);
    aspectRatioWidget->setAspectWidget(realtime, 9.f/16.f);  // 16:9 电影感宽高比
    QHBoxLayout *hLayout = new QHBoxLayout; // horizontal alignment
    QVBoxLayout *vLayout = new QVBoxLayout(); // vertical alignment
    vLayout->setAlignment(Qt::AlignTop);
    hLayout->addLayout(vLayout);
    hLayout->addWidget(aspectRatioWidget, 1);
    this->setLayout(hLayout);

    // Create labels in sidebox
    QFont font;
    font.setPointSize(12);
    font.setBold(true);
    QLabel *tesselation_label = new QLabel(); // Parameters label
    tesselation_label->setText("Effects");
    tesselation_label->setFont(font);
    QLabel *camera_label = new QLabel(); // Camera label
    camera_label->setText("Camera");
    camera_label->setFont(font);

    // From old Project 6
    // QLabel *filters_label = new QLabel(); // Filters label
    // filters_label->setText("Filters");
    // filters_label->setFont(font);

    QLabel *param1_label = new QLabel(); // Parameter 1 label
    param1_label->setText("Bloom Strength:");
    QLabel *param2_label = new QLabel(); // Parameter 2 label
    param2_label->setText("Starfield Scroll Speed:");
    QLabel *near_label = new QLabel(); // Near plane label
    near_label->setText("Near Plane:");
    QLabel *far_label = new QLabel(); // Far plane label
    far_label->setText("Far Plane:");


    // From old Project 6
    // // Create checkbox for per-pixel filter
    // filter1 = new QCheckBox();
    // filter1->setText(QStringLiteral("Per-Pixel Filter"));
    // filter1->setChecked(false);
    // // Create checkbox for kernel-based filter
    // filter2 = new QCheckBox();
    // filter2->setText(QStringLiteral("Kernel-Based Filter"));
    // filter2->setChecked(false);

    // Create file uploader for scene file
    uploadFile = new QPushButton();
    uploadFile->setText(QStringLiteral("Upload Scene File"));
    
    saveImage = new QPushButton();
    saveImage->setText(QStringLiteral("Save Image"));

    // ANIMATION: create play button
    playButton = new QPushButton();
    playButton->setText(QStringLiteral("Play Animation"));

    // Creates the boxes containing the parameter sliders and number boxes
    QGroupBox *p1Layout = new QGroupBox(); // horizonal slider 1 alignment
    QHBoxLayout *l1 = new QHBoxLayout();
    QGroupBox *p2Layout = new QGroupBox(); // horizonal slider 2 alignment
    QHBoxLayout *l2 = new QHBoxLayout();

    // Create slider controls to control parameters
    bloomSlider = new QSlider(Qt::Orientation::Horizontal);
    bloomSlider->setTickInterval(1);
    bloomSlider->setMinimum(0);
    bloomSlider->setMaximum(300); // maps to 0.0 - 3.0
    bloomSlider->setSingleStep(1);
    bloomSlider->setPageStep(1);
    bloomSlider->setValue(int(std::round(settings.bloomStrength * 100.f)));

    bloomBox = new QDoubleSpinBox();
    bloomBox->setDecimals(2);
    bloomBox->setMinimum(0.0);
    bloomBox->setMaximum(3.0);
    bloomBox->setSingleStep(0.1);
    bloomBox->setValue(settings.bloomStrength);

    scrollSlider = new QSlider(Qt::Orientation::Horizontal);
    scrollSlider->setTickInterval(1);
    scrollSlider->setMinimum(0);
    scrollSlider->setMaximum(200); // maps to 0 - 0.02
    scrollSlider->setSingleStep(1);
    scrollSlider->setPageStep(1);
    scrollSlider->setValue(int(std::round(settings.bgScrollSpeed * 10000.f)));

    scrollBox = new QDoubleSpinBox();
    scrollBox->setDecimals(4);
    scrollBox->setMinimum(0.0);
    scrollBox->setMaximum(0.02);
    scrollBox->setSingleStep(0.0005);
    scrollBox->setValue(settings.bgScrollSpeed);

    // Adds the slider and number box to the parameter layouts
    l1->addWidget(bloomSlider);
    l1->addWidget(bloomBox);
    p1Layout->setLayout(l1);

    l2->addWidget(scrollSlider);
    l2->addWidget(scrollBox);
    p2Layout->setLayout(l2);

    // Creates the boxes containing the camera sliders and number boxes
    QGroupBox *nearLayout = new QGroupBox(); // horizonal near slider alignment
    QHBoxLayout *lnear = new QHBoxLayout();
    QGroupBox *farLayout = new QGroupBox(); // horizonal far slider alignment
    QHBoxLayout *lfar = new QHBoxLayout();

    // Create slider controls to control near/far planes
    nearSlider = new QSlider(Qt::Orientation::Horizontal); // Near plane slider
    nearSlider->setTickInterval(1);
    nearSlider->setMinimum(1);
    nearSlider->setMaximum(1000);
    nearSlider->setValue(10);

    nearBox = new QDoubleSpinBox();
    nearBox->setMinimum(0.01f);
    nearBox->setMaximum(10.f);
    nearBox->setSingleStep(0.1f);
    nearBox->setValue(0.1f);

    farSlider = new QSlider(Qt::Orientation::Horizontal); // Far plane slider
    farSlider->setTickInterval(1);
    farSlider->setMinimum(1000);
    farSlider->setMaximum(10000);
    farSlider->setValue(10000);

    farBox = new QDoubleSpinBox();
    farBox->setMinimum(10.f);
    farBox->setMaximum(100.f);
    farBox->setSingleStep(0.1f);
    farBox->setValue(100.f);

    // Adds the slider and number box to the parameter layouts
    lnear->addWidget(nearSlider);
    lnear->addWidget(nearBox);
    nearLayout->setLayout(lnear);

    lfar->addWidget(farSlider);
    lfar->addWidget(farBox);
    farLayout->setLayout(lfar);

    vLayout->addWidget(uploadFile);
    vLayout->addWidget(saveImage);
    vLayout->addWidget(playButton);  // ANIMATION: add play button
    vLayout->addWidget(tesselation_label);
    vLayout->addWidget(param1_label);
    vLayout->addWidget(p1Layout);
    vLayout->addWidget(param2_label);
    vLayout->addWidget(p2Layout);
    vLayout->addWidget(camera_label);
    vLayout->addWidget(near_label);
    vLayout->addWidget(nearLayout);
    vLayout->addWidget(far_label);
    vLayout->addWidget(farLayout);

    // From old Project 6
    // vLayout->addWidget(filters_label);
    // vLayout->addWidget(filter1);
    // vLayout->addWidget(filter2);

    connectUIElements();

    onBloomSliderChanged(bloomSlider->value());
    onScrollSliderChanged(scrollSlider->value());

    // Set default values for near and far planes
    onValChangeNearBox(0.1f);
    onValChangeFarBox(100.f);
}

void MainWindow::finish() {
    realtime->finish();
    delete(realtime);
}

void MainWindow::connectUIElements() {
    // From old Project 6
    //connectPerPixelFilter();
    //connectKernelBasedFilter();
    connectUploadFile();
    connectSaveImage();
    connectPlayButton();  // ANIMATION: connect play button
    connectBloomControls();
    connectScrollControls();
    connectNear();
    connectFar();
}


// From old Project 6
// void MainWindow::connectPerPixelFilter() {
//     connect(filter1, &QCheckBox::clicked, this, &MainWindow::onPerPixelFilter);
// }
// void MainWindow::connectKernelBasedFilter() {
//     connect(filter2, &QCheckBox::clicked, this, &MainWindow::onKernelBasedFilter);
// }

void MainWindow::connectUploadFile() {
    connect(uploadFile, &QPushButton::clicked, this, &MainWindow::onUploadFile);
}

void MainWindow::connectSaveImage() {
    connect(saveImage, &QPushButton::clicked, this, &MainWindow::onSaveImage);
}

// ANIMATION: connect play button
void MainWindow::connectPlayButton() {
    connect(playButton, &QPushButton::clicked, this, &MainWindow::onPlayButton);
}

void MainWindow::connectBloomControls() {
    connect(bloomSlider, &QSlider::valueChanged,
            this, &MainWindow::onBloomSliderChanged);
    connect(bloomBox, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this, &MainWindow::onBloomBoxChanged);
}

void MainWindow::connectScrollControls() {
    connect(scrollSlider, &QSlider::valueChanged,
            this, &MainWindow::onScrollSliderChanged);
    connect(scrollBox, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this, &MainWindow::onScrollBoxChanged);
}

void MainWindow::connectNear() {
    connect(nearSlider, &QSlider::valueChanged, this, &MainWindow::onValChangeNearSlider);
    connect(nearBox, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this, &MainWindow::onValChangeNearBox);
}

void MainWindow::connectFar() {
    connect(farSlider, &QSlider::valueChanged, this, &MainWindow::onValChangeFarSlider);
    connect(farBox, static_cast<void(QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this, &MainWindow::onValChangeFarBox);
}

// From old Project 6
// void MainWindow::onPerPixelFilter() {
//     settings.perPixelFilter = !settings.perPixelFilter;
//     realtime->settingsChanged();
// }
// void MainWindow::onKernelBasedFilter() {
//     settings.kernelBasedFilter = !settings.kernelBasedFilter;
//     realtime->settingsChanged();
// }

void MainWindow::onUploadFile() {
    // Get abs path of scene file
    QString configFilePath = QFileDialog::getOpenFileName(this, tr("Upload File"),
                                                          QDir::currentPath()
                                                              .append(QDir::separator())
                                                              .append("scenefiles")
                                                              .append(QDir::separator())
                                                              .append("realtime")
                                                              .append(QDir::separator())
                                                              .append("required"), tr("Scene Files (*.json)"));
    if (configFilePath.isNull()) {
        std::cout << "Failed to load null scenefile." << std::endl;
        return;
    }

    settings.sceneFilePath = configFilePath.toStdString();

    std::cout << "Loaded scenefile: \"" << configFilePath.toStdString() << "\"." << std::endl;
    realtime->setSceneFilePath(configFilePath.toStdString());
    realtime->sceneChanged();
}

void MainWindow::onSaveImage() {
    if (settings.sceneFilePath.empty()) {
        std::cout << "No scene file loaded." << std::endl;
        return;
    }
    std::string sceneName = settings.sceneFilePath.substr(0, settings.sceneFilePath.find_last_of("."));
    sceneName = sceneName.substr(sceneName.find_last_of("/")+1);
    QString filePath = QFileDialog::getSaveFileName(this, tr("Save Image"),
                                                    QDir::currentPath()
                                                        .append(QDir::separator())
                                                        .append("student_outputs")
                                                        .append(QDir::separator())
                                                        .append("realtime")
                                                        .append(QDir::separator())
                                                        .append("required")
                                                        .append(QDir::separator())
                                                        .append(sceneName), tr("Image Files (*.png)"));
    std::cout << "Saving image to: \"" << filePath.toStdString() << "\"." << std::endl;
    realtime->saveViewportImage(filePath.toStdString());
}

void MainWindow::onBloomSliderChanged(int value) {
    double newValue = value / 100.0;
    {
        QSignalBlocker blocker(bloomBox);
        bloomBox->setValue(newValue);
    }
    settings.bloomStrength = static_cast<float>(newValue);
    realtime->update();
}

void MainWindow::onBloomBoxChanged(double newValue) {
    if (newValue < 0.0) newValue = 0.0;
    if (newValue > 3.0) newValue = 3.0;
    {
        QSignalBlocker blocker(bloomSlider);
        bloomSlider->setValue(int(std::round(newValue * 100.0)));
    }
    settings.bloomStrength = static_cast<float>(newValue);
    realtime->update();
}

void MainWindow::onScrollSliderChanged(int value) {
    double newValue = value / 10000.0;
    {
        QSignalBlocker blocker(scrollBox);
        scrollBox->setValue(newValue);
    }
    settings.bgScrollSpeed = static_cast<float>(newValue);
    realtime->update();
}

void MainWindow::onScrollBoxChanged(double newValue) {
    if (newValue < 0.0) newValue = 0.0;
    if (newValue > 0.02) newValue = 0.02;
    {
        QSignalBlocker blocker(scrollSlider);
        scrollSlider->setValue(int(std::round(newValue * 10000.0)));
    }
    settings.bgScrollSpeed = static_cast<float>(newValue);
    realtime->update();
}

void MainWindow::onValChangeNearSlider(int newValue) {
    //nearSlider->setValue(newValue);
    nearBox->setValue(newValue/100.f);
    settings.nearPlane = nearBox->value();
    realtime->settingsChanged();
}

void MainWindow::onValChangeFarSlider(int newValue) {
    //farSlider->setValue(newValue);
    farBox->setValue(newValue/100.f);
    settings.farPlane = farBox->value();
    realtime->settingsChanged();
}

void MainWindow::onValChangeNearBox(double newValue) {
    nearSlider->setValue(int(newValue*100.f));
    //nearBox->setValue(newValue);
    settings.nearPlane = nearBox->value();
    realtime->settingsChanged();
}

void MainWindow::onValChangeFarBox(double newValue) {
    farSlider->setValue(int(newValue*100.f));
    //farBox->setValue(newValue);
    settings.farPlane = farBox->value();
    realtime->settingsChanged();
}

// ANIMATION: reset animation timer when play button is clicked
void MainWindow::onPlayButton() {
    realtime->resetAnimation();
}

