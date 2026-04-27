#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>

class LoginWindow : public QWidget {
    Q_OBJECT
public:
    explicit LoginWindow(QWidget* parent = nullptr);

private slots:
    void onLogin();
    void onRegister();
    void onEditHouse();
    void onViewHouses();
    void onToggleTheme();

private:
    void setupUI();
    void applyDarkTheme();
    void applyLightTheme();

    QLineEdit* m_houseNumberEdit;
    QLineEdit* m_passwordEdit;
    QPushButton* m_loginBtn;
    QPushButton* m_registerBtn;
    QPushButton* m_editHouseBtn;
    QPushButton* m_viewHousesBtn;
    QPushButton* m_themeBtn;
    QLabel* m_statusLabel;
    bool m_darkMode = true;
};