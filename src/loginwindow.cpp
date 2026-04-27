#include "loginwindow.h"
#include "database.h"
#include "registerwindow.h"
#include "edithousewindow.h"
#include "dashboardwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QFrame>
#include <QApplication>
#include <QScreen>
#include <QInputDialog>
#include <QFont>
#include <QDialog>
#include <QListWidget>
#include <QCryptographicHash>

static const int MAX_ATTEMPTS = 3;

LoginWindow::LoginWindow(QWidget* parent) : QWidget(parent) {
    setWindowTitle("Smart Home - Login");
    setFixedSize(420, 640);
    setupUI();
    applyDarkTheme();

    QRect screen = QApplication::primaryScreen()->geometry();
    move((screen.width() - width()) / 2, (screen.height() - height()) / 2);
}

void LoginWindow::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(40, 36, 40, 36);
    mainLayout->setSpacing(0);

    // Theme toggle — top right
    QHBoxLayout* topRow = new QHBoxLayout;
    topRow->addStretch();
    m_themeBtn = new QPushButton("☀ Light", this);
    m_themeBtn->setObjectName("themeBtn");
    m_themeBtn->setFixedSize(80, 28);
    m_themeBtn->setCursor(Qt::PointingHandCursor);
    topRow->addWidget(m_themeBtn);
    mainLayout->addLayout(topRow);

    // Icon
    QLabel* icon = new QLabel("🏠", this);
    icon->setAlignment(Qt::AlignCenter);
    icon->setFixedHeight(60);
    icon->setStyleSheet("font-size: 48px; background: transparent;");
    mainLayout->addWidget(icon);

    mainLayout->addSpacing(4);

    // Title
    QLabel* title = new QLabel("Smart Home\nElectricity Control & Monitoring System", this);
    title->setObjectName("title");
    title->setAlignment(Qt::AlignCenter);
    title->setFixedHeight(80);
    mainLayout->addWidget(title);

    // Subtitle
    QLabel* subtitle = new QLabel("Control your house from anywhere", this);
    subtitle->setObjectName("subtitle");
    subtitle->setAlignment(Qt::AlignCenter);
    subtitle->setFixedHeight(24);
    mainLayout->addWidget(subtitle);

    mainLayout->addSpacing(20);

    // House Number
    QLabel* houseLabel = new QLabel("House Number", this);
    houseLabel->setObjectName("fieldLabel");
    houseLabel->setFixedHeight(22);
    mainLayout->addWidget(houseLabel);
    mainLayout->addSpacing(6);

    m_houseNumberEdit = new QLineEdit(this);
    m_houseNumberEdit->setPlaceholderText("Enter House Number");
    m_houseNumberEdit->setObjectName("inputField");
    m_houseNumberEdit->setFixedHeight(46);
    mainLayout->addWidget(m_houseNumberEdit);

    mainLayout->addSpacing(16);

    // Password
    QLabel* passLabel = new QLabel("Password", this);
    passLabel->setObjectName("fieldLabel");
    passLabel->setFixedHeight(22);
    mainLayout->addWidget(passLabel);
    mainLayout->addSpacing(6);

    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setPlaceholderText("Enter password");
    m_passwordEdit->setObjectName("inputField");
    m_passwordEdit->setFixedHeight(46);
    mainLayout->addWidget(m_passwordEdit);

    mainLayout->addSpacing(10);

    // Status
    m_statusLabel = new QLabel("", this);
    m_statusLabel->setObjectName("statusLabel");
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setFixedHeight(20);
    mainLayout->addWidget(m_statusLabel);

    mainLayout->addSpacing(12);

    // Login button
    m_loginBtn = new QPushButton("Login", this);
    m_loginBtn->setObjectName("primaryBtn");
    m_loginBtn->setFixedHeight(48);
    m_loginBtn->setCursor(Qt::PointingHandCursor);
    mainLayout->addWidget(m_loginBtn);

    mainLayout->addSpacing(16);

    // Separator
    QFrame* line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setObjectName("separator");
    line->setFixedHeight(1);
    mainLayout->addWidget(line);

    mainLayout->addSpacing(16);

    // Bottom buttons
    QHBoxLayout* bottomLayout = new QHBoxLayout();
    bottomLayout->setSpacing(8);

    m_registerBtn = new QPushButton("Register", this);
    m_registerBtn->setObjectName("secondaryBtn");
    m_registerBtn->setFixedHeight(44);
    m_registerBtn->setCursor(Qt::PointingHandCursor);

    m_editHouseBtn = new QPushButton("Edit House", this);
    m_editHouseBtn->setObjectName("secondaryBtn");
    m_editHouseBtn->setFixedHeight(44);
    m_editHouseBtn->setCursor(Qt::PointingHandCursor);

    m_viewHousesBtn = new QPushButton("View Houses", this);
    m_viewHousesBtn->setObjectName("secondaryBtn");
    m_viewHousesBtn->setFixedHeight(44);
    m_viewHousesBtn->setCursor(Qt::PointingHandCursor);

    bottomLayout->addWidget(m_registerBtn);
    bottomLayout->addWidget(m_editHouseBtn);
    bottomLayout->addWidget(m_viewHousesBtn);
    mainLayout->addLayout(bottomLayout);

    connect(m_loginBtn,        &QPushButton::clicked, this, &LoginWindow::onLogin);
    connect(m_registerBtn,     &QPushButton::clicked, this, &LoginWindow::onRegister);
    connect(m_editHouseBtn,    &QPushButton::clicked, this, &LoginWindow::onEditHouse);
    connect(m_viewHousesBtn,   &QPushButton::clicked, this, &LoginWindow::onViewHouses);
    connect(m_themeBtn,        &QPushButton::clicked, this, &LoginWindow::onToggleTheme);
    connect(m_passwordEdit,    &QLineEdit::returnPressed, this, &LoginWindow::onLogin);
    connect(m_houseNumberEdit, &QLineEdit::returnPressed,
            m_passwordEdit, QOverload<>::of(&QWidget::setFocus));
}

