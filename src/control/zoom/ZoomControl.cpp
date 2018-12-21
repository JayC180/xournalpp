#include "ZoomControl.h"

#include "gui/Layout.h"
#include "gui/PageView.h"
#include "gui/widgets/XournalWidget.h"
#include "gui/XournalView.h"

const double zoomStep = 0.04;

ZoomControl::ZoomControl()
 : view(NULL)
{
	XOJ_INIT_TYPE(ZoomControl);

	this->zoom = 1.0;
	this->lastZoomValue = 1.0;
	this->zoom100Value = 1.0;
	this->zoomFitValue = 1.0;
	this->zoomFitMode = true;
	this->zoomCenterX = -1;
	this->zoomCenterY = -1;

	this->zoomSequenceStart = 1;
}

ZoomControl::~ZoomControl()
{
	XOJ_CHECK_TYPE(ZoomControl);

	XOJ_RELEASE_TYPE(ZoomControl);
}

/**
 * Call this before any zoom is done, it saves the current page and position
 *
 * @param centerX Zoom Center X (use -1 for center of the visible rect)
 * @param centerY Zoom Center Y (use -1 for center of the visible rect)
 */
void ZoomControl::startZoomSequence(double centerX, double centerY)
{
	XOJ_CHECK_TYPE(ZoomControl);

	GtkWidget* widget = view->getWidget();
	Layout* layout = gtk_xournal_get_layout(widget);
	// Save visible rectangle at beginning of zoom
	zoomSequenceRectangle = layout->getVisibleRect();

	this->zoomCenterX = centerX - zoomSequenceRectangle.x;
	this->zoomCenterY = centerY - zoomSequenceRectangle.y;


	// Scale to 100% zoom
	zoomSequenceRectangle.x /= this->zoom;
	zoomSequenceRectangle.y /= this->zoom;

	zoomSequenceStart = this->zoom;
}

/**
 * Change the zoom within a Zoom sequence (startZoomSequence() / endZoomSequence())
 *
 * @param zoom Current zoom value
 * @param relative If the zoom is relative to the start value (for Gesture)
 */
void ZoomControl::zoomSequnceChange(double zoom, bool relative)
{
	XOJ_CHECK_TYPE(ZoomControl);

	if (relative) {
		zoom *= zoomSequenceStart;
	}

	setZoom(zoom);
}

/**
 * Clear all stored data from startZoomSequence()
 */
void ZoomControl::endZoomSequence()
{
	XOJ_CHECK_TYPE(ZoomControl);
	zoomCenterX = -1;
	zoomCenterY = -1;
}

/**
 * Zoom to correct position on zooming
 */
void ZoomControl::scrollToZoomPosition(XojPageView* view)
{
	Layout* layout = gtk_xournal_get_layout(this->view->getWidget());


	printf("ZoomPos: %lf / %lf\n", this->zoomCenterX, this->zoomCenterY);
//	if (this->zoomCenterX == -1 || this->zoomCenterY == -1)
//	{
//		// get margins for relative scroll calculation
//		double marginLeft = (double) view->layout.getMarginLeft();
//		double marginTop = (double) view->layout.getMarginTop();
//
//		int visX = (int) ((zoomCenterX - marginLeft) * (this->zoom / zoomSequenceStart - 1));
//		int visY = (int) ((zoomCenterY - marginTop) * (this->zoom / zoomSequenceStart - 1));
//		layout->scrollAbs(zoomSequenceRectangle.x + visX, zoomSequenceRectangle.y + visY);
//	}
//	else
//	{
		layout->scrollAbs(zoomSequenceRectangle.x * this->zoom, zoomSequenceRectangle.y * this->zoom);
//	}
}


void ZoomControl::addZoomListener(ZoomListener* listener)
{
	XOJ_CHECK_TYPE(ZoomControl);

	this->listener.push_back(listener);
}

void ZoomControl::initZoomHandler(GtkWidget* widget, XournalView* view)
{
	XOJ_CHECK_TYPE(ZoomControl);

	g_signal_connect(widget, "scroll_event", G_CALLBACK(
		+[](GtkWidget* widget, GdkEventScroll* event, ZoomControl* self)
		{
			XOJ_CHECK_TYPE_OBJ(self, ZoomControl);
			return self->onScrolledwindowMainScrollEvent(event);
		}), this);

	this->view = view;
}

