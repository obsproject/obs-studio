#include "winrt-capture.h"
#include <mutex>

extern "C" EXPORT BOOL winrt_capture_supported() {
    try {
        // No contract for IGraphicsCaptureItemInterop, verify 10.0.18362.0
        return winrt::Windows::Foundation::Metadata::ApiInformation::IsApiContractPresent(
            L"Windows.Foundation.UniversalApiContract", 8);
    } catch (const winrt::hresult_error& err) {
        blog(LOG_ERROR, "winrt_capture_supported failed (0x%08X): %s", err.code().value,
             winrt::to_string(err.message()).c_str());
    } catch (...) {
        blog(LOG_ERROR, "winrt_capture_supported failed with unknown error (0x%08X)", winrt::to_hresult().value);
    }
    return FALSE;
}

extern "C" EXPORT BOOL winrt_capture_cursor_toggle_supported() {
    try {
        return winrt::Windows::Foundation::Metadata::ApiInformation::IsPropertyPresent(
            L"Windows.Graphics.Capture.GraphicsCaptureSession", L"IsCursorCaptureEnabled");
    } catch (const winrt::hresult_error& err) {
        blog(LOG_ERROR, "winrt_capture_cursor_toggle_supported failed (0x%08X): %s", err.code().value,
             winrt::to_string(err.message()).c_str());
    } catch (...) {
        blog(LOG_ERROR, "winrt_capture_cursor_toggle_supported failed with unknown error (0x%08X)",
             winrt::to_hresult().value);
    }
    return FALSE;
}

template <typename T>
static winrt::com_ptr<T> GetDXGIInterfaceFromObject(const winrt::Windows::Foundation::IInspectable& object) {
    auto access = object.as<Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess>();
    winrt::com_ptr<T> result;
    winrt::check_hresult(access->GetInterface(winrt::guid_of<T>(), result.put_void()));
    return result;
}

static bool get_client_box(HWND window, uint32_t width, uint32_t height, D3D11_BOX* client_box) {
    RECT client_rect{}, window_rect{};
    POINT upper_left{};

    // Check if window is minimized (iconic) twice to handle possible state changes
    bool client_box_available = !IsIconic(window) && GetClientRect(window, &client_rect) && !IsIconic(window) &&
                                (client_rect.right > 0) && (client_rect.bottom > 0) &&
                                (DwmGetWindowAttribute(window, DWMWA_EXTENDED_FRAME_BOUNDS, &window_rect,
                                                       sizeof(window_rect)) == S_OK) &&
                                ClientToScreen(window, &upper_left);
    if (client_box_available) {
        const uint32_t left = (upper_left.x > window_rect.left) ? (upper_left.x - window_rect.left) : 0;
        client_box->left = left;

        const uint32_t top = (upper_left.y > window_rect.top) ? (upper_left.y - window_rect.top) : 0;
        client_box->top = top;

        uint32_t texture_width = (width > left) ? min(width - left, static_cast<uint32_t>(client_rect.right)) : 1;
        uint32_t texture_height = (height > top) ? min(height - top, static_cast<uint32_t>(client_rect.bottom)) : 1;

        client_box->right = left + texture_width;
        client_box->bottom = top + texture_height;

        client_box->front = 0;
        client_box->back = 1;

        client_box_available = (client_box->right <= width) && (client_box->bottom <= height);
    }

    return client_box_available;
}

static DXGI_FORMAT get_pixel_format(HWND window, HMONITOR monitor, BOOL force_sdr) {
    constexpr DXGI_FORMAT sdr_format = DXGI_FORMAT_B8G8R8A8_UNORM;

    if (force_sdr)
        return sdr_format;

    if (window)
        monitor = MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST);

    return (monitor && gs_is_monitor_hdr(monitor)) ? DXGI_FORMAT_R16G16B16A16_FLOAT : sdr_format;
}

struct winrt_capture : public std::enable_shared_from_this<winrt_capture> {
    HWND window = nullptr;
    BOOL client_area = FALSE;
    BOOL force_sdr = FALSE;
    HMONITOR monitor = nullptr;
    DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;