// ── THEMES ───────────────────────────────────────────────────────────────────

void LoginWindow::applyDarkTheme() {
    m_darkMode = true;
    m_themeBtn->setText("☀ Light");
    setStyleSheet(R"(
        QWidget { background-color: #0f0f1a; color: #e8eaf6; font-family: 'Segoe UI', sans-serif; }
        QLabel#title { font-size: 14px; font-weight: bold; color: #7c9fff; letter-spacing: 0.5px; }
        QLabel#subtitle { font-size: 12px; color: #8892b0; }
        QLabel#fieldLabel { font-size: 13px; font-weight: 600; color: #cdd6f4; }
        QLabel#statusLabel { font-size: 12px; color: #f38ba8; }
        QLineEdit#inputField {
            background-color: #181825; border: 1.5px solid #45475a;
            border-radius: 8px; padding-left: 14px; font-size: 13px; color: #cdd6f4;
        }
        QLineEdit#inputField:focus { border-color: #7c9fff; background-color: #1e1e2e; }
        QLineEdit#inputField:hover { border-color: #585b70; }
        QPushButton#primaryBtn {
            background-color: #7c9fff; color: #11111b; border: none;
            border-radius: 8px; font-size: 14px; font-weight: bold;
        }
        QPushButton#primaryBtn:hover { background-color: #89b4fa; }
        QPushButton#primaryBtn:pressed { background-color: #6c8fef; }
        QPushButton#secondaryBtn {
            background-color: #181825; color: #cdd6f4; border: 1.5px solid #45475a;
            border-radius: 8px; font-size: 12px; font-weight: 600;
        }
        QPushButton#secondaryBtn:hover { background-color: #1e1e2e; border-color: #7c9fff; color: #7c9fff; }
        QPushButton#themeBtn {
            background-color: #1e1e2e; color: #fbbf24; border: 1.5px solid #fbbf24;
            border-radius: 6px; font-size: 11px; font-weight: 600;
        }
        QPushButton#themeBtn:hover { background-color: #2d2d45; }
        QFrame#separator { background-color: #313244; border: none; }
    )");
}

