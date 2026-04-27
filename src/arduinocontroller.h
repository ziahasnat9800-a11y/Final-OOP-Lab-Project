#pragma once
#include <QObject>
#include <QSerialPort>
#include <QString>

class ArduinoController : public QObject {
    Q_OBJECT
public:
    static ArduinoController& instance();
    bool connectToArduino(const QString& portName = "");
    void disconnectArduino();
    bool isConnected() const;
    void controlLight(bool turnOn); // Controls pin 13 light
    QString findArduinoPort();

signals:
    void connectionChanged(bool connected);
    void lightStateChanged(bool isOn);

private:
    ArduinoController(QObject* parent = nullptr);
    QSerialPort* m_serial;
    bool m_connected = false;
};