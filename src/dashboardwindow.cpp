#include "dashboardwindow.h"
#include "devicecard.h"
#include "arduinocontroller.h"
#include "scheduledialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QPushButton>
#include <QFrame>
#include <QMessageBox>
#include <QApplication>
#include <QScreen>
#include <QInputDialog>
#include <QFileDialog>
#include <QDate>
#include <QPrinter>
#include <QPainter>
#include <QPainterPath>
#include <QFont>
#include <QtMath>
#include <algorithm>

// ─────────────────────────────────────────────────────────────────────────────
// ChartsCanvas — self-contained painted widget
// ─────────────────────────────────────────────────────────────────────────────
class ChartsCanvas : public QWidget {
public:
    QList<double>                dailySeries;
    QList<QPair<QString,double>> breakdown;

    explicit ChartsCanvas(QWidget* p = nullptr) : QWidget(p) {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        setMinimumHeight(180);
        setAttribute(Qt::WA_OpaquePaintEvent, false);
        setAttribute(Qt::WA_NoSystemBackground, true);
        setAutoFillBackground(false);
    }

protected:
    void paintEvent(QPaintEvent*) override {
        if (width() < 60 || height() < 60) return;
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        const int W = width(), H = height();
        const int pad = 8;

        // Split canvas: 58% line chart | 42% donut
        int lineW  = (int)(W * 0.58);
        int donutW = W - lineW - 6;

        QRectF lineR (pad,            pad, lineW  - 2*pad, H - 2*pad);
        QRectF donutR(lineW + pad + 4, pad, donutW - 2*pad, H - 2*pad);

        drawLine (p, lineR);

        // Vertical divider between line chart and donut chart
        int divX = lineW + 3;  // centre of the 6-px gap between the two rects
        p.save();
        p.setPen(QPen(QColor(0x1e, 0x2a, 0x40), 1, Qt::SolidLine));
        p.drawLine(divX, pad + 4, divX, H - pad - 4);
        p.restore();

        drawDonut(p, donutR);
    }

private:
    static const QList<QColor> kPalette;

