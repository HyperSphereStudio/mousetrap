// 
// Copyright 2022 Clemens Cords
// Created on 8/1/22 by clem (mail@clemens-cords.com)
//

#include <mousetrap/gl_common.hpp>
#if MOUSETRAP_ENABLE_OPENGL_COMPONENT

#include <mousetrap/render_area.hpp>
#include <mousetrap/render_task.hpp>
#include <mousetrap/msaa_render_texture.hpp>
#include <mousetrap/shape.hpp>

namespace mousetrap
{
    namespace detail
    {
        GdkGLContext* initialize_opengl()
        {
            if (not GDK_IS_GL_CONTEXT(detail::GL_CONTEXT))
            {
                bool failed = false;
                GError *error_maybe = nullptr;
                detail::GL_CONTEXT = gdk_display_create_gl_context(gdk_display_get_default(), &error_maybe);
                auto* context = detail::GL_CONTEXT;

                if (error_maybe != nullptr)
                {
                    failed = true;
                    log::critical("In gdk_window_create_gl_context: " +  std::string(error_maybe->message));
                    g_error_free(error_maybe);
                }

                gdk_gl_context_set_required_version(context, 3, 3);
                gdk_gl_context_realize(context, &error_maybe);

                if (error_maybe != nullptr)
                {
                    failed = true;
                    log::critical("In gdk_gl_context_realize: " +  std::string(error_maybe->message));
                    g_error_free(error_maybe);
                }

                gdk_gl_context_make_current(context);

                glewExperimental = GL_FALSE;
                GLenum glewError = glewInit();
                if (glewError != GLEW_NO_ERROR)
                {
                    std::stringstream str;
                    str << "In glewInit: Unable to initialize glew " << "(" << glewError << ") ";

                    if (glewError == GLEW_ERROR_NO_GL_VERSION)
                        str << "Missing GL version";
                    else if (glewError == GLEW_ERROR_GL_VERSION_10_ONLY)
                        str << "Need at least OpenGL 1.1";
                    else if (glewError == GLEW_ERROR_GLX_VERSION_11_ONLY)
                        str << "Need at least GLX 1.2";
                    else if (glewError == GLEW_ERROR_NO_GLX_DISPLAY)
                        str << "Need GLX Display for GLX support";

                    log::warning(str.str());
                }

                if (g_object_is_floating(GL_CONTEXT))
                    g_object_ref_sink(GL_CONTEXT);

                g_object_ref(GL_CONTEXT);
                detail::mark_gl_initialized();
            }

            return GL_CONTEXT; // intentional memory leak, should persist until end of runtime
        }

        void shutdown_opengl()
        {
            while (GDK_IS_GL_CONTEXT(GL_CONTEXT))
                g_object_unref(GL_CONTEXT);

            GL_CONTEXT = nullptr;
            GL_INITIALIZED = false;
        }
    }

    namespace detail
    {
        DECLARE_NEW_TYPE(RenderAreaInternal, render_area_internal, RENDER_AREA_INTERNAL)

        static void render_area_internal_finalize(GObject* object)
        {
            auto* self = MOUSETRAP_RENDER_AREA_INTERNAL(object);
            G_OBJECT_CLASS(render_area_internal_parent_class)->finalize(object);

            for (auto* task : *self->tasks)
                g_object_unref(task);

            delete self->tasks;
            delete self->render_texture;
            delete self->render_texture_shape;
            delete self->render_texture_shape_task;
        }

        DEFINE_NEW_TYPE_TRIVIAL_INIT(RenderAreaInternal, render_area_internal, RENDER_AREA_INTERNAL)
        DEFINE_NEW_TYPE_TRIVIAL_CLASS_INIT(RenderAreaInternal, render_area_internal, RENDER_AREA_INTERNAL)

