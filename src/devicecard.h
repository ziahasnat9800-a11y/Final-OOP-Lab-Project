#pragma once
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QDateTime>

class DeviceCard : public QFrame {
    Q_OBJECT
public:
    DeviceCard(int deviceId, const QString& name, const QString& type,
               bool isOn, QWidget* parent = nullptr);
    void setState(bool isOn);
    void setOnTime(const QDateTime& since);
    int  deviceId() const { return m_deviceId; }

signals:
    void toggleRequested(int deviceId, const QString& name, const QString& type);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    void updateCardStyle();
    void applyToggleStyle(bool isOn);
    void updateTimeLabel();
    static QString iconFor(const QString& type);
    static int     wattFor(const QString& type);

    int       m_deviceId;
    QString   m_name, m_type;
    bool      m_isOn;
    QDateTime m_onSince;

    QLabel*  m_iconLabel;
    QLabel*  m_wattLabel;
    QLabel*  m_timeLabel;

    QWidget* m_toggleTrack;
    QLabel*  m_toggleKnob;
    QLabel*  m_toggleText;
};