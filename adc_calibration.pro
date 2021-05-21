QT -= gui

CONFIG += c++11 console
CONFIG -= app_bundle

DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        adc_proc.cpp \
        main.cpp 

HEADERS += \
    adc_proc.h \
    axireg.h \
    system_global.h

#target.path = /run/$${TARGET}
target.path = /run/user/1001/$${TARGET}
INSTALLS += target
CCFLAG += -march=cortex-a9



