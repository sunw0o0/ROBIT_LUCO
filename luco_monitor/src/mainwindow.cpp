#include "luco_monitor/mainwindow.hpp"
#include "ui_mainwindow.h"

#include <QApplication>
#include <QDateTime>
#include <QHeaderView>
#include <QKeyEvent>
#include <QStyle>

namespace
{
constexpr int kTeleopPeriodMs = 50;   // 20 Hz (cmd_vel_timeout 0.5 s보다 충분히 짧게)
constexpr int kZeroBurstCount = 5;    // 키를 떼면 정지 명령을 5번(0.25 s) 보내고 멈춤

void repolish(QWidget * w)
{
  w->style()->unpolish(w);
  w->style()->polish(w);
}

QString signedFixed(double value)
{
  return QString("%1%2").arg(value >= 0.0 ? "+" : "").arg(value, 0, 'f', 2);
}
}  // namespace

MainWindow::MainWindow(rclcpp::Node::SharedPtr node, QWidget * parent)
: QMainWindow(parent), ui(new Ui::MainWindow), node_(std::move(node))
{
  ui->setupUi(this);

  // ---- ROS 파라미터 ----
  // cmd_vel_topic : 키보드 명령을 보낼 토픽
  // stamp_with_now: true면 이 PC 시각으로 stamp를 찍음.
  //                 false(기본)면 stamp를 비워 보내고, 받는 쪽 컨트롤러가 수신 시각으로 채움
  //                 → 운영 PC와 로봇 PC 시계가 어긋나도 명령이 버려지지 않음
  const auto topic = node_->declare_parameter<std::string>(
    "cmd_vel_topic", "/diff_drive_controller/cmd_vel");
  stamp_with_now_ = node_->declare_parameter<bool>("stamp_with_now", false);
  const double max_linear = node_->declare_parameter<double>("max_linear", 1.0);
  const double max_angular = node_->declare_parameter<double>("max_angular", 1.2);
  ui->linearSpeedSpin->setMaximum(max_linear);
  ui->angularSpeedSpin->setMaximum(max_angular);

  cmd_pub_ = node_->create_publisher<geometry_msgs::msg::TwistStamped>(topic, 10);
  const QString qtopic = QString::fromStdString(topic);
  ui->teleopBox->setTitle(QString("TELEOP (WASD) → %1").arg(qtopic));

  // ---- 상단 상태줄 / 아래 상태 줄 ----
  const QByteArray domain = qgetenv("ROS_DOMAIN_ID");
  const QString domain_text = domain.isEmpty() ? QString("0") : QString::fromUtf8(domain);
  ui->domainValue->setText(domain_text);
  if (domain.isEmpty()) {
    ui->domainValue->setToolTip("ROS_DOMAIN_ID가 설정되지 않아 기본값 0을 사용 중");
  }
  const QByteArray rmw = qgetenv("RMW_IMPLEMENTATION");
  const QString node_name = QString::fromStdString(node_->get_fully_qualified_name());
  ui->statusbar->showMessage(
    QString("node=%1   ROS_DOMAIN_ID=%2   rmw=%3   cmd_vel=%4   stamp=%5")
    .arg(node_name, domain.isEmpty() ? QString("0(default)") : domain_text,
    rmw.isEmpty() ? QString("default") : QString::fromUtf8(rmw), qtopic,
    stamp_with_now_ ? QString("now") : QString("zero")));

  uptime_.start();
  connect(&uptime_timer_, &QTimer::timeout, this, [this]() {
      const qint64 s = uptime_.elapsed() / 1000;
      ui->uptimeValue->setText(QString("%1:%2:%3")
        .arg(s / 3600, 2, 10, QChar('0'))
        .arg((s / 60) % 60, 2, 10, QChar('0'))
        .arg(s % 60, 2, 10, QChar('0')));
    });
  uptime_timer_.start(1000);

  // ---- 표: 글자 길이에 맞춰 열 너비, 마지막 열은 남은 공간 채우기 ----
  for (QTableWidget * t : {ui->canTable, ui->dxlTable}) {
    t->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    t->horizontalHeader()->setStretchLastSection(true);
  }

  // ---- 키보드 조종 ----
  qApp->installEventFilter(this);
  connect(ui->teleopEnableCheck, &QCheckBox::toggled, this, &MainWindow::onTeleopToggled);
  connect(&teleop_timer_, &QTimer::timeout, this, &MainWindow::publishTeleop);
  teleop_timer_.start(kTeleopPeriodMs);

  // 다른 창을 클릭하는 등 이 창이 포커스를 잃으면 키를 뗀 것으로 처리 (계속 달리는 것 방지)
  connect(qApp, &QGuiApplication::applicationStateChanged, this,
    [this](Qt::ApplicationState state) {
      if (state != Qt::ApplicationActive && !pressed_keys_.isEmpty()) {
        releaseAllKeys("focus lost -> STOP");
      }
    });

  // ---- 버튼 ----
  connect(ui->stopButton, &QPushButton::clicked, this, [this]() {
      ui->teleopEnableCheck->setChecked(false);
      releaseAllKeys("STOP button");
    });
  // TODO: Task Planner와 연결되면 서비스/토픽 호출로 바꾸기
  connect(ui->startButton, &QPushButton::clicked, this, [this]() {
      logEvent("WARN", "START pressed (task planner not connected)");
    });
  connect(ui->pauseButton, &QPushButton::clicked, this, [this]() {
      logEvent("WARN", "PAUSE pressed (task planner not connected)");
    });
  connect(ui->resetButton, &QPushButton::clicked, this, [this]() {
      logEvent("WARN", "RESET pressed (task planner not connected)");
    });

  logEvent("INFO", QString("monitor started  node=%1  domain=%2").arg(node_name, domain_text));
  RCLCPP_INFO(node_->get_logger(), "LUCO monitor started (cmd_vel: %s)", topic.c_str());

  // TODO: 여기에 토픽 구독을 추가해서 화면을 갱신하세요.
  //  예) 미션 상태 → missionStateLabel, missionStepList, missionProgress
  //      하트비트   → setLamp(ui->hbBase, "ok", "BASE")
  //      CAN        → setTableRow(ui->canTable, 0, {"VESC_L", "110/0x6E", "50.0", "18", "OK"})
  //      Dynamixel  → setTableRow(ui->dxlTable, 4, {"J5", "15", "TIMEOUT", "--", "--", "--", "-"}, true)
}