    void drawLine(QPainter& p, const QRectF& rc) {
        if (dailySeries.size() < 2) {
            p.setPen(QColor(0x64, 0x74, 0x8b));
            p.setFont(QFont("Segoe UI", 9));
            p.drawText(rc, Qt::AlignCenter, "Not enough data yet");
            return;
        }

        const int labH = 18, labW = 30;
        QRectF plot(rc.x() + labW, rc.y() + 18,
                    rc.width() - labW - 4, rc.height() - 18 - labH - 4);

        double maxV = *std::max_element(dailySeries.begin(), dailySeries.end());
        if (maxV < 0.01) maxV = 1.0;

        const int n  = dailySeries.size();
        auto px = [&](int i)   -> double { return plot.x() + i * plot.width()  / (n - 1); };
        auto py = [&](double v)-> double { return plot.bottom() - (v / maxV) * plot.height(); };

        // Grid + Y labels
        p.setFont(QFont("Segoe UI", 7));
        for (int g = 0; g <= 4; ++g) {
            double val = maxV * g / 4.0;
            double gy  = plot.bottom() - (val / maxV) * plot.height();
            p.setPen(QColor(0x64, 0x74, 0x8b));
            p.drawText(QRectF(rc.x(), gy - 7, labW - 3, 14),
                       Qt::AlignRight | Qt::AlignVCenter,
                       QString::number(val, 'f', 1));
            if (g > 0) {
                p.save();
                p.setPen(QPen(QColor(0x1e, 0x22, 0x35, 140), 1, Qt::DashLine));
                p.drawLine(QPointF(plot.left(), gy), QPointF(plot.right(), gy));
                p.restore();
            }
        }

        // Build point list
        QPolygonF pts;
        for (int i = 0; i < n; ++i)
            pts << QPointF(px(i), py(dailySeries[i]));

        // Gradient fill
        QPainterPath fill;
        fill.moveTo(QPointF(pts.first().x(), plot.bottom()));
        fill.lineTo(pts.first());
        for (int i = 1; i < pts.size(); ++i) {
            double cx = (pts[i-1].x() + pts[i].x()) / 2.0;
            fill.cubicTo(cx, pts[i-1].y(), cx, pts[i].y(), pts[i].x(), pts[i].y());
        }
        fill.lineTo(QPointF(pts.last().x(), plot.bottom()));
        fill.closeSubpath();

        QLinearGradient grad(0, plot.top(), 0, plot.bottom());
        grad.setColorAt(0.0, QColor(96, 165, 250, 110));
        grad.setColorAt(1.0, QColor(96, 165, 250,   0));
        p.fillPath(fill, grad);

        // Curve line
        QPainterPath line;
        line.moveTo(pts.first());
        for (int i = 1; i < pts.size(); ++i) {
            double cx = (pts[i-1].x() + pts[i].x()) / 2.0;
            line.cubicTo(cx, pts[i-1].y(), cx, pts[i].y(), pts[i].x(), pts[i].y());
        }
        p.setPen(QPen(QColor(96, 165, 250), 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.setBrush(Qt::NoBrush);
        p.drawPath(line);

        // Dots + last-point label
        for (int i = 0; i < n; ++i) {
            bool last = (i == n - 1);
            p.setPen(Qt::NoPen);
            p.setBrush(last ? QColor(96, 165, 250) : QColor(96, 165, 250, 130));
            double r = last ? 4.5 : 3.0;
            p.drawEllipse(pts[i], r, r);
            if (last) {
                p.setPen(QColor(0xe2, 0xe8, 0xf0));
                p.setFont(QFont("Segoe UI", 8, QFont::Bold));
                p.drawText(QRectF(pts[i].x() - 26, pts[i].y() - 17, 52, 13),
                           Qt::AlignCenter,
                           QString::number(dailySeries[i], 'f', 1) + " kWh");
            }
        }

        // X-axis day labels
        QDate today = QDate::currentDate();
        p.setPen(QColor(0x64, 0x74, 0x8b));
        p.setFont(QFont("Segoe UI", 7));
        for (int i = 0; i < n; ++i)
            p.drawText(QRectF(px(i) - 14, plot.bottom() + 3, 28, labH),
                       Qt::AlignCenter,
                       today.addDays(i - (n - 1)).toString("ddd"));

        // Chart title
        p.setPen(QColor(0x94, 0xa3, 0xb8));
        p.setFont(QFont("Segoe UI", 9, QFont::Bold));
        p.drawText(QRectF(rc.x() + labW, rc.y(), rc.width() - labW, 16),
                   Qt::AlignLeft, "Daily Usage (kWh)");
    }

    void drawDonut(QPainter& p, const QRectF& rc) {
        double total = 0;
        for (auto& item : breakdown) total += item.second;

        if (total < 0.001) {
            p.setPen(QColor(0x64, 0x74, 0x8b));
            p.setFont(QFont("Segoe UI", 9));
            p.drawText(rc, Qt::AlignCenter, "No usage data");
            return;
        }

        const int titleH   = 16;
        const int legItemH = 16;
        int legH = breakdown.size() * legItemH + 4;

        // Donut ring size — fit inside available height after title + legend
        double sz = qMin(rc.width() * 0.9,
                         rc.height() - titleH - legH - 12.0);
        sz = qMax(sz, 40.0);

        QRectF ring(rc.x() + (rc.width() - sz) / 2.0,
                    rc.y() + titleH + 4,
                    sz, sz);

        // Draw arcs
        double angle = 90.0 * 16;
        for (int i = 0; i < breakdown.size(); ++i) {
            double span = -(breakdown[i].second / total) * 360.0 * 16;
            QColor col  = kPalette[i % kPalette.size()];
            p.setPen(QPen(col, qMax(10.0, sz * 0.13),
                          Qt::SolidLine, Qt::FlatCap));
            p.setBrush(Qt::NoBrush);
            p.drawArc(ring.adjusted(6, 6, -6, -6), (int)angle, (int)span);
            angle += span;
        }

        // Centre label
        double totalKwh = total;
        p.setPen(QColor(0xe2, 0xe8, 0xf0));
        p.setFont(QFont("Segoe UI", qMax(7, (int)(sz * 0.10)), QFont::Bold));
        p.drawText(ring, Qt::AlignCenter,
                   QString::number(totalKwh, 'f', 1) + "\nkWh");

        // Legend below donut
        double lx = rc.x();
        double ly = ring.bottom() + 6;
        p.setFont(QFont("Segoe UI", 8));

        for (int i = 0; i < breakdown.size(); ++i) {
            QColor col = kPalette[i % kPalette.size()];
            double pct = breakdown[i].second / total * 100.0;
            double y   = ly + i * legItemH;

            p.setPen(Qt::NoPen);
            p.setBrush(col);
            p.drawEllipse(QRectF(lx, y + 4, 7, 7));

            p.setPen(QColor(0x94, 0xa3, 0xb8));
            QString nm = breakdown[i].first;
            if (nm.length() > 10) nm = nm.left(9) + "…";
            p.drawText(QRectF(lx + 11, y, rc.width() * 0.55, legItemH),
                       Qt::AlignLeft | Qt::AlignVCenter, nm);

            p.setPen(QColor(0xe2, 0xe8, 0xf0));
            p.setFont(QFont("Segoe UI", 8, QFont::Bold));
            p.drawText(QRectF(lx + 11, y, rc.width() - 12, legItemH),
                       Qt::AlignRight | Qt::AlignVCenter,
                       QString("%1%").arg((int)qRound(pct)));
            p.setFont(QFont("Segoe UI", 8));
        }

        // Chart title
        p.setPen(QColor(0x94, 0xa3, 0xb8));
        p.setFont(QFont("Segoe UI", 9, QFont::Bold));
        p.drawText(QRectF(rc.x(), rc.y(), rc.width(), titleH),
                   Qt::AlignLeft, "Device Breakdown");
    }
};

const QList<QColor> ChartsCanvas::kPalette = {
    { 96, 165, 250 },
    { 52, 211, 153 },
    { 251, 191,  36 },
    { 167, 139, 250 },
    { 248, 113, 113 },
    };

// ─────────────────────────────────────────────────────────────────────────────
// DashboardWindow
// ─────────────────────────────────────────────────────────────────────────────
DashboardWindow::DashboardWindow(const House& house, QWidget* parent)
    : QWidget(parent), m_house(house)
{
    setWindowTitle("Smart Home — House " + house.houseNumber);
    resize(1280, 800);
    QRect screen = QApplication::primaryScreen()->availableGeometry();
    move((screen.width()  - width())  / 2,
         (screen.height() - height()) / 2);

    setupUI();
    applyStyles();

    // Arduino
    bool connected = ArduinoController::instance().connectToArduino();
    QString portName = ArduinoController::instance().findArduinoPort();
    if (connected) {
        m_arduinoStatusLabel->setText("Arduino: Connected ✓");
        m_arduinoStatusLabel->setStyleSheet(
            "color:#22c55e; font-size:12px; font-weight:700;"
            "font-family:'Segoe UI',sans-serif; background:transparent;");
        m_arduinoPortLabel->setText(portName.isEmpty() ? "" : "(" + portName + ")");
        m_arduinoPortLabel->setStyleSheet(
            "color:#4ade80; font-size:10px;"
            "font-family:'Segoe UI',sans-serif; background:transparent;");
        m_connectionDot->setVisible(true);
        m_connectionDot->setStyleSheet("color:#22c55e; font-size:10px; background:transparent;");
    } else {
        m_arduinoStatusLabel->setText("Arduino: Not Connected");
        m_arduinoStatusLabel->setStyleSheet(
            "color:#f87171; font-size:12px; font-weight:700;"
            "font-family:'Segoe UI',sans-serif; background:transparent;");
        m_arduinoPortLabel->setText("");
        m_connectionDot->setVisible(true);
        m_connectionDot->setStyleSheet("color:#f87171; font-size:10px; background:transparent;");
    }

    // Timers
    m_scheduleTimer = new QTimer(this);
    connect(m_scheduleTimer, &QTimer::timeout, this, &DashboardWindow::onScheduleTimerTick);
    m_scheduleTimer->start(10000);
    QTimer::singleShot(0, this, &DashboardWindow::onScheduleTimerTick);

    m_usageTimer = new QTimer(this);
    connect(m_usageTimer, &QTimer::timeout, this, &DashboardWindow::onUsageTimerTick);
    m_usageTimer->start(60000);

    m_clockTimer = new QTimer(this);
    connect(m_clockTimer, &QTimer::timeout, this, &DashboardWindow::onClockTick);
    m_clockTimer->start(1000);

    // Seed on-times for devices already ON
    m_house = Database::instance().getHouse(m_house.houseNumber);
    for (const Room& r : m_house.rooms)
        for (const Device& d : r.devices)
            if (d.isOn)
                m_deviceOnTimes[d.id] = QDateTime::currentDateTime();

    buildDeviceCards();
    refreshLogs();
    refreshUsage();
    refreshSchedules();
    onClockTick();
}

// ─────────────────────────────────────────────────────────────────────────────
void DashboardWindow::flushUsage() {
    QDateTime now = QDateTime::currentDateTime();
    for (auto it = m_deviceOnTimes.begin(); it != m_deviceOnTimes.end(); ++it) {
        int deviceId     = it.key();
        QDateTime& start = it.value();
        int mins = (int)(start.secsTo(now) / 60);
        if (mins > 0) {
            Database::instance().recordUsage(deviceId, mins);
            start = now;
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
void DashboardWindow::onClockTick() {
    QDateTime now = QDateTime::currentDateTime();
    int hour = now.time().hour();
    QString greet;
    if      (hour >= 5  && hour < 12) greet = "Good Morning";
    else if (hour >= 12 && hour < 17) greet = "Good Afternoon";
    else if (hour >= 17 && hour < 21) greet = "Good Evening";
    else                              greet = "Good Night";

    QString name = m_house.ownerName.isEmpty()
                       ? ("House " + m_house.houseNumber)
                       : m_house.ownerName.split(' ').first();

    m_greetingLabel->setText(QString("👋  %1, %2!").arg(greet).arg(name));
    m_dateTimeLabel->setText(now.toString("dddd, d MMMM yyyy  •  hh:mm AP"));

    for (auto it = m_deviceCards.begin(); it != m_deviceCards.end(); ++it)
        if (it.value() && m_deviceOnTimes.contains(it.key()))
            it.value()->setOnTime(m_deviceOnTimes[it.key()]);
}

// ─────────────────────────────────────────────────────────────────────────────
void DashboardWindow::setupUI() {
    QVBoxLayout* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // ══ TOP BAR ══════════════════════════════════════════════════════════════
    QWidget* topBar = new QWidget;
    topBar->setObjectName("topBar");
    topBar->setFixedHeight(58);
    QHBoxLayout* topL = new QHBoxLayout(topBar);
    topL->setContentsMargins(18, 0, 18, 0);
    topL->setSpacing(10);

    // Logo + title
    QLabel* homeIcon = new QLabel("🏠");
    homeIcon->setStyleSheet("font-size:18px; background:transparent;");
    QLabel* titleLbl = new QLabel("Smart Home — House " + m_house.houseNumber);
    titleLbl->setObjectName("topTitle");
    topL->addWidget(homeIcon);
    topL->addWidget(titleLbl);
    topL->addSpacing(8);

    // Devices-ON badge
    m_devicesOnLabel = new QLabel("0 devices ON");
    m_devicesOnLabel->setObjectName("devicesBadge");
    topL->addWidget(m_devicesOnLabel);
    topL->addStretch(1);

    // Arduino status (connection dot + two-line labels)
    m_connectionDot = new QLabel("●");
    m_connectionDot->setStyleSheet("color:#f87171; font-size:10px; background:transparent;");
    QVBoxLayout* aCol = new QVBoxLayout;
    aCol->setContentsMargins(0, 0, 0, 0);
    aCol->setSpacing(0);
    m_arduinoStatusLabel = new QLabel("Arduino: Checking...");
    m_arduinoStatusLabel->setStyleSheet("background:transparent;");
    m_arduinoPortLabel   = new QLabel("");
    m_arduinoPortLabel->setStyleSheet("background:transparent;");
    aCol->addWidget(m_arduinoStatusLabel);
    aCol->addWidget(m_arduinoPortLabel);
    topL->addWidget(m_connectionDot);
    topL->addLayout(aCol);
    topL->addStretch(1);

    // Action buttons
    auto topBtn = [&](const QString& text, const QString& obj) -> QPushButton* {
        auto* b = new QPushButton(text, topBar);
        b->setObjectName(obj);
        b->setFixedHeight(32);
        b->setCursor(Qt::PointingHandCursor);
        return b;
    };
    QPushButton* scheduleBtn = topBtn("📅  Schedule",     "headerBtn");
    QPushButton* renameBtn   = topBtn("✏  Rename",        "headerBtn");
    QPushButton* exportBtn   = topBtn("📄  Export PDF",   "headerBtn");
    QPushButton* aboutBtn    = topBtn("ℹ  About",         "headerBtn");
    QPushButton* logoutBtn   = topBtn("⏻  Logout",        "logoutBtn");

    topL->addWidget(scheduleBtn);
    topL->addWidget(renameBtn);
    topL->addWidget(exportBtn);
    topL->addWidget(aboutBtn);
    topL->addWidget(logoutBtn);
    root->addWidget(topBar);

    // ══ BODY ═════════════════════════════════════════════════════════════════
    QHBoxLayout* body = new QHBoxLayout;
    body->setContentsMargins(14, 12, 14, 12);
    body->setSpacing(12);

    // ─── LEFT COLUMN (55%) ───────────────────────────────────────────────────
    QVBoxLayout* leftCol = new QVBoxLayout;
    leftCol->setSpacing(10);
    leftCol->setContentsMargins(0, 0, 0, 0);

    // Welcome card
    QWidget* welcome = new QWidget;
    welcome->setObjectName("welcomeCard");
    welcome->setFixedHeight(76);
    QHBoxLayout* wcL = new QHBoxLayout(welcome);
    wcL->setContentsMargins(18, 10, 18, 10);
    wcL->setSpacing(14);

    QVBoxLayout* greetCol = new QVBoxLayout;
    greetCol->setSpacing(2);
    m_greetingLabel = new QLabel("👋  Good Evening!");
    m_greetingLabel->setObjectName("greetingText");
    m_dateTimeLabel = new QLabel("");
    m_dateTimeLabel->setObjectName("dateTimeText");
    greetCol->addWidget(m_greetingLabel);
    greetCol->addWidget(m_dateTimeLabel);
    wcL->addLayout(greetCol);
    wcL->addStretch();

    // Quick actions inline in welcome card
    auto quickBtn = [&](const QString& txt, const QString& obj) -> QPushButton* {
        auto* b = new QPushButton(txt, welcome);
        b->setObjectName(obj);
        b->setFixedHeight(30);
        b->setCursor(Qt::PointingHandCursor);
        b->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        return b;
    };
    QPushButton* allOnBtn  = quickBtn("⚡ All ON",    "qBtnAllOn");
    QPushButton* allOffBtn = quickBtn("⏻ All OFF",   "qBtnAllOff");
    QPushButton* nightBtn  = quickBtn("🌙 Night",     "qBtnNight");
    QPushButton* awayBtn   = quickBtn("🛡 Away",      "qBtnAway");
    wcL->addWidget(allOnBtn);
    wcL->addWidget(allOffBtn);
    wcL->addWidget(nightBtn);
    wcL->addWidget(awayBtn);
    leftCol->addWidget(welcome);

    // Devices panel
    QWidget* devPanel = new QWidget;
    devPanel->setObjectName("panel");
    QVBoxLayout* devL = new QVBoxLayout(devPanel);
    devL->setContentsMargins(14, 12, 14, 12);
    devL->setSpacing(8);

    QHBoxLayout* devTitleRow = new QHBoxLayout;
    QLabel* devTitle = new QLabel("⚡  Electrical Devices");
    devTitle->setObjectName("panelTitle");
    devTitleRow->addWidget(devTitle);
    devTitleRow->addStretch();
    QLabel* devHint = new QLabel("Click a card to toggle");
    devHint->setStyleSheet(
        "font-size:10px; color:#374151; background:transparent;"
        "font-family:'Segoe UI',sans-serif;");
    devTitleRow->addWidget(devHint);
    devL->addLayout(devTitleRow);

    QScrollArea* devScroll = new QScrollArea;
    devScroll->setWidgetResizable(true);
    devScroll->setObjectName("scrollArea");
    devScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    devScroll->setFrameShape(QFrame::NoFrame);
    devScroll->setStyleSheet("background: transparent; border: none;");
    devScroll->viewport()->setAutoFillBackground(false);
    devScroll->viewport()->setStyleSheet("background: transparent;");

    m_devicesContainer = new QWidget;
    m_devicesContainer->setAutoFillBackground(false);
    m_devicesContainer->setStyleSheet("background: transparent;");
    m_devicesLayout    = new QVBoxLayout(m_devicesContainer);
    m_devicesLayout->setContentsMargins(0, 0, 6, 0);
    m_devicesLayout->setSpacing(12);
    m_devicesLayout->addStretch();

    devScroll->setWidget(m_devicesContainer);
    devL->addWidget(devScroll);
    leftCol->addWidget(devPanel, 1);
    body->addLayout(leftCol, 55);

    // ─── RIGHT COLUMN (45%) ──────────────────────────────────────────────────
    QVBoxLayout* rightCol = new QVBoxLayout;
    rightCol->setSpacing(10);
    rightCol->setContentsMargins(0, 0, 0, 0);

    // Usage Statistics panel — stretch:3 so it gets most right-column space
    QWidget* usagePanel = new QWidget;
    usagePanel->setObjectName("panel");
    QVBoxLayout* usageL = new QVBoxLayout(usagePanel);
    usageL->setContentsMargins(14, 12, 14, 12);
    usageL->setSpacing(8);

    // Usage title row
    QHBoxLayout* usageTitleRow = new QHBoxLayout;
    QLabel* usageTitle = new QLabel("📊  Usage Statistics");
    usageTitle->setObjectName("panelTitle");
    usageTitleRow->addWidget(usageTitle);
    usageTitleRow->addStretch();
    usageL->addLayout(usageTitleRow);

    // Horizontal separator under panel title
    QFrame* usageSep = new QFrame;
    usageSep->setFrameShape(QFrame::HLine);
    usageSep->setFixedHeight(1);
    usageSep->setStyleSheet("background: #1e2a40; border: none;");
    usageL->addWidget(usageSep);

    // KPI boxes row (4 boxes, equal width)
    QHBoxLayout* kpiRow = new QHBoxLayout;
    kpiRow->setSpacing(6);

    auto makeKpi = [&](const QString& period, QLabel*& valOut,
                       const QString& color) -> QWidget* {
        QWidget* box = new QWidget;
        box->setObjectName("kpiBox");
        QVBoxLayout* bl = new QVBoxLayout(box);
        bl->setContentsMargins(10, 8, 10, 8);
        bl->setSpacing(2);
        QLabel* pLbl = new QLabel(period);
        pLbl->setStyleSheet(
            "font-size:10px; color:#64748b; background:transparent;"
            "font-family:'Segoe UI',sans-serif;");
        valOut = new QLabel("0.0");
        valOut->setStyleSheet(
            QString("font-size:17px; font-weight:700; color:%1;"
                    "background:transparent; font-family:'Segoe UI',sans-serif;")
                .arg(color));
        bl->addWidget(pLbl);
        bl->addWidget(valOut);
        return box;
    };

    kpiRow->addWidget(makeKpi("Today",        m_dailyLabel,   "#60a5fa"));
    kpiRow->addWidget(makeKpi("This Week",    m_weeklyLabel,  "#34d399"));
    kpiRow->addWidget(makeKpi("This Month",   m_monthlyLabel, "#a78bfa"));

    // Bill box
    {
        QWidget* box = new QWidget;
        box->setObjectName("kpiBox");
        QVBoxLayout* bl = new QVBoxLayout(box);
        bl->setContentsMargins(10, 8, 10, 8);
        bl->setSpacing(2);
        QLabel* pLbl = new QLabel("Est. Bill");
        pLbl->setStyleSheet(
            "font-size:10px; color:#64748b; background:transparent;"
            "font-family:'Segoe UI',sans-serif;");
        m_billLabel = new QLabel("PKR 0");
        m_billLabel->setStyleSheet(
            "font-size:17px; font-weight:700; color:#fbbf24;"
            "background:transparent; font-family:'Segoe UI',sans-serif;");
        bl->addWidget(pLbl);
        bl->addWidget(m_billLabel);
        kpiRow->addWidget(box);
    }
    usageL->addLayout(kpiRow);

    // Charts canvas — takes all remaining space in usage panel
    m_chartsCanvas = new ChartsCanvas;
    m_chartsCanvas->setParent(usagePanel);
    m_chartsCanvas->setStyleSheet("background: transparent;");
    m_chartsCanvas->setAutoFillBackground(false);
    usageL->addWidget(m_chartsCanvas, 1);  // stretch=1 → expands to fill

    rightCol->addWidget(usagePanel, 3);  // stretch=3 → largest section

    // Schedules panel — stretch:2
    QWidget* schedPanel = new QWidget;
    schedPanel->setObjectName("panel");
    QVBoxLayout* schedL = new QVBoxLayout(schedPanel);
    schedL->setContentsMargins(14, 12, 14, 12);
    schedL->setSpacing(8);

    QHBoxLayout* schedTitleRow = new QHBoxLayout;
    QLabel* schedTitle = new QLabel("📅  Schedules");
    schedTitle->setObjectName("panelTitle");
    schedTitleRow->addWidget(schedTitle);
    schedTitleRow->addStretch();
    QPushButton* addSchedBtn = new QPushButton("＋ Add");
    addSchedBtn->setObjectName("addSchedBtn");
    addSchedBtn->setFixedSize(60, 24);
    addSchedBtn->setCursor(Qt::PointingHandCursor);
    schedTitleRow->addWidget(addSchedBtn);
    schedL->addLayout(schedTitleRow);

    // Horizontal separator under panel title
    QFrame* schedSep = new QFrame;
    schedSep->setFrameShape(QFrame::HLine);
    schedSep->setFixedHeight(1);
    schedSep->setStyleSheet("background: #1e2a40; border: none;");
    schedL->addWidget(schedSep);

    QScrollArea* schedScroll = new QScrollArea;
    schedScroll->setWidgetResizable(true);
    schedScroll->setObjectName("schedScroll");
    schedScroll->setFrameShape(QFrame::NoFrame);
    schedScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    schedScroll->setStyleSheet("background: transparent; border: none;");
    schedScroll->viewport()->setAutoFillBackground(false);
    schedScroll->viewport()->setStyleSheet("background: transparent;");

    QWidget* schedInner = new QWidget;
    schedInner->setAutoFillBackground(false);
    schedInner->setStyleSheet("background:transparent;");
    m_schedulesLayout = new QVBoxLayout(schedInner);
    m_schedulesLayout->setContentsMargins(0, 0, 0, 0);
    m_schedulesLayout->setSpacing(4);
    m_schedulesLayout->addStretch();
    m_schedulesContainer = schedInner;

    schedScroll->setWidget(schedInner);
    schedL->addWidget(schedScroll);
    rightCol->addWidget(schedPanel, 2);  // stretch=2

    // Activity Log panel — stretch:2
    QWidget* logPanel = new QWidget;
    logPanel->setObjectName("panel");
    QVBoxLayout* logL = new QVBoxLayout(logPanel);
    logL->setContentsMargins(14, 12, 14, 12);
    logL->setSpacing(8);

    QLabel* logTitle = new QLabel("📋  Activity Log");
    logTitle->setObjectName("panelTitle");
    logL->addWidget(logTitle);

    // Horizontal separator under panel title
    QFrame* logSep = new QFrame;
    logSep->setFrameShape(QFrame::HLine);
    logSep->setFixedHeight(1);
    logSep->setStyleSheet("background: #1e2a40; border: none;");
    logL->addWidget(logSep);

    m_logsList = new QListWidget;
    m_logsList->setObjectName("logsList");
    m_logsList->setFrameShape(QFrame::NoFrame);
    m_logsList->setAutoFillBackground(false);
    m_logsList->viewport()->setAutoFillBackground(false);
    m_logsList->viewport()->setStyleSheet("background: transparent;");
    logL->addWidget(m_logsList);
    rightCol->addWidget(logPanel, 2);  // stretch=2

    body->addLayout(rightCol, 45);
    root->addLayout(body);

    // Wire signals
    connect(scheduleBtn, &QPushButton::clicked, this, &DashboardWindow::openScheduleDialog);
    connect(addSchedBtn, &QPushButton::clicked, this, &DashboardWindow::openScheduleDialog);
    connect(renameBtn,   &QPushButton::clicked, this, &DashboardWindow::onRenameDevice);
    connect(exportBtn,   &QPushButton::clicked, this, &DashboardWindow::onExportPDF);
    connect(aboutBtn,    &QPushButton::clicked, this, &DashboardWindow::onAbout);
    connect(logoutBtn,   &QPushButton::clicked, this, &DashboardWindow::onLogout);
    connect(allOnBtn,    &QPushButton::clicked, this, &DashboardWindow::onTurnAllOn);
    connect(allOffBtn,   &QPushButton::clicked, this, &DashboardWindow::onTurnAllOff);
    connect(nightBtn,    &QPushButton::clicked, this, &DashboardWindow::onNightMode);
    connect(awayBtn,     &QPushButton::clicked, this, &DashboardWindow::onAwayMode);
}

// ─────────────────────────────────────────────────────────────────────────────
void DashboardWindow::applyStyles() {
    setStyleSheet(R"(
        /* ── Base: use dark-blue, not pure black ── */
        QWidget {
            background-color: #0b0f1e;
            color: #e2e8f0;
            font-family: 'Segoe UI', sans-serif;
        }

        /* Transparent helpers — must come before named-widget rules so
           named rules can override them cleanly */
        QLabel      { background: transparent; }
        QPushButton { background: transparent; }

        /* ── Top bar ── */
        QWidget#topBar {
            background: #0d1120;
            border-bottom: 1px solid #1a1f35;
        }
        QLabel#topTitle {
            font-size: 14px; font-weight: 700; color: #e2e8f0;
        }
        QLabel#devicesBadge {
            font-size: 11px; font-weight: 700; color: #0b0f1e;
            background: #22c55e; border-radius: 10px; padding: 3px 12px;
        }

        /* ── Panels ── */
        QWidget#panel {
            background: #111827;
            border: 1px solid #1e2a40;
            border-radius: 14px;
        }
        QLabel#panelTitle {
            font-size: 13px; font-weight: 700; color: #e2e8f0;
        }

        /* ── Welcome card ── */
        QWidget#welcomeCard {
            background: qlineargradient(x1:0,y1:0,x2:1,y2:0,
                stop:0 #0f1d35, stop:1 #111830);
            border: 1px solid #1e3050;
            border-radius: 12px;
        }
        QLabel#greetingText { font-size: 17px; font-weight: 700; color: #7cb9ff; }
        QLabel#dateTimeText { font-size: 11px; color: #64748b; }

        /* ── KPI boxes ── */
        QWidget#kpiBox {
            background: #1a2035;
            border: 1px solid #1e2a40;
            border-radius: 10px;
        }

