#include "html5applicationviewer.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QGridLayout>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsLinearLayout>
#include <QGraphicsWebView>
#include <QWebFrame>
#include "logger.h"
#include "extendedlistitem.h"

#ifdef TOUCH_OPTIMIZED_NAVIGATION
#include <QTimer>
#include <QGraphicsSceneMouseEvent>
#include <QWebElement>

class QWebFrame;

class WebTouchPhysicsInterface : public QObject
{
  Q_OBJECT

public:
  WebTouchPhysicsInterface(QObject *parent = 0);
  virtual ~WebTouchPhysicsInterface();
  static WebTouchPhysicsInterface* getSingleton();

  virtual bool inMotion() = 0;
  virtual void stop() = 0;
  virtual void start(const QPointF &pressPoint, const QWebFrame *frame) = 0;
  virtual bool move(const QPointF &pressPoint) = 0;
  virtual bool release(const QPointF &pressPoint) = 0;

  signals:
  void positionChanged(const QPointF &point, const QPoint &startPressPoint);

  public slots:
  virtual void setRange(const QRectF &range) = 0;

private:
  static WebTouchPhysicsInterface* s_instance;
};

static const int KCumulativeDistanceThreshold = 40;
static const int KDecelerationCount = 4;
static const int KDecelerationTimerSpeed = 10;
static const int KFlickScrollThreshold = 400;
static const int KJitterBufferThreshold = 200;
static const qreal KDecelerationSlowdownFactor = 0.975;

static const int KTouchDownStartTime = 200;
static const int KHoverTimeoutThreshold = 100;
static const int KNodeSearchThreshold = 400;

class WebTouchPhysics : public WebTouchPhysicsInterface
{
  Q_OBJECT

public:
  WebTouchPhysics(QObject *parent = 0);
  virtual ~WebTouchPhysics();

  virtual bool inMotion();
  virtual void stop();
  virtual void start(const QPointF &pressPoint, const QWebFrame *frame);
  virtual bool move(const QPointF &pressPoint);
  virtual bool release(const QPointF &pressPoint);

  signals:
  void setRange(const QRectF &range);

  public slots:
  void decelerationTimerFired();
  void changePosition(const QPointF &point);
  bool isFlick(const QPointF &point, int distance) const;
  bool isPan(const QPointF &point, int distance) const;

private:
  QPointF m_previousPoint;
  QPoint m_startPressPoint;
  QPointF m_decelerationSpeed;
  QList<QPointF> m_decelerationPoints;
  QTimer m_decelerationTimer;
  QPointF m_cumulativeDistance;
  const QWebFrame* m_frame;
  bool m_inMotion;
};

class WebTouchEvent
{
public:
  WebTouchEvent();
  WebTouchEvent(const QMouseEvent *mouseEvent);
  WebTouchEvent(const QGraphicsSceneMouseEvent *graphicsSceneMouseEvent);

  QPoint m_pos;
  QEvent::Type m_type;
  Qt::MouseButton m_button;
  Qt::MouseButtons m_buttons;
  bool m_graphicsSceneEvent;
  bool m_fired;
  bool m_editable;
  Qt::KeyboardModifiers m_modifier;
  QPointF m_scenePos;
  QPoint m_screenPos;
  QPointF m_buttonDownPos;
  QPointF m_buttonDownScenePos;
  QPoint m_buttonDownScreenPos;
  QPointF m_lastPos;
  QPointF m_lastScenePos;
  QPoint m_lastScreenPos;
};

class QWebFrame;

class WebTouchScroller : public QObject
{
  Q_OBJECT

public:
  WebTouchScroller(QObject *parent = 0);
  virtual ~WebTouchScroller();

  signals:
  void rangeChanged(const QRectF &range);

  public slots:
  void setFrame(QWebFrame* frame);
  void scroll(const QPointF &delta, const QPoint &scrollStartPoint);

private:
  QWebFrame* m_webFrame;
  QSize m_range;
};

class QWebPage;

class WebTouchNavigation : public QObject
{
  Q_OBJECT

public:
  WebTouchNavigation(QObject *object, QWebPage *webPage);
  virtual ~WebTouchNavigation();

protected:
  bool eventFilter(QObject *object, QEvent *event);
  void handleDownEvent(const WebTouchEvent &event);
  void handleMoveEvent(const WebTouchEvent &event);
  void handleReleaseEvent(const WebTouchEvent &event);

  public slots:
  void hoverTimerFired();
  void downTimerFired();
  void quickDownTimerFired();
  void quickUpTimerFired();

private:
  void invalidateLastTouchDown();
  void generateMouseEvent(const WebTouchEvent &touchEvent);

private:
  WebTouchPhysicsInterface* m_physics;
  WebTouchScroller* m_scroller;
  WebTouchEvent m_touchDown;
  QObject* m_viewObject;
  QWebPage* m_webPage;
  QWebFrame* m_webFrame;
  QTimer m_downTimer;
  QTimer m_hoverTimer;
  QTimer m_quickDownTimer;
  QTimer m_quickUpTimer;
  bool m_suppressMouseEvents;
};

class QWebPage;
class WebTouchNavigation;

class WebNavigation : public QObject
{
  Q_OBJECT

public:
  WebNavigation(QObject *parent, QWebPage *webPage);
  virtual ~WebNavigation();

private:
  QObject *m_viewObject;
  QWebPage *m_webPage;
  WebTouchNavigation *m_webTouchNavigation;
};

class QGraphicsWebView;
class QWebPage;