MainWindow::~MainWindow()
{
  qApp->removeEventFilter(this);
  delete ui;
}

// ============================================================
// 키보드 조종
// ============================================================

bool MainWindow::eventFilter(QObject * obj, QEvent * event)
{
  const bool is_key_event =
    event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease;
  if (!is_key_event || !ui->teleopEnableCheck->isChecked() || !isActiveWindow()) {
    return QMainWindow::eventFilter(obj, event);
  }

  auto * key_event = static_cast<QKeyEvent *>(event);
  const int key = key_event->key();
  const bool press = event->type() == QEvent::KeyPress;

  // Space: 즉시 정지
  if (key == Qt::Key_Space) {
    if (!key_event->isAutoRepeat()) {
      space_down_ = press;
      if (press) {
        releaseAllKeys("SPACE -> STOP");
      } else {
        updateKeyCaps();
      }
    }
    return true;
  }

  const bool is_drive_key =
    key == Qt::Key_W || key == Qt::Key_A || key == Qt::Key_S || key == Qt::Key_D;
  if (!is_drive_key) {
    return QMainWindow::eventFilter(obj, event);
  }

  // 키를 꾹 누르고 있을 때 운영체제가 만드는 반복 입력은 무시
  if (key_event->isAutoRepeat()) {
    return true;
  }

  if (press) {
    pressed_keys_.insert(key);
  } else {
    pressed_keys_.remove(key);
    if (pressed_keys_.isEmpty()) {
      zero_burst_ = kZeroBurstCount;
    }
  }
  updateKeyCaps();
  return true;   // 다른 위젯(버튼, 입력칸)으로 키가 넘어가지 않게 함
}

void MainWindow::onTeleopToggled(bool enabled)
{
  if (enabled) {
    setLamp(ui->teleopStatePill, "ok", "ON");
    ui->driveSourceValue->setText("KEYBOARD");
    logEvent("INFO", QString("teleop ENABLED  v_max=%1  w_max=%2")
      .arg(ui->linearSpeedSpin->value(), 0, 'f', 2)
      .arg(ui->angularSpeedSpin->value(), 0, 'f', 2));
    setFocus();   // 입력칸에 커서가 있으면 키가 헷갈리지 않게 창으로 포커스 이동
  } else {
    setLamp(ui->teleopStatePill, "off", "OFF");
    ui->driveSourceValue->setText("NONE");
    releaseAllKeys("teleop DISABLED");
  }
}