void LoginWindow::applyLightTheme() {
    m_darkMode = false;
    m_themeBtn->setText("🌙 Dark");
    setStyleSheet(R"(
        QWidget { background-color: #f0f4ff; color: #1e1e2e; font-family: 'Segoe UI', sans-serif; }
        QLabel#title { font-size: 14px; font-weight: bold; color: #3b6be8; letter-spacing: 0.5px; }
        QLabel#subtitle { font-size: 12px; color: #5566aa; }
        QLabel#fieldLabel { font-size: 13px; font-weight: 600; color: #1e1e2e; }
        QLabel#statusLabel { font-size: 12px; color: #dc2626; }
        QLineEdit#inputField {
            background-color: #ffffff; border: 1.5px solid #c0c8e0;
            border-radius: 8px; padding-left: 14px; font-size: 13px; color: #1e1e2e;
        }
        QLineEdit#inputField:focus { border-color: #3b6be8; background-color: #eef2ff; }
        QLineEdit#inputField:hover { border-color: #8899cc; }
        QPushButton#primaryBtn {
            background-color: #3b6be8; color: #ffffff; border: none;
            border-radius: 8px; font-size: 14px; font-weight: bold;
        }
        QPushButton#primaryBtn:hover { background-color: #2255cc; }
        QPushButton#primaryBtn:pressed { background-color: #1a44bb; }
        QPushButton#secondaryBtn {
            background-color: #ffffff; color: #2d2d45; border: 1.5px solid #c0c8e0;
            border-radius: 8px; font-size: 12px; font-weight: 600;
        }
        QPushButton#secondaryBtn:hover { background-color: #eef2ff; border-color: #3b6be8; color: #3b6be8; }
        QPushButton#themeBtn {
            background-color: #e8eeff; color: #3b6be8; border: 1.5px solid #3b6be8;
            border-radius: 6px; font-size: 11px; font-weight: 600;
        }
        QPushButton#themeBtn:hover { background-color: #d0d8ff; }
        QFrame#separator { background-color: #c0c8e0; border: none; }
    )");
}

void LoginWindow::onToggleTheme() {
    if (m_darkMode) applyLightTheme();
    else            applyDarkTheme();
}

// ── LOGIN ────────────────────────────────────────────────────────────────────

void LoginWindow::onLogin() {
    QString houseNum = m_houseNumberEdit->text().trimmed();
    QString password = m_passwordEdit->text();

    if (houseNum.isEmpty() || password.isEmpty()) {
        m_statusLabel->setText("Please fill in all fields.");
        return;
    }

    // Check if house exists — but do NOT reveal this to the user in error messages
    bool exists = Database::instance().houseExists(houseNum);

    // Check lockout only if house exists
    if (exists && Database::instance().isAccountLocked(houseNum)) {
        QDateTime expiry = Database::instance().getLockoutExpiry(houseNum);
        int minutesLeft = (int)(QDateTime::currentDateTime().secsTo(expiry) / 60) + 1;
        m_statusLabel->setText(
            QString("Account locked. Try again in %1 minute(s).").arg(minutesLeft));
        m_passwordEdit->clear();
        return;
    }

    if (Database::instance().validateLogin(houseNum, password)) {
        // Successful login
        Database::instance().resetFailedAttempts(houseNum);
        House house = Database::instance().getHouse(houseNum);
        DashboardWindow* dashboard = new DashboardWindow(house);
        dashboard->show();
        this->hide();
        connect(dashboard, &DashboardWindow::logoutRequested, this, [this, dashboard]() {
            dashboard->close();
            m_passwordEdit->clear();
            m_statusLabel->clear();
            this->show();
        });
    } else {
        // Only record failed attempt if house actually exists
        // This prevents anyone from locking others' accounts by guessing house numbers
        if (exists) {
            Database::instance().recordFailedAttempt(houseNum);
            int attempts = Database::instance().getFailedAttempts(houseNum);
            int remaining = MAX_ATTEMPTS - attempts;
            if (Database::instance().isAccountLocked(houseNum)) {
                m_statusLabel->setText("Account locked for 2 hours after 3 failed attempts.");
            } else {
                m_statusLabel->setText(
                    QString("Invalid credentials. %1 attempt(s) remaining before lockout.")
                        .arg(remaining));
            }
        } else {
            // House doesn't exist — give same generic message, no lockout recorded
            m_statusLabel->setText("Invalid house number or password.");
        }
        m_passwordEdit->clear();
    }
}

// ── REGISTER ─────────────────────────────────────────────────────────────────

void LoginWindow::onRegister() {
    RegisterWindow* regWin = new RegisterWindow(this);
    regWin->exec();
}

// ── EDIT HOUSE ───────────────────────────────────────────────────────────────

