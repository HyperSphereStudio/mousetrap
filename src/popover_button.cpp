// 
// Copyright 2022 Clemens Cords
// Created on 9/18/22 by clem (mail@clemens-cords.com)
//

#include <mousetrap/popover_button.hpp>
#include <mousetrap/log.hpp>

#include <iostream>

namespace mousetrap
{
    namespace detail
    {
        DECLARE_NEW_TYPE(PopoverButtonInternal, popover_button_internal, POPOVER_BUTTON_INTERNAL)
        DEFINE_NEW_TYPE_TRIVIAL_INIT(PopoverButtonInternal, popover_button_internal, POPOVER_BUTTON_INTERNAL)

        static void popover_button_internal_finalize(GObject* object)
        {
            auto* self = MOUSETRAP_POPOVER_BUTTON_INTERNAL(object);
            G_OBJECT_CLASS(popover_button_internal_parent_class)->finalize(object);

            if (self->menu != nullptr)
                g_object_unref(self->menu);

            if (self->popover != nullptr)
                g_object_unref(self->popover);
        }

        DEFINE_NEW_TYPE_TRIVIAL_CLASS_INIT(PopoverButtonInternal, popover_button_internal, POPOVER_BUTTON_INTERNAL)

        static PopoverButtonInternal* popover_button_internal_new(GtkMenuButton* native)
        {
            auto* self = (PopoverButtonInternal*) g_object_new(popover_button_internal_get_type(), nullptr);
            popover_button_internal_init(self);

            self->native = native;
            self->menu = nullptr;

            return self;
        }
    }
    
    PopoverButton::PopoverButton()
        : Widget(gtk_menu_button_new()),
          CTOR_SIGNAL(PopoverButton, activate)
    {
        gtk_menu_button_set_always_show_arrow(GTK_MENU_BUTTON(operator NativeWidget()), true);
    }

    PopoverButton::PopoverButton(detail::PopoverButtonInternal* internal) 
        : Widget(GTK_WIDGET(internal)),
          CTOR_SIGNAL(PopoverButton, activate)
    {
        _internal = g_object_ref(internal);
    }
    
    PopoverButton::~PopoverButton() 
    {
        g_object_unref(_internal);
    }

    NativeObject PopoverButton::get_internal() const 
    {
        return G_OBJECT(_internal);
    }
    
    void PopoverButton::set_child(const Widget& child)
    {
        auto* ptr = &child;
        WARN_IF_SELF_INSERTION(PopoverButton::set_child, this, ptr);

        gtk_menu_button_set_child(GTK_MENU_BUTTON(operator NativeWidget()), child.operator GtkWidget*());

        PopoverMenu(_internal->menu).refresh_widgets();
    }

    void PopoverButton::remove_child()
    {
        gtk_menu_button_set_child(GTK_MENU_BUTTON(operator NativeWidget()), nullptr);
    }

    void PopoverButton::set_popover_position(RelativePosition type)
    {
        gtk_popover_set_position(gtk_menu_button_get_popover(GTK_MENU_BUTTON(operator NativeWidget())), (GtkPositionType) type);
    }

    RelativePosition PopoverButton::get_popover_position() const
    {
        return (RelativePosition) gtk_popover_get_position(gtk_menu_button_get_popover(GTK_MENU_BUTTON(operator NativeWidget())));
    }

    void PopoverButton::set_popover(Popover& popover)
    {
        _internal->menu = nullptr;
        _internal->popover = (detail::PopoverInternal*) popover.get_internal();
        gtk_menu_button_set_popover(GTK_MENU_BUTTON(operator NativeWidget()), popover.operator GtkWidget*());
    }

    void PopoverButton::set_popover_menu(PopoverMenu& popover_menu)
    {
        _internal->menu = (detail::PopoverMenuInternal*) popover_menu.get_internal();;
        _internal->popover = nullptr;

        gtk_menu_button_set_popover(GTK_MENU_BUTTON(operator NativeWidget()), popover_menu.operator GtkWidget*());
        PopoverMenu(_internal->menu).refresh_widgets();
    }

    void PopoverButton::remove_popover()
    {
        _internal->menu = nullptr;
        _internal->popover = nullptr;
        gtk_menu_button_set_popover(GTK_MENU_BUTTON(operator NativeWidget()), nullptr);
    }

    void PopoverButton::popup()
    {
        gtk_menu_button_popup(GTK_MENU_BUTTON(operator NativeWidget()));
    }

    void PopoverButton::popdown()
    {
        gtk_menu_button_popdown(GTK_MENU_BUTTON(operator NativeWidget()));
    }

    void PopoverButton::set_always_show_arrow(bool b)
    {
        gtk_menu_button_set_always_show_arrow(GTK_MENU_BUTTON(operator NativeWidget()), b);
    }

    void PopoverButton::set_has_frame(bool b)
    {
        gtk_menu_button_set_has_frame(GTK_MENU_BUTTON(operator NativeWidget()), b);
    }

    void PopoverButton::set_is_circular(bool b)
    {
        if (b and not get_is_circular())
            gtk_widget_add_css_class(GTK_WIDGET(GTK_MENU_BUTTON(operator NativeWidget())), "circular");
        else if (not b and get_is_circular())
            gtk_widget_remove_css_class(GTK_WIDGET(GTK_MENU_BUTTON(operator NativeWidget())), "circular");
    }

    bool PopoverButton::get_is_circular() const
    {
        return gtk_widget_has_css_class(GTK_WIDGET(GTK_MENU_BUTTON(operator NativeWidget())), "circular");
    }
}