#pragma once
#include <QString>
#include <QList>
#include <QDateTime>
#include <QMap>
#include <QPair>

struct Device {
    int id;
    int roomId;
    QString name;
    bool isOn;
    QString type; // light, fan, ac, tv
};

struct Room {
    int id;
    int houseId;
    QString name;
    QList<Device> devices;
};

struct House {
    int id;
    QString houseNumber;
    QString ownerName;   // ← NEW
    QString password;
    QList<Room> rooms;
};

struct ActivityLog {
    int id;
    int deviceId;
    QString deviceName;
    QString action;
    QDateTime timestamp;
};

struct Schedule {
    int id;
    int deviceId;
    QString deviceName;
    bool turnOn;
    QDateTime scheduledTime;
    bool executed;
};

class Database {
public:
    static Database& instance();
    bool initialize();

    // House operations
    bool registerHouse(const QString& houseNumber, const QString& ownerName, const QString& password); // ← updated
    bool validateLogin(const QString& houseNumber, const QString& password);
    bool houseExists(const QString& houseNumber);
    House getHouse(const QString& houseNumber);
    QStringList getAllHouseNumbers();
    bool changePassword(const QString& houseNumber, const QString& newPassword);

    // Login attempt tracking (lockout after 3 failed attempts)
    int  getFailedAttempts(const QString& houseNumber);
    void recordFailedAttempt(const QString& houseNumber);
    void resetFailedAttempts(const QString& houseNumber);
    bool isAccountLocked(const QString& houseNumber);
    QDateTime getLockoutExpiry(const QString& houseNumber);

    // Room operations
    bool addRoom(int houseId, const QString& roomName);
    bool removeRoom(int roomId);
    QList<Room> getRooms(int houseId);

    // Device operations
    bool addDevice(int roomId, const QString& name, const QString& type);
    bool removeDevice(int deviceId);
    bool renameDevice(int deviceId, const QString& newName);
    bool setDeviceState(int deviceId, bool isOn);
    QList<Device> getDevices(int roomId);
    Device getDevice(int deviceId);

    // Activity log
    bool addActivityLog(int deviceId, const QString& deviceName, const QString& action);
    QList<ActivityLog> getActivityLogs(int houseId, int limit = 50);

    // Schedule
    bool addSchedule(int deviceId, const QString& deviceName, bool turnOn, const QDateTime& scheduledTime);
    QList<Schedule> getPendingSchedules();
    bool markScheduleExecuted(int scheduleId);
    QList<Schedule> getSchedules(int houseId);
    bool deleteSchedule(int scheduleId);

    // Usage stats (minutes device was on per day)
    bool recordUsage(int deviceId, int minutes);
    double getDailyUsageHours(int houseId);
    double getWeeklyUsageHours(int houseId);
    double getMonthlyUsageHours(int houseId);
    double getEstimatedBill(int houseId);

    // Returns per-device usage for PDF export: device name → total minutes this month
    QList<QPair<QString,int>> getMonthlyUsageDetails(int houseId);

private:
    Database() = default;
    bool executeQuery(const QString& sql);
    QString m_dbPath;
};