    bool capture_cursor = false;
    BOOL cursor_visible = FALSE;

    gs_texture_t* texture = nullptr;
    bool texture_written = false;
    winrt::Windows::Graphics::Capture::GraphicsCaptureItem item{ nullptr };
    winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice device{ nullptr };
    winrt::com_ptr<ID3D11DeviceContext> context;
    winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool frame_pool{ nullptr };
    winrt::Windows::Graphics::Capture::GraphicsCaptureSession session{ nullptr };
    winrt::Windows::Graphics::SizeInt32 last_size{};
    winrt::Windows::Graphics::Capture::GraphicsCaptureItem::Closed_revoker closed;
    winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool::FrameArrived_revoker frame_arrived;

    uint32_t texture_width = 0;
    uint32_t texture_height = 0;
    D3D11_BOX client_box{};

    BOOL active = FALSE;

    void on_closed(const winrt::Windows::Graphics::Capture::GraphicsCaptureItem&, const winrt::Windows::Foundation::IInspectable&) {
        active = FALSE;
    }

    void on_frame_arrived(const winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool& sender,
                          const winrt::Windows::Foundation::IInspectable&) {
        try {
            auto frame = sender.TryGetNextFrame();
            auto frame_content_size = frame.ContentSize();

            auto frame_surface = GetDXGIInterfaceFromObject<ID3D11Texture2D>(frame.Surface());

            // Need GetDesc because ContentSize is not reliable
            D3D11_TEXTURE2D_DESC desc{};
            frame_surface->GetDesc(&desc);

            obs_enter_graphics();

            if (desc.Format == get_pixel_format(window, monitor, force_sdr)) {
                if (!client_area || get_client_box(window, desc.Width, desc.Height, &client_box)) {
                    if (client_area) {
                        texture_width = client_box.right - client_box.left;
                        texture_height = client_box.bottom - client_box.top;
                    } else {
                        texture_width = desc.Width;
                        texture_height = desc.Height;
                    }

                    if (texture) {
                        if (texture_width != gs_texture_get_width(texture) ||
                            texture_height != gs_texture_get_height(texture)) {
                            gs_texture_destroy(texture);
                            texture = nullptr;
                        }
                    }

                    if (!texture) {
                        const gs_color_format color_format =
                            desc.Format == DXGI_FORMAT_R16G16B16A16_FLOAT ? GS_RGBA16F : GS_BGRA;
                        texture = gs_texture_create(texture_width, texture_height, color_format, 1, nullptr, 0);
                    }

                    if (client_area) {
                        context->CopySubresourceRegion(static_cast<ID3D11Texture2D*>(gs_texture_get_obj(texture)),
                                                       0, 0, 0, 0, frame_surface.get(), 0, &client_box);
                    } else {
                        // If they gave an SRV, we could avoid this copy
                        context->CopyResource(static_cast<ID3D11Texture2D*>(gs_texture_get_obj(texture)),
                                              frame_surface.get());
                    }

                    texture_written = true;
                }

                if (frame_content_size.Width != last_size.Width || frame_content_size.Height != last_size.Height) {
                    format = desc.Format;
                    frame_pool.Recreate(
                        device,
                        static_cast<winrt::Windows::Graphics::DirectX::DirectXPixelFormat>(format), 2,
                        frame_content_size);

                    last_size = frame_content_size;
                }
            } else {
                active = FALSE;
            }

            obs_leave_graphics();
        } catch (const winrt::hresult_error& err) {
            blog(LOG_ERROR, "on_frame_arrived failed (0x%08X): %s", err.code().value,
                 winrt::to_string(err.message()).c_str());
        } catch (...) {
            blog(LOG_ERROR, "on_frame_arrived failed with unknown error (0x%08X)", winrt::to_hresult().value);
        }
    }
};

static std::mutex capture_list_mutex;
static std::vector<std::shared_ptr<winrt_capture>> capture_list;

