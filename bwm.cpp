#include "bwm.hpp"
#include <iostream>

bool BasicWindowManager::wm_detected = false;

std::unique_ptr<BasicWindowManager> BasicWindowManager::Create()
{
    // Establish connection with the X server.
    Display* display = XOpenDisplay(nullptr);
    if(!display)
    {
        std::cerr << "Error: Failed to establish connection with X Server: "
            << XDisplayName(nullptr) << ".\n";
        
        return nullptr;
    }

    return std::make_unique<BasicWindowManager>(display);
}

int BasicWindowManager::OnXError(Display *display, XErrorEvent *e)
{
    return 0;
}

int BasicWindowManager::OnWMDetected(Display *display, XErrorEvent *e)
{
    // In the case of an already running window manager, the error code from
    // XSelectInput is BadAccess. We don't expect this handler to receive any
    // other errors.

    // Set flag.
    wm_detected = static_cast<int>(e->error_code) == BadAccess;

    // The return value is ignored.
    return 0;
}

BasicWindowManager::BasicWindowManager(Display* display)
: m_Display(display), m_RootWindow(DefaultRootWindow(display)), m_Clients{} { }

void BasicWindowManager::OnCreateNotify(const XCreateWindowEvent& e)
{

}

void BasicWindowManager::OnConfigureRequest(const XConfigureRequestEvent& e)
{
    XWindowChanges changes;

    // Copy fields from e to changes.
    changes.x = e.x;
    changes.y = e.y;
    changes.width = e.width;
    changes.height = e.height;
    changes.border_width = e.border_width;
    changes.sibling = e.above;
    changes.stack_mode = e.detail;

    if (m_Clients.count(e.window)) 
    {
        const Window frame = m_Clients[e.window];
        XConfigureWindow(m_Display, frame, e.value_mask, &changes);
        std::cout << "Info: Resize " << e.window << " to " << e.width << "x" << e.height << ".\n";
    }

    // Grant request by calling XConfigureWindow().
    XConfigureWindow(m_Display, e.window, e.value_mask, &changes);
    std::cout << "Info: Resize " << e.window << " to " << e.width << "x" << e.height << ".\n";
}

void BasicWindowManager::OnConfigureNotify(const XConfigureEvent& e)
{

}

void BasicWindowManager::OnMapRequest(const XMapRequestEvent& e)
{
    // Frame the window.
    Frame(e.window, false);

    // Map the window to the screen.
    XMapWindow(m_Display, e.window);
}

void BasicWindowManager::OnMapNotify(const XMapEvent& e)
{

}

void BasicWindowManager::OnUnmapNotify(const XUnmapEvent& e)
{
    // If the window is a client window we manage, unframe it upon UnmapNotify. We
    // need the check because we will receive an UnmapNotify event for a frame
    // window we just destroyed ourselves.
    if (!m_Clients.count(e.window)) 
    {
        std::cout << "Info: Ignore UnmapNotify for non-client window " << e.window << ".\n";
        return;
    }

    // Ignore event if it is triggered by reparenting a window that was mapped
    // before the window manager started.
    //
    // Since we receive UnmapNotify events from the SubstructureNotify mask, the
    // event attribute specifies the parent window of the window that was
    // unmapped. This means that an UnmapNotify event from a normal client window
    // should have this attribute set to a frame window we maintain. Only an
    // UnmapNotify event triggered by reparenting a pre-existing window will have
    // this attribute set to the root window.
    if (e.event == m_RootWindow) 
    {
        std::cout << "Info: Ignore UnmapNotify for reparented pre-existing window " << e.window << ".\n";
        return;
    }

    Unframe(e.window);
}

void BasicWindowManager::OnReparentNotify(const XReparentEvent& e)
{

}

void BasicWindowManager::OnDestroyNotify(const XDestroyWindowEvent& e)
{

}

