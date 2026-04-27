#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QListWidget>
#include <QStackedWidget>
#include <QComboBox>
#include "database.h"

class RegisterWindow : public QDialog {
    Q_OBJECT
public:
    explicit RegisterWindow(QWidget* parent = nullptr);

private slots:
    void onNextFromCredentials();
    void onAddRoom();
    void onRemoveRoom();
    void onNextFromRooms();
    void onAddDevice();
    void onRemoveDevice();
    void onRoomSelected(int index);
    void onFinish();

private:
    void setupCredentialsPage();
    void setupRoomsPage();
    void setupDevicesPage();
    void applyStyles();

    QStackedWidget* m_stack;

    // Page 1 - credentials
    QLineEdit* m_ownerNameEdit;      // ← NEW
    QLineEdit* m_houseNumEdit;
    QLineEdit* m_passEdit;
    QLineEdit* m_confirmPassEdit;
    QLabel*    m_credStatus;

    // Page 2 - rooms
    QLineEdit*   m_roomNameEdit;
    QListWidget* m_roomsList;
    QLabel*      m_roomStatus;

    // Page 3 - devices
    QComboBox*   m_roomSelector;
    QLineEdit*   m_deviceNameEdit;
    QComboBox*   m_deviceTypeCombo;
    QListWidget* m_devicesList;

    int     m_houseId = -1;
    QString m_houseNumber;
    QList<Room> m_rooms;
};