static void winrt_capture_device_loss_release(void* data) {
    auto capture = static_cast<winrt_capture*>(data);
    capture->active = FALSE;

    capture->frame_arrived.revoke();

    try {
        if (capture->frame_pool)
            capture->frame_pool.Close();
    } catch (const winrt::hresult_error& err) {
        blog(LOG_ERROR, "Direct3D11CaptureFramePool::Close failed (0x%08X): %s", err.code().value,
             winrt::to_string(err.message()).c_str());
    } catch (...) {
        blog(LOG_ERROR, "Direct3D11CaptureFramePool::Close failed with unknown error (0x%08X)",
             winrt::to_hresult().value);
    }

    try {
        if (capture->session)
            capture->session.Close();
    } catch (const winrt::hresult_error& err) {
        blog(LOG_ERROR, "GraphicsCaptureSession::Close failed (0x%08X): %s", err.code().value,
             winrt::to_string(err.message()).c_str());
    } catch (...) {
        blog(LOG_ERROR, "GraphicsCaptureSession::Close failed with unknown error (0x%08X)",
             winrt::to_hresult().value);
    }

    capture->session = nullptr;
    capture->frame_pool = nullptr;
    capture->context = nullptr;
    capture->device = nullptr;
    capture->item = nullptr;
}

static bool winrt_capture_border_toggle_supported() {
    try {
        return winrt::Windows::Foundation::Metadata::ApiInformation::IsPropertyPresent(
            L"Windows.Graphics.Capture.GraphicsCaptureSession", L"IsBorderRequired");
    } catch (const winrt::hresult_error& err) {
        blog(LOG_ERROR, "winrt_capture_border_toggle_supported failed (0x%08X): %s", err.code().value,
             winrt::to_string(err.message()).c_str());
    } catch (...) {
        blog(LOG_ERROR, "winrt_capture_border_toggle_supported failed with unknown error (0x%08X)",
             winrt::to_hresult().value);
    }
    return false;
}

static winrt::Windows::Graphics::Capture::GraphicsCaptureItem
winrt_capture_create_item(IGraphicsCaptureItemInterop* interop_factory, HWND window, HMONITOR monitor) {
    winrt::Windows::Graphics::Capture::GraphicsCaptureItem item{ nullptr };
    HRESULT hr = S_OK;

    try {
        if (window) {
            hr = interop_factory->CreateForWindow(
                window, winrt::guid_of<ABI::Windows::Graphics::Capture::IGraphicsCaptureItem>(),
                reinterpret_cast<void**>(winrt::put_abi(item)));
        } else if (monitor) {
            hr = interop_factory->CreateForMonitor(
                monitor, winrt::guid_of<ABI::Windows::Graphics::Capture::IGraphicsCaptureItem>(),
                reinterpret_cast<void**>(winrt::put_abi(item)));
        } else {
            blog(LOG_ERROR, "No valid window or monitor provided for capture");
            return nullptr;
        }

        if (FAILED(hr)) {
            blog(LOG_ERROR, "CreateForWindow/CreateForMonitor failed (0x%08X)", hr);
            return nullptr;
        }
    } catch (const winrt::hresult_error& err) {
        blog(LOG_ERROR, "winrt_capture_create_item failed (0x%08X): %s", err.code().value,
             winrt::to_string(err.message()).c_str());
        return nullptr;
    } catch (...) {
        blog(LOG_ERROR, "winrt_capture_create_item failed with unknown error (0x%08X)",
             winrt::to_hresult().value);
        return nullptr;
    }

    return item;
}

