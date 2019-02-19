#include <filesystem>

#include <gtkmm.h>
#include <fstream>

#include "../webview/webview.h"
#include "webview_impl.h"
#include "../lib_path.h"
#include "../util/mime.h"

namespace fs = std::filesystem;

namespace {
    void webview_load_changed(GtkWidget*, WebKitLoadEvent load_event, DeskGap::WebView::EventCallbacks* callbacks) {
        switch (load_event) {
        case WEBKIT_LOAD_STARTED:
            callbacks->didStartLoading();
            break;
        case WEBKIT_LOAD_FINISHED:
            callbacks->didStopLoading(std::nullopt);
            break;
        default:
            break;
        }
    }
    const std::string localURLScheme = "deskgap-local";

    std::optional<std::vector<char>> readContentFromPath(const fs::path& filePath) {
        std::ifstream fileStream(filePath, std::ios::binary);
        if (fileStream.fail()) {
            return std::nullopt;
        }
        try {
            return std::vector<char>(std::istreambuf_iterator<char>(fileStream), { });
        }
        catch (...) {
            return std::nullopt;
        }
    }

    void local_file_uri_scheme_request_cb(WebKitURISchemeRequest *request, gpointer servedPathPtr) {
        const auto& servedPath = *static_cast<std::optional<fs::path>*>(servedPathPtr);
        if (!servedPath.has_value()) {
            GError *error = g_error_new(WEBKIT_NETWORK_ERROR, 404, "Requesting Local Files Not Allowed");
            webkit_uri_scheme_request_finish_error (request, error);
            g_error_free(error);
            return;
        }
        const gchar* urlPath = webkit_uri_scheme_request_get_path(request);
        while (*urlPath == '/') ++urlPath;

        std::string filename = Glib::uri_unescape_string(urlPath);
        fs::path fullPath = servedPath.value() / filename;


        gchar* fileContent;
        gsize fileSize;
        GError* error = nullptr;
        g_file_get_contents(fullPath.c_str(), &fileContent, &fileSize, &error);

        if (error != nullptr) {
            webkit_uri_scheme_request_finish_error(request, error);
            g_error_free(error);
            return;
        }

        GInputStream *stream = g_memory_input_stream_new_from_data(fileContent, fileSize, g_free);

        const char* extension  = fullPath.extension().c_str();
        while (*extension == '.') ++extension;
        webkit_uri_scheme_request_finish(request, stream, fileSize, DeskGap::GetMimeTypeOfExtension(extension).c_str());
        g_object_unref(stream);
    }
}

namespace DeskGap {

    WebView::WebView(EventCallbacks&& callbacks): impl_(std::make_unique<Impl>(Impl {
        nullptr, std::move(callbacks)
    })) {
        
        WebKitWebContext* context = webkit_web_context_new();
        webkit_web_context_register_uri_scheme(
            context,
            localURLScheme.c_str(), local_file_uri_scheme_request_cb,
            &(impl_->servedPath), nullptr
        );

        impl_->gtkWebView = WEBKIT_WEB_VIEW(g_object_ref_sink(webkit_web_view_new_with_context(context)));

        g_object_unref(context);

        gtk_widget_show(GTK_WIDGET(impl_->gtkWebView));
        g_signal_connect(impl_->gtkWebView, "load-changed", G_CALLBACK(webview_load_changed), &(impl_->callbacks));
    }


    WebView::~WebView() {
        gtk_widget_destroy(GTK_WIDGET(impl_->gtkWebView));
        g_object_unref(impl_->gtkWebView);
    }

    void WebView::LoadHTMLString(const std::string& html) {
        impl_->servedPath.reset();
        webkit_web_view_load_html(impl_->gtkWebView, html.c_str(), nullptr);
    }

    void WebView::LoadLocalFile(const std::string& path) {
        auto fsPath = fs::path(path);

        impl_->servedPath.emplace(fsPath.parent_path());

        std::string filename = fsPath.filename();
        std::string urlEncodedFilename = Glib::uri_escape_string(filename, std::string(), false);

        webkit_web_view_load_uri(impl_->gtkWebView, (localURLScheme + "://host/" + urlEncodedFilename).c_str());
    }

    void WebView::LoadRequest(
        const std::string& method,
        const std::string& urlString,
        const std::vector<HTTPHeader>& headers,
        const std::optional<std::string>& body
    ) {
        
    }

    void WebView::SetDevToolsEnabled(bool enabled) {
        WebKitSettings* settings = webkit_web_view_get_settings(impl_->gtkWebView);
        webkit_settings_set_enable_developer_extras(settings, true);
    }

    void WebView::Reload() {

    }

    void WebView::EvaluateJavaScript(const std::string& scriptString, std::optional<JavaScriptEvaluationCallback>&& optionalCallback) {
        
    }
}