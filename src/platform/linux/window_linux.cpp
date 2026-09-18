#if defined(__linux__)
#include "types.hpp"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/extensions/shape.h>
#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>
#include <iostream>
#include <algorithm>
#include <cmath>

class LinuxWindowManager {
public:
    LinuxWindowManager()
        : m_display(nullptr), m_window(0), m_width(1920), m_height(106),
          m_cairo(nullptr), m_surface(nullptr) {}

    bool Create(int width, int height) {
        m_width = width;
        m_height = height;

        m_display = XOpenDisplay(NULL);
        if (!m_display) return false;

        int screen = DefaultScreen(m_display);
        XVisualInfo vinfo;
        if (!XMatchVisualInfo(m_display, screen, 32, TrueColor, &vinfo)) {
            std::cerr << "[AetherWave Linux] 32-bit ARGB visual not supported!" << std::endl;
            return false;
        }

        XSetWindowAttributes attrs;
        attrs.colormap = XCreateColormap(m_display, RootWindow(m_display, screen), vinfo.visual, AllocNone);
        attrs.background_pixel = 0;
        attrs.border_pixel = 0;
        attrs.override_redirect = True; // Borderless overlay

        // Query _NET_WORKAREA to respect taskbar / panel at screen bottom
        int screen_h = DisplayHeight(m_display, screen);
        int target_bottom = screen_h;
        
        Atom workarea_atom = XInternAtom(m_display, "_NET_WORKAREA", True);
        if (workarea_atom != None) {
            Atom actual_type;
            int actual_format;
            unsigned long nitems, bytes_after;
            long* workarea_data = nullptr;
            if (XGetWindowProperty(m_display, RootWindow(m_display, screen), workarea_atom,
                                   0, 4, False, XA_CARDINAL, &actual_type, &actual_format,
                                   &nitems, &bytes_after, (unsigned char**)&workarea_data) == Success) {
                if (workarea_data && nitems >= 4) {
                    long work_y = workarea_data[1];
                    long work_h = workarea_data[3];
                    long work_bottom = work_y + work_h;
                    if (work_bottom > 100 && work_bottom < screen_h) {
                        target_bottom = static_cast<int>(work_bottom);
                    }
                    XFree(workarea_data);
                }
            }
        }

        // Dock strictly above panel with 2px gap (Zero overlap!)
        int pos_y = target_bottom - m_height - 2;

        m_window = XCreateWindow(
            m_display, RootWindow(m_display, screen),
            0, pos_y, screen_w, m_height, 0,
            vinfo.depth, InputOutput, vinfo.visual,
            CWColormap | CWBackPixel | CWBorderPixel | CWOverrideRedirect,
            &attrs
        );

        // Make window 100% click-through via XShape
        XShapeCombineRectangles(m_display, m_window, ShapeInput, 0, 0, NULL, 0, ShapeSet, 0);

        // Keep above all windows
        Atom wm_state = XInternAtom(m_display, "_NET_WM_STATE", False);
        Atom wm_above = XInternAtom(m_display, "_NET_WM_STATE_ABOVE", False);
        XChangeProperty(m_display, m_window, wm_state, XA_ATOM, 32, PropModeReplace,
                        (unsigned char*)&wm_above, 1);

        XMapWindow(m_display, m_window);

        m_surface = cairo_xlib_surface_create(m_display, m_window, vinfo.visual, screen_w, m_height);
        m_cairo = cairo_create(m_surface);

        return true;
    }

    void Clear() {
        if (!m_cairo) return;
        cairo_set_operator(m_cairo, CAIRO_OPERATOR_CLEAR);
        cairo_paint(m_cairo);
        cairo_set_operator(m_cairo, CAIRO_OPERATOR_OVER);
    }

    void DrawBar(float x, float y, float w, float h, float r, float g, float b, float a) {
        if (!m_cairo) return;
        cairo_set_source_rgba(m_cairo, r, g, b, a);
        cairo_rectangle(m_cairo, x, y, w, h);
        cairo_fill(m_cairo);
    }

