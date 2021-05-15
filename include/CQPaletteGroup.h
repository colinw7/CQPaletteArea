#ifndef CQPaletteGroup_H
#define CQPaletteGroup_H

#include <CQTabBar.h>

#include <QStackedWidget>
#include <QIcon>
#include <map>

class CQPaletteGroup;
class CQPaletteWindow;
class CQPaletteGroupTabBar;
class CQPaletteGroupStack;
class CQPaletteAreaPage;

#define CQPaletteGroupMgrInst CQPaletteGroupMgr::getInstance()

class CQPaletteGroupMgr {
 public:
  static CQPaletteGroupMgr *getInstance();

  CQPaletteGroup *createGroup(CQPaletteWindow *window);

  void removeGroup(CQPaletteGroup *group);

  CQPaletteGroup *getGroup(const QString &name) const;

  CQPaletteGroup *getGroupFromTabBar(const QString &name) const;

 private:
  CQPaletteGroupMgr();

 private:
  typedef std::vector<CQPaletteGroup *> Groups;

  Groups groups_;
};

// class to hold a tabbed set of widgets displayed in a palette sub window
class CQPaletteGroup : public QWidget {
  Q_OBJECT

 public:
  typedef std::vector<CQPaletteAreaPage *> PageArray;

 public:
  CQPaletteGroup(CQPaletteWindow *window);

 ~CQPaletteGroup();

  CQPaletteWindow *window() const { return window_; }

  void setWindow(CQPaletteWindow *window);

  CQPaletteGroupTabBar *tabbar() const { return tabbar_; }

  CQPaletteGroupStack *stack() const { return stack_; }

  Qt::DockWidgetArea dockArea() const;

  void updateDockArea();

  uint numPages() const;

  void addPage(CQPaletteAreaPage *page);

  void insertPage(int ind, CQPaletteAreaPage *page);

  void removePage(CQPaletteAreaPage *page);

  void showPage(CQPaletteAreaPage *page);

  void hidePage(CQPaletteAreaPage *page);

  CQPaletteAreaPage *currentPage() const;

  int currentIndex() const;

  void setCurrentPage(CQPaletteAreaPage *page);

  CQPaletteAreaPage *getPage(int i) const;

  void getPages(PageArray &pages) const;

  QSize sizeHint() const override;

 signals:
  void currentPageChanged(CQPaletteAreaPage *page);

 private slots:
  void setTabIndex(int ind);

  void pressTabIndex(int ind);

  void tabMovePageSlot(const QString &fromName, int fromIndex, const QString &toName, int toIndex);

 private:
  void updateCurrentPage();

  void updateLayout();

  CQPaletteAreaPage *getPageForIndex(int ind) const;

  void showEvent(QShowEvent *) override;

  void resizeEvent(QResizeEvent *) override;

 private:
  typedef std::map<uint,CQPaletteAreaPage*> Pages;

  CQPaletteWindow      *window_;
  CQPaletteGroupTabBar *tabbar_;
  CQPaletteGroupStack  *stack_;
  Pages                 pages_;
};

//------

// tabbar
class CQPaletteGroupTabBar : public CQTabBar {
  Q_OBJECT

 public:
  CQPaletteGroupTabBar(CQPaletteGroup *group);

  void updateDockArea();

  void addPage(CQPaletteAreaPage *page);

  void insertPage(int ind, CQPaletteAreaPage *page);

  void removePage(CQPaletteAreaPage *page);

  uint getPageId(int ind) const;

  QSize sizeHint() const override;

  QSize minimumSizeHint() const override;

 private:
  CQPaletteGroup *group_;
};

//------

// stacked widget
class CQPaletteGroupStack : public QStackedWidget {
  Q_OBJECT

 public:
  CQPaletteGroupStack(QWidget *parent);
 ~CQPaletteGroupStack();

  void addPage(CQPaletteAreaPage *page);

  void removePage(CQPaletteAreaPage *page);

  void setPage(CQPaletteAreaPage *page);
};

//------

class CQPaletteAreaPage : public QObject {
  Q_OBJECT

 public:
  CQPaletteAreaPage(QWidget *w=0);

  virtual ~CQPaletteAreaPage() { }

  CQPaletteGroup *group() const { return group_; }
  virtual void setGroup(CQPaletteGroup *group) { group_ = group; }

  QWidget *widget() const { return w_; }
  virtual void setWidget(QWidget *w);

  uint id() const { return id_; }

  Qt::DockWidgetArea dockArea() const { return dockArea_; }
  virtual void setDockArea(Qt::DockWidgetArea dockArea) { dockArea_ = dockArea; }

  bool hidden() const { return hidden_; }
  virtual void setHidden(bool hidden) { hidden_ = hidden; }

  bool widthResizable() const { return widthResizable_; }
  virtual void setWidthResizable(bool resizable) { widthResizable_ = resizable; }

  bool heightResizable() const { return heightResizable_; }
  virtual void setHeightResizable(bool resizable) { heightResizable_ = resizable; }

  virtual void setFixedWidth (int width ) { fixedWidth_  = width ; setWidthResizable (false); }
  virtual void setFixedHeight(int height) { fixedHeight_ = height; setHeightResizable(false); }

  virtual QString windowTitle() const { return ""; }

  virtual QString title() const { return ""; }
  virtual QIcon   icon () const { return QIcon(); }

  virtual Qt::DockWidgetAreas allowedAreas() const { return Qt::AllDockWidgetAreas; }

  void getMinMaxWidth (int &min_w, int &max_w) const;
  void getMinMaxHeight(int &min_h, int &max_h) const;

 private:
  static uint lastId_;

  CQPaletteGroup     *group_;           // parent group
  QWidget            *w_;               // child widget
  uint                id_;              // unique id
  Qt::DockWidgetArea  dockArea_;        // dock area
  bool                hidden_;          // hidden
  int                 fixedWidth_;      // fixed width
  int                 fixedHeight_;     // fixed height
  bool                widthResizable_;  // resizable
  bool                heightResizable_; // resizable
};

#endif
