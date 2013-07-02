#ifndef CQPaletteArea_H
#define CQPaletteArea_H

#include <CQDockArea.h>
#include <CQTitleBar.h>

#include <QToolButton>
#include <map>

class CQPaletteArea;
class CQPaletteWindow;
class CQPaletteWindowTitle;
class CQPaletteWindowTitleButton;

class CQPaletteGroup;
class CQPaletteAreaPage;

class QScrollArea;
class QSplitter;
class QRubberBand;

// palette area manager creates palette areas on all four sides of the main
// window and controls palette like children which can be moved between each
// area
class CQPaletteAreaMgr : public QObject {
  Q_OBJECT

 public:
  // create dock areas in main window
  CQPaletteAreaMgr(QMainWindow *window);

  // destructor
 ~CQPaletteAreaMgr();

  // get main window
  QMainWindow *window() const { return window_; }

  // add page to area
  void addPage(CQPaletteAreaPage *page, Qt::DockWidgetArea dockArea);

  // remove page from area
  void removePage(CQPaletteAreaPage *page);

 private:
  // add window to area
  CQPaletteWindow *addWindow(Qt::DockWidgetArea dockArea);

  // remove window from area
  void removeWindow(CQPaletteWindow *window);

  // get area at point
  CQPaletteArea *getAreaAt(const QPoint &pos) const;

  // highlight area (with rubberband)
  void highlightArea(CQPaletteArea *area, const QPoint &p);

  // clear highlight
  void clearHighlight();

 private:
  friend class CQPaletteWindow;

  typedef std::map<Qt::DockWidgetArea,CQPaletteArea *> Palettes;

  QMainWindow   *window_;     // parent main window
  Palettes       palettes_;   // list of palettes (one per area)
  QRubberBand   *rubberBand_; // rubber band
};

//------

// container for child palette windows on a particular side
// windows are stored in splitter so can be resized
class CQPaletteArea : public CQDockArea {
  Q_OBJECT

 public:
  // create in specified dock area
  CQPaletteArea(CQPaletteAreaMgr *mgr, Qt::DockWidgetArea dockArea);

  // get manager
  CQPaletteAreaMgr *mgr() const { return mgr_; }

  // get area
  Qt::DockWidgetArea dockArea() const { return dockArea_; }

  // get first child window
  CQPaletteWindow *getWindow(int i=0);

  // add child window
  CQPaletteWindow *addWindow();

  // size hint
  QSize sizeHint() const;

 private:
  // add child window
  void addWindow(CQPaletteWindow *window);

  // add child window at position
  void addWindowAtPos(CQPaletteWindow *window, const QPoint &gpos);

  // remove child window
  void removeWindow(CQPaletteWindow *window);

  // get highlight rectangle
  QRect getHighlightRect() const;

  // get highlight rectangle at position
  QRect getHighlightRectAtPos(const QPoint &gpos) const;

  // get window rectangle at position
  QRect getWindowRect() const;

  // update size
  void updateSize();

 private:
  friend class CQPaletteAreaMgr;
  friend class CQPaletteWindow;

  typedef std::vector<CQPaletteWindow *> Windows;

  CQPaletteAreaMgr   *mgr_;      // parent manager
  Qt::DockWidgetArea  dockArea_; // dock area
  QSplitter          *splitter_; // splitter widget
  Windows             windows_;  // child windows
};

//------

// container in palette area for one or more user content pages
// the container has a title bar to allow drag/drop and a content
// area for the page widgets
class CQPaletteWindow : public QWidget {
  Q_OBJECT

 public:
  // create window
  CQPaletteWindow(CQPaletteArea *area);

  // get area
  CQPaletteArea *area() const { return area_; }

  // get group
  CQPaletteGroup *group() const { return group_; }

  // add page
  void addPage(CQPaletteAreaPage *page);

  // remove page
  void removePage(CQPaletteAreaPage *page);

  // get title
  QString getTitle() const;

  // get icon
  QIcon getIcon() const;

  // size hint
  QSize sizeHint() const;

 private slots:
  // page of group has changed
  void pageChangedSlot(CQPaletteAreaPage *);

 private:
  friend class CQPaletteArea;
  friend class CQPaletteWindowTitle;

  // set parent area
  void setArea(CQPaletteArea *area);

  // is floating
  bool isFloating() const { return floating_; }

  // set floating
  void setFloating(bool floating, const QPoint &pos=QPoint());

  // animate drop at point
  void animateDrop(const QPoint &p);

  // execute drop at point
  void execDrop(const QPoint &p);

  // clear drop animation
  void clearDrop();

 public slots:
  void closeSlot();

 private:
  CQPaletteAreaMgr     *mgr_;      // parent manager
  CQPaletteArea        *area_;     // current area
  CQPaletteWindowTitle *title_;    // title bar
  CQPaletteGroup       *group_;    // palette group
  QWidget              *parent_;   // parent widget (before float)
  bool                  floating_; // is floating
};

//------

// title bar for container window
class CQPaletteWindowTitle : public CQTitleBar {
 public:
  CQPaletteWindowTitle(CQPaletteWindow *window);

 private:
  friend class CQPaletteWindow;

  QString title() const;

  QIcon icon() const;

  void updateDockArea();

  // handle mouse events
  void mousePressEvent  (QMouseEvent *e);
  void mouseMoveEvent   (QMouseEvent *e);
  void mouseReleaseEvent(QMouseEvent *e);

  // handle key events
  void keyPressEvent(QKeyEvent *e);

  // handle hover events
  bool event(QEvent *e);

 private:
  // mouse state
  struct MouseState {
    bool   pressed;
    bool   moving;
    bool   escapePress;
    QPoint pressPos;

    MouseState(){
      reset();
    }

    void reset() {
      pressed     = false;
      escapePress = false;
    }
  };

  CQPaletteWindow  *window_;      // parent window
  MouseState        mouseState_;  // mouse state
  CQTitleBarButton *closeButton_; // close button
};

#endif
