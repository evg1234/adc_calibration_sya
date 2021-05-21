#include "adc_proc.h"
#include <QDir>
#include <QStringList>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QThread>
#include <unistd.h>

ADC::ADC(QObject * parent) : QObject(parent)
{
}

int ADC::init()
{
    //1. creating lists of UIOs
    getBitSlipFsmUioList();
    if(!bitSlipFsmUioList.size())
        return -ERR_NO_BITSLIP_UIO;

    getReg2AxiList();
    if(!reg2AxiUioList.size())
        return -ERR_NO_REG2AXI_UIO;

    //2. calibrating adc
    int err = adcCalibration();
    if(err)
        return err;

    return 0;
}

void ADC::getBramUioList()
{
    getUioTargetedList(BRAM_UIO_NAME_PATTERN, bramUioList);
}

void ADC::getBitSlipFsmUioList()
{
    getUioTargetedList(BITSLIP_SM_UIO_NAME_PATTERN, bitSlipFsmUioList);
}

void ADC::getReg2AxiList()
{
    getUioTargetedList(REG2AXI_NAME_PATTERN, reg2AxiUioList);
}


void ADC::getUioTargetedList(const QString namePatternToSearch, QList<UioMapAttributes> &listToFill)
{

    QDir qDir(UIO_CLASS_DIR);
    auto uioReferences = qDir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot, QDir::Name);

    // creating a list of UIO calibrating state machines (4 - 8)
    if(!uioReferences.size())
        return;

    foreach(const QString &uioDirName, uioReferences) { //for each uio0...uioNN
        bool uioBitSlipSmFound = false;

        QDir uioDir(QString("%1/%2").arg(UIO_CLASS_DIR).arg(uioDirName));
        auto uioAttrList = uioDir.entryList(QDir::NoFilter, QDir::Name);
        foreach(const QString &uioAttrName,  uioAttrList) { // name: uio0..n
            if(uioAttrName == QString(UIO_DEVICE_NAME_1) ) {
                QFile uioAttrFile( QString("%1/%2/%3").arg(UIO_CLASS_DIR).arg(uioDirName).arg(uioAttrName) );
                if (uioAttrFile.exists() && uioAttrFile.open(QFile::ReadOnly)) {
                    QTextStream nameText(&uioAttrFile);
                    auto name = nameText.readLine();
                    QRegExp rx(namePatternToSearch); //pattern
                    rx.indexIn(name); //searching the name for the pattern
                    if(rx.pos()>=0) {  //match
                        uioBitSlipSmFound=true;
                    }
                }
                break;
            }
        }

        if(uioBitSlipSmFound) {
            foreach(const QString &uioAttrName,  uioAttrList) { // searching for /maps/...
                if(uioAttrName == QString(UIO_DEVICE_MAP_1) ) { // maps found
                    QDir mapDir(QString("%1/%2/%3/%4").arg(UIO_CLASS_DIR).arg(uioDirName).arg(uioAttrName).arg(UIO_DEVICE_MAP_2) );
                    auto uioMapAttrList = mapDir.entryList(QDir::AllEntries | QDir::NoDotAndDotDot, QDir::Name);
                    QString name, size, addr, offset;
                    foreach(const QString &uioMapAttrName, uioMapAttrList) {
                        QFile uioMapAttrFile( QString("%1/%2/%3/%4/%5").arg(UIO_CLASS_DIR)
                                              .arg(uioDirName).arg(uioAttrName).arg(UIO_DEVICE_MAP_2).arg(uioMapAttrName) );

                        QString text_;
                        if (uioMapAttrFile.exists() && uioMapAttrFile.open(QFile::ReadOnly)) {
                            QTextStream text(&uioMapAttrFile);
                            text_ = text.readLine();
                        } else
                            return;

                        if(uioMapAttrName == QString(UIO_DEVICE_SIZE_3)) {
                            size = text_;
                        } else if(uioMapAttrName == QString(UIO_DEVICE_NAME_3)) {
                            name = text_;
                        } else if(uioMapAttrName == QString(UIO_DEVICE_ADDR_3)) {
                            addr = text_;
                        } else if(uioMapAttrName == QString(UIO_DEVICE_OFFSET_3)) {
                            offset = text_;
                        }
                    }

                    auto uioMapAttributes = UioMapAttributes {
                            .path = QString("%1/%2").arg(UIO_DEV_DIR).arg(uioDirName),
                            .size = size,.name = name,.offset = offset
                };
                    listToFill.append(uioMapAttributes);
                    break; //no more search
                }
            }
        }
    }
}