class NavigationController : public QObject
{
  Q_OBJECT

public:
  NavigationController(QWidget *parent, QGraphicsWebView *webView);
  virtual ~NavigationController();

  QWidget *webWidget() const;
  QWebPage* webPage() const;
  QGraphicsWebView* graphicsWebView() const;

  signals:
  void pauseNavigation();
  void resumeNavigation();

private:
  class NavigationControllerPrivate *m_d;
};

WebTouchPhysicsInterface* WebTouchPhysicsInterface::s_instance = 0;

WebTouchPhysicsInterface::WebTouchPhysicsInterface(QObject *parent)
: QObject(parent)
{
}

WebTouchPhysicsInterface::~WebTouchPhysicsInterface()
{
  if (s_instance == this)
    s_instance = 0;
}

WebTouchPhysicsInterface* WebTouchPhysicsInterface::getSingleton()
{
  if (!s_instance)
    s_instance = new WebTouchPhysics;
  return s_instance;
}

int fastDistance(const QPoint &p, const QPoint &q)
{
  return (p.x() - q.x()) * (p.x() - q.x()) + (p.y() - q.y()) * (p.y() - q.y());
}

WebTouchPhysics::WebTouchPhysics(QObject *parent)
: WebTouchPhysicsInterface(parent)
, m_frame(0)
, m_inMotion(false)
{
  connect(&m_decelerationTimer, SIGNAL(timeout()), this, SLOT(decelerationTimerFired()));
}

WebTouchPhysics::~WebTouchPhysics()
{
}

bool WebTouchPhysics::inMotion()
{
  return m_inMotion;
}

void WebTouchPhysics::stop()
{
  m_decelerationTimer.stop();
  m_cumulativeDistance = QPoint();
  m_previousPoint = QPoint();
  m_startPressPoint = QPoint();
  m_decelerationPoints.clear();
  m_inMotion = false;
}

void WebTouchPhysics::start(const QPointF &pressPoint, const QWebFrame *frame)
{
  if (!frame)
    return;

  m_frame = frame;
  m_decelerationPoints.push_front(pressPoint);
  m_decelerationTimer.setSingleShot(false);
  m_decelerationTimer.setInterval(KDecelerationTimerSpeed);
  m_previousPoint = pressPoint;
  m_startPressPoint = pressPoint.toPoint();
}

bool WebTouchPhysics::move(const QPointF &pressPoint)
{
  int distance = fastDistance(pressPoint.toPoint(), m_startPressPoint);
  if (isFlick(pressPoint, distance) || isPan(pressPoint, distance)) {
    changePosition(pressPoint);
    return true;
  }
  return false;
}

bool WebTouchPhysics::release(const QPointF &pressPoint)
{
  if (m_cumulativeDistance.manhattanLength() > KCumulativeDistanceThreshold) {
    m_decelerationSpeed = (m_decelerationPoints.back() - pressPoint) / (m_decelerationPoints.count() + 1);
    m_decelerationTimer.start();
    return true;
  } else {
    m_inMotion = false;
    return false;
  }
}

void WebTouchPhysics::changePosition(const QPointF &point)
{
  m_inMotion = true;
  QPointF diff = m_previousPoint - point;
  emit positionChanged(diff, m_startPressPoint);

  m_cumulativeDistance += (point - m_previousPoint);
  m_previousPoint = point;
  m_decelerationPoints.push_front(point);
  if (m_decelerationPoints.count() > KDecelerationCount)
    m_decelerationPoints.pop_back();
}

void WebTouchPhysics::decelerationTimerFired()
{
  if (!m_frame) {
    m_decelerationTimer.stop();
    return;
  }

  if (qAbs(m_decelerationSpeed.x()) < KDecelerationSlowdownFactor
    && qAbs(m_decelerationSpeed.y()) < KDecelerationSlowdownFactor) {
    m_inMotion = false;
  m_decelerationTimer.stop();
  return;
}

m_decelerationSpeed *= KDecelerationSlowdownFactor;
emit positionChanged(QPoint(m_decelerationSpeed.x(), m_decelerationSpeed.y()), m_startPressPoint);
}

bool WebTouchPhysics::isFlick(const QPointF &pressPoint, int distance) const
{
  Q_UNUSED(pressPoint);
  return !m_inMotion && distance > KFlickScrollThreshold;
}

bool WebTouchPhysics::isPan(const QPointF &pressPoint, int distance) const
{
  Q_UNUSED(pressPoint);
  return distance > KJitterBufferThreshold;
}

WebTouchEvent::WebTouchEvent()
: m_type(QEvent::None)
, m_button(Qt::NoButton)
, m_buttons(Qt::NoButton)
, m_graphicsSceneEvent(false)
, m_fired(false)
, m_editable(false)
, m_modifier(Qt::NoModifier)
{}

WebTouchEvent::WebTouchEvent(const QMouseEvent *mouseEvent)
{
  Q_ASSERT(mouseEvent != 0);
  m_type = mouseEvent->type();
  m_pos = mouseEvent->pos();
  m_button = mouseEvent->button();
  m_buttons = mouseEvent->buttons();
  m_modifier = mouseEvent->modifiers();
  m_fired = false;
  m_editable = false;
  m_graphicsSceneEvent = false;
}

