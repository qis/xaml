#include <version.h>

class Application {
public:
  Application(HINSTANCE instance) :
    manager_(winrt::Windows::UI::Xaml::Hosting::WindowsXamlManager::InitializeForCurrentThread()) {
    // Load window icon.
    const auto icon = LoadIcon(instance, MAKEINTRESOURCE(IDI_MAIN));

    // Register window class.
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = [](HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT {
      auto self = reinterpret_cast<Application*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
      if (msg == WM_NCCREATE) {
        self = reinterpret_cast<Application*>(reinterpret_cast<LPCREATESTRUCT>(lparam)->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        EnableNonClientDpiScaling(hwnd);
        self->hwnd_ = hwnd;
      } else if (msg == WM_DESTROY) {
        self->hwnd_ = nullptr;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
      }
      return self ? self->OnMessage(hwnd, msg, wparam, lparam) : DefWindowProc(hwnd, msg, wparam, lparam);
    };
    wc.hInstance = instance;
    wc.hIcon = icon;
    wc.hIconSm = icon;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW);
    wc.lpszClassName = WINDOW_CLASS;

    if (!RegisterClassEx(&wc)) {
      MessageBox(nullptr, "Could not register window class.", WINDOW_TITLE, MB_OK | MB_ICONERROR | MB_SETFOREGROUND);
      PostQuitMessage(1);
      return;
    }

    // Create window.
    const auto es = 0x0L;
    const auto ws = WS_OVERLAPPEDWINDOW;
    const auto wx = CW_USEDEFAULT;
    const auto wy = CW_USEDEFAULT;
    if (!CreateWindowEx(es, WINDOW_CLASS, WINDOW_TITLE, ws, 0, 0, wx, wy, nullptr, nullptr, instance, this)) {
      MessageBox(nullptr, "Could not create application window.", WINDOW_TITLE, MB_OK | MB_ICONERROR | MB_SETFOREGROUND);
      PostQuitMessage(1);
      return;
    }
  }

  int Run() {
    // Run the main message loop.
    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
      BOOL processed = FALSE;
      if (const auto interop = source_.try_as<IDesktopWindowXamlSourceNative2>()) {
        interop->PreTranslateMessage(&msg, &processed);
      }
      if (!processed) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
    }
    return static_cast<int>(msg.wParam);
  }

private:
  void OnCreate() {
    // Get window size.
    RECT wrc = {};
    GetWindowRect(hwnd_, &wrc);

    // Get client area size.
    RECT crc = {};
    GetClientRect(hwnd_, &crc);

    // Calculate window border sizes.
    const auto bx = (wrc.right - wrc.left) - (crc.right - crc.left);
    const auto by = (wrc.bottom - wrc.top) - (crc.bottom - crc.top);

    // Resize window.
    const auto cx = 800 + bx;
    const auto cy = 600 + by;
    SetWindowPos(hwnd_, nullptr, 0, 0, cx, cy, SWP_NOACTIVATE | SWP_NOMOVE);

    // Move window.
    if (const auto monitor = MonitorFromWindow(hwnd_, MONITOR_DEFAULTTONULL)) {
      MONITORINFO mi = {};
      mi.cbSize = sizeof(mi);
      if (GetMonitorInfo(monitor, &mi)) {
        auto x = ((mi.rcWork.right - mi.rcWork.left) - cx) / 2;
        auto y = ((mi.rcWork.bottom - mi.rcWork.top) - cy) / 2;
        SetWindowPos(hwnd_, nullptr, x, y, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE);
      }
    }

    // Create and attach DesktopWindowXamlSource object to window.
    const auto interop = source_.as<IDesktopWindowXamlSourceNative>();
    winrt::check_hresult(interop->AttachToWindow(hwnd_));

    // Get Xaml window.
    winrt::check_hresult(interop->get_WindowHandle(&xaml_));

    // Resize and show Xaml window.
    GetClientRect(hwnd_, &crc);
    SetWindowPos(xaml_, nullptr, 0, 0, crc.right - crc.left, crc.bottom - crc.top, SWP_SHOWWINDOW);

    // Load Xaml resource.
    const auto hres = FindResource(nullptr, MAKEINTRESOURCE(IDR_XAML), RT_RCDATA);
    if (!hres) {
      winrt::throw_last_error();
    }
    const auto hmem = LoadResource(nullptr, hres);
    if (!hmem) {
      winrt::throw_last_error();
    }
    const auto size = SizeofResource(nullptr, hres);
    const auto data = static_cast<const char*>(LockResource(hmem));
    const auto xaml = winrt::to_hstring(std::string_view(data, size));
    root_ = winrt::Windows::UI::Xaml::Markup::XamlReader::Load(xaml).as<decltype(root_)>();
    root_.Background(winrt::Windows::UI::Xaml::Media::SolidColorBrush(winrt::Windows::UI::Colors::Transparent()));
    root_.Width(crc.right - crc.left);
    root_.Height(crc.bottom - crc.top);
    source_.Content(root_);

    // Set background color.
    winrt::Windows::UI::ViewManagement::UISettings settings;
    const auto background = settings.GetColorValue(winrt::Windows::UI::ViewManagement::UIColorType::Background);
    const auto background_brush = CreateSolidBrush(RGB(background.R, background.G, background.B));
    SetClassLongPtr(hwnd_, GCLP_HBRBACKGROUND, reinterpret_cast<LONG_PTR>(background_brush));

    // Set title bar color.
    BOOL use_immersive_dark_mode = TRUE;
    DwmSetWindowAttribute(hwnd_, 19, &use_immersive_dark_mode, sizeof(use_immersive_dark_mode));

    // Bind UI elements.
    const auto close = root_.FindName(L"Close").as<winrt::Windows::UI::Xaml::Controls::Button>();
    close.Tapped([this](const auto& sender, const auto& args) {
      PostMessage(hwnd_, WM_COMMAND, MAKEWPARAM(IDM_EXIT, 0), 0);
    });

    // Show main window.
    ShowWindow(hwnd_, SW_SHOW);
  }

