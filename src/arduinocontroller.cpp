#include "arduinocontroller.h"
#include <QSerialPortInfo>
#include <QDebug>
#include <QThread>

ArduinoController& ArduinoController::instance() {
    static ArduinoController ctrl;
    return ctrl;
}

ArduinoController::ArduinoController(QObject* parent)
    : QObject(parent), m_serial(new QSerialPort(this)) {}

QString ArduinoController::findArduinoPort() {
    const auto ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo& info : ports) {
        // Arduino typically shows up with these vendor IDs or descriptions
        if (info.description().contains("Arduino", Qt::CaseInsensitive) ||
            info.manufacturer().contains("Arduino", Qt::CaseInsensitive) ||
            info.description().contains("USB Serial", Qt::CaseInsensitive) ||
            info.vendorIdentifier() == 0x2341 || // Arduino
            info.vendorIdentifier() == 0x1A86)   // CH340 chip (common clone)
        {
            return info.portName();
        }
    }
    // Fallback: return first available port
    if (!ports.isEmpty()) return ports.first().portName();
    return "";
}

bool ArduinoController::connectToArduino(const QString& portName) {
    QString port = portName.isEmpty() ? findArduinoPort() : portName;
    if (port.isEmpty()) {
        qDebug() << "No Arduino port found";
        return false;
    }

    m_serial->setPortName(port);
    m_serial->setBaudRate(QSerialPort::Baud9600);
    m_serial->setDataBits(QSerialPort::Data8);
    m_serial->setParity(QSerialPort::NoParity);
    m_serial->setStopBits(QSerialPort::OneStop);
    m_serial->setFlowControl(QSerialPort::NoFlowControl);

    if (m_serial->open(QIODevice::ReadWrite)) {
        m_connected = true;
        // Wait for Arduino to reset after connection
        QThread::msleep(2000);
        emit connectionChanged(true);
        qDebug() << "Connected to Arduino on" << port;
        return true;
    } else {
        qDebug() << "Failed to open port:" << m_serial->errorString();
        m_connected = false;
        return false;
    }
}

void ArduinoController::disconnectArduino() {
    if (m_serial->isOpen()) {
        m_serial->close();
    }
    m_connected = false;
    emit connectionChanged(false);
}

bool ArduinoController::isConnected() const {
    return m_connected && m_serial->isOpen();
}

void ArduinoController::controlLight(bool turnOn) {
    if (!isConnected()) {
        qDebug() << "Arduino not connected";
        return;
    }
    // Send '1' to turn on, '0' to turn off
    QByteArray cmd = turnOn ? "1" : "0";
    m_serial->write(cmd);
    m_serial->flush();
    emit lightStateChanged(turnOn);
    qDebug() << "Sent command to Arduino:" << cmd;
}