WebTouchEvent::WebTouchEvent(const QGraphicsSceneMouseEvent *graphicsSceneMouseEvent)
{
  Q_ASSERT(graphicsSceneMouseEvent != 0);
  m_type = graphicsSceneMouseEvent->type();
  m_pos = graphicsSceneMouseEvent->pos().toPoint();
  m_button = graphicsSceneMouseEvent->button();
  m_buttons = graphicsSceneMouseEvent->buttons();
  m_modifier = graphicsSceneMouseEvent->modifiers();
  m_scenePos = graphicsSceneMouseEvent->scenePos();
  m_screenPos = graphicsSceneMouseEvent->screenPos();
  m_buttonDownPos = graphicsSceneMouseEvent->buttonDownPos(graphicsSceneMouseEvent->button());
  m_buttonDownScenePos = graphicsSceneMouseEvent->buttonDownScenePos(graphicsSceneMouseEvent->button());
  m_buttonDownScreenPos = graphicsSceneMouseEvent->buttonDownScreenPos(graphicsSceneMouseEvent->button());
  m_lastPos = graphicsSceneMouseEvent->lastPos();
  m_lastScenePos = graphicsSceneMouseEvent->lastScenePos();
  m_lastScreenPos = graphicsSceneMouseEvent->lastScreenPos();
  m_fired = false;
  m_editable = false;
  m_graphicsSceneEvent = true;
}

void qtwebkit_webframe_scrollRecursively(QWebFrame* qFrame, int dx, int dy, const QPoint &pos)
{
  Q_UNUSED(pos);

  if (!qFrame)
    return;

  bool scrollHorizontal = false;
  bool scrollVertical = false;

  do {
    if (qFrame->scrollBarPolicy(Qt::Horizontal) == Qt::ScrollBarAlwaysOn) {
      if (dx > 0)
        scrollHorizontal = qFrame->scrollBarValue(Qt::Horizontal) < qFrame->scrollBarMaximum(Qt::Horizontal);
      else if (dx < 0)
        scrollHorizontal = qFrame->scrollBarValue(Qt::Horizontal) > qFrame->scrollBarMinimum(Qt::Horizontal);
    } else {
      scrollHorizontal = true;
    }

    if (qFrame->scrollBarPolicy(Qt::Vertical) == Qt::ScrollBarAlwaysOn) {
      if (dy > 0)
        scrollVertical = qFrame->scrollBarValue(Qt::Vertical) < qFrame->scrollBarMaximum(Qt::Vertical);
      else if (dy < 0) 
        scrollVertical = qFrame->scrollBarValue(Qt::Vertical) > qFrame->scrollBarMinimum(Qt::Vertical);
    } else {
      scrollVertical = true;
    }

    if (scrollHorizontal || scrollVertical) {
      qFrame->scroll(dx, dy);
      return;
    }

    qFrame = qFrame->parentFrame();
  } while (qFrame);
}

WebTouchScroller::WebTouchScroller(QObject *parent)
: QObject(parent)
, m_webFrame(0)
{
}

WebTouchScroller::~WebTouchScroller()
{
}

void WebTouchScroller::setFrame(QWebFrame* frame)
{
  m_webFrame = frame;
}

void WebTouchScroller::scroll(const QPointF &delta, const QPoint &scrollStartPoint)
{
  if (!m_webFrame)
    return;

  qtwebkit_webframe_scrollRecursively(m_webFrame, delta.x(), delta.y(),
    scrollStartPoint - m_webFrame->scrollPosition());
}


int xInRect(const QRect &r, int x)
{
  int x1 = qMin(x, r.right()-2);
  return qMax(x1, r.left()+2);
}

int yInRect(const QRect &r, int y)
{
  int y1 = qMin(y, r.bottom()-2);
  return qMax(y1, r.top()+2);
}

int wtnFastDistance(const QPoint &p, const QPoint &q)
{
  return (p.x() - q.x()) * (p.x() - q.x()) + (p.y() - q.y()) * (p.y() - q.y());
}

QPoint frameViewPosition(QWebFrame *frame)
{
  QPoint fp;
  QWebFrame* f = frame;
  while (1) {
    fp += f->pos();
    f = f->parentFrame();
    if (f) {
      fp -= f->scrollPosition();
    } else
    break;
  }
  return fp;
}

QPoint closestElement(QObject *viewObject, QWebFrame *frame, WebTouchEvent &touchEvent, int searchThreshold)
{
  Q_UNUSED(viewObject)
  QPoint adjustedPoint(touchEvent.m_pos);

  QWebHitTestResult htr = frame->hitTestContent(adjustedPoint);

  if (htr.isContentEditable()) {
    QString type = htr.element().attribute("type").toLower();
    if (type == "hidden") {
      touchEvent.m_editable = false;
      return adjustedPoint;
    }
    QString disabled = htr.element().attribute("disabled").toLower();
    if (disabled == "disabled" || disabled == "true") {
      touchEvent.m_editable = false;
      return adjustedPoint;
    }
    touchEvent.m_editable = true;
    return adjustedPoint;
  }

  if (!htr.element().tagName().toLower().compare("select") && htr.element().hasAttribute("multiple")) {
    touchEvent.m_modifier = Qt::ControlModifier;
    return adjustedPoint;
  }


  QString tagName = htr.element().tagName().toLower();
  if (!htr.linkElement().isNull() ||
    !tagName.compare("input") ||
    !tagName.compare("map") ||
    !tagName.compare("button") ||
    !tagName.compare("textarea") ||
    htr.element().hasAttribute("onClick"))
    return adjustedPoint;

  QString selector = "input[type=\"radio\"], input[type=\"checkbox\"]";
  QWebElementCollection elementList = frame->findAllElements(selector);
  QWebElementCollection::iterator it(elementList.begin());

  QPoint originPoint = frameViewPosition(frame);

  QPoint frameScrollPoint = frame->scrollPosition();
  QPoint framePoint = adjustedPoint - originPoint + frameScrollPoint;

  QPoint adjustedFramePoint = framePoint;

  int maxDist = searchThreshold;
  while (it != elementList.end()) {
    QRect nRect((*it).geometry());
    if (nRect.isValid()) {
      QPoint pt(xInRect(nRect, framePoint.x()), yInRect(nRect, framePoint.y()));
      int dist = wtnFastDistance(pt, framePoint);
      if (dist < maxDist) {
        adjustedFramePoint = pt;
        maxDist = dist;
      }
    }
    ++it;
  }

  adjustedPoint = adjustedFramePoint - frameScrollPoint + originPoint;

  return adjustedPoint;
}