        static RenderAreaInternal* render_area_internal_new(GtkGLArea* area, int32_t msaa_samples)
        {
            auto* self = (RenderAreaInternal*) g_object_new(render_area_internal_get_type(), nullptr);
            render_area_internal_init(self);

            self->native = area;
            self->tasks = new std::vector<detail::RenderTaskInternal*>();
            self->apply_msaa = msaa_samples > 0;

            if (self->apply_msaa)
            {
                self->render_texture = new MultisampledRenderTexture(msaa_samples);

                self->render_texture_shape = new Shape();
                self->render_texture_shape->as_rectangle({-1, 1}, {2, 2});
                self->render_texture_shape->set_texture(self->render_texture);

                static const std::string RENDER_TEXTURE_SHADER_SOURCE = R"(
                    #version 130

                    in vec4 _vertex_color;
                    in vec2 _texture_coordinates;
                    in vec3 _vertex_position;

                    out vec4 _fragment_color;

                    uniform int _texture_set;
                    uniform sampler2D _texture;

                    void main()
                    {
                        // flip horizontally to correct render texture inversion
                        _fragment_color = texture2D(_texture, vec2(_texture_coordinates.x, 1 - _texture_coordinates.y)) * _vertex_color;
                    }
                )";

                self->render_texture_shader = new Shader();
                self->render_texture_shader->create_from_string(ShaderType::FRAGMENT, RENDER_TEXTURE_SHADER_SOURCE);

                self->render_texture_shape_task = new RenderTask(*self->render_texture_shape, self->render_texture_shader);
            }
            else
            {
                self->render_texture = nullptr;
                self->render_texture_shape = nullptr;
                self->render_texture_shape_task = nullptr;
            }