int ADC::adcCalibration()
{
    int err=0;

    // --------------------------------------------------
    // настройка синтезатора
    QFile spi_syn(SPIDEV02);
    if(!spi_syn.exists()) {
        CRITICAL_MESSAGE << SPIDEV02 << "does not exist";
        return -ERR_NO_SPI;
    }

    if(!spi_syn.open(QIODevice::WriteOnly|QIODevice::Unbuffered)) {
        CRITICAL_MESSAGE << SPIDEV02 << "can not be opened";
        return -ERR_SPI_NOT_OPENED;
    }

    spi_syn.write("\x00\x00\x3C",3); // soft reset
    spi_syn.write("\x02\x32\x01",3); // enforce
    spi_syn.write("\x00\x00\x18",3); // deassert reset
    spi_syn.write("\x02\x32\x01",3); // enforce
    spi_syn.write("\x00\x11\x08",3); // R counter = 8
    spi_syn.write("\x00\x16\x04",3); // P prescaler = 8
    spi_syn.write("\x00\x14\x0A",3); // B counter = 10,  A=0
    spi_syn.write("\x02\x32\x01",3); // enforce
    spi_syn.write("\x00\x1C\x07",3); // diff reference
    spi_syn.write("\x00\x10\x7C",3); // launching pll
    usleep(1000);

    //-- channel dividers
    spi_syn.write("\x01\xE0\x00",3); // vco divider = 2
    spi_syn.write("\x01\xE1\x02",3); // select vco
    spi_syn.write("\x01\x9C\x20",3); // bypass 2.2 devider, enable 2.1 devider
    spi_syn.write("\x01\x99\x44",3); // 10 high cycles and 10 low cycles on a devider 2.1

    //-- switching on output
    spi_syn.write("\x01\x41\x42",3); // lvds 05
    spi_syn.write("\x01\x40\x42",3); // lvds 04
    spi_syn.write("\x02\x32\x01",3); // enforce

    //-- monitor points
    spi_syn.write("\x00\x1B\x00",3); // refmon (tp1)
    spi_syn.write("\x00\x17\x18",3); // status (tp2)  PFD up (x14)  PFD down (x18)
    spi_syn.write("\x02\x32\x01",3); // enforce
    usleep(1000);

    //-- calibration
    spi_syn.write("\x00\x18\x06",3); // calibration
    spi_syn.write("\x02\x32\x01",3); // enforce
    spi_syn.write("\x00\x18\x07",3); // calibration
    spi_syn.write("\x02\x32\x01",3); // enforce
    usleep(1000);

    spi_syn.close();

    //------------------------------------------------
    // ADC1
    QFile spi_adc1(SPIDEV00);
    if(!spi_adc1.exists()) {
        CRITICAL_MESSAGE << SPIDEV00 << "does not exist";
        return -ERR_NO_SPI;
    }
    if(!spi_adc1.open(QIODevice::WriteOnly|QIODevice::Unbuffered)) {
        CRITICAL_MESSAGE << SPIDEV00 << "can not be opened";
        return -ERR_SPI_NOT_OPENED;
    }

    //configuring ADC
    spi_adc1.write("\x00\x05\x3F", 3);
    spi_adc1.write("\x00\xff\x01", 3);

    //test pattern 0xA3
    spi_adc1.write("\x00\x0D\x0C", 3);
    spi_adc1.write("\x00\xff\x01", 3);

    // ADC0
    QFile spi_adc0(SPIDEV01);
    if(!spi_adc0.exists()) {
        CRITICAL_MESSAGE << SPIDEV01 << "does not exist";
        return -ERR_NO_SPI;
    }
    if(!spi_adc0.open(QIODevice::WriteOnly|QIODevice::Unbuffered)) {
        CRITICAL_MESSAGE << SPIDEV01 << "can not be opened";
        return -ERR_SPI_NOT_OPENED;
    }

    //configuring ADC
    spi_adc0.write("\x00\x05\x3F", 3);
    spi_adc0.write("\x00\xff\x01", 3);

    //test pattern 0xA3
    spi_adc0.write("\x00\x0D\x0C", 3);
    spi_adc0.write("\x00\xff\x01", 3);

    // -------------------------------------------------------------
    //2. callibrating
    unsigned short pattern = 0xA33;
    uchar pattern_lower = (uchar) (pattern & 0x3F);
    uchar pattern_upper = (uchar) ((pattern >> 6) & 0x3F);
    u_int32_t status;

    foreach( const UioMapAttributes &bitSlipAttributes, bitSlipFsmUioList) {

        QFile bitSlipFile(bitSlipAttributes.path);
        if(!bitSlipFile.exists()) {
            CRITICAL_MESSAGE << bitSlipAttributes.path << "does not exist";
            err=-ERR_NO_BITSLIP_UIO;
            break;
        }
        if(!bitSlipFile.open(QIODevice::ReadWrite|QIODevice::Unbuffered)) {
            CRITICAL_MESSAGE << bitSlipAttributes.path << "can not be opened";
            err=-ERR_NO_BITSLIP_UIO;
            break;
        }
        uchar *ptr = bitSlipFile.map(0, (qint64)bitSlipAttributes.size.toInt(nullptr, 16), QFileDevice::NoOptions);
        if(ptr == nullptr) {
            bitSlipFile.close();
            CRITICAL_MESSAGE << bitSlipAttributes.path << "can not be mapped";
            err=-ERR_NO_BITSLIP_UIO;
            break;
        }

        *((volatile uint *)(ptr + pattern_lower_in)) = pattern_lower;
        *((volatile uint *)(ptr + control_lower_in)) = FSM_COMMAND;

        while(true) {
            status = *((volatile uint *)(ptr + status_lower_out));
            if(status )
                break;
        }

        *((volatile uint *)(ptr + pattern_upper_in)) = pattern_upper;
        *((volatile uint *)(ptr + control_upper_in)) = FSM_COMMAND;

        while(true) {
            status = *((volatile uint *)(ptr + status_upper_out));
            if(status )
                break;
        }

        // resetting a state machine for future usage
        *((volatile uint *)(ptr + control_lower_in)) = 0x00;
        *((volatile uint *)(ptr + control_upper_in)) = 0x00;

        qInfo() << bitSlipAttributes.path << "callibration worked out with the result code:"
                              << QString("0x%1").arg(status, 0, 16);

        bitSlipFile.unmap(ptr);
        bitSlipFile.close();
    }


    qInfo() << "calibarion finished";
    qInfo() << "switching on PRBS...";


    //3. checking prbs
    spi_adc1.write("\x00\x0D\x06", 3);
    spi_adc1.write("\x00\xff\x01", 3); //switching on prbs
    spi_adc0.write("\x00\x0D\x06", 3);
    spi_adc0.write("\x00\xff\x01", 3); //switching on prbs
    err=testPrbs();
    if (err)   {
        spi_adc1.close();
        spi_adc0.close();
        return 11;
    }


    //4. set adc to normal work
    spi_adc1.write("\x00\x0D\x00", 3);
    spi_adc1.write("\x00\xff\x01", 3);

    spi_adc0.write("\x00\x0D\x00", 3);
    spi_adc0.write("\x00\xff\x01", 3);

    //5.  zero offset
    setOffset();

    //6. setting thresholds
    setThreshold( 50);

    spi_adc1.close();
    spi_adc0.close();

    return err;
}

