#include "database.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QDir>
#include <QStandardPaths>
#include <QCryptographicHash>

Database& Database::instance() {
    static Database db;
    return db;
}

static QString hashPassword(const QString& password) {
    return QString(QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256).toHex());
}

bool Database::initialize() {
    m_dbPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(m_dbPath);
    m_dbPath += "/smarthome.db";

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(m_dbPath);

    if (!db.open()) {
        qDebug() << "Cannot open database:" << db.lastError().text();
        return false;
    }

    // Enable foreign keys
    executeQuery("PRAGMA foreign_keys = ON");

    // Create tables
    executeQuery(R"(
        CREATE TABLE IF NOT EXISTS houses (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            house_number TEXT UNIQUE NOT NULL,
            owner_name TEXT NOT NULL DEFAULT '',
            password TEXT NOT NULL
        )
    )");

    // Migrate existing databases that don't yet have owner_name column
    executeQuery("ALTER TABLE houses ADD COLUMN owner_name TEXT NOT NULL DEFAULT ''");

    executeQuery(R"(
        CREATE TABLE IF NOT EXISTS rooms (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            house_id INTEGER NOT NULL,
            name TEXT NOT NULL,
            FOREIGN KEY(house_id) REFERENCES houses(id)
        )
    )");

    executeQuery(R"(
        CREATE TABLE IF NOT EXISTS devices (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            room_id INTEGER NOT NULL,
            name TEXT NOT NULL,
            type TEXT NOT NULL DEFAULT 'light',
            is_on INTEGER NOT NULL DEFAULT 0,
            FOREIGN KEY(room_id) REFERENCES rooms(id)
        )
    )");

    executeQuery(R"(
        CREATE TABLE IF NOT EXISTS activity_logs (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            device_id INTEGER NOT NULL,
            device_name TEXT NOT NULL,
            action TEXT NOT NULL,
            timestamp TEXT NOT NULL
        )
    )");

    executeQuery(R"(
        CREATE TABLE IF NOT EXISTS schedules (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            device_id INTEGER NOT NULL,
            device_name TEXT NOT NULL,
            turn_on INTEGER NOT NULL,
            scheduled_time TEXT NOT NULL,
            executed INTEGER NOT NULL DEFAULT 0
        )
    )");

    executeQuery(R"(
        CREATE TABLE IF NOT EXISTS usage_records (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            device_id INTEGER NOT NULL,
            date TEXT NOT NULL,
            minutes INTEGER NOT NULL DEFAULT 0
        )
    )");

    executeQuery(R"(
        CREATE TABLE IF NOT EXISTS login_attempts (
            house_number TEXT PRIMARY KEY,
            failed_count INTEGER NOT NULL DEFAULT 0,
            locked_until TEXT NOT NULL DEFAULT ''
        )
    )");

    return true;
}

bool Database::executeQuery(const QString& sql) {
    QSqlQuery query;
    if (!query.exec(sql)) {
        // Silently ignore "duplicate column" errors from the ALTER TABLE migration
        if (!query.lastError().text().contains("duplicate column"))
            qDebug() << "SQL Error:" << query.lastError().text() << "\nQuery:" << sql;
        return false;
    }
    return true;
}

bool Database::registerHouse(const QString& houseNumber, const QString& ownerName, const QString& password) {
    QSqlQuery query;
    query.prepare("INSERT INTO houses (house_number, owner_name, password) VALUES (?, ?, ?)");
    query.addBindValue(houseNumber);
    query.addBindValue(ownerName);
    query.addBindValue(hashPassword(password));
    if (!query.exec()) {
        qDebug() << "Register error:" << query.lastError().text();
        return false;
    }
    return true;
}

bool Database::validateLogin(const QString& houseNumber, const QString& password) {
    QSqlQuery query;
    query.prepare("SELECT id FROM houses WHERE house_number = ? AND password = ?");
    query.addBindValue(houseNumber);
    query.addBindValue(hashPassword(password));
    query.exec();
    return query.next();
}

