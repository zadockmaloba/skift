#include <libgraphic/Font.h>
#include <libgraphic/Painter.h>
#include <libsystem/Assert.h>
#include <libsystem/Logger.h>
#include <libsystem/io/Stream.h>
#include <libsystem/math/MinMax.h>
#include <libwidget/Application.h>
#include <libwidget/Event.h>
#include <libwidget/Theme.h>
#include <libwidget/Widget.h>
#include <libwidget/Window.h>

void Widget::id(String id)
{
    _window->register_widget_by_id(id, this);
}

Color Widget::color(ThemeColorRole role)
{
    if (!enabled() || (_parent && !_parent->enabled()))
    {
        if (role == THEME_FOREGROUND)
        {
            role = THEME_FOREGROUND_INACTIVE;
        }

        if (role == THEME_SELECTION)
        {
            role = THEME_SELECTION_INACTIVE;
        }

        if (role == THEME_ACCENT)
        {
            role = THEME_ACCENT_INACTIVE;
        }
    }

    if (_colors[role].overwritten)
    {
        return _colors[role].color;
    }

    return _window->color(role);
}

void Widget::color(ThemeColorRole role, Color color)
{
    _colors[role].overwritten = true;
    _colors[role].color = color;

    should_repaint();
}

Widget::Widget(Widget *parent)
{
    _enabled = true;
    _bound = Recti(32, 32);

    if (parent)
    {
        _window = parent->_window;
        parent->add_child(this);
    }
}

Widget::~Widget()
{
    clear_children();

    if (_parent)
    {
        _parent->remove_child(this);
        _parent = nullptr;
    }

    if (_window)
    {
        _window->widget_removed(this);
        _window = nullptr;
    }
}

void Widget::paint(Painter &, Recti) {}

void Widget::event(Event *) {}

