//
// Created by clem on 4/10/23.
//

#include <mousetrap/column_view.hpp>

namespace mousetrap
{
    namespace detail
    {
        DECLARE_NEW_TYPE(ColumnViewInternal, column_view_internal, COLUMN_VIEW_INTERNAL)
        DEFINE_NEW_TYPE_TRIVIAL_INIT(ColumnViewInternal, column_view_internal, COLUMN_VIEW_INTERNAL)

        static void column_view_internal_finalize(GObject* object)
        {
            auto* self = MOUSETRAP_COLUMN_VIEW_INTERNAL(object);
            G_OBJECT_CLASS(column_view_internal_parent_class)->finalize(object);
            delete self->selection_model;
            g_object_unref(self->list_store);
        }

        DEFINE_NEW_TYPE_TRIVIAL_CLASS_INIT(ColumnViewInternal, column_view_internal, COLUMN_VIEW_INTERNAL)

        static ColumnViewInternal* column_view_internal_new(GtkColumnView* native, SelectionMode mode)
        {
            auto* self = (ColumnViewInternal*) g_object_new(column_view_internal_get_type(), nullptr);
            column_view_internal_init(self);

            self->native = native;
            self->list_store = g_list_store_new(G_TYPE_OBJECT);

            auto gtk_mode = (GtkSelectionMode) mode;
            self->selection_mode = gtk_mode;
            self->selection_model = new SelectionModel(mode, G_LIST_MODEL(self->list_store));

            gtk_column_view_set_model(self->native, self->selection_model->operator GtkSelectionModel*());
            return self;
        }
    }

    namespace detail
    {
        struct _ColumnViewRowItem
        {
            GObject parent;
            std::map<GtkColumnViewColumn*, GtkWidget*>* widgets;
        };

        DECLARE_NEW_TYPE(ColumnViewRowItem, column_view_row_item, COLUMN_VIEW_ROW_ITEM)
        DEFINE_NEW_TYPE_TRIVIAL_INIT(ColumnViewRowItem, column_view_row_item, COLUMN_VIEW_ROW_ITEM)

        static void column_view_row_item_finalize(GObject* object)
        {
            auto* self = MOUSETRAP_COLUMN_VIEW_ROW_ITEM(object);

            for (auto& pair : *(self->widgets))
                g_object_unref(pair.second);

            delete self->widgets;
            G_OBJECT_CLASS(column_view_row_item_parent_class)->finalize(object);
        }

        DEFINE_NEW_TYPE_TRIVIAL_CLASS_INIT(ColumnViewRowItem, column_view_row_item, COLUMN_VIEW_ROW_ITEM)

        static ColumnViewRowItem* column_view_row_item_new()
        {
            auto* self = MOUSETRAP_COLUMN_VIEW_ROW_ITEM(g_object_new(column_view_row_item_get_type(), nullptr));
            column_view_row_item_init(self);
            self->widgets = new std::map<GtkColumnViewColumn*, GtkWidget*>();
            return self;
        }
    }

    ///

    void ColumnView::on_list_item_factory_bind(GtkSignalListItemFactory* self, void* object, GtkColumnViewColumn* column)
    {
        auto* list_item = GTK_LIST_ITEM(object);
        auto* row_item = detail::MOUSETRAP_COLUMN_VIEW_ROW_ITEM(gtk_list_item_get_item(list_item));
        auto it = row_item->widgets->find(column);

        if (it == row_item->widgets->end())
            gtk_list_item_set_child(list_item, nullptr);
        else
            gtk_list_item_set_child(list_item, it->second);
    }

    void ColumnView::on_list_item_factory_unbind(GtkSignalListItemFactory* self, void* object, GtkColumnViewColumn* column)
    {
        auto* list_item = GTK_LIST_ITEM(object);
        gtk_list_item_set_child(list_item, nullptr);
    }

    ColumnView::Column::Column(GtkColumnViewColumn* native)
        : _native(native)
    {}

    NativeObject ColumnView::Column::get_internal() const
    {
        return G_OBJECT(_native);
    }

    ColumnView::Column::operator NativeObject() const
    {
        return this->get_internal();
    }

    void ColumnView::Column::set_title(const std::string& title)
    {
        gtk_column_view_column_set_title(_native, title.c_str());
    }

    std::string ColumnView::Column::get_title() const
    {
        return gtk_column_view_column_get_title(_native);
    }

    void ColumnView::Column::set_expand(bool expand)
    {
        gtk_column_view_column_set_expand(_native, expand);
    }

