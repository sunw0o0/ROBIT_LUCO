# luco_monitor

LUCO 모니터링 화면(Qt) + WASD 키보드 조종 노드.

## 빌드와 실행

```bash
cd ~/test_ws && colcon build --packages-select luco_monitor && source install/setup.bash
ros2 run luco_monitor luco_monitor
```

화면이 있는 PC(노트북, 운영 PC)에서 실행합니다. SSH 접속 상태에서는 창이 뜨지 않습니다.
로봇 PC와 `ROS_DOMAIN_ID`가 같아야 합니다(상단 `DOMAIN` 칸에서 확인).

## 파라미터

| 이름 | 기본값 | 설명 |
|---|---|---|
| `cmd_vel_topic` | `/diff_drive_controller/cmd_vel` | 키보드 명령을 보낼 토픽 |
| `stamp_with_now` | `false` | `false`면 stamp를 0으로 보내고 컨트롤러가 수신 시각으로 채움 (PC 간 시계 차이 영향 없음) |
| `max_linear` | `1.0` | 직진 속도 입력 상한 [m/s] |
| `max_angular` | `1.2` | 회전 속도 입력 상한 [rad/s] |

```bash
ros2 run luco_monitor luco_monitor --ros-args -p cmd_vel_topic:=/cmd_vel_joy
```

## 키보드 조종 (TELEOP)

- `ENABLE` 체크 후 W/S(전진/후진), A/D(좌/우 회전). 누르고 있는 동안만 명령을 보냄 (20 Hz)
- 키를 떼면 0 명령 5회(0.25 s) 후 아무것도 보내지 않음 → 다른 주행 소스와 충돌하지 않음
- `SPACE` 또는 `STOP` 버튼: 즉시 정지 (`STOP`은 조종도 꺼짐)
- 창 포커스를 잃으면 자동 정지
- 한/영 키가 한글이면 키가 안 먹을 수 있음
- 조이스틱 노드가 같은 토픽에 0을 계속 보내면 섞이므로, 키보드로 조종할 때는
  `ros2 launch luco_bringup luco.launch.py with_joy:=false`

**처음 시험할 때는 바퀴를 띄운 상태에서 방향을 먼저 확인하세요.**

## 화면 갱신 도우미 (mainwindow.cpp)

토픽 구독 콜백에서 아래 함수로 화면을 바꿉니다.

```cpp
setLamp(ui->hbBase, "ok", "BASE");                 // off / ok / warn / error
setTableRow(ui->canTable, 0, {"VESC_L", "110/0x6E", "50.0", "18", "OK"});
setTableRow(ui->dxlTable, 4, {"J5", "15", "TIMEOUT", "--", "--", "--", "-"}, true);  // true = 강조
logEvent("WARN", "DXL J5 (ID 15) no response");
```

| 위젯 | 내용 |
|---|---|
| `runStatePill`, `driveSourceValue`, `batteryValue`, `estopPill`, `domainValue`, `uptimeValue` | 상단 상태줄 |
| `mainView`, `lineView`, `mapView`, `armView` | 영상 칸 (제목의 토픽 이름은 예시, `.ui`에서 수정) |
| `missionStateLabel`, `missionDescLabel`, `missionTimeLabel`, `missionRetryLabel`, `missionProgress`, `missionStepList` | 미션 |
| `hbBase`, `hbNav`, `hbDrive`, `hbVision`, `hbArm`, `hbPlanner` | 하트비트 램프 |
| `canStatePill`, `canTable`, `canBusStats` | CAN0 탭 |
| `dxlSummaryPill`, `dxlTable` | Dynamixel 탭 (J1~J7, 실제 이름으로 수정) |
| `eventLog` | 이벤트 로그 |
