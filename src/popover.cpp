//
// Copyright 2022 Clemens Cords
// Created on 9/3/22 by clem (mail@clemens-cords.com)
//

#include <mousetrap/popover.hpp>
#include <mousetrap/log.hpp>

namespace mousetrap
{
    Popover::Popover()
    : WidgetImplementation<GtkPopover>(GTK_POPOVER(gtk_popover_new())),
      CTOR_SIGNAL(Popover, closed)
    {}

    void Popover::popup()
    {
        gtk_popover_popup(get_native());
    }

    void Popover::popdown()
    {
        gtk_popover_popdown(get_native());
    }

    void Popover::present()
    {
        gtk_popover_present(get_native());
    }

    void Popover::set_child(const Widget& child)
    {
        _child = &child;
        WARN_IF_SELF_INSERTION(Popover::set_child, this, _child);

        gtk_popover_set_child(get_native(), _child->operator GtkWidget*());
    }

    void Popover::remove_child()
    {
        _child = nullptr;
        gtk_popover_set_child(get_native(), nullptr);
    }

    Widget* Popover::get_child() const
    {
        return const_cast<Widget*>(_child);
    }

    void Popover::set_relative_position(RelativePosition position)
    {
        gtk_popover_set_position(get_native(), (GtkPositionType) position);
    }

    RelativePosition Popover::get_relative_position() const
    {
        return (RelativePosition) gtk_popover_get_position(get_native());
    }

    void Popover::set_has_base_arrow(bool b)
    {
        gtk_popover_set_has_arrow(get_native(), b);
    }

    bool Popover::get_has_base_arrow() const
    {
        return gtk_popover_get_has_arrow(get_native());
    }

    void Popover::set_autohide(bool b)
    {
        gtk_popover_set_autohide(get_native(), b);
    }

    void Popover::attach_to(Widget* widget)
    {
        gtk_widget_set_parent(GTK_WIDGET(get_native()), widget->operator GtkWidget*());
    }
}