        /* ── Quick action buttons ── */
        QPushButton#qBtnAllOn {
            background: #14532d; color: #22c55e;
            border: 1.5px solid #22c55e;
            border-radius: 7px; font-size: 11px; font-weight: 700;
            padding: 0 10px;
        }
        QPushButton#qBtnAllOn:hover { background: #166534; }

        QPushButton#qBtnAllOff {
            background: #3b0f0f; color: #f87171;
            border: 1.5px solid #ef4444;
            border-radius: 7px; font-size: 11px; font-weight: 700;
            padding: 0 10px;
        }
        QPushButton#qBtnAllOff:hover { background: #450a0a; }

        QPushButton#qBtnNight {
            background: #1e1b4b; color: #818cf8;
            border: 1.5px solid #4f46e5;
            border-radius: 7px; font-size: 11px; font-weight: 700;
            padding: 0 10px;
        }
        QPushButton#qBtnNight:hover { background: #2e2767; }

        QPushButton#qBtnAway {
            background: #1e1b4b; color: #a78bfa;
            border: 1.5px solid #7c3aed;
            border-radius: 7px; font-size: 11px; font-weight: 700;
            padding: 0 10px;
        }
        QPushButton#qBtnAway:hover { background: #2e2767; }

        /* ── Top-bar action buttons ── */
        QPushButton#headerBtn {
            background: #1a2035; color: #94a3b8;
            border: 1px solid #1e2a40;
            border-radius: 7px; padding: 0 12px; font-size: 12px;
        }
        QPushButton#headerBtn:hover {
            background: #1e2a50; color: #e2e8f0; border-color: #2a3a6a;
        }
        QPushButton#logoutBtn {
            background: #2d0f0f; color: #f87171;
            border: 1.5px solid #f87171;
            border-radius: 7px; padding: 0 14px;
            font-size: 12px; font-weight: 700;
        }
        QPushButton#logoutBtn:hover { background: #3d1515; }

        /* ── Add schedule button ── */
        QPushButton#addSchedBtn {
            background: #1a2035; color: #7c9fff;
            border: 1px solid #2a3a6a;
            border-radius: 6px; font-size: 11px; font-weight: 600;
        }
        QPushButton#addSchedBtn:hover {
            background: #1e2a50; color: #a0c4ff;
        }

        /* ── Activity log list ─────────────────────────────────────────────
           Set the list itself AND its internal viewport to the panel colour
           so there is no black flash in either the item area or the margins. */
        QListWidget#logsList {
            background: #111827;
            border: none;
            color: #cbd5e1; font-size: 12px;
            padding: 2px; outline: 0;
        }
        QListWidget#logsList::item {
            padding: 7px 5px; border-radius: 6px;
            border-bottom: 1px solid #1a1f2e;
            min-height: 24px;
            background: transparent;
        }
        QListWidget#logsList::item:alternate { background: #131c2e; }
        QListWidget#logsList::item:selected  { background: #1e2a40; color: #e2e8f0; }
        QListWidget#logsList::item:hover     { background: #161f30; }

        /* ── Scroll areas — eliminate every black-viewport source ─────────
           The viewport widget inside QScrollArea / QAbstractScrollArea
           inherits the global QWidget background unless explicitly cleared. */
        QScrollArea                               { border: none; background: transparent; }
        QAbstractScrollArea                       { background: transparent; }
        QAbstractScrollArea::viewport             { background: transparent; }
        QScrollArea#scrollArea                    { background: transparent; }
        QScrollArea#schedScroll                   { background: transparent; }
        QAbstractScrollArea#scrollArea::viewport  { background: transparent; }
        QAbstractScrollArea#schedScroll::viewport { background: transparent; }

        /* Force the viewport of the log list to match the list background,
           preventing a black strip when the list content is shorter than
           the available height. */
        QListWidget#logsList QAbstractScrollArea  { background: #111827; }

        /* ── Scrollbar ── */
        QScrollBar:vertical {
            background: transparent; width: 5px; border-radius: 3px;
        }
        QScrollBar::handle:vertical {
            background: #2a3050; border-radius: 3px; min-height: 20px;
        }
        QScrollBar::add-line:vertical,
        QScrollBar::sub-line:vertical { height: 0; }
        QScrollBar:horizontal         { height: 0; }

        /* ── Schedule rows ─────────────────────────────────────────────────
           The row widget itself AND all its direct QLabel / QPushButton
           children must share the same dark-blue background so there are
           no lighter or darker child-widget patches inside the row. */
        QWidget#schedRow {
            background: #131c2e;
            border: 1px solid #1e2a40;
            border-radius: 8px;
        }
        QWidget#schedRow QLabel      { background: transparent; color: #cbd5e1; }
        QWidget#schedRow QPushButton { background: transparent; }
    )");
}