static void winrt_capture_device_loss_rebuild(void* device_void, void* data) {
    auto capture = static_cast<winrt_capture*>(data);

    auto activation_factory =
        winrt::get_activation_factory<winrt::Windows::Graphics::Capture::GraphicsCaptureItem>();
    auto interop_factory = activation_factory.as<IGraphicsCaptureItemInterop>();
    auto item = winrt_capture_create_item(interop_factory.get(), capture->window, capture->monitor);
    if (!item)
        return;

    ID3D11Device* d3d_device = static_cast<ID3D11Device*>(device_void);
    winrt::com_ptr<IDXGIDevice> dxgi_device;
    if (FAILED(d3d_device->QueryInterface(dxgi_device.put()))) {
        blog(LOG_ERROR, "Failed to get DXGI device");
        return;
    }

    winrt::com_ptr<IInspectable> inspectable;
    if (FAILED(CreateDirect3D11DeviceFromDXGIDevice(dxgi_device.get(), inspectable.put()))) {
        blog(LOG_ERROR, "Failed to get WinRT device");
        return;
    }

    auto device = inspectable.as<winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice>();
    auto frame_pool = winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool::Create(
        device, static_cast<winrt::Windows::Graphics::DirectX::DirectXPixelFormat>(capture->format), 2,
        capture->last_size);
    auto session = frame_pool.CreateCaptureSession(item);

    if (winrt_capture_border_toggle_supported()) {
        winrt::Windows::Graphics::Capture::GraphicsCaptureAccess::RequestAccessAsync(
            winrt::Windows::Graphics::Capture::GraphicsCaptureAccessKind::Borderless).get();
        session.IsBorderRequired(false);
    }

    if (winrt_capture_cursor_toggle_supported())
        session.IsCursorCaptureEnabled(capture->capture_cursor && capture->cursor_visible);

    capture->item = item;
    capture->device = device;
    d3d_device->GetImmediateContext(capture->context.put());
    capture->frame_pool = frame_pool;
    capture->session = session;
    capture->frame_arrived = frame_pool.FrameArrived(winrt::auto_revoke, { capture, &winrt_capture::on_frame_arrived });

    try {
        session.StartCapture();
        capture->active = TRUE;
    } catch (const winrt::hresult_error& err) {
        blog(LOG_ERROR, "StartCapture failed (0x%08X): %s", err.code().value,
             winrt::to_string(err.message()).c_str());
    } catch (...) {
        blog(LOG_ERROR, "StartCapture failed with unknown error (0x%08X)", winrt::to_hresult().value);
    }
}

static std::shared_ptr<winrt_capture> winrt_capture_init_internal(BOOL cursor, HWND window, BOOL client_area,
                                                                  BOOL force_sdr, HMONITOR monitor) {
    try {
        ID3D11Device* d3d_device = static_cast<ID3D11Device*>(gs_get_device_obj());
        winrt::com_ptr<IDXGIDevice> dxgi_device;

        HRESULT hr = d3d_device->QueryInterface(dxgi_device.put());
        if (FAILED(hr)) {
            blog(LOG_ERROR, "Failed to get DXGI device");
            return nullptr;
        }

        winrt::com_ptr<IInspectable> inspectable;
        hr = CreateDirect3D11DeviceFromDXGIDevice(dxgi_device.get(), inspectable.put());
        if (FAILED(hr)) {
            blog(LOG_ERROR, "Failed to get WinRT device");
            return nullptr;
        }

        auto activation_factory =
            winrt::get_activation_factory<winrt::Windows::Graphics::Capture::GraphicsCaptureItem>();
        auto interop_factory = activation_factory.as<IGraphicsCaptureItemInterop>();
        auto item = winrt_capture_create_item(interop_factory.get(), window, monitor);
        if (!item)
            return nullptr;

        auto device = inspectable.as<winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice>();
        auto size = item.Size();
        auto format = get_pixel_format(window, monitor, force_sdr);
        auto frame_pool = winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool::Create(
            device, static_cast<winrt::Windows::Graphics::DirectX::DirectXPixelFormat>(format), 2, size);
        auto session = frame_pool.CreateCaptureSession(item);

        if (winrt_capture_border_toggle_supported()) {
            winrt::Windows::Graphics::Capture::GraphicsCaptureAccess::RequestAccessAsync(
                winrt::Windows::Graphics::Capture::GraphicsCaptureAccessKind::Borderless).get();
            session.IsBorderRequired(false);
        }

        BOOL cursor_toggle_supported = winrt_capture_cursor_toggle_supported();
        if (cursor_toggle_supported)
            session.IsCursorCaptureEnabled(cursor);

        auto capture = std::make_shared<winrt_capture>();
        capture->window = window;
        capture->client_area = client_area;
        capture->force_sdr = force_sdr;
        capture->monitor = monitor;
        capture->format = format;
        capture->capture_cursor = cursor && cursor_toggle_supported;
        capture->cursor_visible = cursor;
        capture->item = item;
        capture->device = device;
        d3d_device->GetImmediateContext(capture->context.put());
        capture->frame_pool = frame_pool;
        capture->session = session;
        capture->last_size = size;
        capture->closed = item.Closed(winrt::auto_revoke, { capture.get(), &winrt_capture::on_closed });
        capture->frame_arrived = frame_pool.FrameArrived(winrt::auto_revoke, { capture.get(), &winrt_capture::on_frame_arrived });

        {
            std::lock_guard<std::mutex> lock(capture_list_mutex);
            capture_list.push_back(capture);
        }

        session.StartCapture();
        capture->active = TRUE;

        gs_device_loss callbacks{};
        callbacks.device_loss_release = winrt_capture_device_loss_release;
        callbacks.device_loss_rebuild = winrt_capture_device_loss_rebuild;
        callbacks.data = capture.get();
        gs_register_loss_callbacks(&callbacks);

        return capture;
    } catch (const winrt::hresult_error& err) {
        blog(LOG_ERROR, "winrt_capture_init failed (0x%08X): %s", err.code().value,
             winrt::to_string(err.message()).c_str());
    } catch (...) {
        blog(LOG_ERROR, "winrt_capture_init failed with unknown error (0x%08X)", winrt::to_hresult().value);
    }
    return nullptr;
}

