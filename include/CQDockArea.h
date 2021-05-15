#ifndef CQDockArea_H
#define CQDockArea_H

#include <QDockWidget>
#include <QSize>

class QMainWindow;
class QTimer;

class CQDockArea : public QDockWidget {
  Q_OBJECT

  Q_PROPERTY(Qt::DockWidgetArea dockArea READ dockArea)

 public:
  CQDockArea(QMainWindow *window);

  //! get area
  Qt::DockWidgetArea dockArea() const { return dockArea_; }

  //! get whether is on a vertical dock area (left or right)
  bool isVerticalDockArea() const;
  //! get whether is on a horizontal dock area (top or bottom)
  bool isHorizontalDockArea() const;

  //! set dock widget
  void setWidget(QWidget *w);

  //! get/set dock width
  int dockWidth() const { return dockWidth_ ; }
  void setDockWidth(int width) { dockWidth_ = width; }

  //! get/set dock height
  int dockHeight() const { return dockHeight_; }
  void setDockHeight(int height) { dockHeight_ = height; }

  void applyDockWidth (int width , bool fixed=false);
  void applyDockHeight(int height, bool fixed=false);

  //! get/set fixed state
  bool isFixed() const { return fixed_; }
  void setFixed(bool fixed) { fixed_ = fixed; }

 signals:
  void dockWidthChanged(int width);
  void dockHeightChanged(int height);

  void resizeFinished();

  void centralWidgetResized();

 protected:
  //! set dock area
  void setDockArea(Qt::DockWidgetArea dockArea) { dockArea_ = dockArea; }

  //! update title
  void updateWidgetTitle();

  bool ignoreSize() const { return ignoreSize_; }

  bool setIgnoreSize(bool ignore) { std::swap(ignoreSize_, ignore); return ignore; }

  // handle resize
  void resizeEvent(QResizeEvent *) override;

  bool eventFilter(QObject *obj, QEvent *event) override;

  void handleEvent(QObject *obj, QEvent *event);

  bool isInsideFloatingBorder(const QPoint &p) const;

 protected:
  QMainWindow        *window_;          //! parent main window
  Qt::DockWidgetArea  dockArea_;        //! dock area
  bool                ignoreSize_;      //! ignore resize events
  int                 dockWidth_;       //! current dock width
  int                 dockHeight_;      //! current dock height
  bool                fixed_;           //! is fixed size
  bool                splitterPressed_; //! splitter pressed
};

#endif
