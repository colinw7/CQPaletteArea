#ifndef CQPaletteArea_H
#define CQPaletteArea_H

#include <CQDockArea.h>
#include <CQTitleBar.h>

#include <QToolButton>
#include <QFrame>
#include <map>

class CQPaletteArea;
class CQPaletteAreaTitle;
class CQPaletteAreaNoTitle;
class CQPaletteWindow;
class CQPaletteWindowTitle;
class CQPaletteWindowTitleButton;
class CQPalettePreview;

class CQPaletteGroup;
class CQPaletteAreaPage;

class CQSplitterArea;
class CQWidgetResizer;
class CQRubberBand;

class QScrollArea;

//! palette area manager creates palette areas on all four sides of the main
//! window and controls palette like children which can be moved between each
//! area
class CQPaletteAreaMgr : public QObject {
  Q_OBJECT

 public:
  //! create dock areas in main window
  CQPaletteAreaMgr(QMainWindow *window);

  //! destructor
 ~CQPaletteAreaMgr();

  //! get main window
  QMainWindow *window() { return window_; }

  //! add page to area
  void addPage(CQPaletteAreaPage *page, Qt::DockWidgetArea dockArea);

  //! remove page from area
  void removePage(CQPaletteAreaPage *page);

  void showExpandedPage(CQPaletteAreaPage *page);

  void hidePage(CQPaletteAreaPage *page);

 private:
  //! get dock area name
  QString dockAreaName(Qt::DockWidgetArea area) const;

  //! add window to area
  CQPaletteWindow *addWindow(Qt::DockWidgetArea dockArea);

  //! remove window from area
  void removeWindow(CQPaletteWindow *window);

  CQPaletteArea *getArea(Qt::DockWidgetArea area);

  CQPaletteArea *createArea(Qt::DockWidgetArea dockArea);

  void deleteArea(CQPaletteArea *area);

  //! get area at point
  CQPaletteArea *getAreaAt(const QPoint &pos, Qt::DockWidgetAreas allowedAreas) const;

  void swapAreas(CQPaletteArea *area1, CQPaletteArea *area2);

  //! highlight area (with rubberband)
  void highlightArea(CQPaletteArea *area, const QPoint &p);

  //! clear highlight
  void clearHighlight();

 private:
  friend class CQPaletteArea;
  friend class CQPaletteWindow;
  friend class CQPaletteAreaTitle;

  typedef std::vector<CQPaletteArea *>       Areas;
  typedef std::map<Qt::DockWidgetArea,Areas> Palettes;

  QMainWindow   *window_;     //! parent main window
  Palettes       palettes_;   //! list of palettes (one per area)
  CQRubberBand  *rubberBand_; //! rubber band
};

//------

//! container for child palette windows on a particular side
//! windows are stored in splitter so can be resized
class CQPaletteArea : public CQDockArea {
  Q_OBJECT

  Q_PROPERTY(bool        hideTitle   READ hideTitle)
  Q_PROPERTY(WindowState windowState READ windowState)
  Q_PROPERTY(bool        visible     READ isVisible)
  Q_PROPERTY(bool        expanded    READ isExpanded)
  Q_PROPERTY(bool        pinned      READ isPinned)
  Q_PROPERTY(bool        floating    READ isFloating)
  Q_PROPERTY(bool        detached    READ isDetached)

  Q_ENUMS(WindowState);

 public:
  enum WindowState {
    NormalState,
    FloatingState,
    DetachedState
  };

 private:
  typedef std::vector<CQPaletteAreaPage*> Pages;

 public:
  //! create in specified dock area
  CQPaletteArea(CQPaletteAreaMgr *mgr, Qt::DockWidgetArea dockArea);

  //! destroy area
 ~CQPaletteArea();

  //! get manager
  CQPaletteAreaMgr *mgr() const { return mgr_; }

  //! get splitter
  CQSplitterArea *splitter() const { return splitter_; }

  bool hideTitle() const { return hideTitle_; }

  bool isVisible() const { return visible_; }
  void setVisible(bool visible) override;

  bool isExpanded() const { return expanded_; }

  bool isPinned() const { return pinned_; }

  Qt::DockWidgetAreas allowedAreas() const { return allowedAreas_; }

  //! get first docked child window
  CQPaletteWindow *getDockedWindow();

