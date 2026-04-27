#pragma once
#include <QDialog>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QRadioButton>
#include "database.h"

class ScheduleDialog : public QDialog {
    Q_OBJECT
public:
    explicit ScheduleDialog(const QList<Room>& rooms, QWidget* parent = nullptr);

private slots:
    void onConfirm();

private:
    QComboBox* m_deviceCombo;
    QDateTimeEdit* m_dateTimeEdit;
    QRadioButton* m_turnOnRadio;
    QRadioButton* m_turnOffRadio;
    QList<Room> m_rooms;
};