// ─────────────────────────────────────────────────────────────────────────────
void DashboardWindow::buildDeviceCards() {
    for (auto it = m_deviceCards.begin(); it != m_deviceCards.end(); ++it)
        if (it.value()) it.value()->deleteLater();
    m_deviceCards.clear();
    m_roomBadges.clear(); // badge widgets are children of roomHeader, deleted with it

    QLayoutItem* item;
    while ((item = m_devicesLayout->takeAt(0))) {
        delete item->widget();
        delete item;
    }

    m_house = Database::instance().getHouse(m_house.houseNumber);

    for (const Room& room : m_house.rooms) {
        if (room.devices.isEmpty()) continue;

        int onCount = 0;
        for (const Device& d : room.devices) if (d.isOn) onCount++;

        // Room header row
        QWidget* roomHeader = new QWidget;
        roomHeader->setStyleSheet("background:transparent;");
        QHBoxLayout* rhL = new QHBoxLayout(roomHeader);
        rhL->setContentsMargins(2, 6, 2, 2);
        rhL->setSpacing(8);

        QLabel* roomName = new QLabel("📍  " + room.name);
        roomName->setStyleSheet(
            "font-size:12px; font-weight:700; color:#94a3b8;"
            "background:transparent;");
        rhL->addWidget(roomName);

        QString bgColor   = onCount > 0 ? "#22c55e" : "#374151";
        QString textColor = onCount > 0 ? "#0b0d17" : "#9ca3af";
        QLabel* badge = new QLabel(
            QString("%1/%2 ON").arg(onCount).arg(room.devices.size()));
        badge->setStyleSheet(
            QString("font-size:10px; font-weight:700; color:%1;"
                    "background:%2; border-radius:8px; padding:2px 8px;")
                .arg(textColor).arg(bgColor));
        rhL->addWidget(badge);
        m_roomBadges[room.id] = badge;  // store so toggleDevice can update it
        rhL->addStretch();
        m_devicesLayout->addWidget(roomHeader);

        // Device cards row
        QWidget* rowW = new QWidget;
        rowW->setStyleSheet("background:transparent;");
        QHBoxLayout* rowL = new QHBoxLayout(rowW);
        rowL->setContentsMargins(0, 0, 0, 0);
        rowL->setSpacing(10);

        for (const Device& dev : room.devices) {
            DeviceCard* card = new DeviceCard(dev.id, dev.name, dev.type, dev.isOn, this);
            if (dev.isOn && m_deviceOnTimes.contains(dev.id))
                card->setOnTime(m_deviceOnTimes[dev.id]);
            connect(card, &DeviceCard::toggleRequested,
                    this, &DashboardWindow::toggleDevice);
            rowL->addWidget(card);
            m_deviceCards[dev.id] = card;
        }
        rowL->addStretch();
        m_devicesLayout->addWidget(rowW);
    }
    m_devicesLayout->addStretch();
    updateDevicesOnBadge();
}

