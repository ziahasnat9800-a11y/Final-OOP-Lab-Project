#include "devicecard.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QEvent>

DeviceCard::DeviceCard(int deviceId, const QString& name, const QString& type,
                       bool isOn, QWidget* parent)
    : QFrame(parent), m_deviceId(deviceId), m_name(name), m_type(type), m_isOn(isOn)
{
    setObjectName("deviceCard");
    setFixedSize(180, 148);
    setFrameShape(QFrame::NoFrame);
    setCursor(Qt::PointingHandCursor);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(14, 12, 14, 12);
    layout->setSpacing(4);

    // Icon circle
    m_iconLabel = new QLabel(iconFor(type), this);
    m_iconLabel->setAlignment(Qt::AlignCenter);
    m_iconLabel->setFixedSize(44, 44);
    m_iconLabel->setObjectName("iconCircle");
    layout->addWidget(m_iconLabel, 0, Qt::AlignLeft);

    // Device name
    QLabel* nameLabel = new QLabel(name, this);
    nameLabel->setObjectName("cardName");
    nameLabel->setWordWrap(true);
    layout->addWidget(nameLabel);

    // Bottom row
    QHBoxLayout* bottomRow = new QHBoxLayout;
    bottomRow->setContentsMargins(0, 0, 0, 0);
    bottomRow->setSpacing(6);

    // Toggle pill track
    m_toggleTrack = new QWidget(this);
    m_toggleTrack->setFixedSize(52, 26);
    m_toggleTrack->setCursor(Qt::PointingHandCursor);

    m_toggleKnob = new QLabel(m_toggleTrack);
    m_toggleKnob->setFixedSize(20, 20);
    m_toggleKnob->move(isOn ? 28 : 4, 3);

    m_toggleText = new QLabel(isOn ? "ON" : "OFF", this);
    m_toggleText->setFixedWidth(28);

    bottomRow->addWidget(m_toggleTrack);
    bottomRow->addWidget(m_toggleText);
    bottomRow->addStretch();

    // Watt + time info
    QVBoxLayout* infoCol = new QVBoxLayout;
    infoCol->setSpacing(1);
    infoCol->setContentsMargins(0, 0, 0, 0);

    m_wattLabel = new QLabel(isOn ? QString("⚡ %1W").arg(wattFor(type)) : "", this);
    m_timeLabel = new QLabel(isOn ? "⏱ just now" : "—", this);

    infoCol->addWidget(m_wattLabel);
    infoCol->addWidget(m_timeLabel);
    bottomRow->addLayout(infoCol);
    layout->addLayout(bottomRow);

    applyToggleStyle(isOn);
    updateCardStyle();

    m_toggleTrack->installEventFilter(this);
    installEventFilter(this);
}

bool DeviceCard::eventFilter(QObject* obj, QEvent* event) {
    if (event->type() == QEvent::MouseButtonRelease) {
        emit toggleRequested(m_deviceId, m_name, m_type);
        return true;
    }
    return QFrame::eventFilter(obj, event);
}

void DeviceCard::applyToggleStyle(bool isOn) {
    if (isOn) {
        m_toggleTrack->setStyleSheet(
            "QWidget { background: #22c55e; border-radius: 13px; }");
        m_toggleKnob->setStyleSheet(
            "QLabel { background: white; border-radius: 10px; }");
        m_toggleText->setStyleSheet(
            "QLabel { font-size:11px; font-weight:bold; color:#22c55e;"
            "font-family:'Segoe UI',sans-serif; }");
        m_wattLabel->setStyleSheet(
            "QLabel { font-size:10px; color:#fbbf24;"
            "font-family:'Segoe UI',sans-serif; }");
        m_timeLabel->setStyleSheet(
            "QLabel { font-size:10px; color:#6b7280;"
            "font-family:'Segoe UI',sans-serif; }");
    } else {
        m_toggleTrack->setStyleSheet(
            "QWidget { background: #2d2d45; border-radius: 13px; }");
        m_toggleKnob->setStyleSheet(
            "QLabel { background: #6b7280; border-radius: 10px; }");
        m_toggleText->setStyleSheet(
            "QLabel { font-size:11px; font-weight:bold; color:#4b5563;"
            "font-family:'Segoe UI',sans-serif; }");
        m_wattLabel->setStyleSheet(
            "QLabel { font-size:10px; color:transparent; }");
        m_timeLabel->setStyleSheet(
            "QLabel { font-size:10px; color:#374151;"
            "font-family:'Segoe UI',sans-serif; }");
    }
}

void DeviceCard::setState(bool isOn) {
    m_isOn = isOn;
    if (!isOn) m_onSince = QDateTime();

    m_toggleKnob->move(isOn ? 28 : 4, 3);
    m_toggleText->setText(isOn ? "ON" : "OFF");
    m_wattLabel->setText(isOn ? QString("⚡ %1W").arg(wattFor(m_type)) : "");
    m_timeLabel->setText(isOn ? "⏱ just now" : "—");

    applyToggleStyle(isOn);
    updateCardStyle();
}

void DeviceCard::setOnTime(const QDateTime& since) {
    m_onSince = since;
    updateTimeLabel();
}

void DeviceCard::updateTimeLabel() {
    if (!m_isOn || !m_onSince.isValid()) {
        m_timeLabel->setText("—");
        return;
    }
    int secs = (int)m_onSince.secsTo(QDateTime::currentDateTime());
    if (secs < 60)
        m_timeLabel->setText("⏱ just now");
    else if (secs < 3600)
        m_timeLabel->setText(QString("⏱ %1 min ago").arg(secs / 60));
    else
        m_timeLabel->setText(QString("⏱ %1 hr ago").arg(secs / 3600));
}

void DeviceCard::updateCardStyle() {
    if (m_isOn) {
        setStyleSheet(R"(
            QFrame#deviceCard {
                background: qlineargradient(x1:0,y1:0,x2:1,y2:1,
                    stop:0 #132337, stop:1 #111d30);
                border: 1.5px solid #2a6496;
                border-radius: 14px;
            }
            QLabel#cardName {
                font-size: 13px; font-weight: 600; color: #e2e8f0;
                font-family: 'Segoe UI', sans-serif;
            }
            QLabel#iconCircle {
                font-size: 22px;
                background: rgba(100,160,255,0.15);
                border-radius: 22px;
                border: 1px solid rgba(100,160,255,0.3);
            }
        )");
    } else {
        setStyleSheet(R"(
            QFrame#deviceCard {
                background: #111827;
                border: 1.5px solid #1e2a40;
                border-radius: 14px;
            }
            QLabel#cardName {
                font-size: 13px; font-weight: 600; color: #6b7280;
                font-family: 'Segoe UI', sans-serif;
            }
            QLabel#iconCircle {
                font-size: 22px;
                background: rgba(255,255,255,0.05);
                border-radius: 22px;
                border: 1px solid rgba(255,255,255,0.1);
            }
        )");
    }
}

QString DeviceCard::iconFor(const QString& type) {
    if (type == "light") return "💡";
    if (type == "fan")   return "🌀";
    if (type == "ac")    return "❄️";
    if (type == "tv")    return "📺";
    return "🔌";
}

int DeviceCard::wattFor(const QString& type) {
    if (type == "light") return 12;
    if (type == "fan")   return 35;
    if (type == "ac")    return 1500;
    if (type == "tv")    return 80;
    return 50;
}