  //! add child window
  CQPaletteWindow *addWindow();

  bool moveSplitter(int d);

  //! size hint
  QSize sizeHint() const override;

 public slots:
  void expandSlot();
  void collapseSlot();

  void pinSlot();
  void unpinSlot();

  void attachSlot();
  void detachSlot();

  //! called when Qt changes dock location
  void updateDockLocation(Qt::DockWidgetArea area);

  //! called when Qt changes floating state
  void updateFloating(bool floating);

  //! called when Qt changes visibility
  void updateVisibility(bool visible);

 private:
  friend class CQPaletteAreaTitle;
  friend class CQPaletteWindowTitle;
  friend class CQPalettePreview;

  typedef std::vector<CQPaletteWindow *> Windows;

  const Windows &windows() const { return windows_; }

  uint numWindows() const { return windows_.size(); }

  uint numVisibleWindows() const;

  bool isFirstWindow(const CQPaletteWindow *window) const;

  //! add page to area
  void addPage(CQPaletteAreaPage *page, bool current=false);

  void setCollapsedSize();

  void updateSizeConstraints();

  //! update preview state
  void updatePreviewState();

  //! update preview widgets and rects
  void updatePreview();

  //! add child window
  void addWindow(CQPaletteWindow *window);

  //! add child window at position
  void addWindowAtPos(CQPaletteWindow *window, const QPoint &gpos);

  //! remove child window
  void removeWindow(CQPaletteWindow *window);

  void updateDockArea();

  void updateSplitterSizes();

  Pages getPages() const;

  Qt::DockWidgetAreas calcAllowedAreas() const;

  //! is floating
  bool isFloating() const { return floating_; }
  //! set floating
  void setFloating(bool floating);

  //! is detached
  bool isDetached() const { return detached_; }
  //! set detached
  void setDetached(bool detached);

  //! set floated
  void setFloated(bool floating, const QPoint &pos=QPoint(), bool dragAll=false);
  //! cancel floating
  void cancelFloating();

  //! animate drop at point
  void animateDrop(const QPoint &p);

  //! execute drop at point
  void execDrop(const QPoint &p, bool floating);

  //! clear drop animation
  void clearDrop();

  //! get highlight rectangle
  QRect getHighlightRect() const;

  //! get/set window state
  WindowState windowState() const { return windowState_; }
  void setWindowState(WindowState state);

  //! get highlight rectangle at position
  QRect getHighlightRectAtPos(const QPoint &gpos) const;

  //! get window rectangle at position
  QRect getWindowRect() const;

  //! update size
  void updateSize();

  //! set size constraints
  void setSizeConstraints();

  //! get dock min/max width
  void getDockMinMaxWidth(int &min_w, int &max_w) const;

  //! get dock min/max height
  void getDockMinMaxHeight(int &min_h, int &max_h) const;

  void dockAt(Qt::DockWidgetArea area);

  int getDetachPos(int w, int h) const;

  void updateTitle();

  //! handle resize
  void resizeEvent(QResizeEvent *) override;

  //! handle move
  void moveEvent(QMoveEvent *) override;

 private slots:
  void updateSplitter();

 private:
  friend class CQPaletteAreaMgr;
  friend class CQPaletteWindow;

  static int windowId_; //! window id

  CQPaletteAreaMgr     *mgr_;            //! parent manager
  CQPaletteAreaTitle   *title_;          //! title bar
  CQPaletteAreaNoTitle *noTitle_;        //! dummy widget to hide title bar
  WindowState           windowState_;    //! window state
  bool                  hideTitle_;      //! auto hide title
  bool                  visible_;        //! is visible
  bool                  expanded_;       //! expanded
  bool                  pinned_;         //! pinned
  CQSplitterArea       *splitter_;       //! splitter widget
  CQWidgetResizer      *resizer_;        //! resizer
  Windows               windows_;        //! child windows
  bool                  floating_;       //! is floating
  bool                  detached_;       //! is detached
  Qt::DockWidgetAreas   allowedAreas_;   //! allowed areas
  CQPalettePreview     *previewHandler_; //! preview (unpinned) handler
};

//------

//! container in palette area for one or more user content pages
//! the container has a title bar to allow drag/drop and a content
//! area for the page widgets
class CQPaletteWindow : public QFrame {
  Q_OBJECT

