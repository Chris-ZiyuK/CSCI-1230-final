#ifndef SETTINGS_H
#define SETTINGS_H

#include <string>

struct Settings {
    std::string sceneFilePath;
    int shapeParameter1 = 25;
    int shapeParameter2 = 25;
    float nearPlane = 1;
    float farPlane = 1;
    float bloomStrength = 1.4f;
    float bgScrollSpeed = 0.005f;
    bool perPixelFilter = false;
    bool kernelBasedFilter = false;
    bool extraCredit1 = false;
    bool extraCredit2 = false;
    bool extraCredit3 = false;
    bool extraCredit4 = false;
};


// The global Settings object, will be initialized by MainWindow
extern Settings settings;

#endif // SETTINGS_H
