#pragma once
#include <QWidget>
#include <QLabel>
#include <QListWidget>
#include <QVBoxLayout>
#include <QTimer>
#include <QMap>
#include <QDateTime>
#include <QStringList>
#include "database.h"

class DeviceCard;

class DashboardWindow : public QWidget {
    Q_OBJECT
public:
    explicit DashboardWindow(const House& house, QWidget* parent = nullptr);

signals:
    void logoutRequested();

private slots:
    void refreshAll();
    void onScheduleTimerTick();
    void onUsageTimerTick();
    void openScheduleDialog();
    void onLogout();
    void onDeleteSchedule();
    void onRenameDevice();
    void onExportPDF();
    void onAbout();
    void onTurnAllOn();
    void onTurnAllOff();
    void onNightMode();
    void onAwayMode();
    void onClockTick();

private:
    void setupUI();
    void applyStyles();
    void buildDeviceCards();
    void toggleDevice(int deviceId, const QString& deviceName, const QString& type);
    void refreshLogs();
    void refreshUsage();
    void refreshSchedules();
    void updateArduinoForDevice(int deviceId, bool isOn, const QString& type);
    void flushUsage();
    void updateDevicesOnBadge();
    void drawUsageCharts();

    House m_house;

    // Top bar
    QLabel* m_devicesOnLabel;
    QLabel* m_connectionDot;
    QLabel* m_arduinoStatusLabel;
    QLabel* m_arduinoPortLabel;

    // Welcome card
    QLabel* m_greetingLabel;
    QLabel* m_dateTimeLabel;

    // Devices area
    QWidget*     m_devicesContainer;
    QVBoxLayout* m_devicesLayout;

    // Usage KPI labels
    QLabel* m_dailyLabel;
    QLabel* m_weeklyLabel;
    QLabel* m_monthlyLabel;
    QLabel* m_billLabel;

    // Charts canvas
    QWidget* m_chartsCanvas;

    // Panels
    QListWidget* m_logsList;
    QWidget*     m_schedulesContainer;
    QVBoxLayout* m_schedulesLayout;

    QTimer* m_scheduleTimer;
    QTimer* m_usageTimer;
    QTimer* m_clockTimer;

    QMap<int, QDateTime>   m_deviceOnTimes;
    QMap<int, DeviceCard*> m_deviceCards;
    QMap<int, int>         m_scheduleRowToId;
    QMap<int, QLabel*>     m_roomBadges;     // roomId → per-room "X/Y ON" badge label

    // Charts data
    QList<double>              m_dailySeries;
    QList<QPair<QString,double>> m_breakdown;
};