            return self;
        }
    }

    RenderArea::RenderArea(AntiAliasingQuality msaa_samples)
        : Widget(gtk_gl_area_new()),
          CTOR_SIGNAL(RenderArea, render),
          CTOR_SIGNAL(RenderArea, resize),
          CTOR_SIGNAL(RenderArea, realize),
          CTOR_SIGNAL(RenderArea, unrealize),
          CTOR_SIGNAL(RenderArea, destroy),
          CTOR_SIGNAL(RenderArea, hide),
          CTOR_SIGNAL(RenderArea, show),
          CTOR_SIGNAL(RenderArea, map),
          CTOR_SIGNAL(RenderArea, unmap)
    {
        _internal = detail::render_area_internal_new(GTK_GL_AREA(operator NativeWidget()), (int) msaa_samples);
        detail::attach_ref_to(G_OBJECT(_internal->native), _internal);

        gtk_gl_area_set_auto_render(GTK_GL_AREA(operator NativeWidget()), TRUE);
        gtk_widget_set_size_request(GTK_WIDGET(GTK_GL_AREA(operator NativeWidget())), 1, 1);

        g_signal_connect(_internal->native, "realize", G_CALLBACK(on_realize), _internal);
        g_signal_connect(_internal->native, "resize", G_CALLBACK(on_resize), _internal);
        g_signal_connect(_internal->native, "render", G_CALLBACK(on_render), _internal);
        g_signal_connect(_internal->native, "create-context", G_CALLBACK(on_create_context), _internal);
    }

    RenderArea::~RenderArea()
    {}

    RenderArea::RenderArea(detail::RenderAreaInternal* internal)
        : Widget(GTK_WIDGET(internal->native)),
          CTOR_SIGNAL(RenderArea, render),
          CTOR_SIGNAL(RenderArea, resize),
          CTOR_SIGNAL(RenderArea, realize),
          CTOR_SIGNAL(RenderArea, unrealize),
          CTOR_SIGNAL(RenderArea, destroy),
          CTOR_SIGNAL(RenderArea, hide),
          CTOR_SIGNAL(RenderArea, show),
          CTOR_SIGNAL(RenderArea, map),
          CTOR_SIGNAL(RenderArea, unmap)
    {
        _internal = g_object_ref(internal);

        gtk_gl_area_set_auto_render(GTK_GL_AREA(operator NativeWidget()), TRUE);
        gtk_widget_set_size_request(GTK_WIDGET(GTK_GL_AREA(operator NativeWidget())), 1, 1);

        g_signal_connect(_internal->native, "realize", G_CALLBACK(on_realize), _internal);
        g_signal_connect(_internal->native, "resize", G_CALLBACK(on_resize), _internal);
        g_signal_connect(_internal->native, "render", G_CALLBACK(on_render), _internal);
        g_signal_connect(_internal->native, "create-context", G_CALLBACK(on_create_context), _internal);
    }

    void RenderArea::add_render_task(RenderTask task)
    {
        auto* task_internal = (detail::RenderTaskInternal*) task.operator GObject*();
        _internal->tasks->push_back(task_internal);
        g_object_ref(task_internal);
    }

    void RenderArea::clear_render_tasks()
    {
        for (auto& task : *_internal->tasks)
            g_object_unref(task);

        _internal->tasks->clear();
    }

    void RenderArea::flush()
    {
        glFlush();
    }

    void RenderArea::clear()
    {
        glClear(GL_COLOR_BUFFER_BIT);
        glClearColor(0, 0, 0, 0);
    }

    GdkGLContext* RenderArea::on_create_context(GtkGLArea* area, GdkGLContext* context, detail::RenderAreaInternal* internal)
    {
        detail::initialize_opengl();
        g_object_ref(detail::GL_CONTEXT);
        gdk_gl_context_make_current(detail::GL_CONTEXT);
        return detail::GL_CONTEXT;
    }

    void RenderArea::on_realize(GtkWidget* area, detail::RenderAreaInternal* internal)
    {
        gtk_gl_area_queue_render(GTK_GL_AREA(area));
    }

    void RenderArea::on_resize(GtkGLArea* area, gint width, gint height, detail::RenderAreaInternal* internal)
    {
        assert(GDK_IS_GL_CONTEXT(detail::GL_CONTEXT));

        if (internal->apply_msaa)
            internal->render_texture->create(width, height);

        gtk_gl_area_make_current(area);
        gtk_gl_area_queue_render(area);
    }

    gboolean RenderArea::on_render(GtkGLArea* area, GdkGLContext* context, detail::RenderAreaInternal* internal)
    {
        assert(GDK_IS_GL_CONTEXT(detail::GL_CONTEXT));
        gtk_gl_area_make_current(area);

        if (internal->apply_msaa)
        {
            internal->render_texture->bind_as_render_target();

            RenderArea::clear();
            glEnable(GL_BLEND);
            set_current_blend_mode(BlendMode::NORMAL);

            for (auto* internal:*(internal->tasks))
                RenderTask(internal).render();

            RenderArea::flush();

            internal->render_texture->unbind_as_render_target();

            RenderArea::clear();
            glEnable(GL_BLEND);
            set_current_blend_mode(BlendMode::NORMAL);

            internal->render_texture_shape_task->render();
            RenderArea::flush();
        }
        else
        {
            RenderArea::clear();

            glEnable(GL_BLEND);
            set_current_blend_mode(BlendMode::NORMAL);

            for (auto* internal:*(internal->tasks))
                RenderTask(internal).render();

            RenderArea::flush();
        }

        return TRUE;
    }

    void RenderArea::render_render_tasks()
    {
        for (auto* internal : *(_internal->tasks))
        {
            auto task = RenderTask(internal);
            task.render();
        }
    }

    void RenderArea::queue_render()
    {
        gtk_gl_area_queue_render(GTK_GL_AREA(operator NativeWidget()));
        gtk_widget_queue_draw(GTK_WIDGET(GTK_GL_AREA(operator NativeWidget())));
    }

    void RenderArea::make_current()
    {
        gtk_gl_area_make_current(GTK_GL_AREA(operator NativeWidget()));
    }

    Vector2f RenderArea::from_gl_coordinates(Vector2f in)
    {
        auto out = in;
        out /= 2;
        out += 0.5;
        out.y = 1 - out.y;

        auto size = this->get_allocated_size();
        return {out.x * size.x, out.y * size.y};
    }

    Vector2f RenderArea::to_gl_coordinates(Vector2f in)
    {
        auto out = in;

        auto size = this->get_allocated_size();
        out.x /= size.x;
        out.y /= size.y;

        out.y = 1 - out.y;
        out -= 0.5;
        out *= 2;

        return out;
    }

    GObject* RenderArea::get_internal() const
    {
        return G_OBJECT(_internal);
    }
}

#endif // MOUSETRAP_ENABLE_OPENGL_COMPONENT