WebTouchNavigation::WebTouchNavigation(QObject *parent, QWebPage *webPage)
: m_viewObject(parent)
, m_webPage(webPage)
, m_suppressMouseEvents(false)

{
  Q_ASSERT(m_webPage);
  connect(&m_downTimer, SIGNAL(timeout()), this, SLOT(downTimerFired()));
  connect(&m_hoverTimer, SIGNAL(timeout()), this, SLOT(hoverTimerFired()));
  connect(&m_quickDownTimer, SIGNAL(timeout()), this, SLOT(quickDownTimerFired()));
  connect(&m_quickUpTimer, SIGNAL(timeout()), this, SLOT(quickUpTimerFired()));

  m_physics = WebTouchPhysicsInterface::getSingleton();
  m_physics->setParent(this);
  m_scroller = new WebTouchScroller(this);

  connect(m_physics, SIGNAL(positionChanged(QPointF,QPoint)), m_scroller, SLOT(scroll(QPointF,QPoint)));
  connect(m_scroller, SIGNAL(rangeChanged(QRectF)), m_physics, SLOT(setRange(QRectF)));
}

WebTouchNavigation::~WebTouchNavigation()
{
}

bool WebTouchNavigation::eventFilter(QObject *object, QEvent *event)
{
  if (object != m_viewObject)
    return false;

  bool eventHandled = false;

  switch (event->type()) {
    case QEvent::MouseButtonPress:
    if (static_cast<QMouseEvent*>(event)->buttons() & Qt::LeftButton) {
      WebTouchEvent e(static_cast<QMouseEvent*>(event));
      handleDownEvent(e);
    }
    eventHandled = true;
    break;
    case QEvent::MouseMove:
    if (static_cast<QMouseEvent*>(event)->buttons() & Qt::LeftButton) {
      WebTouchEvent e(static_cast<QMouseEvent*>(event));
      handleMoveEvent(e);
    }
    eventHandled = true;
    break;
    case QEvent::MouseButtonRelease:
    {
      WebTouchEvent e(static_cast<QMouseEvent*>(event));
      handleReleaseEvent(e);
      eventHandled = true;
    }
    break;
    case QEvent::GraphicsSceneMousePress:
    if (static_cast<QGraphicsSceneMouseEvent*>(event)->buttons() & Qt::LeftButton) {
      WebTouchEvent e(static_cast<QGraphicsSceneMouseEvent*>(event));
      handleDownEvent(e);
    }
    eventHandled = true;
    break;
    case QEvent::GraphicsSceneMouseMove:
    if (static_cast<QGraphicsSceneMouseEvent *>(event)->buttons() & Qt::LeftButton) {
      WebTouchEvent e(static_cast<QGraphicsSceneMouseEvent*>(event));
      handleMoveEvent(e);
    }
    eventHandled = true;
    break;
    case QEvent::GraphicsSceneMouseRelease:
    {
      WebTouchEvent e(static_cast<QGraphicsSceneMouseEvent*>(event));
      handleReleaseEvent(e);
      eventHandled = true;
    }
    break;
    case QEvent::MouseButtonDblClick:
    case QEvent::ContextMenu:
    case QEvent::CursorChange:
    case QEvent::DragEnter:
    case QEvent::DragLeave:
    case QEvent::DragMove:
    case QEvent::Drop:
    case QEvent::GrabMouse:
    case QEvent::GraphicsSceneContextMenu:
    case QEvent::GraphicsSceneDragEnter:
    case QEvent::GraphicsSceneDragLeave:
    case QEvent::GraphicsSceneDragMove:
    case QEvent::GraphicsSceneDrop:
    case QEvent::GraphicsSceneHelp:
    case QEvent::GraphicsSceneHoverEnter:
    case QEvent::GraphicsSceneHoverLeave:
    case QEvent::GraphicsSceneHoverMove:
    case QEvent::HoverEnter:
    case QEvent::HoverLeave:
    case QEvent::HoverMove:
    case QEvent::Gesture:
    case QEvent::GestureOverride:
    eventHandled = true;
    break;
    default:
    break;
  }

  return eventHandled;
}

void WebTouchNavigation::quickDownTimerFired()
{
  m_touchDown.m_type = (m_touchDown.m_graphicsSceneEvent) ? QEvent::GraphicsSceneMousePress : QEvent::MouseButtonPress;
  m_touchDown.m_pos = closestElement(m_viewObject, m_webFrame, m_touchDown, KNodeSearchThreshold);
  m_touchDown.m_button = Qt::LeftButton;
  m_touchDown.m_buttons = Qt::NoButton;
  generateMouseEvent(m_touchDown);
  m_quickUpTimer.setSingleShot(true);
  m_quickUpTimer.start(0);
}

