#include <rclcpp/rclcpp.hpp>

#include <QApplication>
#include <QTimer>

#include "luco_monitor/mainwindow.hpp"
#include "luco_monitor/theme.hpp"

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  QApplication app(argc, argv);

  // 바탕화면이 다크 모드여도 항상 같은 밝은 회색 화면으로 표시
  luco_monitor::applyLightTheme(app);

  auto node = std::make_shared<rclcpp::Node>("luco_monitor");

  MainWindow window(node);
  window.show();

  // Qt 화면과 ROS를 함께 돌리기: 20 ms마다 ROS 콜백(토픽 수신 등)을 처리
  // 터미널에서 Ctrl+C를 누르면 ROS가 종료되고 창도 닫힘
  QTimer ros_timer;
  QObject::connect(&ros_timer, &QTimer::timeout, [&app, node]() {
    if (!rclcpp::ok()) {
      app.quit();
      return;
    }
    rclcpp::spin_some(node);
  });
  ros_timer.start(20);

  const int ret = app.exec();
  rclcpp::shutdown();
  return ret;
}
