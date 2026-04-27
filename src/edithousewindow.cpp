#include "edithousewindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QMessageBox>
#include <QTabWidget>

EditHouseWindow::EditHouseWindow(const House& house, QWidget* parent)
    : QDialog(parent), m_house(house) {
    setWindowTitle("Edit House — " + house.houseNumber);
    setFixedSize(560, 580);
    setModal(true);
    setupUI();
    applyStyles();
}

void EditHouseWindow::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    mainLayout->setSpacing(16);

    QLabel* title = new QLabel("Edit House: " + m_house.houseNumber);
    title->setObjectName("pageTitle");
    mainLayout->addWidget(title);

    QTabWidget* tabs = new QTabWidget;
    tabs->setObjectName("tabWidget");

    // --- Rooms Tab ---
    QWidget* roomsTab = new QWidget;
    QVBoxLayout* roomsLayout = new QVBoxLayout(roomsTab);
    roomsLayout->setSpacing(12);

    QHBoxLayout* addRoomLayout = new QHBoxLayout;
    m_roomNameEdit = new QLineEdit;
    m_roomNameEdit->setPlaceholderText("New room name");
    m_roomNameEdit->setObjectName("inputField");
    QPushButton* addRoomBtn = new QPushButton("Add Room");
    addRoomBtn->setObjectName("addBtn");
    addRoomLayout->addWidget(m_roomNameEdit);
    addRoomLayout->addWidget(addRoomBtn);
    roomsLayout->addLayout(addRoomLayout);

    m_roomsList = new QListWidget;
    m_roomsList->setObjectName("listWidget");
    roomsLayout->addWidget(m_roomsList);

    // Load rooms into list
    m_rooms = Database::instance().getRooms(m_house.id);
    for (const Room& r : m_rooms) {
        m_roomsList->addItem(r.name);
    }

    QPushButton* removeRoomBtn = new QPushButton("Remove Selected Room");
    removeRoomBtn->setObjectName("dangerBtn");
    roomsLayout->addWidget(removeRoomBtn);

    connect(addRoomBtn, &QPushButton::clicked, this, &EditHouseWindow::onAddRoom);
    connect(removeRoomBtn, &QPushButton::clicked, this, &EditHouseWindow::onRemoveRoom);
    connect(m_roomNameEdit, &QLineEdit::returnPressed, this, &EditHouseWindow::onAddRoom);

    tabs->addTab(roomsTab, "🏠  Rooms");

    // --- Devices Tab ---
    QWidget* devicesTab = new QWidget;
    QVBoxLayout* devLayout = new QVBoxLayout(devicesTab);
    devLayout->setSpacing(12);

    devLayout->addWidget(new QLabel("Select Room:"));
    m_roomSelector = new QComboBox;
    m_roomSelector->setObjectName("comboBox");
    devLayout->addWidget(m_roomSelector);

    QHBoxLayout* addDevLayout = new QHBoxLayout;
    m_deviceNameEdit = new QLineEdit;
    m_deviceNameEdit->setPlaceholderText("Device name");
    m_deviceNameEdit->setObjectName("inputField");
    m_deviceTypeCombo = new QComboBox;
    m_deviceTypeCombo->addItems({"light", "fan", "ac", "tv", "other"});
    m_deviceTypeCombo->setObjectName("comboBox");
    QPushButton* addDevBtn = new QPushButton("Add Device");
    addDevBtn->setObjectName("addBtn");
    addDevLayout->addWidget(m_deviceNameEdit);
    addDevLayout->addWidget(m_deviceTypeCombo);
    devLayout->addLayout(addDevLayout);
    devLayout->addWidget(addDevBtn);

    m_devicesList = new QListWidget;
    m_devicesList->setObjectName("listWidget");
    devLayout->addWidget(m_devicesList);

    QPushButton* removeDevBtn = new QPushButton("Remove Selected Device");
    removeDevBtn->setObjectName("dangerBtn");
    devLayout->addWidget(removeDevBtn);

    // Populate room selector
    for (const Room& r : m_rooms) {
        m_roomSelector->addItem(r.name, r.id);
    }
    onRoomSelected(0);

    connect(addDevBtn, &QPushButton::clicked, this, &EditHouseWindow::onAddDevice);
    connect(removeDevBtn, &QPushButton::clicked, this, &EditHouseWindow::onRemoveDevice);
    connect(m_roomSelector, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &EditHouseWindow::onRoomSelected);

    tabs->addTab(devicesTab, "💡  Devices");

    mainLayout->addWidget(tabs);

    QPushButton* doneBtn = new QPushButton("Done");
    doneBtn->setObjectName("primaryBtn");
    mainLayout->addWidget(doneBtn);
    connect(doneBtn, &QPushButton::clicked, this, &QDialog::accept);
}

