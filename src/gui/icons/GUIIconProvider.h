#pragma once
#include <QtGui>
#ifdef Q_OS_WIN
#include <QtWin>
#endif

class GUIIconProvider {
 public:
  enum ICON_ID {
    SETTINGS_GENERAL,
    SETTINGS_ACCOUNT,
    SETTINGS_NETWORK,
    SETTINGS_ADVANCED,

    FOLDER_ADD,
    FOLDER_DELETE,
    LINK_OPEN,
    SETTINGS,
    TRAYICON,
    REVEAL_FOLDER
  };

  [[nodiscard]] QIcon icon(ICON_ID id) const;

#ifdef Q_OS_MAC
  class MacIcon : public QIconEngine {
   public:
    MacIcon(const QString& name) : QIconEngine(), name_(name) {}
    virtual void paint(QPainter* painter, const QRect& rect, QIcon::Mode mode,
                       QIcon::State state) override { /* no-op */
    }
    virtual QIconEngine* clone() const override { /* no-op */
      return nullptr;
    }

   private:
    QString name_;
    virtual QString iconName() const override { return name_; }
  };
#endif

#ifdef Q_OS_WIN
  static QIcon get_shell_icon(LPCTSTR dll, WORD num) {
    return QIcon(QtWin::fromHICON(LoadIcon(LoadLibrary(dll), MAKEINTRESOURCE(num))));
  }
#endif
};