void WebTouchNavigation::quickUpTimerFired()
{
  m_touchDown.m_type = (m_touchDown.m_graphicsSceneEvent) ? QEvent::GraphicsSceneMouseRelease : QEvent::MouseButtonRelease;
  m_touchDown.m_button = Qt::LeftButton;
  m_touchDown.m_buttons = Qt::NoButton;
  generateMouseEvent(m_touchDown);
}

void WebTouchNavigation::downTimerFired()
{
  m_touchDown.m_type = (m_touchDown.m_graphicsSceneEvent) ? QEvent::GraphicsSceneMousePress : QEvent::MouseButtonPress;
  m_touchDown.m_pos = closestElement(m_viewObject, m_webFrame, m_touchDown, KNodeSearchThreshold);
  m_touchDown.m_button = Qt::LeftButton;
  m_touchDown.m_buttons = Qt::NoButton;
  generateMouseEvent(m_touchDown);
  m_touchDown.m_fired = true;
}

void WebTouchNavigation::hoverTimerFired()
{
  m_touchDown.m_type = (m_touchDown.m_graphicsSceneEvent) ? QEvent::GraphicsSceneMouseMove : QEvent::MouseMove;
  m_touchDown.m_button = Qt::NoButton;
  m_touchDown.m_buttons = Qt::NoButton;
  generateMouseEvent(m_touchDown);
}

void WebTouchNavigation::invalidateLastTouchDown()
{
  if (m_touchDown.m_fired) {
    m_touchDown.m_type = (m_touchDown.m_graphicsSceneEvent) ? QEvent::GraphicsSceneMouseRelease : QEvent::MouseButtonRelease;
    m_touchDown.m_pos = QPoint(-1, -1);
    m_touchDown.m_button = Qt::LeftButton;
    m_touchDown.m_buttons = Qt::NoButton;
    m_touchDown.m_editable = false;
    generateMouseEvent(m_touchDown);
  }
}

void WebTouchNavigation::handleDownEvent(const WebTouchEvent &event)
{
  m_physics->stop();

  m_downTimer.stop();
  m_hoverTimer.stop();
  m_quickDownTimer.stop();
  m_quickUpTimer.stop();

  m_webFrame = m_webPage->frameAt(event.m_pos);
  if (!m_webFrame)
    m_webFrame = m_webPage->currentFrame();

  m_scroller->setFrame(m_webFrame);

  m_touchDown = event;

  m_hoverTimer.setSingleShot(true);
  m_hoverTimer.start(KHoverTimeoutThreshold);

  m_downTimer.setSingleShot(true);
  m_downTimer.start(KTouchDownStartTime);

  m_physics->start(event.m_pos, m_webFrame);
}

void WebTouchNavigation::handleMoveEvent(const WebTouchEvent &event)
{
  if (m_physics->move(event.m_pos)) {
    m_downTimer.stop();
    m_hoverTimer.stop();
  }
}

void WebTouchNavigation::handleReleaseEvent(const WebTouchEvent &event)
{
  if (!m_physics->inMotion() && (m_hoverTimer.isActive() || m_downTimer.isActive())) {
    if (m_hoverTimer.isActive()) {
      m_touchDown.m_type = (m_touchDown.m_graphicsSceneEvent) ? QEvent::GraphicsSceneMouseMove : QEvent::MouseMove;
      m_touchDown.m_button = Qt::NoButton;
      m_touchDown.m_buttons = Qt::NoButton;
      generateMouseEvent(m_touchDown);
    }

    m_hoverTimer.stop();
    m_downTimer.stop();

    m_quickDownTimer.setSingleShot(true);
    m_quickDownTimer.start(0);
    return;
  }

  m_hoverTimer.stop();
  m_downTimer.stop();

  if (m_physics->release(event.m_pos)) {
    invalidateLastTouchDown();
    return;
  }

  if (m_touchDown.m_fired) {
    m_touchDown.m_type = (m_touchDown.m_graphicsSceneEvent) ? QEvent::GraphicsSceneMouseRelease : QEvent::MouseButtonRelease;
    m_touchDown.m_button = Qt::LeftButton;
    m_touchDown.m_buttons = Qt::NoButton;
    generateMouseEvent(m_touchDown);
  }
}