    void DrawWaveformCurve(const std::vector<float>& bands, float rms_energy) {
        if (!m_cairo || bands.size() < 4) return;
        size_t count = bands.size();
        float step_x = static_cast<float>(m_width) / (count - 1);
        float baseline_y = static_cast<float>(m_height) - 3.0f;
        float max_amp = static_cast<float>(m_height) * 0.88f;

        // Light 3-tap spatial filter: 1:1 synchronized with Equalizer Bars speed!
        std::vector<float> curve_heights(count);
        for (size_t i = 0; i < count; ++i) {
            float prev = (i > 0) ? bands[i - 1] : bands[i];
            float curr = bands[i];
            float next = (i + 1 < count) ? bands[i + 1] : bands[i];
            curve_heights[i] = prev * 0.15f + curr * 0.70f + next * 0.15f;
            if (i < 3) curve_heights[i] *= (static_cast<float>(i) / 3.0f);
            else if (i >= count - 3) curve_heights[i] *= (static_cast<float>(count - 1 - i) / 3.0f);
        }

        cairo_new_path(m_cairo);
        cairo_move_to(m_cairo, 0, baseline_y - curve_heights[0] * max_amp);
        for (size_t i = 0; i < count - 1; ++i) {
            float x1 = i * step_x;
            float y1 = baseline_y - curve_heights[i] * max_amp;
            float x2 = (i + 1) * step_x;
            float y2 = baseline_y - curve_heights[i + 1] * max_amp;
            float cpx1 = x1 + (x2 - x1) / 2.0f;
            float cpy1 = y1;
            float cpx2 = x1 + (x2 - x1) / 2.0f;
            float cpy2 = y2;
            cairo_curve_to(m_cairo, cpx1, cpy1, cpx2, cpy2, x2, y2);
        }

        // Multi-color spectrum gradient: Crimson -> Orange -> Violet -> Azure
        cairo_pattern_t* pat = cairo_pattern_create_linear(0, 0, m_width, 0);
        cairo_pattern_add_color_stop_rgba(pat, 0.00, 1.0, 0.10, 0.29, 0.95);
        cairo_pattern_add_color_stop_rgba(pat, 0.30, 1.0, 0.34, 0.13, 0.95);
        cairo_pattern_add_color_stop_rgba(pat, 0.65, 0.55, 0.36, 0.96, 0.95);
        cairo_pattern_add_color_stop_rgba(pat, 1.00, 0.0, 0.64, 1.0, 0.95);

        cairo_set_source(m_cairo, pat);
        cairo_set_line_width(m_cairo, 2.5);
        cairo_stroke_preserve(m_cairo);

        // Translucent fill under wave
        cairo_line_to(m_cairo, m_width, m_height);
        cairo_line_to(m_cairo, 0, m_height);
        cairo_close_path(m_cairo);

        cairo_pattern_t* fill_pat = cairo_pattern_create_linear(0, 0, m_width, 0);
        cairo_pattern_add_color_stop_rgba(fill_pat, 0.00, 1.0, 0.10, 0.29, 0.25);
        cairo_pattern_add_color_stop_rgba(fill_pat, 0.30, 1.0, 0.34, 0.13, 0.25);
        cairo_pattern_add_color_stop_rgba(fill_pat, 0.65, 0.55, 0.36, 0.96, 0.25);
        cairo_pattern_add_color_stop_rgba(fill_pat, 1.00, 0.0, 0.64, 1.0, 0.25);
        cairo_set_source(m_cairo, fill_pat);
        cairo_fill(m_cairo);

        cairo_pattern_destroy(pat);
        cairo_pattern_destroy(fill_pat);
    }

    void Flush() {
        if (m_display) XFlush(m_display);
    }

    void Destroy() {
        if (m_cairo) { cairo_destroy(m_cairo); m_cairo = nullptr; }
        if (m_surface) { cairo_surface_destroy(m_surface); m_surface = nullptr; }
        if (m_display && m_window) { XDestroyWindow(m_display, m_window); m_window = 0; }
        if (m_display) { XCloseDisplay(m_display); m_display = nullptr; }
    }

private:
    Display* m_display;
    Window m_window;
    int m_width;
    int m_height;
    cairo_t* m_cairo;
    cairo_surface_t* m_surface;
};
#endif