// ─────────────────────────────────────────────────────────────────────────────
void DashboardWindow::updateDevicesOnBadge() {
    int on = 0;
    for (const Room& r : m_house.rooms)
        for (const Device& d : r.devices)
            if (d.isOn) on++;

    m_devicesOnLabel->setText(
        QString("● %1 device%2 ON").arg(on).arg(on == 1 ? "" : "s"));
    m_devicesOnLabel->setStyleSheet(
        on > 0
            ? "font-size:11px;font-weight:700;color:#0b0d17;"
              "background:#22c55e;border-radius:10px;padding:3px 12px;"
            : "font-size:11px;font-weight:700;color:#64748b;"
              "background:#1e2235;border-radius:10px;padding:3px 12px;");
}

// ─────────────────────────────────────────────────────────────────────────────
void DashboardWindow::toggleDevice(int deviceId, const QString& deviceName,
                                   const QString& type)
{
    Device dev   = Database::instance().getDevice(deviceId);
    bool newState = !dev.isOn;

    Database::instance().setDeviceState(deviceId, newState);
    Database::instance().addActivityLog(
        deviceId, deviceName,
        QString("%1 turned %2").arg(deviceName).arg(newState ? "ON" : "OFF"));

    if (newState) {
        m_deviceOnTimes[deviceId] = QDateTime::currentDateTime();
    } else {
        if (m_deviceOnTimes.contains(deviceId)) {
            int mins = (int)(m_deviceOnTimes[deviceId]
                                  .secsTo(QDateTime::currentDateTime()) / 60);
            if (mins > 0) Database::instance().recordUsage(deviceId, mins);
            m_deviceOnTimes.remove(deviceId);
        }
    }

    if (m_deviceCards.contains(deviceId) && m_deviceCards[deviceId]) {
        m_deviceCards[deviceId]->setState(newState);
        if (newState)
            m_deviceCards[deviceId]->setOnTime(m_deviceOnTimes[deviceId]);
    }

    updateArduinoForDevice(deviceId, newState, type);
    m_house = Database::instance().getHouse(m_house.houseNumber);
    updateDevicesOnBadge();

    // Update the per-room badge for the room that contains this device
    for (const Room& r : m_house.rooms) {
        bool found = false;
        for (const Device& d : r.devices) {
            if (d.id == deviceId) { found = true; break; }
        }
        if (!found) continue;

        int onCount = 0;
        for (const Device& d : r.devices) if (d.isOn) onCount++;
        int total = r.devices.size();

        if (m_roomBadges.contains(r.id) && m_roomBadges[r.id]) {
            QLabel* badge = m_roomBadges[r.id];
            badge->setText(QString("%1/%2 ON").arg(onCount).arg(total));
            QString bg   = onCount > 0 ? "#22c55e" : "#374151";
            QString text = onCount > 0 ? "#0b0d17" : "#9ca3af";
            badge->setStyleSheet(
                QString("font-size:10px; font-weight:700; color:%1;"
                        "background:%2; border-radius:8px; padding:2px 8px;")
                    .arg(text).arg(bg));
        }
        break; // device found and badge updated, no need to check other rooms
    }

    refreshLogs();
    refreshUsage();
}

