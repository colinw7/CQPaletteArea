#ifndef CQDockArea_H
#define CQDockArea_H

#include <QDockWidget>
#include <QSize>

class QMainWindow;
class QTimer;

class CQDockArea : public QDockWidget {
  Q_OBJECT

  Q_PROPERTY(bool dragResize READ dragResize WRITE setDragResize)

 public:
  CQDockArea(QMainWindow *window);

  bool dragResize() const { return dragResize_; }

  void setDragResize(bool dragResize) { dragResize_ = dragResize; }

  void setWidget(QWidget *w);

  void setDockWidth (int width , bool fixed=false);
  void setDockHeight(int height, bool fixed=false);

  int dockWidth () const { return dockWidth_ ; }
  int dockHeight() const { return dockHeight_; }

 private slots:
  void resetMinMaxSizes();

 private:
  void updateWidgetTitle();

  int resizeTimeOut() const { return resizeTimeout_; }

  bool ignoreSize() const { return ignoreSize_; }

  bool setIgnoreSize(bool ignore) { std::swap(ignoreSize_, ignore); return ignore; }

  void dragToDockWidth (int width );
  void dragToDockHeight(int height);

  static void resetWidgetMinMaxWidth (QWidget *w);
  static void resetWidgetMinMaxHeight(QWidget *w);

 private:
  QMainWindow *mainWindow_;
  bool         dragResize_;
  int          resizeTimeout_;
  QTimer      *resizeTimer_;
  bool         ignoreSize_;
  int          dockWidth_;
  int          dockHeight_;
  QSize        oldMinSize_;
  QSize        oldMaxSize_;
  bool         fixed_;
};

#endif