void WebTouchNavigation::generateMouseEvent(const WebTouchEvent &touchEvent)
{
  if (!touchEvent.m_editable && m_suppressMouseEvents) 
    return;

  if (touchEvent.m_type == QEvent::GraphicsSceneMousePress
    || touchEvent.m_type == QEvent::GraphicsSceneMouseMove
    || touchEvent.m_type == QEvent::GraphicsSceneMouseRelease) {
    QGraphicsSceneMouseEvent ievmm(touchEvent.m_type);
  ievmm.setPos(touchEvent.m_pos);
  ievmm.setScenePos(touchEvent.m_scenePos);
  ievmm.setScreenPos(touchEvent.m_screenPos);
  ievmm.setButtonDownPos(touchEvent.m_button, touchEvent.m_buttonDownPos);
  ievmm.setButtonDownScenePos( touchEvent.m_button, touchEvent.m_buttonDownScenePos);
  ievmm.setButtonDownScreenPos( touchEvent.m_button, touchEvent.m_buttonDownScreenPos);
  ievmm.setLastPos(touchEvent.m_lastPos);
  ievmm.setLastScenePos(touchEvent.m_lastScenePos);
  ievmm.setLastScreenPos(touchEvent.m_lastScreenPos);
  ievmm.setButtons(touchEvent.m_buttons);
  ievmm.setButton( touchEvent.m_button);
  ievmm.setModifiers(touchEvent.m_modifier);
  m_webPage->event(&ievmm);
} else {
  bool inputMethodEnabled = static_cast<QWidget*>(m_viewObject)->testAttribute(Qt::WA_InputMethodEnabled);
  if (touchEvent.m_type == QEvent::MouseButtonRelease && inputMethodEnabled) {
    if (touchEvent.m_editable) {
      QEvent rsipevent(QEvent::RequestSoftwareInputPanel);
      QCoreApplication::sendEvent(static_cast<QWidget*>(m_viewObject), &rsipevent);
    } else {
      static_cast<QWidget*>(m_viewObject)->setAttribute(Qt::WA_InputMethodEnabled, false); 
    }

    QMouseEvent ievmm(touchEvent.m_type, touchEvent.m_pos, touchEvent.m_button, touchEvent.m_buttons, touchEvent.m_modifier);
    m_webPage->event(&ievmm);

    if (touchEvent.m_type == QEvent::MouseButtonRelease && inputMethodEnabled)
      static_cast<QWidget*>(m_viewObject)->setAttribute(Qt::WA_InputMethodEnabled, inputMethodEnabled); 
  }


  WebNavigation::WebNavigation(QObject *parent, QWebPage *webPage)
  : m_viewObject(parent)
  , m_webPage(webPage)
  , m_webTouchNavigation(0)
  {
    m_webTouchNavigation = new WebTouchNavigation(m_viewObject, m_webPage);
    m_viewObject->installEventFilter(m_webTouchNavigation);
  }

  WebNavigation::~WebNavigation()
  {
    delete m_webTouchNavigation;
  }

  class NavigationControllerPrivate
  {
  public:
    NavigationControllerPrivate(QWidget *parent, QGraphicsWebView *webView);
    ~NavigationControllerPrivate();

    QWebPage *m_webPage;
    QWidget *m_webWidget;
    QGraphicsWebView *m_graphicsWebView;
    WebNavigation *m_webNavigation;
  };

  NavigationControllerPrivate::NavigationControllerPrivate(QWidget *parent, QGraphicsWebView *webView)
  : m_webPage(0)
  , m_webWidget(0)
  , m_graphicsWebView(webView)
  , m_webNavigation(0)
  {
    Q_UNUSED(parent);
    m_graphicsWebView->setAcceptTouchEvents(true);
    m_webPage = new QWebPage;
    m_graphicsWebView->setPage(m_webPage);
    m_webNavigation = new WebNavigation(m_graphicsWebView, m_webPage);
    m_webNavigation->setParent(m_graphicsWebView);
  }

  NavigationControllerPrivate::~NavigationControllerPrivate()
  {
    if (m_webNavigation)
      delete m_webNavigation;
    if (m_webPage)
      delete m_webPage;
    if (m_graphicsWebView)
      delete m_graphicsWebView;
  }

  NavigationController::NavigationController(QWidget *parent, QGraphicsWebView *webView)
  : m_d(new NavigationControllerPrivate(parent, webView))
  {
  }

  NavigationController::~NavigationController()
  {
    delete m_d;
  }

  QWebPage* NavigationController::webPage() const
  {
    return m_d->m_webPage;
  }

  QGraphicsWebView* NavigationController::graphicsWebView() const
  {
    return m_d->m_graphicsWebView;
  }
#endif 
  class Html5ApplicationViewerPrivate : public QGraphicsView
  {
    Q_OBJECT
  public:
    Html5ApplicationViewerPrivate(QWidget *parent = 0);

    void resizeEvent(QResizeEvent *event);
    static QString adjustPath(const QString &path);

    public slots:
    void quit();

    private slots:
    void addToJavaScript();

    signals:
    void quitRequested();

  public:
    QGraphicsWebView *m_webView;
#ifdef TOUCH_OPTIMIZED_NAVIGATION
    NavigationController *m_controller;
#endif 
  };

  Html5ApplicationViewerPrivate::Html5ApplicationViewerPrivate(QWidget *parent)
  : QGraphicsView(parent)
  {
    QGraphicsScene *scene = new QGraphicsScene;
    setScene(scene);
    setFrameShape(QFrame::NoFrame);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_webView = new QGraphicsWebView;
    m_webView->setAcceptTouchEvents(true);
    m_webView->setAcceptHoverEvents(false);
    setAttribute(Qt::WA_AcceptTouchEvents, true);
    scene->addItem(m_webView);
    scene->setActiveWindow(m_webView);
#ifdef TOUCH_OPTIMIZED_NAVIGATION
    m_controller = new NavigationController(parent, m_webView);
#endif 
    connect(m_webView->page()->mainFrame(),
      SIGNAL(javaScriptWindowObjectCleared()), SLOT(addToJavaScript()));
  }

  void Html5ApplicationViewerPrivate::resizeEvent(QResizeEvent *event)
  {
    m_webView->resize(event->size());
  }

  QString Html5ApplicationViewerPrivate::adjustPath(const QString &path)
  {
#ifdef Q_OS_UNIX
#ifdef Q_OS_MAC
    if (!QDir::isAbsolutePath(path))
      return QCoreApplication::applicationDirPath()
    + QLatin1String("/../Resources/") + path;
#else
    const QString pathInInstallDir = QCoreApplication::applicationDirPath()
    + QLatin1String("/../") + path;
    if (pathInInstallDir.contains(QLatin1String("opt"))
      && pathInInstallDir.contains(QLatin1String("bin"))
      && QFileInfo(pathInInstallDir).exists()) {
      return pathInInstallDir;
  }
#endif
#endif
  return QFileInfo(path).absoluteFilePath();
}

void Html5ApplicationViewerPrivate::quit()
{
  emit quitRequested();
}

void Html5ApplicationViewerPrivate::addToJavaScript()
{
  m_webView->page()->mainFrame()->addToJavaScriptWindowObject("Qt", this);
}

Html5ApplicationViewer::Html5ApplicationViewer(QWidget *parent)
: QWidget(parent)
{

  QHBoxLayout *hbox = new QHBoxLayout;
  QFrame *right_bottom = new QFrame(this);
  QGridLayout *layout_RB = new QGridLayout;
  listOfGraphs=new QListWidget;
  view=new Html5ApplicationViewerPrivate*[0];
  listOfGraphNames<<"Current Wheel Angle"<<"Desired Wheel Angle"<<"Wheel Power R"<<"Wheel Power L"<<"Physics Timestep"<<"Control Interval"<<"Line Position";
  ExtendedListItem **listOfGraph=new ExtendedListItem*[listOfGraphNames.length()];
  for(int i=0;i<listOfGraphNames.length();i++)
  {
    listOfGraph[i]=new ExtendedListItem(listOfGraphs,listOfGraphNames[i],false);
    connect(listOfGraph[i],SIGNAL(checkBoxChanged(int)),SLOT(potomNazovuFunc()));
  }
  connect(listOfGraphs,SIGNAL(itemClicked(QListWidgetItem*)),SLOT(selectItem(QListWidgetItem*)));
  listOfGraphs->show();
  layout_RB->addWidget(listOfGraphs,0,0);
  right_bottom->setLayout(layout_RB);
  QFrame *right_top = new QFrame(this);
  QPushButton *button_0=new QPushButton("Open File");
  connect(button_0,SIGNAL(clicked()),SLOT(openFile()));
  QPushButton *button_1=new QPushButton("Open Folder");
  connect(button_1,SIGNAL(clicked()),SLOT(openFolder()));
  QGridLayout *layout_RT = new QGridLayout;
  listOfOpenedFiles = new QListWidget();
  connect(listOfOpenedFiles,SIGNAL(itemClicked(QListWidgetItem*)),SLOT(selectItem(QListWidgetItem*)));
  listOfOpenedFiles->show();
  layout_RT->addWidget(button_0,0,0);
  layout_RT->addWidget(button_1,0,1);
  layout_RT->addWidget(listOfOpenedFiles,1,0,1,0);
  right_top->setLayout(layout_RT);
  QSplitter *splitter1 = new QSplitter(Qt::Vertical, this);
  right_top->setMaximumWidth(190);
  right_bottom->setMaximumWidth(190);
  splitter1->addWidget(right_top);
  splitter1->addWidget(right_bottom);
  frameWithGraphs = new QFrame(this);
  redivisionGraph(1);
  QSplitter *splitter2 = new QSplitter(Qt::Horizontal, this);
  splitter2->addWidget(frameWithGraphs);
  splitter2->addWidget(splitter1);
  hbox->addWidget(splitter2);
  setLayout(hbox);
  layout_RT->setMargin(0);
  layout_RB->setMargin(0);
}
void Html5ApplicationViewer::deleteItem()
{
  delete sender();
}
void Html5ApplicationViewer::selectItem(QListWidgetItem *listWidgetItem)
{
  listWidgetItem->setSelected(false);
  ((ExtendedListItem*)((QListWidget*)sender())->itemWidget(listWidgetItem))->Check();
}

void Html5ApplicationViewer::openFolder()
{
  QString fileName=QFileDialog::getExistingDirectory(this,tr("Open Directory"),lastPatch,QFileDialog::ShowDirsOnly|QFileDialog::DontResolveSymlinks);
  if (fileName.length()>0)
  {
    QDir dir(fileName);
    QStringList files=dir.entryList(QStringList("*.dat"),QDir::Files|QDir::NoSymLinks);
    if(files.length()>0)
    {
      lastPatch=fileName;
      for(int j=0;j<files.length();j++)
      {
        addFileToList(fileName+"/"+files[j]);
      }
    }
  }
}

void Html5ApplicationViewer::redivisionGraph(int count)
{
  delete frameWithGraphs->layout();
  if(count<2)
    count=1;
  QGridLayout *layout_L = new QGridLayout;
  delete view;
  view=new Html5ApplicationViewerPrivate*[count];

  QSplitter **splitter3 = new QSplitter*[3];
  for (int i = 0; i < count; ++i)
    view[i]=new Html5ApplicationViewerPrivate(this);
  for (int i = 0; i < 3; ++i)
    splitter3[i] = new QSplitter(this);
  if(count==1)
    layout_L->addWidget(view[0]);
  else
  {
    int i;
    for (i = 0; i < (count/2); ++i)
      splitter3[0]->addWidget(view[i]);
    for (; i < count; ++i)
      splitter3[1]->addWidget(view[i]);

    splitter3[2]->addWidget(splitter3[0]);
    splitter3[2]->addWidget(splitter3[1]);
    splitter3[2]->setOrientation(Qt::Vertical);
    layout_L->addWidget(splitter3[2]);
  }
  for (int i = 0; i < count; ++i)
  {
    view[i]->m_webView->page()->mainFrame()->evaluateJavaScript("document.write(\""+QString::number(i)+"\")");

    connect(view[i]->m_webView,SIGNAL(loadFinished(bool)),SLOT(show1()));
    load(i,"html/index.html");
  }
  layout_L->setMargin(0);
  frameWithGraphs->setLayout(layout_L);
}



void Html5ApplicationViewer::addFileToList(QString fileName)
{
  ExtendedListItem *item=new ExtendedListItem(listOfOpenedFiles,fileName);
  connect(item,SIGNAL(buttonClicked()),SLOT(deleteItem()));
  connect(item,SIGNAL(checkBoxChanged(int)),SLOT(selectItemToShow(int)));
}

void Html5ApplicationViewer::openFile()
{
  QString fileName=QFileDialog::getOpenFileName(this,tr("Open File"),lastPatch,("Dat files(*.dat)"));
  if (fileName.length()>0)
  {
    lastPatch=fileName;
    addFileToList(fileName);
  }
}

void Html5ApplicationViewer::selectItemToShow(int k)
{
  int j;

  if(k){
    qDebug()<<k;
    for(int i=0;i<listOfOpenedFiles->count();i++)
    {
        ExtendedListItem* item=qobject_cast<ExtendedListItem*>(listOfOpenedFiles->itemWidget(listOfOpenedFiles->item(i)));
     if(item->isChecked())
     {
       Logger l;
       DataSet myDataSet;
       l.setFileName(item->getFileSrc());

       if(data.createNew(item->getLabelText()))
       {
        l.beginRead();
        for(j=0;l.canRead();j++)
        {
          l>>myDataSet;
          data.addTo(item->getLabelText(),QString::number(myDataSet.current_wheel_angle,'f'), QString::number(myDataSet.desired_wheel_angle,'f'),QString::number(myDataSet.wheel_power_r,'f'),QString::number(myDataSet.wheel_power_l,'f'),QString::number(myDataSet.physics_timestep,'f'),QString::number(myDataSet.control_interval,'f'),QString::number(myDataSet.line_position));
        }      
        l.endRead();
      }
    }
  }
  }
  else
  {
      data.deleteByName(((ExtendedListItem*)sender())->getLabelText());
      ((ExtendedListItem*)sender())->setClearColorOfCheckBox();
  }  
  show1();
}
void Html5ApplicationViewer::show1()
{
    int k=0;
    for (int i = 0; i <listOfGraphs->count(); ++i) {
        if(((ExtendedListItem*)listOfGraphs->itemWidget(listOfGraphs->item(i)))->isChecked())
          {
            webView(k)->page()->mainFrame()->evaluateJavaScript("var DATA=[];var name;");
            k++;
        }
    }

  for(int i=0;i<data.length();i++)
  {
      QString color="#000";
      color=color+QString::number(i,2);
      if(color.length()>7)
          color=color.mid(-1,5)+color.mid(6,color.length());
      color=color.mid(-1,4-(color.length()-5))+color.mid(4,color.length());
      for (int j = 0; j < color.length(); ++j) {
          if(color[j]=='1')
              color[j]='a';
      }
      for (int j = 0; j <listOfOpenedFiles->count(); ++j) {
          if(data.get_name(i)==((ExtendedListItem*)listOfOpenedFiles->itemWidget(listOfOpenedFiles->item(j)))->getLabelText())
            {
              ((ExtendedListItem*)listOfOpenedFiles->itemWidget(listOfOpenedFiles->item(j)))->setColorOfCheckBox(color);
                break;
          }
      }
      k=0;
      for (int j = 0; j <listOfGraphs->count(); ++j) {
          if(((ExtendedListItem*)listOfGraphs->itemWidget(listOfGraphs->item(j)))->isChecked())
            {
              webView(k)->page()->mainFrame()->evaluateJavaScript("name='"+listOfGraphNames[j]+"';");

              webView(k)->page()->mainFrame()->evaluateJavaScript("DATA.push({name: '"+data.get_name(i)+"',color:'"+color+"',data: ["+data.get(i,j)+"],type: 'spline',tooltip: {valueDecimals: 5}});");

              k++;

          }
      }
  }
  k=0;
  for (int i = 0; i <listOfGraphs->count(); ++i) {
      if(((ExtendedListItem*)listOfGraphs->itemWidget(listOfGraphs->item(i)))->isChecked())
        {
  webView(k)->page()->mainFrame()->evaluateJavaScript("$.getScript('js/main.js');");
  k++;
      }
  }
}
void Html5ApplicationViewer::changeC(QString color)
{
    qDebug()<<color;
}

void Html5ApplicationViewer::potomNazovuFunc()
{
  int count=0;
  for(int i=0;i<listOfGraphs->count();i++)
    if(((ExtendedListItem*)listOfGraphs->itemWidget(listOfGraphs->item(i)))->isChecked())
      count++;
    redivisionGraph(count);
  }
  Html5ApplicationViewer::~Html5ApplicationViewer()
  {
    delete listOfOpenedFiles;
    delete frameWithGraphs;
    delete view;
  }

  void Html5ApplicationViewer::load(int index, const QString &stringFileSrc)
  {
    view[index]->m_webView->setUrl(QUrl::fromLocalFile(Html5ApplicationViewerPrivate::adjustPath(stringFileSrc)));
  }


  void Html5ApplicationViewer::showExpanded()
  {
#if defined(Q_WS_MAEMO_5)
    showMaximized();
#else
    show();
#endif
  }

  QGraphicsWebView *Html5ApplicationViewer::webView(int index) const
  {
    return view[index]->m_webView;
  }

#include "html5applicationviewer.moc"
