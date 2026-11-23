/*
 * This file is part of the DSView project.
 * DSView is based on PulseView.
 *
 * Copyright (C) 2012 Joel Holdsworth <joel@airwebreathe.org.uk>
 * Copyright (C) 2013 DreamSourceLab <support@dreamsourcelab.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include "header.h" 
  
#include <QColorDialog>
#include <QInputDialog>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QRect>
#include <QStyleOption>
#include <QApplication>
#include <assert.h>
#include <algorithm>
#include <QFont>
#include <QScrollBar>

#include "view.h"
#include "trace.h"
#include "dsosignal.h"
#include "logicsignal.h"
#include "analogsignal.h"
#include "groupsignal.h"
#include "decodetrace.h"
#include "../sigsession.h"
#include "../dsvdef.h"
#include "../ui/langresource.h"
#include "../appcontrol.h"
#include "../config/appconfig.h"
#include "../ui/fn.h"
#include "../log.h"
 
using namespace std;

namespace pv {
namespace view {

Header::Header(View &parent) :
	QWidget(&parent),
    _view(parent)
{
    _moveFlag = false;
    _colorFlag = false;
    _nameFlag = false;
    _context_trace = NULL;
    _mouse_is_down = false;
    
    nameEdit = new PopupLineEdit(this);
    nameEdit->setFixedWidth(100);
    nameEdit->hide();
    nameEdit->set_instant_mode(true);

	setMouseTracking(true);

    connect(nameEdit, SIGNAL(editingFinished()),
            this, SLOT(on_action_set_name_triggered()));


    ADD_UI(this);
}

Header::~Header()
{
    REMOVE_UI(this);
}

void Header::retranslateUi()
{
    update();
}


int Header::get_nameEditWidth()
{ 
    if (nameEdit->hasFocus())
        return nameEdit->width();
    else
        return 0;
}

pv::view::Trace* Header::get_mTrace(int &action, const QPoint &pt)
{
    const int w = width();
    std::vector<Trace*> traces;
    _view.get_traces(ALL_VIEW, traces);

    for(auto t : traces)
    {
        if (_view.trace_covered(t, pt.y()))
            continue;
        if ((action = t->pt_in_rect(t->get_y(), w, pt)))
            return t;
    }

    action = 0;
    return NULL;
}

// A small pushpin, drawn after the name of a pinned decoder.
static void paint_pin_icon(QPainter &p, QPointF c, double size, QColor colour)
{
    p.save();
    p.setRenderHint(QPainter::Antialiasing, true);
    p.translate(c);
    p.rotate(35);
    const double s = size / 12.0;
    p.setPen(Qt::NoPen);
    p.setBrush(colour);
    p.drawRoundedRect(QRectF(-3 * s, -6 * s, 6 * s, 5 * s), 1 * s, 1 * s); // head
    p.drawRect(QRectF(-4.5 * s, -1.5 * s, 9 * s, 1.5 * s));                // collar
    p.setPen(QPen(colour, std::max(1.0, 1.2 * s)));
    p.drawLine(QPointF(0, 0), QPointF(0, 6 * s));                           // needle
    p.restore();
}

void Header::paintEvent(QPaintEvent*)
{ 
    using pv::view::Trace;

    QStyleOption o;
    o.initFrom(this);
    QPainter painter(this);
    if(AppConfig::Instance().appOptions.antialias) {
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::TextAntialiasing);
    }
    style()->drawPrimitive(QStyle::PE_Widget, &o, &painter, this);

	const int w = width();
    std::vector<Trace*> traces;
    _view.get_traces(ALL_VIEW, traces);

    const bool dragging = !_drag_traces.empty();
    QColor fore(QWidget::palette().color(QWidget::foregroundRole()));
    fore.setAlpha(View::ForeAlpha);
 
    QFont font(painter.font());
    float fSize = AppConfig::Instance().GetTraceFontSize();
    // Grow channel-label text together with the trace height (logic mode).
    fSize *= _view.get_trace_font_scale();
    font.setPointSizeF(fSize);
    painter.setFont(font);

    // Pinned decoders own the band at the top; other labels scroll under it.
    const int pinned_band = _view.pinned_band_bottom();

    for(auto t : traces)
	{
        painter.save();
        // (The band exists in logic mode only, where every trace is in the time pane.)
        if (pinned_band > 0) {
            if (t->pinned())
                painter.setClipRect(0, 0, w, pinned_band);
            else
                painter.setClipRect(0, pinned_band, w, std::max(height() - pinned_band, 0));
        }
        const double name_w = painter.fontMetrics().boundingRect(t->get_name()).width();
        t->paint_label(painter, w, dragging ? QPoint(-1, -1) : _mouse_point, fore);

        if (t->pinned() && t->enabled()) {
            // Just after the name, in the (dotted) space before the channel numbers.
            const QRectF name_rect = t->get_rect("name", t->get_y(), w);
            const double size = std::min(14.0, name_rect.height() * 0.7);
            const double x = std::min(name_rect.left() + name_w + size * 0.9, w * 0.6);
            paint_pin_icon(painter, QPointF(x, t->get_y()), size, fore);
        }
        painter.restore();
	}

    if (pinned_band > 0) {
        QColor edge(fore);
        edge.setAlpha(150);
        painter.setPen(QPen(edge, 1));
        painter.drawLine(0, pinned_band - 1, w, pinned_band - 1);
    }

	painter.end();
}

void Header::mouseDoubleClickEvent(QMouseEvent *event)
{
    assert(event);

    std::vector<Trace*> traces;

    _view.get_traces(ALL_VIEW, traces);

    if (event->button() & Qt::LeftButton) {
        _mouse_down_point = event->pos();

        // Save the offsets of any Traces which will be dragged
        for(auto t : traces){
            if (t->selected())
                _drag_traces.push_back(
                    make_pair(t, t->get_v_offset()));
        }

        // Select the Trace if it has been clicked
        for(auto t : traces){
            if (_view.trace_covered(t, event->pos().y()))
                continue;
            if (t->mouse_double_click(width(), event->pos()))
                break;
        }
    }

}

void Header::mousePressEvent(QMouseEvent *event)
{
	assert(event);

    _mouse_is_down = true;

    std::vector<Trace*> traces;
    _view.get_traces(ALL_VIEW, traces);
    int action;

    const bool instant = _view.session().is_instant();
    if (instant && _view.session().is_running_status()) {
        return;
    }

	if (event->button() & Qt::LeftButton) {
		_mouse_down_point = event->pos();

        // Save the offsets of any Traces which will be dragged
        for(auto t : traces){
            if (t->selected())
                _drag_traces.push_back(
                    make_pair(t, t->get_v_offset()));
        }

        // Select the Trace if it has been clicked
        const auto mTrace = get_mTrace(action, event->pos());
        if (action == Trace::COLOR && mTrace) {
            _colorFlag = true;
        }
        else if (action == Trace::NAME && mTrace) {
            _nameFlag = true;
        }
        else if (action == Trace::LABEL && mTrace) {
            mTrace->select(true);

            if (~QApplication::keyboardModifiers() & Qt::ControlModifier)
                _drag_traces.clear();
            
            _drag_traces.push_back(make_pair(mTrace, mTrace->get_zero_vpos()));
            mTrace->set_old_v_offset(mTrace->get_v_offset());
        }

        for(auto t : traces){
            if (t->signal_type() == SR_CHANNEL_LOGIC && _view.session().is_working()){
                // Disable set trigger from left pannel when capturing.
                break;
            }
            if (_view.trace_covered(t, event->pos().y()))
                continue;
            if (t->mouse_press(width(), event->pos()))
                break;
        }

        if (~QApplication::keyboardModifiers() & Qt::ControlModifier) {
            // Unselect all other Traces because the Ctrl is not
            // pressed
            for(auto t : traces){
                if (t != mTrace)
                    t->select(false);
            }
        }
        update();
    }
}

void Header::mouseReleaseEvent(QMouseEvent *event)
{
	assert(event);

    _mouse_is_down = false;

    // judge for color / name / trigger / move
    int action;
    const auto mTrace = get_mTrace(action, event->pos());

    if (mTrace){
        if (action == Trace::COLOR && _colorFlag) {
            _context_trace = mTrace;
            changeColor(event);
            _view.set_all_update(true);
        }
        else if (action == Trace::NAME && _nameFlag) {
            _context_trace = mTrace;
            changeName(event);
        }
    }

    // Make view index by Y value;
    int mode = _view.session().get_device()->get_work_mode();    
    if (_moveFlag && mode == LOGIC)
    {
        std::vector<Trace*> traces;

        for (auto s : _view.session().get_decode_signals()){
            traces.push_back(s);
        }

        for (auto s : _view.session().get_signals()){
            traces.push_back(s);
        }
    
        sort(traces.begin(), traces.end(), View::compare_trace_y);

        int index = 0;
        for (auto t : traces){
            t->set_view_index(index++);
        }
    }

    if (_moveFlag) {
        _drag_traces.clear();
        _view.signals_changed(mTrace);
        _view.set_all_update(true);

        std::vector<Trace*> traces;
        _view.get_traces(ALL_VIEW, traces);

        for(auto t : traces){
            t->select(false);
        }            
    }

    _colorFlag = false;
    _nameFlag = false;
    _moveFlag = false;

    _view.normalize_layout();
}

void Header::wheelEvent(QWheelEvent *event)
{
    assert(event);

    int x = 0;
    int y = 0;
    int delta = 0;
    bool isVertical = true;
    QPoint pos;
    (void)x;
    (void)y;
     
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    x = (int)event->position().x();
    y = (int)event->position().y();    
    int anglex = event->angleDelta().x();
    int angley = event->angleDelta().y();

    pos.setX(x);
    pos.setY(y);

    if (anglex == 0 || ABS_VAL(angley) >= ABS_VAL(anglex)){
        delta = angley;
        isVertical = true;
    }
    else{
        delta = anglex;
        isVertical = false; //hori direction
    }
#else
    x = event->x();
    delta = event->delta();
    isVertical = event->orientation() == Qt::Vertical;
    pos = event->pos(); 
#endif

    if (isVertical)
    {
        std::vector<Trace*> traces;
        _view.get_traces(ALL_VIEW, traces);
        // Vertical scrolling
        double shift = 0;

        // The dials the traces expose react in whole steps only, so the raw
        // delta has to be turned into whole wheel detents first - high
        // resolution wheels report a fraction of a detent per event and would
        // otherwise always be rounded down to zero.
#ifdef Q_OS_DARWIN
        static bool active = true;
        static int64_t last_time;
        if (event->source() == Qt::MouseEventSynthesizedBySystem)
        {
            if (active)
            {
                last_time = QDateTime::currentMSecsSinceEpoch();
                shift = delta > 1.5 ? -1 : delta < -1.5 ? 1 : 0;
            }
            int64_t cur_time = QDateTime::currentMSecsSinceEpoch();
            if (cur_time - last_time > 100)
                active = true;
            else
                active = false;

            if (shift == 0)
                _wheel_accum.reset();
        }
        else
        {
            shift = -_wheel_accum.take(delta);
        }
#else
        shift = _wheel_accum.take(delta);
#endif

        if (shift != 0)
        {
            for (auto t : traces)
            {
                if (t->mouse_wheel(width(), pos, shift))
                    break;
            }
        }
        _view.verticalScrollBar()->setValue(_view.verticalScrollBar()->value() - delta);
        update();
    }
}

void Header::changeName(QMouseEvent *event)
{
    // A decoder with a fixed label can't be renamed.
    if (_context_trace != NULL && _context_trace->get_type() == SR_CHANNEL_DECODER
        && static_cast<DecodeTrace*>(_context_trace)->has_fixed_label())
        return;

    if (_context_trace != NULL
        && _context_trace->get_type() != SR_CHANNEL_DSO
        && event->button() == Qt::LeftButton) 
    {
        header_resize();
        QFont font = this->font();
        float fsize = AppConfig::Instance().GetTraceFontSize();
        // Match the scaled label font used when painting the channel name.
        fsize *= std::max(1.0, _context_trace->get_label_scale());
        font.setPointSizeF(fsize);
        nameEdit->setFont(font);

        nameEdit->setText(_context_trace->get_name());
        nameEdit->selectAll();
        nameEdit->setFocus();
        nameEdit->show();
        header_updated();
    }
}

void Header::changeColor(QMouseEvent *event)
{
     if (_view.session().is_working() && _view.session().get_device()->get_work_mode() == ANALOG){
        //Disable to select color when working on analog mode.
        return;
    }

    if ((event->button() == Qt::LeftButton)) {
        const QColor new_color = QColorDialog::getColor(_context_trace->get_colour(), this, L_S(STR_PAGE_DLG, S_ID(IDS_DLG_SET_CHANNEL_COLOUR), "Set Channel Colour"));
        if (new_color.isValid())
            _context_trace->set_colour(new_color);
    }
}

void Header::mouseMoveEvent(QMouseEvent *event)
{
	assert(event);

    if (_view.session().is_working() && _view.session().get_device()->get_work_mode() == LOGIC){
        //Disable the hover status of trig button on left pannel.
        return;
    }

	_mouse_point = event->pos();

    // Move the Traces if we are dragging
    if (!_drag_traces.empty()) {
		const int delta = event->pos().y() - _mouse_down_point.y();

        for (auto i = _drag_traces.begin(); i != _drag_traces.end(); i++) {
            const auto t = (*i).first;
			if (t) {
                int y = (*i).second + delta;
                if (t->get_type() == SR_CHANNEL_DSO) {
                    DsoSignal *dsoSig = NULL;
                    if ((dsoSig = dynamic_cast<DsoSignal*>(t))) {
                        dsoSig->set_zero_vpos(y);
                        _moveFlag = true;
                        traces_moved();
                    }
                } else if (t->get_type() == SR_CHANNEL_MATH) {
                    MathTrace *mathTrace = NULL;
                    if ((mathTrace = dynamic_cast<MathTrace*>(t))) {
                       mathTrace->set_zero_vpos(y);
                       _moveFlag = true;
                       traces_moved();
                    }
                 } else if (t->get_type() == SR_CHANNEL_ANALOG) {
                    AnalogSignal *analogSig = NULL;
                    if ((analogSig = dynamic_cast<AnalogSignal*>(t))) {
                        analogSig->set_zero_vpos(y);
                        _moveFlag = true;
                        traces_moved();
                    }
                } else {
                    if (~QApplication::keyboardModifiers() & Qt::ControlModifier) {
                        const int y_snap =
                            ((y + View::SignalSnapGridSize / 2) /
                                View::SignalSnapGridSize) *
                                View::SignalSnapGridSize;
                        if (y_snap != t->get_v_offset()) {
                            _moveFlag = true;
                            t->set_v_offset(y_snap);
                        }
                    }
                }
			}
		}
	}
	update();
}

void Header::leaveEvent(QEvent*)
{
	_mouse_point = QPoint(-1, -1);
	update();
}

void Header::contextMenuEvent(QContextMenuEvent *event)
{
    if (_view.session().get_device()->get_work_mode() != LOGIC)
        return;

    // Right-click anywhere on a decoder's rows: pin it to the top or unpin it.
    const int y = event->pos().y();
    std::vector<Trace*> traces;
    _view.get_traces(ALL_VIEW, traces);

    Trace *decoder = NULL;
    for (auto t : traces) {
        if (t->get_type() != SR_CHANNEL_DECODER || !t->enabled()
            || _view.trace_covered(t, y))
            continue;
        const int half = t->get_totalHeight() / 2 + View::get_signal_margin();
        if (y >= t->get_y() - half && y < t->get_y() + half) {
            decoder = t;
            break;
        }
    }
    if (decoder == NULL)
        return;

    QMenu menu(this);
    QAction *pin = menu.addAction(decoder->pinned()
        ? L_S(STR_PAGE_DLG, S_ID(IDS_DLG_UNPIN_DECODER), "Unpin")
        : L_S(STR_PAGE_DLG, S_ID(IDS_DLG_PIN_DECODER), "Pin to top"));
    if (menu.exec(event->globalPos()) == pin)
        _view.set_trace_pinned(decoder, !decoder->pinned());
}

void Header::on_action_set_name_triggered()
{
    auto context_Trace = _context_trace;
    if (!context_Trace)
		return;

    if (nameEdit->isModified()) {
        QString v = nameEdit->text().trimmed();
        if (v == "")
            v = QString::number(context_Trace->get_index());
        
        _view.session().set_trace_name(context_Trace, v);
    }

    nameEdit->hide();
    header_updated();
}

void Header::header_resize()
{
    if (_context_trace) {
        const int y = _context_trace->get_y();
        nameEdit->move(QPoint(_context_trace->get_leftWidth(), y - nameEdit->height() / 2));
    }
}

void Header::UpdateLanguage()
{
    retranslateUi();
}

void Header::UpdateTheme()
{
    retranslateUi();
}

void Header::UpdateFont()
{
}


} // namespace view
} // namespace pv
