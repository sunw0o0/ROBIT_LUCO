#pragma once

#include <geometry_msgs/msg/twist_stamped.hpp>
#include <rclcpp/rclcpp.hpp>

#include <QElapsedTimer>
#include <QLabel>
#include <QMainWindow>
#include <QSet>
#include <QStringList>
#include <QTableWidget>
#include <QTimer>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  explicit MainWindow(rclcpp::Node::SharedPtr node, QWidget * parent = nullptr);
  ~MainWindow() override;

protected:
  // 창 어디에 포커스가 있든 W/A/S/D/Space 키를 가로채기 위해 사용
  bool eventFilter(QObject * obj, QEvent * event) override;

private:
  // ---- 화면 갱신 도우미 (토픽 구독 콜백에서 사용) ----
  // 상태 램프. state: "off"(정보 없음) "ok"(정상) "warn"(경고) "error"(에러)
  void setLamp(QLabel * label, const QString & state, const QString & text);
  // 표의 한 줄을 통째로 채우기 (CAN 표, Dynamixel 표). highlight=true면 회색 배경 + 굵게
  void setTableRow(QTableWidget * table, int row, const QStringList & cells, bool highlight = false);
  // 이벤트 로그. level: "INFO" "WARN" "ERROR"
  void logEvent(const QString & level, const QString & text);

  // ---- 키보드 조종 ----
  void onTeleopToggled(bool enabled);
  void publishTeleop();                 // 20 Hz 타이머에서 호출
  void releaseAllKeys(const QString & reason);
  void updateKeyCaps();

  Ui::MainWindow * ui;
  rclcpp::Node::SharedPtr node_;

  rclcpp::Publisher<geometry_msgs::msg::TwistStamped>::SharedPtr cmd_pub_;
  QTimer teleop_timer_;
  QSet<int> pressed_keys_;   // 지금 눌려 있는 키 (W/A/S/D)
  bool space_down_ = false;  // Space 키 표시용
  int zero_burst_ = 0;       // 키를 뗀 뒤 보낼 남은 정지(0) 명령 수
  bool stamp_with_now_ = false;

  QTimer uptime_timer_;
  QElapsedTimer uptime_;
};