    bool ColumnView::Column::get_expand() const
    {
        return gtk_column_view_column_get_expand(_native);
    }

    void ColumnView::Column::set_fixed_width(float width)
    {
        gtk_column_view_column_set_fixed_width(_native, width);
    }

    float ColumnView::Column::get_fixed_width() const
    {
        return gtk_column_view_column_get_fixed_width(_native);
    }

    void ColumnView::Column::set_header_menu(const MenuModel& model)
    {
        gtk_column_view_column_set_header_menu(_native, model.operator GMenuModel*());
    }

    void ColumnView::Column::set_is_visible(bool b)
    {
        gtk_column_view_column_set_visible(_native, b);
    }

    bool ColumnView::Column::get_is_visible() const
    {
        return gtk_column_view_column_get_visible(_native);
    }

    void ColumnView::Column::set_is_resizable(bool b)
    {
        gtk_column_view_column_set_resizable(_native, b);
    }

    bool ColumnView::Column::get_is_resizable() const
    {
        return gtk_column_view_column_get_resizable(_native);
    }

    ///

    ColumnView::ColumnView(SelectionMode mode)
        : Widget(gtk_column_view_new(nullptr)),
          CTOR_SIGNAL(ColumnView, activate),
          CTOR_SIGNAL(ColumnView, realize),
          CTOR_SIGNAL(ColumnView, unrealize),
          CTOR_SIGNAL(ColumnView, destroy),
          CTOR_SIGNAL(ColumnView, hide),
          CTOR_SIGNAL(ColumnView, show),
          CTOR_SIGNAL(ColumnView, map),
          CTOR_SIGNAL(ColumnView, unmap)
    {
        _internal = detail::column_view_internal_new(GTK_COLUMN_VIEW(Widget::operator NativeWidget()), mode);
        detail::attach_ref_to(G_OBJECT(GTK_COLUMN_VIEW(Widget::operator NativeWidget())), _internal);
        g_object_ref(_internal);
    }

    ColumnView::ColumnView(detail::ColumnViewInternal* internal)
        : Widget(GTK_WIDGET(internal->native)),
          CTOR_SIGNAL(ColumnView, activate),
          CTOR_SIGNAL(ColumnView, realize),
          CTOR_SIGNAL(ColumnView, unrealize),
          CTOR_SIGNAL(ColumnView, destroy),
          CTOR_SIGNAL(ColumnView, hide),
          CTOR_SIGNAL(ColumnView, show),
          CTOR_SIGNAL(ColumnView, map),
          CTOR_SIGNAL(ColumnView, unmap)
    {
        _internal = g_object_ref(internal);
    }

    ColumnView::~ColumnView()
    {
        g_object_unref(_internal);
    }

    NativeObject ColumnView::get_internal() const
    {
        return G_OBJECT(_internal);
    }

    GtkColumnViewColumn* ColumnView::new_column(const std::string& title)
    {
        auto* factory = gtk_signal_list_item_factory_new();
        auto* column = gtk_column_view_column_new(title.c_str(), factory);
        g_signal_connect(factory, "bind", G_CALLBACK(on_list_item_factory_bind), column);
        g_signal_connect(factory, "unbind", G_CALLBACK(on_list_item_factory_unbind), column);
        detail::attach_ref_to(G_OBJECT(column), factory);
        return column;
    }

    ColumnView::Column ColumnView::push_back_column(const std::string& title)
    {
        auto* column = new_column(title);
        gtk_column_view_append_column(GTK_COLUMN_VIEW(operator NativeWidget()), column);
        return Column(column);
    }

    ColumnView::Column ColumnView::push_front_column(const std::string& title)
    {
        auto* column = new_column(title);
        gtk_column_view_insert_column(GTK_COLUMN_VIEW(operator NativeWidget()), 0, column);
        return Column(column);
    }

    ColumnView::Column ColumnView::insert_column(uint64_t i, const std::string& title)
    {
        auto* column = new_column(title);
        gtk_column_view_insert_column(GTK_COLUMN_VIEW(operator NativeWidget()), i, column);
        return Column(column);
    }

    void ColumnView::remove_column(const Column& column)
    {
        gtk_column_view_remove_column(GTK_COLUMN_VIEW(operator NativeWidget()), column._native);
    }

    ColumnView::Column ColumnView::get_column_at(uint64_t column_i)
    {
        auto* model = gtk_column_view_get_columns(GTK_COLUMN_VIEW(operator NativeWidget()));
        if (column_i > g_list_model_get_n_items(model))
            log::critical("In ColumnView::get_column_at: Index " + std::to_string(column_i) + " out of bounds for a ColumnView with " + std::to_string(g_list_model_get_n_items(model)) + " columns");

        return Column(GTK_COLUMN_VIEW_COLUMN(g_list_model_get_item(model, column_i)));
    }

