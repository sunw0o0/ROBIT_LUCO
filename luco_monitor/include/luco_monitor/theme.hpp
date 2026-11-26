#pragma once

#include <QApplication>
#include <QColor>
#include <QPalette>
#include <QStyleFactory>

namespace luco_monitor
{

// 바탕화면 테마(다크 모드 등)와 상관없이 항상 같은 밝은 회색 화면이 되도록
// Fusion 스타일 + 모든 색을 직접 지정한 팔레트를 적용
inline void applyLightTheme(QApplication & app)
{
  app.setStyle(QStyleFactory::create("Fusion"));

  const QColor window(0xef, 0xef, 0xef);
  const QColor text(0x1a, 0x1a, 0x1a);
  const QColor base(0xff, 0xff, 0xff);
  const QColor button(0xe6, 0xe6, 0xe6);
  const QColor disabled_text(0x9a, 0x9a, 0x9a);

  QPalette p;
  for (auto group : {QPalette::Active, QPalette::Inactive, QPalette::Disabled}) {
    const bool dis = group == QPalette::Disabled;
    p.setColor(group, QPalette::Window, window);
    p.setColor(group, QPalette::WindowText, dis ? disabled_text : text);
    p.setColor(group, QPalette::Base, base);
    p.setColor(group, QPalette::AlternateBase, QColor(0xf5, 0xf5, 0xf5));
    p.setColor(group, QPalette::Text, dis ? disabled_text : text);
    p.setColor(group, QPalette::Button, button);
    p.setColor(group, QPalette::ButtonText, dis ? disabled_text : text);
    p.setColor(group, QPalette::BrightText, QColor(0xff, 0xff, 0xff));
    p.setColor(group, QPalette::Highlight, QColor(0x3a, 0x3a, 0x3a));
    p.setColor(group, QPalette::HighlightedText, QColor(0xff, 0xff, 0xff));
    p.setColor(group, QPalette::ToolTipBase, QColor(0xff, 0xff, 0xe1));
    p.setColor(group, QPalette::ToolTipText, text);
    p.setColor(group, QPalette::PlaceholderText, QColor(0x80, 0x80, 0x80));
    p.setColor(group, QPalette::Light, QColor(0xff, 0xff, 0xff));
    p.setColor(group, QPalette::Midlight, QColor(0xe0, 0xe0, 0xe0));
    p.setColor(group, QPalette::Mid, QColor(0xb0, 0xb0, 0xb0));
    p.setColor(group, QPalette::Dark, QColor(0x9a, 0x9a, 0x9a));
    p.setColor(group, QPalette::Shadow, QColor(0x6a, 0x6a, 0x6a));
    p.setColor(group, QPalette::Link, text);
  }
  app.setPalette(p);
}

}  // namespace luco_monitor
