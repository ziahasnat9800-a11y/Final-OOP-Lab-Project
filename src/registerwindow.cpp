#include "registerwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QMessageBox>

RegisterWindow::RegisterWindow(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Register New House");
    setFixedSize(480, 560);
    setModal(true);

    m_stack = new QStackedWidget(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(m_stack);

    setupCredentialsPage();
    setupRoomsPage();
    setupDevicesPage();
    applyStyles();
}

void RegisterWindow::setupCredentialsPage() {
    QWidget* page = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(page);
    layout->setContentsMargins(40, 40, 40, 40);
    layout->setSpacing(14);

    QLabel* title = new QLabel("Register Your House");
    title->setObjectName("pageTitle");
    title->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);

    QLabel* step = new QLabel("Step 1 of 3 — Credentials");
    step->setObjectName("stepLabel");
    step->setAlignment(Qt::AlignCenter);
    layout->addWidget(step);

    layout->addSpacing(10);

    // ── Owner Name (NEW) ──────────────────────────────────────────────────────
    layout->addWidget(new QLabel("Owner Name:"));
    m_ownerNameEdit = new QLineEdit;
    m_ownerNameEdit->setPlaceholderText("Enter your full name");
    m_ownerNameEdit->setObjectName("inputField");
    layout->addWidget(m_ownerNameEdit);

    layout->addWidget(new QLabel("House Number:"));
    m_houseNumEdit = new QLineEdit;
    m_houseNumEdit->setPlaceholderText("Enter House Number (1-1000)");
    m_houseNumEdit->setObjectName("inputField");
    layout->addWidget(m_houseNumEdit);

    layout->addWidget(new QLabel("Password:"));
    m_passEdit = new QLineEdit;
    m_passEdit->setEchoMode(QLineEdit::Password);
    m_passEdit->setObjectName("inputField");
    layout->addWidget(m_passEdit);

    layout->addWidget(new QLabel("Confirm Password:"));
    m_confirmPassEdit = new QLineEdit;
    m_confirmPassEdit->setEchoMode(QLineEdit::Password);
    m_confirmPassEdit->setObjectName("inputField");
    layout->addWidget(m_confirmPassEdit);

    m_credStatus = new QLabel("");
    m_credStatus->setObjectName("statusLabel");
    m_credStatus->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_credStatus);

    layout->addStretch();

    QPushButton* nextBtn = new QPushButton("Next →");
    nextBtn->setObjectName("primaryBtn");
    layout->addWidget(nextBtn);
    connect(nextBtn, &QPushButton::clicked, this, &RegisterWindow::onNextFromCredentials);

    m_stack->addWidget(page);
}

void RegisterWindow::setupRoomsPage() {
    QWidget* page = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(page);
    layout->setContentsMargins(40, 40, 40, 40);
    layout->setSpacing(14);

    QLabel* title = new QLabel("Add Rooms");
    title->setObjectName("pageTitle");
    title->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);

    QLabel* step = new QLabel("Step 2 of 3 — Rooms");
    step->setObjectName("stepLabel");
    step->setAlignment(Qt::AlignCenter);
    layout->addWidget(step);

    layout->addSpacing(6);

    QHBoxLayout* addLayout = new QHBoxLayout;
    m_roomNameEdit = new QLineEdit;
    m_roomNameEdit->setPlaceholderText("Room name (e.g. Bedroom, Kitchen)");
    m_roomNameEdit->setObjectName("inputField");
    QPushButton* addRoomBtn = new QPushButton("Add");
    addRoomBtn->setObjectName("addBtn");
    addLayout->addWidget(m_roomNameEdit);
    addLayout->addWidget(addRoomBtn);
    layout->addLayout(addLayout);

    m_roomsList = new QListWidget;
    m_roomsList->setObjectName("listWidget");
    layout->addWidget(m_roomsList);

    m_roomStatus = new QLabel("");
    m_roomStatus->setObjectName("statusLabel");
    layout->addWidget(m_roomStatus);

    QHBoxLayout* btnLayout = new QHBoxLayout;
    QPushButton* removeBtn = new QPushButton("Remove Selected");
    removeBtn->setObjectName("dangerBtn");
    QPushButton* nextBtn = new QPushButton("Next →");
    nextBtn->setObjectName("primaryBtn");
    btnLayout->addWidget(removeBtn);
    btnLayout->addWidget(nextBtn);
    layout->addLayout(btnLayout);

    connect(addRoomBtn, &QPushButton::clicked, this, &RegisterWindow::onAddRoom);
    connect(removeBtn,  &QPushButton::clicked, this, &RegisterWindow::onRemoveRoom);
    connect(nextBtn,    &QPushButton::clicked, this, &RegisterWindow::onNextFromRooms);
    connect(m_roomNameEdit, &QLineEdit::returnPressed, this, &RegisterWindow::onAddRoom);

    m_stack->addWidget(page);
}