void LoginWindow::onEditHouse() {
    QString houseNum = m_houseNumberEdit->text().trimmed();
    if (houseNum.isEmpty()) {
        m_statusLabel->setText("Enter house number to edit.");
        return;
    }

    bool ok;
    QString pass = QInputDialog::getText(this, "Authentication",
                                         "Enter password:",
                                         QLineEdit::Password, "", &ok);
    if (!ok || pass.isEmpty()) return;

    // Use same generic message whether house doesn't exist or password is wrong
    if (!Database::instance().validateLogin(houseNum, pass)) {
        m_statusLabel->setText("Invalid house number or password.");
        return;
    }

    QStringList options = {"Edit Rooms & Devices", "Change Password"};
    QString choice = QInputDialog::getItem(this, "Edit House",
                                           "What would you like to do?",
                                           options, 0, false, &ok);
    if (!ok) return;

    if (choice == "Change Password") {
        QString newPass = QInputDialog::getText(this, "Change Password",
                                                "Enter new password:",
                                                QLineEdit::Password, "", &ok);
        if (!ok || newPass.isEmpty()) return;
        QString confirm = QInputDialog::getText(this, "Change Password",
                                                "Confirm new password:",
                                                QLineEdit::Password, "", &ok);
        if (!ok) return;
        if (newPass != confirm) {
            QMessageBox::warning(this, "Mismatch", "Passwords do not match.");
            return;
        }
        if (Database::instance().changePassword(houseNum, newPass)) {
            QMessageBox::information(this, "Success", "Password changed successfully.");
        } else {
            QMessageBox::critical(this, "Error", "Failed to change password.");
        }
    } else {
        House house = Database::instance().getHouse(houseNum);
        EditHouseWindow* editWin = new EditHouseWindow(house, this);
        editWin->exec();
    }
}

// ── VIEW HOUSES (protected by master password) ────────────────────────────────

void LoginWindow::onViewHouses() {
    bool ok;
    QString masterPass = QInputDialog::getText(this, "View Houses",
                                               "Enter master password to view registered houses:",
                                               QLineEdit::Password, "", &ok);
    if (!ok || masterPass.isEmpty()) return;

    // Master password: "admin1234" — change this string to whatever you want
    QString hashedInput  = QString(QCryptographicHash::hash(
                                      masterPass.toUtf8(), QCryptographicHash::Sha256).toHex());
    QString hashedMaster = QString(QCryptographicHash::hash(
                                       QString("admin1234").toUtf8(), QCryptographicHash::Sha256).toHex());

    if (hashedInput != hashedMaster) {
        m_statusLabel->setText("Incorrect master password.");
        return;
    }

    QStringList houses = Database::instance().getAllHouseNumbers();

    QDialog* dlg = new QDialog(this);
    dlg->setWindowTitle("Registered Houses");
    dlg->setFixedSize(300, 360);
    dlg->setStyleSheet(R"(
        QDialog { background-color: #0f0f1a; }
        QLabel  { color: #7c9fff; font-size: 16px; font-weight: bold; font-family: 'Segoe UI', sans-serif; }
        QListWidget {
            background: #1e1e2e; border: 1.5px solid #2d2d45;
            border-radius: 8px; color: #cdd6f4; font-size: 13px;
            font-family: 'Segoe UI', sans-serif; padding: 4px;
        }
        QListWidget::item { padding: 6px 10px; }
        QListWidget::item:selected { background: #2d2d45; border-radius: 4px; }
        QPushButton {
            background: #7c9fff; color: #0f0f1a; border: none;
            border-radius: 8px; padding: 9px; font-weight: bold;
            font-family: 'Segoe UI', sans-serif;
        }
        QPushButton:hover { background: #89b4fa; }
    )");

    QVBoxLayout* layout = new QVBoxLayout(dlg);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(14);

    QLabel* titleLbl = new QLabel("Registered Houses");
    titleLbl->setAlignment(Qt::AlignCenter);
    layout->addWidget(titleLbl);

    QListWidget* list = new QListWidget;
    if (houses.isEmpty()) {
        list->addItem("No houses registered yet.");
    } else {
        for (const QString& h : houses)
            list->addItem("🏠  " + h);
    }
    layout->addWidget(list);

    QPushButton* closeBtn = new QPushButton("Close");
    connect(closeBtn, &QPushButton::clicked, dlg, &QDialog::accept);
    layout->addWidget(closeBtn);

    dlg->exec();
    dlg->deleteLater();
}