int ADC::testPrbs()
{
    QThread::usleep(1000);
    QFile reg2axi1(reg2AxiUioList[0].path);
    if(!reg2axi1.exists()) {
        CRITICAL_MESSAGE << reg2AxiUioList[0].path << "does not exist";
        return -ERR_NO_REG2AXI;
    }
    if(!reg2axi1.open(QIODevice::ReadWrite|QIODevice::Unbuffered)) {
        CRITICAL_MESSAGE << reg2AxiUioList[0].path << "can not be opened";
        return -ERR_NO_REG2AXI;
    }
    uchar *ptr = reg2axi1.map(0, (qint64)reg2AxiUioList[0].size.toInt(nullptr, 16), QFileDevice::NoOptions);
    if(ptr == nullptr) {
        reg2axi1.close();
        CRITICAL_MESSAGE << reg2AxiUioList[0].path << "can not be mapped";
        return -ERR_NO_REG2AXI;
    }

    uint reg,offset =0;

    *((volatile uint *)(ptr + offset)) =0x04;  // reser BER flag
    QThread::usleep(100);
    *((volatile uint *)(ptr + offset)) =0x00;
    QThread::usleep(100000);

    reg = *((volatile uint *)(ptr + offset));
    reg &= 0x0F; //extracting 4 bits of the ber counter

    reg2axi1.unmap(ptr);
    reg2axi1.close();


    //------------------------------------------------------
    QFile reg2axi2(reg2AxiUioList[1].path);
    if(!reg2axi2.exists()) {
        CRITICAL_MESSAGE << reg2AxiUioList[0].path << "does not exist";
        return -ERR_NO_REG2AXI;
    }
    if(!reg2axi2.open(QIODevice::ReadWrite|QIODevice::Unbuffered)) {
        CRITICAL_MESSAGE << reg2AxiUioList[0].path << "can not be opened";
        return -ERR_NO_REG2AXI;
    }

    ptr = reg2axi2.map(0, (qint64)reg2AxiUioList[0].size.toInt(nullptr, 16), QFileDevice::NoOptions);
    if(ptr == nullptr) {
        reg2axi2.close();
        CRITICAL_MESSAGE << reg2AxiUioList[0].path << "can not be mapped";
        return -ERR_NO_REG2AXI;
    }

    *((volatile uint *)(ptr + offset)) =0x04;  // reser BER flag
    QThread::usleep(100);
    *((volatile uint *)(ptr + offset)) =0x00;
    QThread::usleep(10000);

    reg = *((volatile uint *)(ptr + offset));
    reg &= 0x0F; //extracting 4 bits of the ber counter

    reg2axi2.unmap(ptr);
    reg2axi2.close();

    return reg;
}