  void OnDestroy() {
    // Destroy Xaml objects.
    root_ = nullptr;
    source_ = nullptr;
    manager_ = nullptr;

    // Stop the main message loop.
    PostQuitMessage(0);
  }

  void OnSize(int cx, int cy) {
    SetWindowPos(xaml_, nullptr, 0, 0, cx, cy, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER);
    if (root_) {
      root_.Width(cx);
      root_.Height(cy);
    }
  }

  void OnDpiChanged(UINT dpi, LPCRECT rc) {
    SetWindowPos(hwnd_, nullptr, rc->left, rc->top, rc->right - rc->left, rc->bottom - rc->top, SWP_NOACTIVATE | SWP_NOZORDER);
  }

  void OnCommand(UINT id) {
    switch (id) {
    case IDM_EXIT:
      PostMessage(hwnd_, WM_CLOSE, 0, 0);
      break;
    }
  }

  LRESULT OnMessage(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    try {
      switch (msg) {
      case WM_CREATE:
        OnCreate();
        return 0;
      case WM_DESTROY:
        OnDestroy();
        return 0;
      case WM_SIZE:
        OnSize(LOWORD(lparam), HIWORD(lparam));
        return 0;
      case WM_DPICHANGED:
        OnDpiChanged(HIWORD(wparam), reinterpret_cast<LPCRECT>(lparam));
        return 0;
      case WM_COMMAND:
        OnCommand(LOWORD(wparam));
        return 0;
      }
    }
    catch (const winrt::hresult_error& e) {
      const auto message = fmt::format("Error: {}", winrt::to_string(e.message()));
      MessageBox(hwnd, message.data(), WINDOW_TITLE, MB_OK | MB_ICONERROR | MB_SETFOREGROUND);
      DestroyWindow(hwnd);
    }
    catch (const std::exception& e) {
      const auto message = fmt::format("Error: {}", e.what());
      MessageBox(hwnd, message.data(), WINDOW_TITLE, MB_OK | MB_ICONERROR | MB_SETFOREGROUND);
      DestroyWindow(hwnd);
    }
    return DefWindowProc(hwnd, msg, wparam, lparam);
  }

  HWND hwnd_ = nullptr;
  HWND xaml_ = nullptr;

  winrt::Windows::UI::Xaml::Hosting::WindowsXamlManager manager_;
  winrt::Windows::UI::Xaml::Hosting::DesktopWindowXamlSource source_;
  winrt::Windows::UI::Xaml::Controls::Grid root_{ nullptr };
};

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR cmd, int show) {
  // Initialize Windows Runtime and COM.
  winrt::init_apartment(winrt::apartment_type::single_threaded);

  // Create the main application window.
  Application application{ instance };
  return application.Run();
}
