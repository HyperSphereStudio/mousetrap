//
// Copyright 2022 Clemens Cords
// Created on 9/18/22 by clem (mail@clemens-cords.com)
//

#include <mousetrap/expander.hpp>
#include <mousetrap/log.hpp>

namespace mousetrap
{
    Expander::Expander()
        : Widget(gtk_expander_new(nullptr)),
          CTOR_SIGNAL(Expander, activate)
    {
        _internal = GTK_EXPANDER(Widget::operator NativeWidget());
    }
    
    Expander::Expander(detail::ExpanderInternal* internal) 
        : Widget(GTK_WIDGET(internal)),
          CTOR_SIGNAL(Expander, activate)
    {
        _internal = g_object_ref(internal);
    }
    
    Expander::~Expander() 
    {
        g_object_unref(_internal);
    }

    NativeObject Expander::get_internal() const 
    {
        return G_OBJECT(_internal);
    }

    void Expander::set_child(const Widget& widget)
    {
        auto* ptr = &widget;
        WARN_IF_SELF_INSERTION(Expander::set_child, this, ptr);

        gtk_expander_set_child(GTK_EXPANDER(operator NativeWidget()), widget.operator NativeWidget());
    }

    void Expander::remove_child()
    {
        gtk_expander_set_child(GTK_EXPANDER(operator NativeWidget()), nullptr);
    }

    bool Expander::get_expanded()
    {
        return gtk_expander_get_expanded(GTK_EXPANDER(operator NativeWidget()));
    }

    void Expander::set_expanded(bool b)
    {
        gtk_expander_set_expanded(GTK_EXPANDER(operator NativeWidget()), b);
    }

    void Expander::set_label_widget(const Widget& widget)
    {
        auto* ptr = &widget;
        WARN_IF_SELF_INSERTION(Expander::set_label_widget, this, ptr);

        gtk_expander_set_label_widget(GTK_EXPANDER(operator NativeWidget()), widget.operator NativeWidget());
    }
}