int ADC::setOffset( )
{

    QFile reg2axi1(reg2AxiUioList[1].path);
    if(!reg2axi1.exists()) {
        CRITICAL_MESSAGE << reg2AxiUioList[1].path << "does not exist";
        return -ERR_NO_REG2AXI;
    }
    if(!reg2axi1.open(QIODevice::ReadWrite|QIODevice::Unbuffered)) {
        CRITICAL_MESSAGE << reg2AxiUioList[1].path << "can not be opened";
        return -ERR_NO_REG2AXI;
    }
    uchar *ptr1 = reg2axi1.map(0, (qint64)reg2AxiUioList[1].size.toInt(nullptr, 16), QFileDevice::NoOptions);
    if(ptr1 == nullptr) {
        reg2axi1.close();
        CRITICAL_MESSAGE << reg2AxiUioList[1].path << " can not be mapped";
        return -ERR_NO_REG2AXI;
    }

    QFile reg2axi0(reg2AxiUioList[0].path);
    if(!reg2axi0.exists()) {
        CRITICAL_MESSAGE << reg2AxiUioList[0].path << "does not exist";
        return -ERR_NO_REG2AXI;
    }
    if(!reg2axi0.open(QIODevice::ReadWrite|QIODevice::Unbuffered)) {
        CRITICAL_MESSAGE << reg2AxiUioList[0].path << "can not be opened";
        return -ERR_NO_REG2AXI;
    }
    uchar *ptr0 = reg2axi0.map(0, (qint64)reg2AxiUioList[0].size.toInt(nullptr, 16), QFileDevice::NoOptions);
    if(ptr0 == nullptr) {
        reg2axi0.close();
        CRITICAL_MESSAGE << reg2AxiUioList[0].path << " can not be mapped";
        return -ERR_NO_REG2AXI;
    }


    const uint32_t iteration = 1000; // количеество итераций для расчета смещения нуля
    float tmp;
    // ------------------------------------------------
    tmp = 0;
    for (u32 i = 1; i < iteration; i++) {
        usleep(1);
        setupA0.Data32 = *((volatile uint *)(ptr0 + 4));
        tmp += (0.3 * ( setupA0.BitField.rawADC - tmp));
    }
    controlA0.BitField.offset = static_cast<int16_t>(qRound(tmp));
    *((volatile uint *)(ptr0 + 4)) = controlA0.Data32;
    // ##
    tmp = 0;
    for (u32 i = 1; i < iteration; i++) {
        usleep(1);
        setupB0.Data32 = *((volatile uint *)(ptr0 + 8));
        tmp += (0.3 * ( setupB0.BitField.rawADC - tmp));
    }
    controlB0.BitField.offset = static_cast<int16_t>(qRound(tmp));
    *((volatile uint *)(ptr0 + 8)) = controlB0.Data32;
    // ##
    tmp = 0;
    for (u32 i = 1; i < iteration; i++) {
        usleep(1);
        setupC0.Data32 = *((volatile uint *)(ptr0 + 12));
        tmp += (0.3 * ( setupC0.BitField.rawADC - tmp));
    }
    controlC0.BitField.offset = static_cast<int16_t>(qRound(tmp));
    *((volatile uint *)(ptr0 + 12)) = controlC0.Data32;
    // ##
    tmp = 0;
    for (u32 i = 1; i < iteration; i++) {
        usleep(1);
        setupD0.Data32 = *((volatile uint *)(ptr0 + 16));
        tmp += (0.3 * ( setupD0.BitField.rawADC - tmp));
    }
    controlD0.BitField.offset = static_cast<int16_t>(qRound(tmp));
    *((volatile uint *)(ptr0 + 16)) = controlD0.Data32;

    //--------------------------------------------------------
    tmp = 0;
    for (u32 i = 1; i < iteration; i++) {
        usleep(1);
        setupA1.Data32 = *((volatile uint *)(ptr1 + 4));
        tmp += (0.3 * ( setupA1.BitField.rawADC - tmp));
    }
    controlA1.BitField.offset = static_cast<int16_t>(qRound(tmp));
    *((volatile uint *)(ptr1 + 4)) = controlA1.Data32;
    // ##
    tmp = 0;
    for (u32 i = 1; i < iteration; i++) {
        usleep(1);
        setupB1.Data32 = *((volatile uint *)(ptr1 + 8));
        tmp += (0.3 * ( setupB1.BitField.rawADC - tmp));
    }
    controlB1.BitField.offset = static_cast<int16_t>(qRound(tmp));
    *((volatile uint *)(ptr1 + 8)) = controlB1.Data32;
    // ##
    tmp = 0;
    for (u32 i = 1; i < iteration; i++) {
        usleep(1);
        setupC1.Data32 = *((volatile uint *)(ptr1 + 12));
        tmp += (0.3 * ( setupC1.BitField.rawADC - tmp));
    }
    controlC1.BitField.offset = static_cast<int16_t>(qRound(tmp));
    *((volatile uint *)(ptr1 + 12)) = controlC1.Data32;
    // ##
    tmp = 0;
    for (u32 i = 1; i < iteration; i++) {
        usleep(1);
        setupD0.Data32 = *((volatile uint *)(ptr1 + 16));
        tmp += (0.3 * ( setupD0.BitField.rawADC - tmp));
    }
    controlD1.BitField.offset = static_cast<int16_t>(qRound(tmp));
    *((volatile uint *)(ptr1 + 16)) = controlD1.Data32;


    reg2axi1.unmap(ptr1);
    reg2axi1.close();
    reg2axi0.unmap(ptr0);
    reg2axi0.close();

    return 0;
}

