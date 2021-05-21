#include <QCoreApplication>
#include "adc_proc.h"
#include <QDebug>
#include "system_global.h"

int main(int argc, char *argv[])
{
    // QCoreApplication a(argc, argv);

    ADC adcdevice;
    int err = adcdevice.init();
    if(err) {
        CRITICAL_MESSAGE << "error at init(): " << err;
        return err;
    }
    return 0;
    // return a.exec();
}
