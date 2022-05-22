#ifndef CQTabBar_H
#define CQTabBar_H

#include <QTabBar>
#include <QToolButton>
#include <QIcon>
#include <QVariant>

class QMimeData;

class CQTabBarButton;
class CQTabBarScrollButton;

/*!
 * \ingroup WinFWCore
 *
 * \brief Tab bar widget
 *
 * This widget differs from a normal QTabBar widget in the following ways:
 *  . it allows zero tabs to be selected (instead of always one)
 *  . it supports pending state for the tabs (to match page pending state)
*/
class CQTabBar : public QWidget {
  Q_OBJECT

  Q_PROPERTY(Position            position    READ position    WRITE setPosition   )
  Q_PROPERTY(bool                allowNoTab  READ allowNoTab  WRITE setAllowNoTab )
  Q_PROPERTY(Qt::ToolButtonStyle buttonStyle READ buttonStyle WRITE setButtonStyle)
  Q_PROPERTY(QSize               iconSize    READ iconSize    WRITE setIconSize   )

  Q_ENUMS(Position)

 private:
  enum { TAB_BORDER=8, RESIZE_WIDTH=5 };

 public:
  // tab position
  enum Position {
    NoPos = 0,
    North = 1,
    South = 2,
    West  = 3,
    East  = 4
  };

 public:
  //! create tab bar
  CQTabBar(QWidget *parent=nullptr);

  //! delete tab bar
 ~CQTabBar();

  //! clear tabs
  void clear();

  //! add tab for specified text and widget
  int addTab(const QString &text, QWidget *widget=nullptr);
  //! add tab for specified text, icon and widget
  int addTab(const QIcon &icon, const QString &text, QWidget *widget=nullptr);
  //! add tab with specified button
  int addTab(CQTabBarButton *button);

  //! insert tab for specified text and widget
  int insertTab(int ind, const QString &text, QWidget *widget=nullptr);
  //! insert tab for specified text, icon and widget
  int insertTab(int ind, const QIcon &icon, const QString &text, QWidget *widget=nullptr);
  //! insert tab with specified button
  int insertTab(int ind, CQTabBarButton *button);

  //! remove tab for widget
  void removeTab(QWidget *widget);
  //! remove tab for index
  void removeTab(int ind);

  //! get number of tabs
  int count() const;
  //! get tab count position
  int tabInd(int i) const;

  //! get current tab
  int currentIndex() const { return currentIndex_; }
  //! get set tab
  void setCurrentIndex(int index);

  //! get tab index for widget
  int getTabIndex(QWidget *w) const;

  //! get tab position (relative to contents)
  Position position() const { return position_; }
  //! set tab position (relative to contents)
  void setPosition(const Position &position);

  //! get if allow no current tab
  bool allowNoTab() const { return allowNoTab_; }
  //! set allow no current tab
  void setAllowNoTab(bool allowNoTab);

  //! get button style (icon and/or text)
  Qt::ToolButtonStyle buttonStyle() const { return buttonStyle_; }
  //! set button style (icon and/or text)
  void setButtonStyle(const Qt::ToolButtonStyle &buttonStyle);

  //! get icon width
  int iconWidth() const { return iw_; }

  //! set text for tab
  void setTabText(int index, const QString &text);

  //! set icon for tab
  void setTabIcon(int index, const QIcon &icon);

  //! set tooltip for tab
  void setTabToolTip(int index, const QString &tip);

  //! set whether tab is visible
  void setTabVisible(int index, bool visible);

  //! set whether tab is pending
  void setTabPending(int index, bool pending);

  //! get/set tab data
  QVariant tabData(int index) const;
  void setTabData(int index, const QVariant &data);

  //! get tab widget
  QWidget *tabWidget(int index) const;

  //! size hint (preferred size)
  QSize sizeHint() const override;

  //! minimum size hint
  QSize minimumSizeHint() const override;

  //! get index of tab at specified point
  int tabAt(const QPoint &p) const;

  //! get icon size
  QSize iconSize() const;

  //! set icon size
  void setIconSize(const QSize &size);

  //! update sizes of tabs
  void updateSizes();

 private:
  //! get tab button
  CQTabBarButton *tabButton(int index) const;

  //! get tab button array pos
  int tabButtonPos(int index) const;

  //! handle paint event
  void paintEvent(QPaintEvent *) override;

  //! handle resize event
  void resizeEvent(QResizeEvent *) override;

  //! handle mouse press event
  void mousePressEvent(QMouseEvent *) override;

  //! handle mouse move event
  void mouseMoveEvent(QMouseEvent *) override;

  //! handle mouse release event
  void mouseReleaseEvent(QMouseEvent *) override;

  //! handle drag enter
  void dragEnterEvent(QDragEnterEvent *event) override;

  //! handle drag move
  void dragMoveEvent(QDragMoveEvent *event) override;

  //! handle drop
  void dropEvent(QDropEvent *event) override;