void ZoomControl::fireZoomChanged()
{
	XOJ_CHECK_TYPE(ZoomControl);

	if (this->zoom < MIN_ZOOM)
	{
		this->zoom = MIN_ZOOM;
	}

	if (this->zoom > MAX_ZOOM)
	{
		this->zoom = MAX_ZOOM;
	}

	for (ZoomListener* z : this->listener)
	{
		z->zoomChanged();
	}
}

void ZoomControl::fireZoomRangeValueChanged()
{
	XOJ_CHECK_TYPE(ZoomControl);

	for (ZoomListener* z : this->listener)
	{
		z->zoomRangeValuesChanged();
	}
}

double ZoomControl::getZoom()
{
	XOJ_CHECK_TYPE(ZoomControl);

	return this->zoom;
}

void ZoomControl::setZoom(double zoom)
{
	XOJ_CHECK_TYPE(ZoomControl);

	this->zoom = zoom;
	this->zoomFitMode = false;
	fireZoomChanged();
}

void ZoomControl::setZoom100(double zoom)
{
	XOJ_CHECK_TYPE(ZoomControl);

	this->zoom100Value = zoom;
	fireZoomRangeValueChanged();
}

void ZoomControl::setZoomFit(double zoom)
{
	XOJ_CHECK_TYPE(ZoomControl);

	this->zoomFitValue = zoom;
	fireZoomRangeValueChanged();

	if (this->zoomFitMode)
	{
		this->zoom = this->zoomFitValue;
		fireZoomChanged();
	}
}

double ZoomControl::getZoomFit()
{
	XOJ_CHECK_TYPE(ZoomControl);

	return this->zoomFitValue;
}

double ZoomControl::getZoom100()
{
	XOJ_CHECK_TYPE(ZoomControl);

	return this->zoom100Value;
}

void ZoomControl::zoom100()
{
	XOJ_CHECK_TYPE(ZoomControl);

	startZoomSequence(-1, -1);

	this->zoom = this->zoom100Value;
	this->zoomFitMode = false;
	fireZoomChanged();

	endZoomSequence();
}

void ZoomControl::zoomFit()
{
	XOJ_CHECK_TYPE(ZoomControl);

	startZoomSequence(-1, -1);

	this->zoom = this->zoomFitValue;
	this->zoomFitMode = true;
	fireZoomChanged();

	endZoomSequence();
}

void ZoomControl::zoomIn(double x, double y)
{
	XOJ_CHECK_TYPE(ZoomControl);

	startZoomSequence(x, y);

	this->zoom += zoomStep;
	this->zoomFitMode = false;
	fireZoomChanged();

	endZoomSequence();
}

void ZoomControl::zoomOut(double x, double y)
{
	XOJ_CHECK_TYPE(ZoomControl);

	startZoomSequence(x, y);

	this->zoom -= zoomStep;
	this->zoomFitMode = false;
	fireZoomChanged();

	endZoomSequence();
}

bool ZoomControl::onScrolledwindowMainScrollEvent(GdkEventScroll* event)
{
	XOJ_CHECK_TYPE(ZoomControl);

	guint state = event->state & gtk_accelerator_get_default_mod_mask();

	// do not handle e.g. ALT + Scroll (e.g. Compiz use this shortcut for setting transparency...)
	if (state != 0 && (state & ~(GDK_CONTROL_MASK | GDK_SHIFT_MASK)))
	{
		return true;
	}

	if (state & GDK_CONTROL_MASK)
	{
		GtkWidget* topLevel = gtk_widget_get_toplevel(view->getWidget());
		int wx = 0;
		int wy = 0;
		gtk_widget_translate_coordinates(view->getWidget(), topLevel, 0, 0, &wx, &wy);

		printf("event: %lf / %lf (%lf / %lf)\n", event->x, event->y,
				event->x_root, event->y_root

		);

		if (event->direction == GDK_SCROLL_UP ||
			(event->direction == GDK_SCROLL_SMOOTH && event->delta_y > 0))
		{
			zoomOut(event->x - wx, event->y - wy);
		}
		else if (event->direction == GDK_SCROLL_DOWN ||
			(event->direction == GDK_SCROLL_SMOOTH && event->delta_y < 0))
		{
			zoomIn(event->x - wx, event->y - wy);
		}
		return true;
	}

	return false;
}