  Q_PROPERTY(bool               detachToArea READ detachToArea WRITE setDetachToArea)
  Q_PROPERTY(WindowState        windowState  READ windowState)
  Q_PROPERTY(bool               visible      READ isVisible)
  Q_PROPERTY(Qt::DockWidgetArea dockArea     READ dockArea)
  Q_PROPERTY(bool               floating     READ isFloating)
  Q_PROPERTY(bool               expanded     READ isExpanded)
  Q_PROPERTY(bool               detached     READ isDetached)

  Q_ENUMS(WindowState)

 public:
  enum WindowState {
    NormalState,
    FloatingState,
    DetachedState
  };

 private:
  typedef std::vector<CQPaletteAreaPage*> Pages;

 public:
  //! create window
  CQPaletteWindow(CQPaletteArea *area, uint id);

 ~CQPaletteWindow();

  //! get area
  CQPaletteArea *area() const { return area_; }

  //! get group
  CQPaletteGroup *group() const { return group_; }

  bool detachToArea() const { return detachToArea_; }
  void setDetachToArea(bool detach) { detachToArea_ = detach; }

  bool isVisible() const { return visible_; }
  void setVisible(bool visible) override;

  Qt::DockWidgetArea dockArea() const;

  //! get whether is on a vertical dock area (left or right)
  bool isVerticalDockArea() const {
    return (dockArea() == Qt::LeftDockWidgetArea || dockArea() == Qt::RightDockWidgetArea);
  }
  //! get whether is on a horizontal dock area (top or bottom)
  bool isHorizontalDockArea() const {
    return (dockArea() == Qt::TopDockWidgetArea || dockArea() == Qt::BottomDockWidgetArea);
  }

  //! add page
  void addPage(CQPaletteAreaPage *page);

  //! add page at index
  void insertPage(int ind, CQPaletteAreaPage *page);

  //! remove page
  void removePage(CQPaletteAreaPage *page);

  void showPage(CQPaletteAreaPage *page);

  void hidePage(CQPaletteAreaPage *page);

  void movePage(CQPaletteAreaPage *page, CQPaletteWindow *newWindow);

  CQPaletteAreaPage *currentPage() const;

  void setCurrentPage(CQPaletteAreaPage *page);

  int dockWidth () const;
  int dockHeight() const;

  Qt::DockWidgetAreas allowedAreas() const { return allowedAreas_; }

  //! get title
  QString getTitle() const;

  //! get icon
  QIcon getIcon() const;

  //! size hint
  QSize sizeHint() const override;

 private slots:
  //! page of group has changed
  void pageChangedSlot(CQPaletteAreaPage *);

  void deleteLaterSlot();

 private:
  friend class CQPaletteArea;
  friend class CQPaletteWindowTitle;

  bool isFirstArea() const;

  void updateLayout();

  void updateDockArea();

  //! set parent area
  void setArea(CQPaletteArea *area);

  Pages getPages() const;

  uint numPages() const;

  //! is floating
  bool isFloating() const { return floating_; }
  //! set floating
  void setFloating(bool floating);

  //! is expanded
  bool isExpanded() const { return expanded_; }

  //! is detached
  bool isDetached() const { return detached_; }
  //! set detached
  void setDetached(bool detached);

  bool isDetachedNoArea() const;

  //! set floated
  void setFloated(bool floating, const QPoint &pos=QPoint(), bool dragAll=false);

  //! cancel floating
  void cancelFloating();

  //! get/set window state
  WindowState windowState() const { return windowState_; }
  void setWindowState(WindowState state);

  //! detach window into new area
  void detachToNewArea();

  //! animate drop at point
  void animateDrop(const QPoint &p);

  //! execute drop at point
  void execDrop(const QPoint &p, bool floating);

  //! clear drop animation
  void clearDrop();

  void dockAt(Qt::DockWidgetArea area);

  Qt::DockWidgetAreas calcAllowedAreas() const;

  void updateTitle();

  void resizeEvent(QResizeEvent *) override;

  void updateDetachSize();

 public slots:
  void dockLeftSlot();
  void dockRightSlot();
  void dockTopSlot();
  void dockBottomSlot();

  void togglePinSlot();
  void toggleExpandSlot();
  void expandSlot();
  void collapseSlot();

  void attachSlot();
  void detachSlot();
  void splitSlot();
  void joinSlot();
  void closeSlot();