  //! check if drag valid
  bool dragValid(const QMimeData *m, QString &type, int &tabNum) const;

  //! drag pos valid
  bool dragPosValid(const QString &type, const QPoint &pos) const;

  //! handle context menu event
  void contextMenuEvent(QContextMenuEvent *e) override;

  //! handle generic event
  bool event(QEvent *e) override;

  //! show scroll buttons
  void showScrollButtons(bool show);

  //! get tab shape for current position
  QTabBar::Shape getTabShape() const;

  //! is vertical
  bool isVertical() const;

  //! set press state
  void setPressPoint(const QPoint &p);

 signals:
  //! signal that the current tab has changed
  void currentChanged(int index);

  //! signal that the current tab was pressed
  void currentPressed(int index);

  //! signal that the specified tab is pressed with its active state
  void tabPressedSignal(int index, bool active);

  //! request context menu
  void showContextMenuSignal(const QPoint &p);

  //! notify tab moved (same group)
  void tabMoved(int, int);

  //! request page move (to another palette group)
  void tabMovePageSignal(const QString &, int, const QString &, int);

 private slots:
  //! handle left/bottom scroll button press
  void lscrollSlot();
  //! handle right/top scroll button press
  void rscrollSlot();

 private:
  using TabButtons = std::vector<CQTabBarButton *>;

  TabButtons            buttons_;                  //! tab page buttons
  int                   currentIndex_ { -1 };      //! current tab index (-1 if none)
  Position              position_     { North };   //! tab position (relative to contents)
  bool                  allowNoTab_   { false };   //! allow no current tab
  Qt::ToolButtonStyle   buttonStyle_  { Qt::ToolButtonIconOnly };
                                                   //! tab button style (text and/or icon)
  CQTabBarScrollButton *lscroll_      { nullptr }; //! left/bottom scroll button if clipped
  CQTabBarScrollButton *rscroll_      { nullptr }; //! right/top scroll button if clipped
  QSize                 iconSize_     { 16, 16 };  //! icon size
  mutable int           iw_           { 0 };       //! tab bar icon width
  mutable int           w_            { 0 };       //! tab bar width
  mutable int           h_            { 0 };       //! tab bar height
  mutable int           clipNum_      { -1 };      //! first item clipped (-1 if none)
  mutable int           offset_       { 0 };       //! scroll offset
  mutable bool          pressed_      { false };   //! button pressed
  mutable QPoint        pressPos_;                 //! button press pos (for drag)
  mutable int           pressIndex_   { -1 };      //! tab at press position
  mutable int           moveIndex_    { -1 };      //! tab at current mouse position
};

//---

/*! base class for tab bar button
*/
class CQTabBarButton {
 public:
  CQTabBarButton(CQTabBar *bar);

  //! get index
  int index() const { return index_; }
  //! set index
  void setIndex(int ind) { index_ = ind; }

  //! get text
  const QString &text() const { return text_; }
  //! set text
  void setText(const QString &text);

  //! get icon
  const QIcon &icon() const { return icon_; }
  //! set icon
  void setIcon(const QIcon &icon);

  //! get data
  const QVariant &data() const { return data_; }
  //! set icon
  void setData(const QVariant &data);

  //! get icon for specified tab position
  const QIcon &positionIcon(CQTabBar::Position pos) const;

  //! get tooltip
  const QString &toolTip() const;
  //! set tooltip
  void setToolTip(const QString &tip);

  //! get associated widget
  QWidget *widget() const { return w_; }
  //! set associated widget
  void setWidget(QWidget *w);

  //! get visible
  bool visible() const { return visible_; }
  //! set visible
  void setVisible(bool visible);

  //! get pending
  bool pending() const { return pending_; }
  //! set pending
  void setPending(bool pending);

  //! get bounding box
  const QRect &rect() const { return r_; }
  //! set bounding box
  void setRect(const QRect &r);

  //! get pixmap of icon
  QPixmap pixmap() const;

  //! calc button width
  int width() const;

 private:
  using Position = CQTabBar::Position;

  CQTabBar*        bar_          { nullptr };         //! icon position
  int              index_        { 0 };               //! index
  QString          text_;                             //! text
  QIcon            icon_;                             //! icon
  QVariant         data_;                             //! data
  mutable QIcon    positionIcon_;                     //! icon for position (cached)
  mutable Position iconPosition_ { CQTabBar::North }; //! position used for above (cached)
  QString          toolTip_;                          //! tooltip
  QWidget*         w_            { nullptr };         //! associated widget
  bool             visible_      { true };            //! is visible
  bool             pending_      { false };           //! is pending
  QRect            r_;                                //! bounding box
};

//---

/*! internal class for tab bar scroll button
*/
class CQTabBarScrollButton : public QToolButton {
  Q_OBJECT

 public:
  CQTabBarScrollButton(CQTabBar *bar, const char *name);
};

#endif