extern "C" EXPORT winrt_capture* winrt_capture_init_window(BOOL cursor, HWND window, BOOL client_area,
                                                           BOOL force_sdr) {
    return winrt_capture_init_internal(cursor, window, client_area, force_sdr, nullptr).get();
}

extern "C" EXPORT winrt_capture* winrt_capture_init_monitor(BOOL cursor, HMONITOR monitor, BOOL force_sdr) {
    return winrt_capture_init_internal(cursor, nullptr, FALSE, force_sdr, monitor).get();
}

extern "C" EXPORT void winrt_capture_free(winrt_capture* capture_ptr) {
    if (capture_ptr) {
        std::shared_ptr<winrt_capture> capture;
        {
            std::lock_guard<std::mutex> lock(capture_list_mutex);
            auto it = std::find_if(capture_list.begin(), capture_list.end(),
                                   [capture_ptr](const std::shared_ptr<winrt_capture>& c) {
                                       return c.get() == capture_ptr;
                                   });
            if (it != capture_list.end()) {
                capture = *it;
                capture_list.erase(it);
            }
        }

        if (capture) {
            obs_enter_graphics();
            gs_unregister_loss_callbacks(capture.get());
            gs_texture_destroy(capture->texture);
            obs_leave_graphics();

            capture->frame_arrived.revoke();
            capture->closed.revoke();

            try {
                if (capture->frame_pool)
                    capture->frame_pool.Close();
            } catch (const winrt::hresult_error& err) {
                blog(LOG_ERROR, "Direct3D11CaptureFramePool::Close failed (0x%08X): %s", err.code().value,
                     winrt::to_string(err.message()).c_str());
            } catch (...) {
                blog(LOG_ERROR, "Direct3D11CaptureFramePool::Close failed with unknown error (0x%08X)",
                     winrt::to_hresult().value);
            }

            try {
                if (capture->session)
                    capture->session.Close();
            } catch (const winrt::hresult_error& err) {
                blog(LOG_ERROR, "GraphicsCaptureSession::Close failed (0x%08X): %s", err.code().value,
                     winrt::to_string(err.message()).c_str());
            } catch (...) {
                blog(LOG_ERROR, "GraphicsCaptureSession::Close failed with unknown error (0x%08X)",
                     winrt::to_hresult().value);
            }
        }
    }
}