int ADC::setThreshold( int value)
{

    QFile reg2axi1(reg2AxiUioList[0].path);
    if(!reg2axi1.exists()) {
        CRITICAL_MESSAGE << reg2AxiUioList[0].path << "does not exist";
        return -ERR_NO_REG2AXI;
    }
    if(!reg2axi1.open(QIODevice::ReadWrite|QIODevice::Unbuffered)) {
        CRITICAL_MESSAGE << reg2AxiUioList[0].path << "can not be opened";
        return -ERR_NO_REG2AXI;
    }
    uchar *ptr1 = reg2axi1.map(0, (qint64)reg2AxiUioList[0].size.toInt(nullptr, 16), QFileDevice::NoOptions);
    if(ptr1 == nullptr) {
        reg2axi1.close();
        CRITICAL_MESSAGE << reg2AxiUioList[0].path << " can not be mapped";
        return -ERR_NO_REG2AXI;
    }


    QFile reg2axi0(reg2AxiUioList[0].path);
    if(!reg2axi0.exists()) {
        CRITICAL_MESSAGE << reg2AxiUioList[0].path << "does not exist";
        return -ERR_NO_REG2AXI;
    }
    if(!reg2axi0.open(QIODevice::ReadWrite|QIODevice::Unbuffered)) {
        CRITICAL_MESSAGE << reg2AxiUioList[0].path << "can not be opened";
        return -ERR_NO_REG2AXI;
    }
    uchar *ptr0 = reg2axi1.map(0, (qint64)reg2AxiUioList[0].size.toInt(nullptr, 16), QFileDevice::NoOptions);
    if(ptr0 == nullptr) {
        reg2axi0.close();
        CRITICAL_MESSAGE << reg2AxiUioList[0].path << " can not be mapped";
        return -ERR_NO_REG2AXI;
    }

    controlA0.BitField.threshold = value;
    controlB0.BitField.threshold = value;
    controlC0.BitField.threshold = value;
    controlD0.BitField.threshold = value;

    *((volatile uint *)(ptr0 + 4))  = controlA0.Data32;
    *((volatile uint *)(ptr0 + 8))  = controlB0.Data32;
    *((volatile uint *)(ptr0 + 12)) = controlC0.Data32;
    *((volatile uint *)(ptr0 + 16)) = controlD0.Data32;

    controlA1.BitField.threshold = value;
    controlB1.BitField.threshold = value;
    controlC1.BitField.threshold = value;
    controlD1.BitField.threshold = value;

    *((volatile uint *)(ptr1 + 4))  = controlA1.Data32;
    *((volatile uint *)(ptr1 + 8))  = controlB1.Data32;
    *((volatile uint *)(ptr1 + 12)) = controlC1.Data32;
    *((volatile uint *)(ptr1 + 16)) = controlD1.Data32;

    reg2axi1.unmap(ptr1);
    reg2axi1.close();

    reg2axi0.unmap(ptr0);
    reg2axi0.close();

    return value;
}


ADC::~ADC() {}