bool Database::houseExists(const QString& houseNumber) {
    QSqlQuery query;
    query.prepare("SELECT id FROM houses WHERE house_number = ?");
    query.addBindValue(houseNumber);
    query.exec();
    return query.next();
}

House Database::getHouse(const QString& houseNumber) {
    House house;
    QSqlQuery query;
    query.prepare("SELECT id, house_number, owner_name, password FROM houses WHERE house_number = ?");
    query.addBindValue(houseNumber);
    query.exec();
    if (query.next()) {
        house.id          = query.value(0).toInt();
        house.houseNumber = query.value(1).toString();
        house.ownerName   = query.value(2).toString();
        house.password    = query.value(3).toString();
        house.rooms       = getRooms(house.id);
    }
    return house;
}

QStringList Database::getAllHouseNumbers() {
    QStringList list;
    QSqlQuery query;
    query.exec("SELECT house_number FROM houses ORDER BY house_number ASC");
    while (query.next())
        list << query.value(0).toString();
    return list;
}

bool Database::changePassword(const QString& houseNumber, const QString& newPassword) {
    QSqlQuery query;
    query.prepare("UPDATE houses SET password = ? WHERE house_number = ?");
    query.addBindValue(hashPassword(newPassword));
    query.addBindValue(houseNumber);
    return query.exec();
}

int Database::getFailedAttempts(const QString& houseNumber) {
    QSqlQuery query;
    query.prepare("SELECT failed_count FROM login_attempts WHERE house_number = ?");
    query.addBindValue(houseNumber);
    query.exec();
    if (query.next()) return query.value(0).toInt();
    return 0;
}

bool Database::isAccountLocked(const QString& houseNumber) {
    QSqlQuery query;
    query.prepare("SELECT locked_until FROM login_attempts WHERE house_number = ?");
    query.addBindValue(houseNumber);
    query.exec();
    if (query.next()) {
        QString lockedUntilStr = query.value(0).toString();
        if (lockedUntilStr.isEmpty()) return false;
        QDateTime lockedUntil = QDateTime::fromString(lockedUntilStr, Qt::ISODate);
        return QDateTime::currentDateTime() < lockedUntil;
    }
    return false;
}

QDateTime Database::getLockoutExpiry(const QString& houseNumber) {
    QSqlQuery query;
    query.prepare("SELECT locked_until FROM login_attempts WHERE house_number = ?");
    query.addBindValue(houseNumber);
    query.exec();
    if (query.next()) {
        QString str = query.value(0).toString();
        if (!str.isEmpty())
            return QDateTime::fromString(str, Qt::ISODate);
    }
    return QDateTime();
}

void Database::recordFailedAttempt(const QString& houseNumber) {
    int current = getFailedAttempts(houseNumber);
    int newCount = current + 1;
    QString lockedUntil = "";

    if (newCount >= 3) {
        lockedUntil = QDateTime::currentDateTime().addSecs(2 * 3600).toString(Qt::ISODate);
    }

    QSqlQuery check;
    check.prepare("SELECT house_number FROM login_attempts WHERE house_number = ?");
    check.addBindValue(houseNumber);
    check.exec();

    if (check.next()) {
        QSqlQuery update;
        update.prepare("UPDATE login_attempts SET failed_count = ?, locked_until = ? WHERE house_number = ?");
        update.addBindValue(newCount);
        update.addBindValue(lockedUntil);
        update.addBindValue(houseNumber);
        update.exec();
    } else {
        QSqlQuery insert;
        insert.prepare("INSERT INTO login_attempts (house_number, failed_count, locked_until) VALUES (?, ?, ?)");
        insert.addBindValue(houseNumber);
        insert.addBindValue(newCount);
        insert.addBindValue(lockedUntil);
        insert.exec();
    }
}

void Database::resetFailedAttempts(const QString& houseNumber) {
    QSqlQuery query;
    query.prepare("UPDATE login_attempts SET failed_count = 0, locked_until = '' WHERE house_number = ?");
    query.addBindValue(houseNumber);
    query.exec();
}