extern "C" EXPORT BOOL winrt_capture_active(const winrt_capture* capture) {
    return capture ? capture->active : FALSE;
}

extern "C" EXPORT BOOL winrt_capture_show_cursor(winrt_capture* capture, BOOL visible) {
    if (!capture)
        return FALSE;

    try {
        if (capture->capture_cursor) {
            if (capture->cursor_visible != visible) {
                capture->session.IsCursorCaptureEnabled(visible);
                capture->cursor_visible = visible;
            }
        }
        return TRUE;
    } catch (const winrt::hresult_error& err) {
        blog(LOG_ERROR, "IsCursorCaptureEnabled failed (0x%08X): %s", err.code().value,
             winrt::to_string(err.message()).c_str());
    } catch (...) {
        blog(LOG_ERROR, "IsCursorCaptureEnabled failed with unknown error (0x%08X)", winrt::to_hresult().value);
    }
    return FALSE;
}

extern "C" EXPORT enum gs_color_space winrt_capture_get_color_space(const winrt_capture* capture) {
    if (!capture)
        return GS_CS_SRGB;
    return (capture->format == DXGI_FORMAT_R16G16B16A16_FLOAT) ? GS_CS_709_EXTENDED : GS_CS_SRGB;
}

extern "C" EXPORT void winrt_capture_render(winrt_capture* capture) {
    if (capture && capture->texture_written) {
        const char* tech_name = "Draw";
        float multiplier = 1.f;
        const gs_color_space current_space = gs_get_color_space();
        if (capture->format == DXGI_FORMAT_R16G16B16A16_FLOAT) {
            switch (current_space) {
            case GS_CS_SRGB:
            case GS_CS_SRGB_16F:
                tech_name = "DrawMultiplyTonemap";
                multiplier = 80.f / obs_get_video_sdr_white_level();
                break;
            case GS_CS_709_EXTENDED:
                tech_name = "DrawMultiply";
                multiplier = 80.f / obs_get_video_sdr_white_level();
                break;
            default:
                break;
            }
        } else if (current_space == GS_CS_709_SCRGB) {
            tech_name = "DrawMultiply";
            multiplier = obs_get_video_sdr_white_level() / 80.f;
        }

        gs_effect_t* effect = obs_get_base_effect(OBS_EFFECT_DEFAULT);
        gs_technique_t* tech = gs_effect_get_technique(effect, tech_name);

        const bool previous = gs_framebuffer_srgb_enabled();
        gs_enable_framebuffer_srgb(true);

        gs_blend_state_push();
        gs_blend_function(GS_BLEND_ONE, GS_BLEND_INVSRCALPHA);

        gs_effect_set_texture_srgb(gs_effect_get_param_by_name(effect, "image"), capture->texture);
        gs_effect_set_float(gs_effect_get_param_by_name(effect, "multiplier"), multiplier);

        const size_t passes = gs_technique_begin(tech);
        for (size_t i = 0; i < passes; i++) {
            if (gs_technique_begin_pass(tech, i)) {
                gs_draw_sprite(capture->texture, 0, 0, 0);
                gs_technique_end_pass(tech);
            }
        }
        gs_technique_end(tech);

        gs_blend_state_pop();
        gs_enable_framebuffer_srgb(previous);
    }
}

extern "C" EXPORT uint32_t winrt_capture_width(const winrt_capture* capture) {
    return capture ? capture->texture_width : 0;
}

extern "C" EXPORT uint32_t winrt_capture_height(const winrt_capture* capture) {
    return capture ? capture->texture_height : 0;
}

extern "C" EXPORT void winrt_capture_thread_start() {
    std::lock_guard<std::mutex> lock(capture_list_mutex);
    void* device = gs_get_device_obj();
    for (auto& capture : capture_list) {
        winrt_capture_device_loss_rebuild(device, capture.get());
    }
}

extern "C" EXPORT void winrt_capture_thread_stop() {
    std::lock_guard<std::mutex> lock(capture_list_mutex);
    for (auto& capture : capture_list) {
        winrt_capture_device_loss_release(capture.get());
    }
}
