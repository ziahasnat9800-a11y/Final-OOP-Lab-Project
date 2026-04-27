#include "scheduledialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>
#include <QButtonGroup>

ScheduleDialog::ScheduleDialog(const QList<Room>& rooms, QWidget* parent)
    : QDialog(parent), m_rooms(rooms) {
    setWindowTitle("Schedule Device");
    setFixedSize(360, 340);
    setModal(true);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(14);

    QLabel* title = new QLabel("⏰  Schedule a Device");
    title->setStyleSheet("font-size: 18px; font-weight: bold; color: #7c9fff;");
    layout->addWidget(title);

    layout->addWidget(new QLabel("Select Device:"));
    m_deviceCombo = new QComboBox;
    m_deviceCombo->setObjectName("comboBox");
    for (const Room& r : rooms) {
        for (const Device& d : r.devices) {
            m_deviceCombo->addItem(QString("%1 — %2").arg(r.name).arg(d.name), d.id);
        }
    }
    layout->addWidget(m_deviceCombo);

    layout->addWidget(new QLabel("Action:"));
    QHBoxLayout* radioLayout = new QHBoxLayout;
    m_turnOnRadio = new QRadioButton("Turn ON");
    m_turnOnRadio->setObjectName("turnOn");
    m_turnOffRadio = new QRadioButton("Turn OFF");
    m_turnOffRadio->setObjectName("turnOff");
    m_turnOnRadio->setChecked(true);
    radioLayout->addWidget(m_turnOnRadio);
    radioLayout->addWidget(m_turnOffRadio);
    layout->addLayout(radioLayout);

    layout->addWidget(new QLabel("Schedule Date & Time:"));
    // Use second=0 for both the initial value and the minimum.
    // QDateTimeEdit clamps all user-entered values to be >= minimumDateTime.
    // If the minimum has e.g. :34 seconds, every picked time silently keeps
    // :34 seconds even though the display only shows hh:mm — causing schedules
    // to fire one minute late.
    QDateTime baseTime = QDateTime::currentDateTime();
    baseTime.setTime(QTime(baseTime.time().hour(), baseTime.time().minute(), 0));
    m_dateTimeEdit = new QDateTimeEdit(baseTime.addSecs(60));
    m_dateTimeEdit->setObjectName("dateTimeEdit");
    m_dateTimeEdit->setDisplayFormat("yyyy-MM-dd  hh:mm");
    m_dateTimeEdit->setMinimumDateTime(baseTime);
    m_dateTimeEdit->setCalendarPopup(true);
    layout->addWidget(m_dateTimeEdit);

    layout->addStretch();

    QHBoxLayout* btnLayout = new QHBoxLayout;
    QPushButton* cancelBtn = new QPushButton("Cancel");
    cancelBtn->setObjectName("cancelBtn");
    QPushButton* confirmBtn = new QPushButton("Schedule");
    confirmBtn->setObjectName("primaryBtn");
    btnLayout->addWidget(cancelBtn);
    btnLayout->addWidget(confirmBtn);
    layout->addLayout(btnLayout);

    connect(confirmBtn, &QPushButton::clicked, this, &ScheduleDialog::onConfirm);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    setStyleSheet(R"(
        QWidget { background: #0f0f1a; color: #e0e0f0; font-family: 'Segoe UI', sans-serif; }
        QLabel { color: #9ca3af; font-size: 13px; }
        QComboBox#comboBox, QDateTimeEdit#dateTimeEdit {
            background: #1e1e2e; border: 1.5px solid #2d2d45;
            border-radius: 8px; padding: 8px 12px; color: #e0e0f0;
        }
        QRadioButton { color: #e0e0f0; font-size: 13px; }
        QRadioButton#turnOn { color: #4ade80; font-weight: 600; }
        QRadioButton#turnOff { color: #f87171; font-weight: 600; }
        QRadioButton#turnOn::indicator:checked {
            background: #4ade80; border-radius: 7px;
            border: 2px solid #22c55e;
}
        QRadioButton#turnOff::indicator:checked {
            background: #f87171; border-radius: 7px;
            border: 2px solid #ef4444;
}
        QRadioButton::indicator:unchecked {
            background: #2d2d45; border-radius: 7px;
            border: 2px solid #4b5563;
}
        QPushButton#primaryBtn {
            background: #7c9fff; color: #0f0f1a; border: none;
            border-radius: 8px; padding: 10px 20px; font-weight: bold;
        }
        QPushButton#primaryBtn:hover { background: #6080ee; }
        QPushButton#cancelBtn {
            background: #2d2d45; color: #e0e0f0; border: 1.5px solid #6b7280;
            border-radius: 8px; padding: 10px 20px; font-weight: 600;
        }
        QPushButton#cancelBtn:hover { background: #3d3d55; border-color: #9ca3af; }
    )");
}

void ScheduleDialog::onConfirm() {
    if (m_deviceCombo->count() == 0) {
        QMessageBox::warning(this, "No Devices", "No devices available to schedule.");
        return;
    }
    int deviceId = m_deviceCombo->currentData().toInt();
    QString deviceName = m_deviceCombo->currentText();
    bool turnOn = m_turnOnRadio->isChecked();
    QDateTime schedTime = m_dateTimeEdit->dateTime();
    // Belt-and-suspenders: ensure seconds are always 0 before storing.
    schedTime.setTime(QTime(schedTime.time().hour(), schedTime.time().minute(), 0));

    if (Database::instance().addSchedule(deviceId, deviceName, turnOn, schedTime)) {
        QMessageBox::information(this, "Scheduled",
                                 QString("Device will be turned %1 at %2")
                                     .arg(turnOn ? "ON" : "OFF")
                                     .arg(schedTime.toString("yyyy-MM-dd hh:mm")));
        accept();
    }
}