bool Database::addRoom(int houseId, const QString& roomName) {
    QSqlQuery query;
    query.prepare("INSERT INTO rooms (house_id, name) VALUES (?, ?)");
    query.addBindValue(houseId);
    query.addBindValue(roomName);
    return query.exec();
}

bool Database::removeRoom(int roomId) {
    // Remove all devices in the room first (foreign key cascade alternative)
    QSqlQuery devQuery;
    devQuery.prepare("SELECT id FROM devices WHERE room_id = ?");
    devQuery.addBindValue(roomId);
    devQuery.exec();
    while (devQuery.next())
        removeDevice(devQuery.value(0).toInt());

    QSqlQuery query;
    query.prepare("DELETE FROM rooms WHERE id = ?");
    query.addBindValue(roomId);
    return query.exec();
}

QList<Room> Database::getRooms(int houseId) {
    QList<Room> rooms;
    QSqlQuery query;
    query.prepare("SELECT id, house_id, name FROM rooms WHERE house_id = ?");
    query.addBindValue(houseId);
    query.exec();
    while (query.next()) {
        Room room;
        room.id      = query.value(0).toInt();
        room.houseId = query.value(1).toInt();
        room.name    = query.value(2).toString();
        room.devices = getDevices(room.id);
        rooms.append(room);
    }
    return rooms;
}

bool Database::addDevice(int roomId, const QString& name, const QString& type) {
    QSqlQuery query;
    query.prepare("INSERT INTO devices (room_id, name, type, is_on) VALUES (?, ?, ?, 0)");
    query.addBindValue(roomId);
    query.addBindValue(name);
    query.addBindValue(type);
    return query.exec();
}

bool Database::removeDevice(int deviceId) {
    QSqlQuery query;
    query.prepare("DELETE FROM devices WHERE id = ?");
    query.addBindValue(deviceId);
    return query.exec();
}

bool Database::renameDevice(int deviceId, const QString& newName) {
    QSqlQuery query;
    query.prepare("UPDATE devices SET name = ? WHERE id = ?");
    query.addBindValue(newName);
    query.addBindValue(deviceId);
    return query.exec();
}

bool Database::setDeviceState(int deviceId, bool isOn) {
    QSqlQuery query;
    query.prepare("UPDATE devices SET is_on = ? WHERE id = ?");
    query.addBindValue(isOn ? 1 : 0);
    query.addBindValue(deviceId);
    return query.exec();
}

QList<Device> Database::getDevices(int roomId) {
    QList<Device> devices;
    QSqlQuery query;
    query.prepare("SELECT id, room_id, name, type, is_on FROM devices WHERE room_id = ?");
    query.addBindValue(roomId);
    query.exec();
    while (query.next()) {
        Device dev;
        dev.id     = query.value(0).toInt();
        dev.roomId = query.value(1).toInt();
        dev.name   = query.value(2).toString();
        dev.type   = query.value(3).toString();
        dev.isOn   = query.value(4).toBool();
        devices.append(dev);
    }
    return devices;
}

Device Database::getDevice(int deviceId) {
    Device dev;
    QSqlQuery query;
    query.prepare("SELECT id, room_id, name, type, is_on FROM devices WHERE id = ?");
    query.addBindValue(deviceId);
    query.exec();
    if (query.next()) {
        dev.id     = query.value(0).toInt();
        dev.roomId = query.value(1).toInt();
        dev.name   = query.value(2).toString();
        dev.type   = query.value(3).toString();
        dev.isOn   = query.value(4).toBool();
    }
    return dev;
}

bool Database::addActivityLog(int deviceId, const QString& deviceName, const QString& action) {
    QSqlQuery query;
    query.prepare("INSERT INTO activity_logs (device_id, device_name, action, timestamp) VALUES (?, ?, ?, ?)");
    query.addBindValue(deviceId);
    query.addBindValue(deviceName);
    query.addBindValue(action);
    query.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));
    return query.exec();
}