void BasicWindowManager::Frame(Window w, bool createdBeforeWM)
{
    // Visual properties of the frame to create.
    const unsigned int BORDER_WIDTH = 5;
    const unsigned long BORDER_COLOR = 0x9c9c9c;
    const unsigned long BG_COLOR = 0x636363;

    // Retrieve attributes of window to frame.
    XWindowAttributes x_window_attrs;
    if(!XGetWindowAttributes(m_Display, w, &x_window_attrs))
    {
        std::cerr << "Error: Failed to retrieve attributes of window.\n";
        return;
    }

    // If window was created before window manager started, we should frame
    // it only if it is visible and doesn't set override_redirect.
    if (createdBeforeWM && (x_window_attrs.override_redirect || x_window_attrs.map_state != IsViewable))
        return;

    // Create frame.
    const Window frame = XCreateSimpleWindow(m_Display, m_RootWindow, x_window_attrs.x, x_window_attrs.y,
        x_window_attrs.width, x_window_attrs.height, BORDER_WIDTH, BORDER_COLOR, BG_COLOR);
    
    // Select event masks for frame.
    XSelectInput(m_Display, frame, SubstructureRedirectMask | SubstructureNotifyMask);

    // Add client to save set, so that it will be restored and kept alive if we crash.
    XAddToSaveSet(m_Display, w);

    // Reparent client window.
    XReparentWindow( m_Display, w, frame, 0, 0);  // Offset of client window within frame.
    
    // Map frame.
    XMapWindow(m_Display, frame);

    // Save frame handle.
    m_Clients[w] = frame;
}

void BasicWindowManager::Unframe(Window w)
{
    // We reverse the steps taken in Frame().
    const Window frame = m_Clients[w];

    // Unmap frame.
    XUnmapWindow(m_Display, frame);

    // Reparent client window back to root window.
    XReparentWindow(m_Display, w, m_RootWindow, 0, 0);  // Offset of client window within root.

    // Remove client window from save set, as it is now unrelated to us.
    XRemoveFromSaveSet(m_Display, w);

    // Destroy frame.
    XDestroyWindow(m_Display, frame);

    // Drop reference to frame handle.
    m_Clients.erase(w);

    std::cout << "Info: Unframed window " << w << " [" << frame << "].\n";
}

BasicWindowManager::~BasicWindowManager()
{
    XCloseDisplay(m_Display);
}

void BasicWindowManager::Run()
{
    // Select events on root window. Use a special error handler so we can
    // exit gracefully if another window manager is already running.
    wm_detected = false;
    XSetErrorHandler(&BasicWindowManager::OnWMDetected);
    XSelectInput(m_Display, m_RootWindow, SubstructureRedirectMask | SubstructureNotifyMask);
    XSync(m_Display, false);

    if (wm_detected) 
    {
        std::cerr << "Error: Another Window manager is currently running on display "
            << XDisplayString(m_Display) << ", terminating.\n";
        return;
    }

    // Set error handler.
    XSetErrorHandler(&BasicWindowManager::OnXError);

    // Grab X server to prevent windows from changing under us while we frame them.
    XGrabServer(m_Display);

    // Frame existing top-level windows.
    // Query existing top-level windows.
    Window returned_root, returned_parent;
    Window* top_level_windows;
    unsigned int num_top_level_windows;
    if(!XQueryTree(m_Display, m_RootWindow, &returned_root, &returned_parent, &top_level_windows, &num_top_level_windows))
    {
        std::cerr << "Error: Failed to query tree.\n";
        return;
    }

    if(returned_root != m_RootWindow)
    {
        std::cerr << "Error: Returned root window is not the same as root window.\n";
        return;
    }

    // Frame each top-level window.
    for (unsigned int i = 0; i < num_top_level_windows; ++i) 
        Frame(top_level_windows[i], true);

    // Free top-level window array.
    XFree(top_level_windows);

    // Ungrab X server.
    XUngrabServer(m_Display);

    // Event Loop
    for (;;) 
    {
        // Get next event.
        XEvent e;
        XNextEvent(m_Display, &e);

        // Dispatch event.
        switch (e.type) 
        {
        case CreateNotify: OnCreateNotify(e.xcreatewindow); break;
        case ConfigureRequest: OnConfigureRequest(e.xconfigurerequest); break;
        case ConfigureNotify: OnConfigureNotify(e.xconfigure); break;
        case MapRequest: OnMapRequest(e.xmaprequest); break;
        case MapNotify: OnMapNotify(e.xmap); break;
        case UnmapNotify: OnUnmapNotify(e.xunmap); break;
        case ReparentNotify: OnReparentNotify(e.xreparent); break;
        case DestroyNotify: OnDestroyNotify(e.xdestroywindow); break;
        default: break;
        }
    }
}