void Widget::do_layout()
{
    switch (_layout.type)
    {

    case LAYOUT_STACK:
    {
        _childs.foreach ([this](Widget *child) {
            auto bound = content_bound();

            if (child->max_width() > 0 && bound.width() > child->max_width())
            {
                bound = bound.with_width(child->max_width());
            }

            if (child->max_height() > 0 && bound.height() > child->max_height())
            {
                bound = bound.with_height(child->max_height());
            }

            child->bound(bound.centered_within(content_bound()));

            return Iteration::CONTINUE;
        });
    }
    break;

    case LAYOUT_GRID:
    {
        int index = 0;

        _childs.foreach ([&](auto child) {
            int x = index % _layout.hcell;
            int y = index / _layout.hcell;

            Recti row = content_bound().row(_layout.vcell, y, _layout.spacing.y());
            Recti column = row.column(_layout.hcell, x, _layout.spacing.x());

            child->bound(column);
            index++;
            return Iteration::CONTINUE;
        });
    }
    break;

    case LAYOUT_HGRID:
    {
        int index = 0;

        _childs.foreach ([&](auto child) {
            auto bound = content_bound().column(_childs.count(), index, _layout.spacing.x());
            child->bound(bound);
            index++;

            return Iteration::CONTINUE;
        });
    }
    break;

    case LAYOUT_VGRID:
    {
        int index = 0;

        _childs.foreach ([&](auto child) {
            auto bound = content_bound().row(_childs.count(), index, _layout.spacing.y());
            child->bound(bound);
            index++;

            return Iteration::CONTINUE;
        });
    }
    break;

    case LAYOUT_HFLOW:
    {
        int fixed_child_count = 0;
        int fixed_child_total_width = 0;

        int fill_child_count = 0;

        _childs.foreach ([&](auto child) {
            if (child->flags() & Widget::FILL)
            {
                fill_child_count++;
            }
            else
            {
                fixed_child_count++;

                if (child->flags() & Widget::SQUARE)
                {
                    fixed_child_total_width += content_bound().height();
                }
                else
                {
                    fixed_child_total_width += child->compute_size().x();
                }
            }

            return Iteration::CONTINUE;
        });

        int usable_space =
            content_bound().width() -
            _layout.spacing.x() * (_childs.count() - 1);

        int fill_child_total_width = MAX(0, usable_space - fixed_child_total_width);

        int fill_child_width = (fill_child_total_width) / MAX(1, fill_child_count);

        int current = content_bound().x();

        _childs.foreach ([&](auto child) {
            if (child->flags() & Widget::FILL)
            {
                child->bound(Recti(
                    current,
                    content_bound().y(),
                    fill_child_width,
                    content_bound().height()));

                current += fill_child_width + _layout.spacing.x();
            }
            else
            {
                if (child->flags() & Widget::SQUARE)
                {
                    child->bound(Recti(
                        current,
                        content_bound().y(),
                        content_bound().height(),
                        content_bound().height()));

                    current += content_bound().height() + _layout.spacing.x();
                }
                else
                {
                    child->bound(Recti(
                        current,
                        content_bound().y(),
                        child->compute_size().x(),
                        content_bound().height()));

                    current += child->compute_size().x() + _layout.spacing.x();
                }
            }

            return Iteration::CONTINUE;
        });
    }
    break;

    case LAYOUT_VFLOW:
    {
        int fixed_child_count = 0;
        int fixed_child_total_height = 0;

        int fill_child_count = 0;

        _childs.foreach ([&](auto child) {
            if (child->flags() & Widget::FILL)
            {
                fill_child_count++;
            }
            else
            {
                fixed_child_count++;
                fixed_child_total_height += child->compute_size().y();
            }

            return Iteration::CONTINUE;
        });

        int usable_space =
            content_bound().height() -
            _layout.spacing.y() * (_childs.count() - 1);

        int fill_child_total_height = MAX(0, usable_space - fixed_child_total_height);

        int fill_child_height = (fill_child_total_height) / MAX(1, fill_child_count);

        int current = content_bound().y();

        _childs.foreach ([&](auto child) {
            if (child->flags() & Widget::FILL)
            {
                child->bound(Recti(
                    content_bound().x(),
                    current,
                    content_bound().width(),
                    fill_child_height));

                current += fill_child_height + _layout.spacing.y();
            }
            else
            {
                if (child->flags() & Widget::SQUARE)
                {
                    child->bound(Recti(
                        content_bound().x(),
                        current,
                        content_bound().width(),
                        content_bound().width()));

                    current += content_bound().width() + _layout.spacing.y();
                }
                else
                {
                    child->bound(Recti(
                        content_bound().x(),
                        current,
                        content_bound().width(),
                        child->compute_size().y()));

                    current += child->compute_size().y() + _layout.spacing.y();
                }
            }

            return Iteration::CONTINUE;
        });
    }
    break;

    default:
        break;
    }
}

void Widget::relayout()
{
    do_layout();

    if (_childs.empty())
    {
        return;
    }

    _childs.foreach ([&](auto child) {
        child->relayout();
        return Iteration::CONTINUE;
    });
}

void Widget::should_relayout()
{
    if (_window)
    {
        _window->should_relayout();
    }
}

Vec2i Widget::size()
{
    if (_childs.count() == 0)
    {
        return Vec2i(0);
    }

    int width = 0;
    int height = 0;

    if (_layout.type == LAYOUT_STACK)
    {
        _childs.foreach ([&](auto child) {
            Vec2i child_size = child->compute_size();

            width = MAX(width, child_size.x());
            height = MAX(height, child_size.y());

            return Iteration::CONTINUE;
        });
    }
    else
    {
        _childs.foreach ([&](auto child) {
            Vec2i child_size = child->compute_size();

            switch (_layout.type)
            {
            case LAYOUT_HFLOW:
            case LAYOUT_HGRID:
                width += child_size.x();
                height = MAX(height, child_size.y());
                break;

            case LAYOUT_VFLOW:
            case LAYOUT_VGRID:
                width = MAX(width, child_size.x());
                height += child_size.y();
                break;

            default:
                width = MAX(width, child_size.x());
                height = MAX(height, child_size.y());
                break;
            }

            return Iteration::CONTINUE;
        });

        if (_layout.type == LAYOUT_HFLOW || _layout.type == LAYOUT_HGRID)
        {
            width += _layout.spacing.x() * (_childs.count() - 1);
        }

        if (_layout.type == LAYOUT_VFLOW || _layout.type == LAYOUT_VGRID)
        {
            height += _layout.spacing.y() * (_childs.count() - 1);
        }
    }

    return Vec2i(width, height);
}