QList<ActivityLog> Database::getActivityLogs(int houseId, int limit) {
    QList<ActivityLog> logs;

    QSqlQuery devQuery;
    devQuery.prepare(R"(
        SELECT d.id FROM devices d
        JOIN rooms r ON d.room_id = r.id
        WHERE r.house_id = ?
    )");
    devQuery.addBindValue(houseId);
    devQuery.exec();

    QStringList deviceIds;
    while (devQuery.next())
        deviceIds << devQuery.value(0).toString();
    if (deviceIds.isEmpty()) return logs;

    QString inClause = deviceIds.join(",");
    QSqlQuery query;
    query.prepare(QString("SELECT id, device_id, device_name, action, timestamp FROM activity_logs WHERE device_id IN (%1) ORDER BY timestamp DESC LIMIT ?").arg(inClause));
    query.addBindValue(limit);
    query.exec();

    while (query.next()) {
        ActivityLog log;
        log.id         = query.value(0).toInt();
        log.deviceId   = query.value(1).toInt();
        log.deviceName = query.value(2).toString();
        log.action     = query.value(3).toString();
        log.timestamp  = QDateTime::fromString(query.value(4).toString(), Qt::ISODate);
        logs.append(log);
    }
    return logs;
}

bool Database::addSchedule(int deviceId, const QString& deviceName, bool turnOn, const QDateTime& scheduledTime) {
    QSqlQuery query;
    query.prepare("INSERT INTO schedules (device_id, device_name, turn_on, scheduled_time, executed) VALUES (?, ?, ?, ?, 0)");
    query.addBindValue(deviceId);
    query.addBindValue(deviceName);
    query.addBindValue(turnOn ? 1 : 0);
    query.addBindValue(scheduledTime.toString(Qt::ISODate));
    return query.exec();
}

QList<Schedule> Database::getPendingSchedules() {
    QList<Schedule> schedules;
    QDateTime now = QDateTime::currentDateTime();
    QDateTime nowTrunc = now;
    nowTrunc.setTime(QTime(now.time().hour(), now.time().minute(), 0));

    QSqlQuery query;
    query.prepare(
        "SELECT id, device_id, device_name, turn_on, scheduled_time "
        "FROM schedules WHERE executed = 0 AND scheduled_time <= ?");
    query.addBindValue(nowTrunc.toString(Qt::ISODate));
    query.exec();

    while (query.next()) {
        Schedule s;
        s.id            = query.value(0).toInt();
        s.deviceId      = query.value(1).toInt();
        s.deviceName    = query.value(2).toString();
        s.turnOn        = query.value(3).toBool();
        s.scheduledTime = QDateTime::fromString(query.value(4).toString(), Qt::ISODate);
        s.executed      = false;
        schedules.append(s);
    }
    return schedules;
}

bool Database::markScheduleExecuted(int scheduleId) {
    QSqlQuery query;
    query.prepare("UPDATE schedules SET executed = 1 WHERE id = ?");
    query.addBindValue(scheduleId);
    return query.exec();
}

QList<Schedule> Database::getSchedules(int houseId) {
    QList<Schedule> schedules;

    QSqlQuery devQuery;
    devQuery.prepare(R"(
        SELECT d.id FROM devices d
        JOIN rooms r ON d.room_id = r.id
        WHERE r.house_id = ?
    )");
    devQuery.addBindValue(houseId);
    devQuery.exec();

    QStringList deviceIds;
    while (devQuery.next())
        deviceIds << devQuery.value(0).toString();
    if (deviceIds.isEmpty()) return schedules;

    QString inClause = deviceIds.join(",");
    QSqlQuery query;
    query.exec(QString("SELECT id, device_id, device_name, turn_on, scheduled_time, executed FROM schedules WHERE device_id IN (%1) ORDER BY scheduled_time DESC").arg(inClause));
    while (query.next()) {
        Schedule s;
        s.id            = query.value(0).toInt();
        s.deviceId      = query.value(1).toInt();
        s.deviceName    = query.value(2).toString();
        s.turnOn        = query.value(3).toBool();
        s.scheduledTime = QDateTime::fromString(query.value(4).toString(), Qt::ISODate);
        s.executed      = query.value(5).toBool();
        schedules.append(s);
    }
    return schedules;
}