// ─────────────────────────────────────────────────────────────────────────────
void DashboardWindow::updateArduinoForDevice(int, bool isOn, const QString& type) {
    if (type == "light")
        ArduinoController::instance().controlLight(isOn);
}

// ─────────────────────────────────────────────────────────────────────────────
void DashboardWindow::refreshLogs() {
    m_logsList->clear();
    QList<ActivityLog> logs = Database::instance().getActivityLogs(m_house.id, 50);
    for (const ActivityLog& log : logs) {
        bool isOn   = log.action.contains("turned ON");
        QString lower = log.deviceName.toLower();
        QString icon;
        if      (lower.contains("light")) icon = "💡";
        else if (lower.contains("fan"))   icon = "🌀";
        else if (lower.contains("ac"))    icon = "❄️";
        else if (lower.contains("tv"))    icon = "📺";
        else                              icon = isOn ? "🟢" : "🔴";

        QString time = log.timestamp.toString("hh:mm AP");
        QString date = log.timestamp.toString("d MMM");

        QListWidgetItem* itm = new QListWidgetItem;
        itm->setText(QString("%1  %2   %3   %4")
                         .arg(icon).arg(time).arg(log.action).arg(date));
        itm->setForeground(isOn ? QColor("#e2e8f0") : QColor("#94a3b8"));
        m_logsList->addItem(itm);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
void DashboardWindow::refreshUsage() {
    double daily   = Database::instance().getDailyUsageHours(m_house.id)   * 0.5;
    double weekly  = Database::instance().getWeeklyUsageHours(m_house.id)  * 0.5;
    double monthly = Database::instance().getMonthlyUsageHours(m_house.id) * 0.5;
    double bill    = Database::instance().getEstimatedBill(m_house.id);

    m_dailyLabel->setText(  QString::number(daily,   'f', 1) + " kWh");
    m_weeklyLabel->setText( QString::number(weekly,  'f', 1) + " kWh");
    m_monthlyLabel->setText(QString::number(monthly, 'f', 1) + " kWh");
    m_billLabel->setText(   "PKR " + QString::number(bill, 'f', 0));

    // Daily series (7 days) — last point = today's actual value
    m_dailySeries.clear();
    double base = weekly / 7.0;
    for (double o : {0.6, 1.1, 0.9, 0.7, 0.5, 0.85, 1.0})
        m_dailySeries << qMax(0.0, base * o);
    if (!m_dailySeries.isEmpty())
        m_dailySeries.last() = daily;

    // Device breakdown grouped by type
    m_breakdown.clear();
    {
        auto details = Database::instance().getMonthlyUsageDetails(m_house.id);
        double kWhLights = 0, kWhFans = 0, kWhAC = 0, kWhTV = 0, kWhOther = 0;
        for (auto& pr : details) {
            double kWh = (pr.second / 60.0) * 0.5;
            if (kWh <= 0) continue;
            QString low = pr.first.toLower();
            if      (low.contains("light") || low.contains("lamp") || low.contains("bulb"))
                kWhLights += kWh;
            else if (low.contains("fan"))
                kWhFans   += kWh;
            else if (low.contains("ac")  || low.contains("air"))
                kWhAC     += kWh;
            else if (low.contains("tv")  || low.contains("television"))
                kWhTV     += kWh;
            else
                kWhOther  += kWh;
        }
        if (kWhLights > 0) m_breakdown << qMakePair(QString("Lights"), kWhLights);
        if (kWhFans   > 0) m_breakdown << qMakePair(QString("Fans"),   kWhFans);
        if (kWhAC     > 0) m_breakdown << qMakePair(QString("AC"),     kWhAC);
        if (kWhTV     > 0) m_breakdown << qMakePair(QString("TV"),     kWhTV);
        if (kWhOther  > 0) m_breakdown << qMakePair(QString("Other"),  kWhOther);
    }

    // Push data to canvas and repaint
    if (auto* canvas = static_cast<ChartsCanvas*>(m_chartsCanvas)) {
        canvas->dailySeries = m_dailySeries;
        canvas->breakdown   = m_breakdown;
        canvas->update();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
void DashboardWindow::refreshSchedules() {
    QLayoutItem* item;
    while ((item = m_schedulesLayout->takeAt(0))) {
        delete item->widget();
        delete item;
    }
    m_scheduleRowToId.clear();

    QList<Schedule> schedules = Database::instance().getSchedules(m_house.id);
    int row = 0;

    for (const Schedule& s : schedules) {
        if (s.executed) continue;

        QWidget* rowW = new QWidget;
        rowW->setObjectName("schedRow");
        rowW->setStyleSheet(
            "QWidget#schedRow { background:#111527; border-radius:8px;"
            "border:1px solid #1a1f35; }");
        rowW->setFixedHeight(50);

        QHBoxLayout* rl = new QHBoxLayout(rowW);
        rl->setContentsMargins(10, 5, 10, 5);
        rl->setSpacing(8);

        QLabel* timeLbl = new QLabel(s.scheduledTime.toString("hh:mm AP"));
        timeLbl->setStyleSheet(
            "font-size:12px; font-weight:700; color:#e2e8f0;"
            "background:transparent;");
        timeLbl->setFixedWidth(80);
        rl->addWidget(timeLbl);

        QLabel* devLbl = new QLabel(s.deviceName);
        devLbl->setStyleSheet(
            "font-size:12px; color:#cbd5e1; background:transparent;");
        rl->addWidget(devLbl, 1);

        QString actionStr = s.turnOn ? "Turn ON" : "Turn OFF";
        QLabel* actLbl = new QLabel(actionStr);
        actLbl->setStyleSheet(
            s.turnOn
                ? "font-size:11px; color:#4ade80; font-weight:700; background:transparent;"
                : "font-size:11px; color:#f87171; font-weight:700; background:transparent;");
        rl->addWidget(actLbl);

        QLabel* toggleViz = new QLabel;
        toggleViz->setFixedSize(14, 14);
        toggleViz->setStyleSheet(
            s.turnOn ? "background:#22c55e; border-radius:7px;"
                     : "background:#ef4444; border-radius:7px;");
        rl->addWidget(toggleViz);

        QPushButton* delBtn = new QPushButton("✕");
        delBtn->setFixedSize(20, 20);
        delBtn->setStyleSheet(
            "QPushButton { background:transparent; color:#4b5563; border:none; font-size:13px; }"
            "QPushButton:hover { color:#f87171; }");
        delBtn->setCursor(Qt::PointingHandCursor);
        int schedId = s.id;
        connect(delBtn, &QPushButton::clicked, this, [this, schedId]() {
            if (Database::instance().deleteSchedule(schedId))
                refreshSchedules();
        });
        rl->addWidget(delBtn);

        m_schedulesLayout->addWidget(rowW);
        m_scheduleRowToId[row++] = s.id;
    }

    if (row == 0) {
        QLabel* empty = new QLabel("No schedules set.");
        empty->setStyleSheet(
            "font-size:12px; color:#374151; background:transparent;");
        empty->setAlignment(Qt::AlignCenter);
        m_schedulesLayout->addWidget(empty);
    }
    m_schedulesLayout->addStretch();
}

// ─────────────────────────────────────────────────────────────────────────────
void DashboardWindow::refreshAll() {
    buildDeviceCards();
    refreshLogs();
    refreshUsage();
    refreshSchedules();
}

// ─────────────────────────────────────────────────────────────────────────────
void DashboardWindow::onScheduleTimerTick() {
    QList<Schedule> pending = Database::instance().getPendingSchedules();
    for (const Schedule& s : pending) {
        Database::instance().setDeviceState(s.deviceId, s.turnOn);
        Database::instance().markScheduleExecuted(s.id);
        Database::instance().addActivityLog(
            s.deviceId, s.deviceName,
            QString("[Scheduled] %1 turned %2")
                .arg(s.deviceName).arg(s.turnOn ? "ON" : "OFF"));

        if (s.turnOn) {
            m_deviceOnTimes[s.deviceId] = QDateTime::currentDateTime();
        } else {
            if (m_deviceOnTimes.contains(s.deviceId)) {
                int mins = (int)(m_deviceOnTimes[s.deviceId]
                                      .secsTo(QDateTime::currentDateTime()) / 60);
                if (mins > 0) Database::instance().recordUsage(s.deviceId, mins);
                m_deviceOnTimes.remove(s.deviceId);
            }
        }

        Device dev = Database::instance().getDevice(s.deviceId);
        updateArduinoForDevice(s.deviceId, s.turnOn, dev.type);

        if (m_deviceCards.contains(s.deviceId) && m_deviceCards[s.deviceId])
            m_deviceCards[s.deviceId]->setState(s.turnOn);
    }

    if (!pending.isEmpty()) {
        m_house = Database::instance().getHouse(m_house.houseNumber);
        updateDevicesOnBadge();
        refreshLogs();
        refreshSchedules();
        buildDeviceCards();
    }

    flushUsage();
    refreshUsage();
}

// ─────────────────────────────────────────────────────────────────────────────
void DashboardWindow::onUsageTimerTick() {
    flushUsage();
    refreshUsage();
}

// ─────────────────────────────────────────────────────────────────────────────
void DashboardWindow::openScheduleDialog() {
    m_house = Database::instance().getHouse(m_house.houseNumber);
    ScheduleDialog dlg(m_house.rooms, this);
    if (dlg.exec() == QDialog::Accepted)
        refreshSchedules();
}

// ─────────────────────────────────────────────────────────────────────────────
void DashboardWindow::onLogout() {
    flushUsage();
    ArduinoController::instance().disconnectArduino();
    emit logoutRequested();
}

// ─────────────────────────────────────────────────────────────────────────────
void DashboardWindow::onTurnAllOn() {
    m_house = Database::instance().getHouse(m_house.houseNumber);
    for (const Room& r : m_house.rooms)
        for (const Device& d : r.devices)
            if (!d.isOn) {
                Database::instance().setDeviceState(d.id, true);
                Database::instance().addActivityLog(d.id, d.name, d.name + " turned ON");
                m_deviceOnTimes[d.id] = QDateTime::currentDateTime();
                if (d.type == "light") ArduinoController::instance().controlLight(true);
            }
    buildDeviceCards(); refreshLogs(); refreshUsage();
}

void DashboardWindow::onTurnAllOff() {
    m_house = Database::instance().getHouse(m_house.houseNumber);
    for (const Room& r : m_house.rooms)
        for (const Device& d : r.devices)
            if (d.isOn) {
                if (m_deviceOnTimes.contains(d.id)) {
                    int mins = (int)(m_deviceOnTimes[d.id]
                                          .secsTo(QDateTime::currentDateTime()) / 60);
                    if (mins > 0) Database::instance().recordUsage(d.id, mins);
                    m_deviceOnTimes.remove(d.id);
                }
                Database::instance().setDeviceState(d.id, false);
                Database::instance().addActivityLog(d.id, d.name, d.name + " turned OFF");
                if (d.type == "light") ArduinoController::instance().controlLight(false);
            }
    buildDeviceCards(); refreshLogs(); refreshUsage();
}

void DashboardWindow::onNightMode() {
    onTurnAllOff();
    QMessageBox::information(this, "Night Mode",
                             "Night mode activated.\nAll devices turned off.");
}

void DashboardWindow::onAwayMode() {
    onTurnAllOff();
    QMessageBox::information(this, "Away Mode",
                             "Away mode activated.\nAll devices turned off for safety.");
}

// ─────────────────────────────────────────────────────────────────────────────
void DashboardWindow::onDeleteSchedule() { /* handled via inline delete buttons */ }

// ─────────────────────────────────────────────────────────────────────────────
void DashboardWindow::onRenameDevice() {
    m_house = Database::instance().getHouse(m_house.houseNumber);
    QStringList names;
    QList<int>  ids;
    for (const Room& r : m_house.rooms)
        for (const Device& d : r.devices) {
            names << QString("%1 — %2").arg(r.name).arg(d.name);
            ids   << d.id;
        }
    if (names.isEmpty()) {
        QMessageBox::information(this, "No Devices", "No devices to rename.");
        return;
    }
    bool ok;
    QString sel = QInputDialog::getItem(this, "Rename Device",
                                        "Select device:", names, 0, false, &ok);
    if (!ok) return;
    int idx = names.indexOf(sel);
    QString newName = QInputDialog::getText(this, "Rename Device",
                                            "New name:", QLineEdit::Normal, "", &ok);
    if (!ok || newName.trimmed().isEmpty()) return;
    if (Database::instance().renameDevice(ids[idx], newName.trimmed()))
        QTimer::singleShot(0, this, [this]{ buildDeviceCards(); refreshSchedules(); });
    else
        QMessageBox::warning(this, "Error", "Failed to rename device.");
}

// ─────────────────────────────────────────────────────────────────────────────
void DashboardWindow::drawUsageCharts() { /* handled by ChartsCanvas::paintEvent */ }

// ─────────────────────────────────────────────────────────────────────────────
void DashboardWindow::onExportPDF() {
    QString nameSlug = m_house.ownerName.isEmpty()
    ? m_house.houseNumber
    : QString(m_house.ownerName).replace(' ', '_');
    QString defaultName = QString("%1_UsageReport_%2.pdf")
                              .arg(nameSlug)
                              .arg(QDate::currentDate().toString("yyyy-MM"));
    QString path = QFileDialog::getSaveFileName(
        this, "Export Usage Report as PDF", defaultName, "PDF Files (*.pdf)");
    if (path.isEmpty()) return;

    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(path);
    printer.setPageSize(QPageSize(QPageSize::A4));
    printer.setPageMargins(QMarginsF(15, 15, 15, 15), QPageLayout::Millimeter);

    QPainter p;
    if (!p.begin(&printer)) {
        QMessageBox::warning(this, "Error", "Could not open file for writing.");
        return;
    }

    const int W = printer.pageRect(QPrinter::DevicePixel).width();
    int y = 0;

    p.fillRect(0, 0, W, 160, QColor("#0f1117"));
    p.setPen(QColor("#7cb9ff"));
    p.setFont(QFont("Segoe UI", 28, QFont::Bold));
    p.drawText(0, 0, W, 100, Qt::AlignCenter, "Smart Home");
    p.setFont(QFont("Segoe UI", 14));
    p.setPen(QColor("#64748b"));
    p.drawText(0, 80, W, 50, Qt::AlignCenter, "Electricity Usage Report");
    y = 170;

    auto infoRow = [&](const QString& label, const QString& value) {
        p.setFont(QFont("Segoe UI", 11)); p.setPen(QColor("#64748b"));
        p.drawText(40, y, 300, 36, Qt::AlignLeft | Qt::AlignVCenter, label);
        p.setPen(QColor("#e2e8f0")); p.setFont(QFont("Segoe UI", 11, QFont::Bold));
        p.drawText(340, y, W - 380, 36, Qt::AlignLeft | Qt::AlignVCenter, value);
        y += 38;
    };
    if (!m_house.ownerName.isEmpty()) infoRow("Owner Name:", m_house.ownerName);
    infoRow("House Number:", m_house.houseNumber);
    infoRow("Report Period:", "Last 30 Days");
    infoRow("Generated:", QDateTime::currentDateTime().toString("yyyy-MM-dd  hh:mm"));
    y += 10;
    p.save(); p.setPen(QPen(QColor("#1e2235"), 2));
    p.drawLine(0, y, W, y); p.restore(); y += 20;

    double daily   = Database::instance().getDailyUsageHours(m_house.id);
    double weekly  = Database::instance().getWeeklyUsageHours(m_house.id);
    double monthly = Database::instance().getMonthlyUsageHours(m_house.id);
    double bill    = Database::instance().getEstimatedBill(m_house.id);

    int bw = (W - 60) / 4;
    auto statBox = [&](const QString& label, const QString& value,
                       const QColor& color, int x, int w) {
        p.fillRect(x, y, w, 100, QColor("#13161f"));
        p.setPen(color); p.setFont(QFont("Segoe UI", 18, QFont::Bold));
        p.drawText(x, y, w, 65, Qt::AlignCenter, value);
        p.setPen(QColor("#64748b")); p.setFont(QFont("Segoe UI", 10));
        p.drawText(x, y + 60, w, 36, Qt::AlignCenter, label);
    };
    statBox("Today",      QString::number(daily,   'f', 1) + " hrs", QColor("#7cb9ff"), 0,        bw);
    statBox("This Week",  QString::number(weekly,  'f', 1) + " hrs", QColor("#22c55e"), bw + 20,  bw);
    statBox("This Month", QString::number(monthly, 'f', 1) + " hrs", QColor("#fbbf24"), bw*2 + 40, bw);
    statBox("Est. Bill",  "PKR " + QString::number(bill, 'f', 0),    QColor("#f87171"), bw*3 + 60, bw);
    y += 120;
    p.save(); p.setPen(QPen(QColor("#1e2235"), 1));
    p.drawLine(0, y, W, y); p.restore(); y += 20;

    p.setFont(QFont("Segoe UI", 13, QFont::Bold));
    p.setPen(QColor("#7cb9ff"));
    p.drawText(0, y, W, 40, Qt::AlignLeft | Qt::AlignVCenter,
               "  Device Breakdown (Last 30 Days)");
    y += 46;
    p.fillRect(0, y, W, 40, QColor("#13161f"));
    p.setFont(QFont("Segoe UI", 10, QFont::Bold)); p.setPen(QColor("#64748b"));
    int c1 = 40, c2 = W / 2, c3 = W * 3 / 4;
    p.drawText(c1, y, c2 - c1, 40, Qt::AlignLeft  | Qt::AlignVCenter, "Device Name");
    p.drawText(c2, y, c3 - c2, 40, Qt::AlignCenter | Qt::AlignVCenter, "Usage (hrs)");
    p.drawText(c3, y, W - c3 - 20, 40, Qt::AlignRight | Qt::AlignVCenter, "Est. Cost (PKR)");
    y += 44;

    auto details = Database::instance().getMonthlyUsageDetails(m_house.id);
    int totalMins = 0; bool shade = false;
    for (auto& pair : details) {
        if (shade) p.fillRect(0, y, W, 38, QColor("#0f1117"));
        shade = !shade;
        double hrs  = pair.second / 60.0;
        double cost = hrs * 0.5 * 50.0;
        totalMins  += pair.second;
        p.setFont(QFont("Segoe UI", 10)); p.setPen(QColor("#e2e8f0"));
        p.drawText(c1, y, c2 - c1, 38, Qt::AlignLeft  | Qt::AlignVCenter, pair.first);
        p.setPen(QColor("#7cb9ff"));
        p.drawText(c2, y, c3 - c2, 38, Qt::AlignCenter | Qt::AlignVCenter, QString::number(hrs, 'f', 2));
        p.setPen(QColor("#22c55e"));
        p.drawText(c3, y, W - c3 - 20, 38, Qt::AlignRight | Qt::AlignVCenter, QString::number(cost, 'f', 0));
        y += 38;
    }
    y += 6;
    p.fillRect(0, y, W, 44, QColor("#0f1d2e"));
    double totalHrs  = totalMins / 60.0;
    double totalCost = totalHrs * 0.5 * 50.0;
    p.setFont(QFont("Segoe UI", 11, QFont::Bold)); p.setPen(QColor("#fbbf24"));
    p.drawText(c1, y, c2 - c1, 44, Qt::AlignLeft  | Qt::AlignVCenter, "TOTAL");
    p.drawText(c2, y, c3 - c2, 44, Qt::AlignCenter | Qt::AlignVCenter,
               QString::number(totalHrs, 'f', 2) + " hrs");
    p.drawText(c3, y, W - c3 - 20, 44, Qt::AlignRight | Qt::AlignVCenter,
               "PKR " + QString::number(totalCost, 'f', 0));
    y += 60;
    p.setFont(QFont("Segoe UI", 9)); p.setPen(QColor("#374151"));
    p.drawText(0, y, W, 30, Qt::AlignCenter,
               "Generated by Smart Home v1.0  •  OOP Lab Project");
    p.end();

    QMessageBox::information(this, "Exported",
                             QString("PDF report saved to:\n%1").arg(path));
}

// ─────────────────────────────────────────────────────────────────────────────
void DashboardWindow::onAbout() {
    QMessageBox about(this);
    about.setWindowTitle("About");
    about.setTextFormat(Qt::RichText);
    about.setText(R"(
        <div style='font-family:Segoe UI; text-align:center;'>
            <h2 style='color:#7cb9ff;'>🏠 Smart Home</h2>
            <p style='color:#94a3b8;'>Electricity Control & Monitoring System</p>
            <hr>
            <p><b>Version:</b> 1.0.0</p>
            <p><b>Project:</b> OOP Lab Project</p>
            <p><b>Built with:</b> Qt6 · C++17 · SQLite · Arduino</p>
        </div>
    )");
    about.setStyleSheet(
        "QMessageBox { background-color:#0f1117; color:#e2e8f0; }"
        "QLabel { color:#e2e8f0; min-width:280px; }"
        "QPushButton { background:#7cb9ff; color:#0b0d17; border:none;"
        "border-radius:6px; padding:6px 20px; font-weight:bold; }");
    about.exec();
}