void EditHouseWindow::applyStyles() {
    setStyleSheet(R"(
        QWidget { background-color: #0f0f1a; color: #e0e0f0; font-family: 'Segoe UI', sans-serif; }
        QLabel { color: #9ca3af; font-size: 13px; }
        QLabel#pageTitle { font-size: 20px; font-weight: bold; color: #7c9fff; }
        QTabWidget#tabWidget::pane { border: 1.5px solid #2d2d45; border-radius: 8px; }
        QTabBar::tab {
            background: #1e1e2e; color: #9ca3af; padding: 8px 20px;
            border-radius: 6px 6px 0 0; margin-right: 4px;
        }
        QTabBar::tab:selected { background: #2d2d45; color: #7c9fff; }
        QLineEdit#inputField {
            background: #1e1e2e; border: 1.5px solid #2d2d45;
            border-radius: 8px; padding: 9px 12px; font-size: 13px; color: #e0e0f0;
        }
        QLineEdit#inputField:focus { border-color: #7c9fff; }
        QComboBox#comboBox {
            background: #1e1e2e; border: 1.5px solid #2d2d45;
            border-radius: 8px; padding: 8px 12px; color: #e0e0f0;
        }
        QListWidget#listWidget {
            background: #1e1e2e; border: 1.5px solid #2d2d45;
            border-radius: 8px; color: #e0e0f0; font-size: 13px; padding: 4px;
        }
        QListWidget#listWidget::item:selected { background: #2d2d45; border-radius: 4px; }
        QPushButton#primaryBtn {
            background: #7c9fff; color: #0f0f1a; border: none;
            border-radius: 8px; padding: 10px; font-weight: bold;
        }
        QPushButton#primaryBtn:hover { background: #6080ee; }
        QPushButton#addBtn {
            background: #1e3a2e; color: #4ade80; border: 1.5px solid #4ade80;
            border-radius: 8px; padding: 9px 16px;
        }
        QPushButton#addBtn:hover { background: #2a4a3e; }
        QPushButton#dangerBtn {
            background: #2e1e1e; color: #f87171; border: 1.5px solid #f87171;
            border-radius: 8px; padding: 9px 16px;
        }
        QPushButton#dangerBtn:hover { background: #3e2e2e; }
    )");
}

void EditHouseWindow::onAddRoom() {
    QString name = m_roomNameEdit->text().trimmed();
    if (name.isEmpty()) return;
    if (Database::instance().addRoom(m_house.id, name)) {
        m_roomsList->addItem(name);
        m_rooms = Database::instance().getRooms(m_house.id);
        m_roomSelector->addItem(name, m_rooms.last().id);
        m_roomNameEdit->clear();
    }
}

void EditHouseWindow::onRemoveRoom() {
    QListWidgetItem* item = m_roomsList->currentItem();
    if (!item) return;
    QString roomName = item->text();
    for (const Room& r : m_rooms) {
        if (r.name == roomName) {
            Database::instance().removeRoom(r.id);
            // Also remove from combo
            for (int i = 0; i < m_roomSelector->count(); ++i) {
                if (m_roomSelector->itemText(i) == roomName) {
                    m_roomSelector->removeItem(i);
                    break;
                }
            }
            break;
        }
    }
    delete m_roomsList->takeItem(m_roomsList->row(item));
    m_rooms = Database::instance().getRooms(m_house.id);
}

void EditHouseWindow::onRoomSelected(int index) {
    m_devicesList->clear();
    if (index < 0 || m_roomSelector->count() == 0) return;
    refreshDevices();
}

void EditHouseWindow::refreshDevices() {
    m_devicesList->clear();
    int roomId = m_roomSelector->currentData().toInt();
    QList<Device> devices = Database::instance().getDevices(roomId);
    for (const Device& d : devices) {
        m_devicesList->addItem(QString("[%1] %2").arg(d.type).arg(d.name));
    }
}

void EditHouseWindow::onAddDevice() {
    QString name = m_deviceNameEdit->text().trimmed();
    if (name.isEmpty() || m_roomSelector->count() == 0) return;
    int roomId = m_roomSelector->currentData().toInt();
    QString type = m_deviceTypeCombo->currentText();
    if (Database::instance().addDevice(roomId, name, type)) {
        m_deviceNameEdit->clear();
        refreshDevices();
    }
}

void EditHouseWindow::onRemoveDevice() {
    QListWidgetItem* item = m_devicesList->currentItem();
    if (!item) return;
    int roomId = m_roomSelector->currentData().toInt();
    QList<Device> devices = Database::instance().getDevices(roomId);
    QString text = item->text();
    for (const Device& d : devices) {
        if (text == QString("[%1] %2").arg(d.type).arg(d.name)) {
            Database::instance().removeDevice(d.id);
            break;
        }
    }
    refreshDevices();
}