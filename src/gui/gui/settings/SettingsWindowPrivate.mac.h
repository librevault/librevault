#include <QMacToolbar>
#include <QShowEvent>
#include <QVBoxLayout>

class SettingsWindow;
class SettingsWindowPrivate {
 public:
  SettingsWindowPrivate(SettingsWindow* window);

  void add_pane(QWidget* pane);

 private:
  // Window
  SettingsWindow* window_;
  QVBoxLayout* window_layout_;
  QVBoxLayout* pane_layout_;

  // Pager
  QMacToolBar* toolbar;
  QList<QMacToolBarItem*> buttons_;
  QList<QWidget*> panes_;

 private slots:
  void showPane(int pane);
};
