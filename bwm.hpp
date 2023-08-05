extern "C"
{
    #include <X11/Xlib.h>
}

#include <memory>
#include <unordered_map>

class BasicWindowManager
{
    public:
    /// @brief Establishes Connection with the X server.
    /// @return bwm instance.
    static std::unique_ptr<BasicWindowManager> Create();

    /// @brief Xlib error handler. It must be static as its address is passed to Xlib.
    static int OnXError(Display* display, XErrorEvent* e);

    /// @brief Xlib error handler used to determine whether another window manager is running.
    static int OnWMDetected(Display* display, XErrorEvent* e);

    /// @brief Whether an existing window manager has been detected. Set by OnWMDetected.
    static bool wm_detected;

    /// @brief Internally creates a window when calling Create().
    /// @param display Connection to the X Server
    BasicWindowManager(Display* display);
    
    // Removes to ability to create bwm from another bwm.
    BasicWindowManager(const BasicWindowManager&) = delete;
    BasicWindowManager& operator=(const BasicWindowManager&) = delete;

    ~BasicWindowManager();

    /// @brief Handles the event loop.
    void Run();

    private:
    void OnCreateNotify(const XCreateWindowEvent&);
    void OnConfigureRequest(const XConfigureRequestEvent&);
    void OnConfigureNotify(const XConfigureEvent&);
    void OnMapRequest(const XMapRequestEvent&);

    void OnMapNotify(const XMapEvent&);
    void OnUnmapNotify(const XUnmapEvent&);
    void OnReparentNotify(const XReparentEvent&);
    void OnDestroyNotify(const XDestroyWindowEvent&);

    void Frame(Window, bool);
    void Unframe(Window);

    /// @brief Handle to the x server connection.
    Display* m_Display;

    /// @brief Handle to the root window.
    const Window m_RootWindow;

    /// @brief Maps top-level windows to their frame windows.
    std::unordered_map<Window, Window> m_Clients;
    
};