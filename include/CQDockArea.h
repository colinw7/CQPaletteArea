#ifndef CQDockArea_H
#define CQDockArea_H

#include <QDockWidget>
#include <QSize>

class QMainWindow;
class QTimer;

class CQDockArea : public QDockWidget {
  Q_OBJECT

  Q_PROPERTY(Qt::DockWidgetArea dockArea   READ dockArea)
  Q_PROPERTY(bool               dragResize READ dragResize WRITE setDragResize)

 public:
  CQDockArea(QMainWindow *window);

  //! get area
  Qt::DockWidgetArea dockArea() const { return dockArea_; }

  //! get/set drag resize
  bool dragResize() const { return dragResize_; }
  void setDragResize(bool dragResize) { dragResize_ = dragResize; }

  //! set dock widget
  void setWidget(QWidget *w);

  //! get/set dock width
  int dockWidth () const { return dockWidth_ ; }
  void setDockWidth (int width , bool fixed=false);

  //! get/set dock height
  int dockHeight() const { return dockHeight_; }
  void setDockHeight(int height, bool fixed=false);

 private slots:
  void resetMinMaxSizes();

 protected:
  //! set dock area
  void setDockArea(Qt::DockWidgetArea dockArea) { dockArea_ = dockArea; }

  //! update title
  void updateWidgetTitle();

  int resizeTimeOut() const { return resizeTimeout_; }

  bool ignoreSize() const { return ignoreSize_; }

  bool setIgnoreSize(bool ignore) { std::swap(ignoreSize_, ignore); return ignore; }

  //! drag to dock width
  void dragToDockWidth (int width );
  //! drag to dock height
  void dragToDockHeight(int height);

  static void resetWidgetMinMaxWidth (QWidget *w);
  static void resetWidgetMinMaxHeight(QWidget *w);

 protected:
  QMainWindow        *mainWindow_;    //! parent main window
  Qt::DockWidgetArea  dockArea_;      //! dock area
  bool                dragResize_;    //! use drag resize
  int                 resizeTimeout_; //! resize timeout
  QTimer             *resizeTimer_;   //! resize timer
  bool                ignoreSize_;    //! ignore resize events
  int                 dockWidth_;     //! current dock width
  int                 dockHeight_;    //! current dock height
  mutable QSize       oldMinSize_;    //! saved min size
  mutable QSize       oldMaxSize_;    //! saved max size
  bool                fixed_;         //! is fixed size
};

#endif
