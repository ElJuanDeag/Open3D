// ----------------------------------------------------------------------------
// -                        Open3D: www.open3d.org                            -
// ----------------------------------------------------------------------------
// The MIT License (MIT)
//
// Copyright (c) 2020 www.open3d.org
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
// ----------------------------------------------------------------------------

#include "open3d/visualization/gui/WebRTCWindowSystem.h"

#include <chrono>
#include <sstream>
#include <thread>

#include "open3d/core/Tensor.h"
#include "open3d/io/ImageIO.h"
#include "open3d/utility/Console.h"
#include "open3d/visualization/gui/Application.h"
#include "open3d/visualization/utility/Draw.h"
#include "open3d/visualization/webrtc_server/WebRTCServer.h"

namespace open3d {
namespace visualization {
namespace gui {

struct WebRTCWindowSystem::Impl {
    // TODO: can this be unique?
    std::shared_ptr<webrtc_server::WebRTCServer> webrtc_server_ = nullptr;
    std::thread webrtc_thread_;
    bool sever_started_ = false;
};

std::shared_ptr<WebRTCWindowSystem> WebRTCWindowSystem::GetInstance() {
    static std::shared_ptr<WebRTCWindowSystem> instance(new WebRTCWindowSystem);
    return instance;
}

WebRTCWindowSystem::WebRTCWindowSystem()
    : BitmapWindowSystem(
#if !defined(__APPLE__) && !defined(_WIN32) && !defined(_WIN64)
              BitmapWindowSystem::Rendering::HEADLESS
#else
              BitmapWindowSystem::Rendering::NORMAL
#endif
              ),
      impl_(new WebRTCWindowSystem::Impl()) {
    // Initialize WebRTC server. The sever is started at the first AddWindow,
    impl_->webrtc_server_ = std::make_shared<webrtc_server::WebRTCServer>();

    // Server->client send frame.
    auto draw_callback = [this](const gui::Window *window,
                                std::shared_ptr<core::Tensor> im) -> void {
        this->impl_->webrtc_server_->OnFrame(window->GetUID(), im);
    };
    SetOnWindowDraw(draw_callback);
}

WebRTCWindowSystem::~WebRTCWindowSystem() {}

void WebRTCWindowSystem::SetMouseEventCallback(
        std::function<void(const std::string &, const MouseEvent &)> f) {
    impl_->webrtc_server_->SetMouseEventCallback(f);
}

void WebRTCWindowSystem::SetRedrawCallback(
        std::function<void(const std::string &)> f) {
    impl_->webrtc_server_->SetRedrawCallback(f);
}

void WebRTCWindowSystem::StartWebRTCServer() {
    if (!impl_->sever_started_) {
        auto start_webrtc_thread = [this]() {
            this->impl_->webrtc_server_->Run();
        };
        impl_->webrtc_thread_ = std::thread(start_webrtc_thread);
        impl_->sever_started_ = true;
    }
}

}  // namespace gui
}  // namespace visualization
}  // namespace open3d