void RegisterWindow::setupDevicesPage() {
    QWidget* page = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(page);
    layout->setContentsMargins(40, 40, 40, 40);
    layout->setSpacing(14);

    QLabel* title = new QLabel("Add Devices");
    title->setObjectName("pageTitle");
    title->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);

    QLabel* step = new QLabel("Step 3 of 3 — Devices");
    step->setObjectName("stepLabel");
    step->setAlignment(Qt::AlignCenter);
    layout->addWidget(step);

    layout->addSpacing(6);

    layout->addWidget(new QLabel("Select Room:"));
    m_roomSelector = new QComboBox;
    m_roomSelector->setObjectName("comboBox");
    layout->addWidget(m_roomSelector);

    QHBoxLayout* devLayout = new QHBoxLayout;
    m_deviceNameEdit = new QLineEdit;
    m_deviceNameEdit->setPlaceholderText("Device name (e.g. Ceiling Light)");
    m_deviceNameEdit->setObjectName("inputField");
    m_deviceTypeCombo = new QComboBox;
    m_deviceTypeCombo->addItems({"light", "fan", "ac", "tv", "other"});
    m_deviceTypeCombo->setObjectName("comboBox");
    devLayout->addWidget(m_deviceNameEdit);
    devLayout->addWidget(m_deviceTypeCombo);
    layout->addLayout(devLayout);

    QPushButton* addDevBtn = new QPushButton("Add Device");
    addDevBtn->setObjectName("addBtn");
    layout->addWidget(addDevBtn);

    m_devicesList = new QListWidget;
    m_devicesList->setObjectName("listWidget");
    layout->addWidget(m_devicesList);

    QHBoxLayout* btnLayout = new QHBoxLayout;
    QPushButton* removeDevBtn = new QPushButton("Remove Selected");
    removeDevBtn->setObjectName("dangerBtn");
    QPushButton* finishBtn = new QPushButton("Finish ✓");
    finishBtn->setObjectName("primaryBtn");
    btnLayout->addWidget(removeDevBtn);
    btnLayout->addWidget(finishBtn);
    layout->addLayout(btnLayout);

    connect(addDevBtn,    &QPushButton::clicked, this, &RegisterWindow::onAddDevice);
    connect(removeDevBtn, &QPushButton::clicked, this, &RegisterWindow::onRemoveDevice);
    connect(finishBtn,    &QPushButton::clicked, this, &RegisterWindow::onFinish);
    connect(m_roomSelector, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &RegisterWindow::onRoomSelected);

    m_stack->addWidget(page);
}

void RegisterWindow::applyStyles() {
    setStyleSheet(R"(
        QWidget { background-color: #0f0f1a; color: #e0e0f0; font-family: 'Segoe UI', sans-serif; }
        QLabel { color: #9ca3af; font-size: 13px; }
        QLabel#pageTitle { font-size: 22px; font-weight: bold; color: #7c9fff; }
        QLabel#stepLabel { font-size: 12px; color: #6b7280; }
        QLabel#statusLabel { color: #ef4444; font-size: 12px; }
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
            border-radius: 8px; color: #e0e0f0; font-size: 13px;
        }
        QListWidget#listWidget::item:selected { background: #2d2d45; }
        QPushButton#primaryBtn {
            background: #7c9fff; color: #0f0f1a; border: none;
            border-radius: 8px; padding: 10px; font-weight: bold; font-size: 14px;
        }
        QPushButton#primaryBtn:hover { background: #6080ee; }
        QPushButton#addBtn {
            background: #1e3a2e; color: #4ade80; border: 1.5px solid #4ade80;
            border-radius: 8px; padding: 9px 16px; font-size: 13px;
        }
        QPushButton#addBtn:hover { background: #2a4a3e; }
        QPushButton#dangerBtn {
            background: #2e1e1e; color: #f87171; border: 1.5px solid #f87171;
            border-radius: 8px; padding: 9px 16px; font-size: 13px;
        }
        QPushButton#dangerBtn:hover { background: #3e2e2e; }
    )");
}

