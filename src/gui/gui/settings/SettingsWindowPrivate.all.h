#include <QHBoxLayout>
#include <QStackedWidget>
#include <QToolButton>

class SettingsWindow;
class SettingsWindowPrivate {
 public:
  SettingsWindowPrivate(SettingsWindow* window);

  void add_pane(QWidget* pane);

 private:
  // Window
  SettingsWindow* window_;
  QVBoxLayout* window_layout_;

  // Pager
  QHBoxLayout* button_layout_;
  QStackedWidget* panes_stacked_;
  QList<QToolButton*> buttons_;

 private slots:
  void buttonClicked(int pane);
};