/* --- Enable/ Disable state ------------------------------------------------ */

bool Widget::enabled() { return _enabled; }

bool Widget::disabled() { return !_enabled; }

void Widget::enable()
{
    if (disabled())
    {
        _enabled = true;
        should_repaint();
    }
}

void Widget::disable()
{
    if (enabled())
    {
        _enabled = false;
        should_repaint();
    }
}

void Widget::disable_if(bool condition)
{
    if (condition)
    {
        disable();
    }
    else
    {
        enable();
    }
}

void Widget::enable_if(bool condition)
{
    if (condition)
    {
        enable();
    }
    else
    {
        disable();
    }
}

/* --- Childs --------------------------------------------------------------- */

Widget *Widget::child_at(Vec2i position)
{
    if (_flags & Widget::GREEDY)
    {
        return this;
    }

    Widget *result = this;

    _childs.foreach ([&](auto child) {
        if (child->bound().contains(position))
        {
            result = child->child_at(position);

            return Iteration::STOP;
        }
        else
        {
            return Iteration::CONTINUE;
        }
    });

    return result;
}

void Widget::add_child(Widget *child)
{
    assert(child);
    assert(child->_parent == nullptr);

    child->_parent = this;
    child->_window = _window;
    _childs.push_back(child);

    should_relayout();
    should_repaint();
}

void Widget::remove_child(Widget *child)
{
    assert(child->_parent == this);
    _childs.remove_value(child);
    should_relayout();
}

void Widget::clear_children()
{
    while (_childs.any())
    {
        delete _childs.peek();
    }
}

/* --- Focus state ---------------------------------------------------------- */

bool Widget::focused()
{
    return _window && _window->focused_widget == this;
}

void Widget::focus()
{
    if (_window)
        _window->focus_widget(this);
}

/* --- Paint ---------------------------------------------------------------- */

void Widget::repaint(Painter &painter, Recti rectangle)
{
    if (bound().width() == 0 || bound().height() == 0)
    {
        return;
    }

    painter.push();
    painter.clip(bound());

    if (Application::show_wireframe())
    {
        painter.fill_insets(bound(), _insets, Colors::MAGENTA.with_alpha(0.25));
    }

    painter.push();
    paint(painter, rectangle);
    painter.pop();

    _childs.foreach ([&](auto child) {
        if (rectangle.colide_with(child->bound()))
        {
            child->repaint(painter, rectangle);
        }

        return Iteration::CONTINUE;
    });

    if (Application::show_wireframe())
    {
        painter.draw_rectangle(bound(), Colors::CYAN.with_alpha(0.25));
    }

    painter.pop();
}

void Widget::should_repaint()
{
    if (_window)
    {
        _window->should_repaint(bound());
    }
}

void Widget::should_repaint(Recti rectangle)
{
    if (_window)
    {
        _window->should_repaint(rectangle);
    }
}

/* --- Events ----------------------------------------------------------------*/

void Widget::on(EventType event_type, EventHandler handler)
{
    assert(event_type < EventType::__COUNT);
    _handlers[event_type] = move(handler);
}

void Widget::dispatch_event(Event *event)
{
    this->event(event);

    if (!event->accepted && _handlers[event->type])
    {
        event->accepted = true;
        _handlers[event->type](event);
    }

    if (!event->accepted && _parent)
    {
        _parent->dispatch_event(event);
    }
}

Vec2i Widget::compute_size()
{
    Vec2i size = this->size();

    int width = size.x();
    int height = size.y();

    width += _insets.left() + _insets.right();
    height += _insets.top() + _insets.bottom();

    if (_max_width)
    {
        width = MIN(width, _max_width);
    }

    if (_max_height)
    {
        height = MIN(height, _max_height);
    }

    if (_min_width)
    {
        width = MAX(width, _min_width);
    }

    if (_min_height)
    {
        height = MAX(height, _min_height);
    }

    return Vec2i(width, height);
}