 private:
  CQPaletteAreaMgr     *mgr_;          //! parent manager
  CQPaletteArea        *area_;         //! current area
  uint                  id_;           //! window id
  CQPaletteWindowTitle *title_;        //! title bar
  CQPaletteGroup       *group_;        //! palette group
  CQWidgetResizer      *resizer_;      //! resizer
  WindowState           windowState_;  //! window state
  CQPaletteWindow      *newWindow_;    //! window for other tabs
  QWidget              *parent_;       //! parent widget (before float)
  int                   parentPos_;    //! parent splitter index
  bool                  detachToArea_; //! detach to new area
  bool                  visible_;      //! is visible
  bool                  expanded_;     //! is expanded
  bool                  floating_;     //! is floating
  bool                  detached_;     //! is detached
  Qt::DockWidgetAreas   allowedAreas_; //! allowed areas
  int                   detachWidth_;  //! detach width
  int                   detachHeight_; //! detach height
};

//------

//! title bar for palette
class CQPaletteAreaTitle : public CQTitleBar {
  Q_OBJECT

 public:
  CQPaletteAreaTitle(CQPaletteArea *area);

 private:
  friend class CQPaletteArea;

  QString title() const override;

  QIcon icon() const override;

  void updateDockArea();

  void contextMenuEvent(QContextMenuEvent *e) override;

  //! handle mouse events
  void mousePressEvent  (QMouseEvent *e) override;
  void mouseMoveEvent   (QMouseEvent *e) override;
  void mouseReleaseEvent(QMouseEvent *e) override;

  //! handle key events
  void keyPressEvent(QKeyEvent *e) override;

  //! handle hover events
  bool event(QEvent *e) override;

 private:
  void updateState();

 private slots:
  void attachSlot();
  void pinSlot();
  void expandSlot();

  void dockLeftSlot();
  void dockRightSlot();
  void dockTopSlot();
  void dockBottomSlot();

 private:
  //! mouse state
  struct MouseState {
    bool   pressed;
    bool   moving;
    bool   escapePress;
    QPoint pressPos;
    bool   floating;
    bool   dragAll;

    MouseState(){
      reset();
    }

    void reset() {
      pressed     = false;
      moving      = false;
      escapePress = false;
      floating    = false;
      dragAll     = false;
    }
  };

  CQPaletteArea    *area_;         //! parent area
  MouseState        mouseState_;   //! mouse state
  CQTitleBarButton *pinButton_;    //! pin button
  CQTitleBarButton *expandButton_; //! expand button
  QMenu            *contextMenu_;  //! context menu
};

//------

class CQPaletteAreaNoTitle : public QWidget {
 public:
  CQPaletteAreaNoTitle(QWidget *parent=0);

  QSize sizeHint() const override;
  QSize minimumSizeHint() const override;
};

//------

//! title bar for container window
class CQPaletteWindowTitle : public CQTitleBar {
  Q_OBJECT

 public:
  CQPaletteWindowTitle(CQPaletteWindow *window);

 private:
  friend class CQPaletteWindow;

  QString title() const override;

  QIcon icon() const override;

  void updateDockArea();

  void updateState();

  void contextMenuEvent(QContextMenuEvent *e) override;

  //! handle mouse events
  void mousePressEvent  (QMouseEvent *e) override;
  void mouseMoveEvent   (QMouseEvent *e) override;
  void mouseReleaseEvent(QMouseEvent *e) override;

  //! handle key events
  void keyPressEvent(QKeyEvent *e) override;

  //! handle hover events
  bool event(QEvent *e) override;

 private:
  //! mouse state
  struct MouseState {
    bool   pressed;
    bool   moving;
    bool   escapePress;
    QPoint pressPos;
    bool   floating;
    bool   dragAll;

    MouseState(){
      reset();
    }

    void reset() {
      pressed     = false;
      moving      = false;
      escapePress = false;
      floating    = false;
      dragAll     = false;
    }
  };

  CQPaletteWindow  *window_;       //! parent window
  MouseState        mouseState_;   //! mouse state
  CQTitleBarButton *pinButton_;    //! pin button
  CQTitleBarButton *expandButton_; //! expand button
  CQTitleBarButton *closeButton_;  //! close button
  QMenu            *contextMenu_;  //! context menu
};

#endif