// ── Slot implementations ──────────────────────────────────────────────────────

void RegisterWindow::onNextFromCredentials() {
    QString ownerName = m_ownerNameEdit->text().trimmed();
    QString houseNum  = m_houseNumEdit->text().trimmed();
    QString pass      = m_passEdit->text();
    QString confirm   = m_confirmPassEdit->text();

    if (ownerName.isEmpty()) {
        m_credStatus->setText("Please enter the owner name."); return;
    }
    if (houseNum.isEmpty() || pass.isEmpty()) {
        m_credStatus->setText("All fields are required."); return;
    }
    if (pass != confirm) {
        m_credStatus->setText("Passwords do not match."); return;
    }
    if (Database::instance().houseExists(houseNum)) {
        m_credStatus->setText("House number already registered."); return;
    }
    if (Database::instance().registerHouse(houseNum, ownerName, pass)) {
        m_houseNumber = houseNum;
        House house   = Database::instance().getHouse(houseNum);
        m_houseId     = house.id;
        m_stack->setCurrentIndex(1);
    } else {
        m_credStatus->setText("Registration failed. Try again.");
    }
}

void RegisterWindow::onAddRoom() {
    QString name = m_roomNameEdit->text().trimmed();
    if (name.isEmpty()) return;
    if (Database::instance().addRoom(m_houseId, name)) {
        m_roomsList->addItem(name);
        m_roomNameEdit->clear();
        m_roomStatus->clear();
    }
}

void RegisterWindow::onRemoveRoom() {
    QListWidgetItem* item = m_roomsList->currentItem();
    if (!item) return;
    m_rooms = Database::instance().getRooms(m_houseId);
    for (const Room& r : m_rooms) {
        if (r.name == item->text()) {
            Database::instance().removeRoom(r.id);
            break;
        }
    }
    delete m_roomsList->takeItem(m_roomsList->row(item));
}

void RegisterWindow::onNextFromRooms() {
    if (m_roomsList->count() == 0) {
        m_roomStatus->setText("Add at least one room."); return;
    }
    m_rooms = Database::instance().getRooms(m_houseId);
    m_roomSelector->clear();
    for (const Room& r : m_rooms)
        m_roomSelector->addItem(r.name, r.id);
    onRoomSelected(0);
    m_stack->setCurrentIndex(2);
}

void RegisterWindow::onRoomSelected(int index) {
    m_devicesList->clear();
    if (index < 0 || m_rooms.isEmpty()) return;
    int roomId = m_roomSelector->itemData(index).toInt();
    QList<Device> devices = Database::instance().getDevices(roomId);
    for (const Device& d : devices)
        m_devicesList->addItem(QString("[%1] %2").arg(d.type).arg(d.name));
}

void RegisterWindow::onAddDevice() {
    QString name = m_deviceNameEdit->text().trimmed();
    if (name.isEmpty()) return;
    int roomId   = m_roomSelector->currentData().toInt();
    QString type = m_deviceTypeCombo->currentText();
    if (Database::instance().addDevice(roomId, name, type)) {
        m_devicesList->addItem(QString("[%1] %2").arg(type).arg(name));
        m_deviceNameEdit->clear();
    }
}

void RegisterWindow::onRemoveDevice() {
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
    delete m_devicesList->takeItem(m_devicesList->row(item));
}

void RegisterWindow::onFinish() {
    QMessageBox::information(this, "Success",
                             "House registered successfully!\nYou can now log in with house number: " + m_houseNumber);
    accept();
}