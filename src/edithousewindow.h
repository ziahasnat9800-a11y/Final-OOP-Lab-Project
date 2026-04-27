#pragma once
#include <QDialog>
#include <QListWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include "database.h"

class EditHouseWindow : public QDialog {
    Q_OBJECT
public:
    explicit EditHouseWindow(const House& house, QWidget* parent = nullptr);

private slots:
    void onAddRoom();
    void onRemoveRoom();
    void onRoomSelected(int index);
    void onAddDevice();
    void onRemoveDevice();

private:
    void setupUI();
    void applyStyles();
    void refreshDevices();

    House m_house;
    QList<Room> m_rooms;

    QListWidget* m_roomsList;
    QLineEdit* m_roomNameEdit;

    QComboBox* m_roomSelector;
    QLineEdit* m_deviceNameEdit;
    QComboBox* m_deviceTypeCombo;
    QListWidget* m_devicesList;
};