void MainWindow::releaseAllKeys(const QString & reason)
{
  pressed_keys_.clear();
  zero_burst_ = kZeroBurstCount;
  updateKeyCaps();
  logEvent("INFO", reason);
}

void MainWindow::publishTeleop()
{
  if (!ui->teleopEnableCheck->isChecked()) {
    pressed_keys_.clear();
  }

  double v = 0.0;   // 직진 속도 (m/s), + 앞
  double w = 0.0;   // 회전 속도 (rad/s), + 왼쪽(반시계)

  if (pressed_keys_.isEmpty()) {
    // 키를 안 누르고 있으면 아무것도 보내지 않음
    // (Nav나 Task Planner 등 다른 주행 소스와 명령이 섞이지 않게)
    // 단, 키를 막 뗐을 때는 정지 명령을 몇 번 보내서 확실히 멈춤
    if (zero_burst_ <= 0) {
      ui->teleopCmdLabel->setText("cmd: --");
      return;
    }
    --zero_burst_;
  } else {
    const double lin = ui->linearSpeedSpin->value();
    const double ang = ui->angularSpeedSpin->value();
    if (pressed_keys_.contains(Qt::Key_W)) {v += lin;}
    if (pressed_keys_.contains(Qt::Key_S)) {v -= lin;}
    if (pressed_keys_.contains(Qt::Key_A)) {w += ang;}
    if (pressed_keys_.contains(Qt::Key_D)) {w -= ang;}
  }

  geometry_msgs::msg::TwistStamped msg;
  if (stamp_with_now_) {
    msg.header.stamp = node_->now();
  }   // 아니면 stamp = 0 → 컨트롤러가 받은 시각으로 채움
  msg.twist.linear.x = v;
  msg.twist.angular.z = w;
  cmd_pub_->publish(msg);

  ui->teleopCmdLabel->setText(
    QString("cmd: v=%1 m/s  w=%2 rad/s").arg(signedFixed(v), signedFixed(w)));
}

void MainWindow::updateKeyCaps()
{
  const std::pair<QLabel *, bool> caps[] = {
    {ui->keyW, pressed_keys_.contains(Qt::Key_W)},
    {ui->keyA, pressed_keys_.contains(Qt::Key_A)},
    {ui->keyS, pressed_keys_.contains(Qt::Key_S)},
    {ui->keyD, pressed_keys_.contains(Qt::Key_D)},
    {ui->keySpace, space_down_}};
  for (const auto & [label, active] : caps) {
    label->setProperty("active", active);
    repolish(label);
  }
}

// ============================================================
// 화면 갱신 도우미
// ============================================================

void MainWindow::setLamp(QLabel * label, const QString & state, const QString & text)
{
  label->setText(text);
  label->setProperty("state", state);
  repolish(label);   // 스타일시트 다시 적용
}

void MainWindow::setTableRow(
  QTableWidget * table, int row, const QStringList & cells, bool highlight)
{
  if (row < 0 || row >= table->rowCount()) {
    return;
  }
  for (int col = 0; col < table->columnCount() && col < cells.size(); ++col) {
    auto * item = table->item(row, col);
    if (item == nullptr) {
      item = new QTableWidgetItem();
      table->setItem(row, col, item);
    }
    item->setText(cells.at(col));
    // 숫자 열은 오른쪽 정렬, 글자 열은 왼쪽 정렬
    bool is_number = false;
    cells.at(col).toDouble(&is_number);
    item->setTextAlignment(Qt::AlignVCenter | (is_number ? Qt::AlignRight : Qt::AlignLeft));
    item->setBackground(highlight ? QBrush(QColor(0xcf, 0xcf, 0xcf)) : QBrush());
    QFont f = item->font();
    f.setBold(highlight);
    item->setFont(f);
  }
}

void MainWindow::logEvent(const QString & level, const QString & text)
{
  ui->eventLog->appendPlainText(
    QString("[%1] [%2] %3")
    .arg(QDateTime::currentDateTime().toString("HH:mm:ss.zzz"), level.leftJustified(5), text));
}