    ColumnView::Column ColumnView::get_column_with_title(const std::string& title)
    {
        auto* model = gtk_column_view_get_columns(GTK_COLUMN_VIEW(operator NativeWidget()));
        for (uint64_t i = 0; i < g_list_model_get_n_items(model); ++i)
        {
            auto* out = GTK_COLUMN_VIEW_COLUMN(g_list_model_get_item(model, i));
            if (gtk_column_view_column_get_title(out) == title)
                return Column(out);
        }

        log::critical("In ColumnView::get_column_with_title: No column with title `" + title + "`");
        return Column(nullptr);
    }

    bool ColumnView::has_column_with_title(const std::string& title)
    {
        auto* model = gtk_column_view_get_columns(GTK_COLUMN_VIEW(operator NativeWidget()));
        for (uint64_t i = 0; i < g_list_model_get_n_items(model); ++i)
        {
            if (gtk_column_view_column_get_title(GTK_COLUMN_VIEW_COLUMN(g_list_model_get_item(model, i))) == title)
                return true;
        }

        return false;
    }
	
	void ColumnView::remove_row(int32_t row){
		g_list_store_remove(_internal->list_store, row);
	}

    uint64_t ColumnView::get_n_columns() const
    {
        return g_list_model_get_n_items(gtk_column_view_get_columns(GTK_COLUMN_VIEW(operator NativeWidget())));
    }

    void ColumnView::set_widget_at(const Column& column, uint64_t row_i, const Widget& widget)
    {
        if (column._native == nullptr)
        {
            log::critical("In ColumnView::set_widget: Column does not exist, no insertion will take place", MOUSETRAP_DOMAIN);
            return;
        }

        auto* model = _internal->list_store;
        while (g_list_model_get_n_items(G_LIST_MODEL(model)) <= row_i)
            g_list_store_append(model, detail::column_view_row_item_new());

        auto* item = detail::MOUSETRAP_COLUMN_VIEW_ROW_ITEM(g_list_model_get_item(G_LIST_MODEL(model), row_i));

        auto it = item->widgets->find(column._native);

        if (it != item->widgets->end())
            g_object_unref(it->second);

        item->widgets->insert_or_assign(column._native, g_object_ref(widget.operator NativeWidget()));
    }

    void ColumnView::set_enable_rubberband_selection(bool b)
    {
        gtk_column_view_set_enable_rubberband(GTK_COLUMN_VIEW(operator NativeWidget()), b);
    }

    bool ColumnView::get_enable_rubberband_selection() const
    {
        return gtk_column_view_get_enable_rubberband(GTK_COLUMN_VIEW(operator NativeWidget()));
    }

    void ColumnView::set_show_row_separators(bool b)
    {
        gtk_column_view_set_show_row_separators(GTK_COLUMN_VIEW(operator NativeWidget()), true);
    }

    bool ColumnView::get_show_row_separators() const
    {
        return gtk_column_view_get_show_row_separators(GTK_COLUMN_VIEW(operator NativeWidget()));
    }

    void ColumnView::set_show_column_separators(bool b)
    {
        gtk_column_view_set_show_column_separators(GTK_COLUMN_VIEW(operator NativeWidget()), b);
    }

    bool ColumnView::get_show_column_separators() const
    {
        return gtk_column_view_get_show_column_separators(GTK_COLUMN_VIEW(operator NativeWidget()));
    }

    SelectionModel* ColumnView::get_selection_model()
    {
        return _internal->selection_model;
    }

    uint64_t ColumnView::get_n_rows() const
    {
        return g_list_model_get_n_items(G_LIST_MODEL(_internal->list_store));
    }

    void ColumnView::set_is_reorderable(bool b)
    {
        gtk_column_view_set_reorderable(GTK_COLUMN_VIEW(operator NativeWidget()), b);
    }

    bool ColumnView::get_is_reorderable() const
    {
        return gtk_column_view_get_reorderable(GTK_COLUMN_VIEW(operator NativeWidget()));
    }

    bool ColumnView::get_single_click_activate() const
    {
        return gtk_column_view_get_single_click_activate(GTK_COLUMN_VIEW(operator NativeWidget()));
    }

    void ColumnView::set_single_click_activate(bool b)
    {
        gtk_column_view_set_single_click_activate(GTK_COLUMN_VIEW(operator NativeWidget()), b);
    }
}