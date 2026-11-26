from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    return LaunchDescription([
        Node(
            package='luco_monitor',
            executable='luco_monitor',
            name='luco_monitor',
            output='screen',
        ),
    ])