double Database::getDailyUsageHours(int houseId) {
    QString today = QDate::currentDate().toString("yyyy-MM-dd");
    QSqlQuery query;
    query.prepare(R"(
        SELECT COALESCE(SUM(ur.minutes), 0) FROM usage_records ur
        JOIN devices d ON ur.device_id = d.id
        JOIN rooms r ON d.room_id = r.id
        WHERE r.house_id = ? AND ur.date = ?
    )");
    query.addBindValue(houseId);
    query.addBindValue(today);
    query.exec();
    if (query.next()) return query.value(0).toDouble() / 60.0;
    return 0.0;
}

double Database::getWeeklyUsageHours(int houseId) {
    QString weekAgo = QDate::currentDate().addDays(-7).toString("yyyy-MM-dd");
    QSqlQuery query;
    query.prepare(R"(
        SELECT COALESCE(SUM(ur.minutes), 0) FROM usage_records ur
        JOIN devices d ON ur.device_id = d.id
        JOIN rooms r ON d.room_id = r.id
        WHERE r.house_id = ? AND ur.date >= ?
    )");
    query.addBindValue(houseId);
    query.addBindValue(weekAgo);
    query.exec();
    if (query.next()) return query.value(0).toDouble() / 60.0;
    return 0.0;
}

double Database::getMonthlyUsageHours(int houseId) {
    QString monthAgo = QDate::currentDate().addDays(-30).toString("yyyy-MM-dd");
    QSqlQuery query;
    query.prepare(R"(
        SELECT COALESCE(SUM(ur.minutes), 0) FROM usage_records ur
        JOIN devices d ON ur.device_id = d.id
        JOIN rooms r ON d.room_id = r.id
        WHERE r.house_id = ? AND ur.date >= ?
    )");
    query.addBindValue(houseId);
    query.addBindValue(monthAgo);
    query.exec();
    if (query.next()) return query.value(0).toDouble() / 60.0;
    return 0.0;
}

double Database::getEstimatedBill(int houseId) {
    return getMonthlyUsageHours(houseId) * 0.5 * 50.0;
}

QList<QPair<QString,int>> Database::getMonthlyUsageDetails(int houseId) {
    QList<QPair<QString,int>> result;
    QString monthAgo = QDate::currentDate().addDays(-30).toString("yyyy-MM-dd");
    QSqlQuery query;
    query.prepare(R"(
        SELECT d.name, COALESCE(SUM(ur.minutes), 0)
        FROM devices d
        JOIN rooms r ON d.room_id = r.id
        LEFT JOIN usage_records ur ON ur.device_id = d.id AND ur.date >= ?
        WHERE r.house_id = ?
        GROUP BY d.id, d.name
        ORDER BY d.name
    )");
    query.addBindValue(monthAgo);
    query.addBindValue(houseId);
    query.exec();
    while (query.next())
        result.append({query.value(0).toString(), query.value(1).toInt()});
    return result;
}

bool Database::deleteSchedule(int scheduleId) {
    QSqlQuery query;
    query.prepare("DELETE FROM schedules WHERE id = ?");
    query.addBindValue(scheduleId);
    return query.exec();
}

bool Database::recordUsage(int deviceId, int minutes) {
    QString today = QDate::currentDate().toString("yyyy-MM-dd");
    QSqlQuery check;
    check.prepare("SELECT id FROM usage_records WHERE device_id = ? AND date = ?");
    check.addBindValue(deviceId);
    check.addBindValue(today);
    check.exec();

    if (check.next()) {
        QSqlQuery update;
        update.prepare("UPDATE usage_records SET minutes = minutes + ? WHERE device_id = ? AND date = ?");
        update.addBindValue(minutes);
        update.addBindValue(deviceId);
        update.addBindValue(today);
        return update.exec();
    } else {
        QSqlQuery insert;
        insert.prepare("INSERT INTO usage_records (device_id, date, minutes) VALUES (?, ?, ?)");
        insert.addBindValue(deviceId);
        insert.addBindValue(today);
        insert.addBindValue(minutes);
        return insert.exec();
    }
}