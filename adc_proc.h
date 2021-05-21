#ifndef FRONTEND_H
#define FRONTEND_H

#include <QObject>
#include "system_global.h"
#include "axireg.h"
#include <QString>


typedef struct UioMapAttributes_ {
    QString path;
    QString size;
    QString name;
    QString addr;
    QString offset;
} UioMapAttributes;



class ADC : public QObject
{
    Q_OBJECT

public:
    ADC(QObject * parent = nullptr);
    virtual ~ADC();
    int init();

private:
    QList<UioMapAttributes> bitSlipFsmUioList;
    QList<UioMapAttributes> bramUioList;
    QList<UioMapAttributes> reg2AxiUioList;
    void getBitSlipFsmUioList();
    void getBramUioList();
    void getReg2AxiList();
    int testPrbs();
    void getUioTargetedList(const QString namePatternToSearch, QList<UioMapAttributes> &listToFill); //utility

    int adcCalibration();
    int setThreshold(int value);
    int setOffset();

    control_chanal_pd_t controlA1, controlB1,controlC1,controlD1 ;
    state_chanal_pd_t setupA1,setupB1,setupC1,setupD1 ;
    control_chanal_pd_t controlA0, controlB0,controlC0,controlD0 ;
    state_chanal_pd_t setupA0,setupB0,setupC0,setupD0 ;


};

#endif // FRONTEND_H
