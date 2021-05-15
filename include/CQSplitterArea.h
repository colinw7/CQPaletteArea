#ifndef CQSplitterArea_H
#define CQSplitterArea_H

#include <QWidget>

class QMainWindow;
class CQPaletteArea;
class QSplitter;
class CQSplitterHandle;

class CQSplitterArea : public QWidget {
  Q_OBJECT

  Q_PROPERTY(Qt::DockWidgetArea dockArea  READ dockArea)
  Q_PROPERTY(bool               resizable READ isResizable)
  Q_PROPERTY(bool               floating  READ isFloating)

 public:
  CQSplitterArea(CQPaletteArea *palette);

  CQPaletteArea *palette() { return palette_; }

  Qt::DockWidgetArea dockArea() const { return dockArea_; }
  void setDockArea(Qt::DockWidgetArea dockArea);

  bool isVerticalDockArea() const {
    return (dockArea() == Qt::LeftDockWidgetArea || dockArea() == Qt::RightDockWidgetArea);
  }

  bool isResizable() const { return resizable_; }
  void setResizable(bool resize);

  bool isFloating() const { return floating_; }
  void setFloating(bool floating);

  QSplitter *splitter() { return splitter_; }

  void updateLayout();

 private:
  void showEvent(QShowEvent *) override;

 private:
  CQPaletteArea      *palette_;
  Qt::DockWidgetArea  dockArea_;
  bool                resizable_;
  bool                floating_;
  QSplitter          *splitter_;
  CQSplitterHandle   *handle_;
};

class CQSplitterHandle : public QWidget {
  Q_OBJECT

 public:
  CQSplitterHandle(CQSplitterArea *area);

  CQSplitterArea *area() { return area_; }

 private:
  friend class CQSplitterArea;

  void updateState();

  void mousePressEvent  (QMouseEvent *e) override;
  void mouseMoveEvent   (QMouseEvent *e) override;
  void mouseReleaseEvent(QMouseEvent *e) override;

  void enterEvent(QEvent *e) override;
  void leaveEvent(QEvent *e) override;

  void paintEvent(QPaintEvent *) override;

 private:
  struct MouseState {
    bool   pressed;
    QPoint pressPos;

    MouseState() {
      pressed = false;
    }
  };

  CQSplitterArea *area_;
  MouseState      mouseState_;
  bool            mouseOver_;
};

#endif
