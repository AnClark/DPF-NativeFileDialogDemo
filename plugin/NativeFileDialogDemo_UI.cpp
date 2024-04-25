/** Copyright (c) AnClark 2024 - GPL-3.0-or-later */

#include "DistrhoUI.hpp"
#include "veramobd.hpp"

#include "nfd.h"

#include "extra/Thread.hpp"

#include <sstream>

// For ImGuiPluginUI::OsOpenInShell()
#ifdef __WIN32
#include "windows.h"
#endif

START_NAMESPACE_DISTRHO

class ImGuiPluginUI;    // Forward decls.

class NfdThread : public Thread
{
    nfdchar_t *fNfdPath;

public:
    bool gotNewPath;
    std::stringstream pathStream;
    nfdresult_t nfdResult;    

    NfdThread() : Thread(), fNfdPath(NULL), gotNewPath(false), nfdResult(NFD_OKAY)
    {
    }

    void run() override
    {
        gotNewPath = false;

        pathStream.str("");

        nfdResult = NFD_OpenDialog(NULL, NULL, &fNfdPath);

        if ( nfdResult == NFD_OKAY ) {
            d_stderr("Got file path: %s", fNfdPath);

            pathStream << fNfdPath;
            gotNewPath = true;

            // Only free fNfdPath on success, otherwise program may crash!
            free(fNfdPath);
        }
        else if ( nfdResult == NFD_CANCEL ) {
            d_stderr("User pressed cancel.");
        }
        else {
            d_stderr2("Error: %s\n", NFD_GetError() );
        }
    }
};

class ImGuiPluginUI : public UI
{
    String fFileName;

    NfdThread fNfdThread;

    // ----------------------------------------------------------------------------------------------------------------

public:
   /**
      UI class constructor.
      The UI should be initialized to a default state that matches the plugin side.
    */
    ImGuiPluginUI()
        : UI(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT),
        fFileName()
    {
        setGeometryConstraints(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT, true);

        ImGuiIO& io(ImGui::GetIO());

        ImFontConfig fc;
        fc.FontDataOwnedByAtlas = true;
        fc.OversampleH = 1;
        fc.OversampleV = 1;
        fc.PixelSnapH = true;

        io.Fonts->AddFontFromMemoryCompressedTTF((void*)veramobd_compressed_data, veramobd_compressed_size, 16.0f * getScaleFactor(), &fc);
        io.Fonts->AddFontFromMemoryCompressedTTF((void*)veramobd_compressed_data, veramobd_compressed_size, 21.0f * getScaleFactor(), &fc);
        io.Fonts->AddFontFromMemoryCompressedTTF((void*)veramobd_compressed_data, veramobd_compressed_size, 11.0f * getScaleFactor(), &fc);
        io.Fonts->Build();
        io.FontDefault = io.Fonts->Fonts[1];
    }

    ~ImGuiPluginUI()
    {
        fNfdThread.stopThread(-1);
    }

protected:
    // ----------------------------------------------------------------------------------------------------------------
    // DSP/Plugin Callbacks

   /**
      A parameter has changed on the plugin side.@n
      This is called by the host to inform the UI about parameter changes.
    */
    void parameterChanged(uint32_t index, float value) override
    {
        repaint();
    }

    /**
      A state is changed on the plugin side.@n
    */
    void stateChanged(const char* key, const char* value) override
    {
        if (strcmp(key, "file_name") == 0) {
            fFileName = value;
        }
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Widget Callbacks

   /**
      ImGui specific onDisplay function.
    */
    void onImGuiDisplay() override
    {
        const float width = getWidth();
        const float height = getHeight();
        const float margin = 0.0f;

        ImGui::SetNextWindowPos(ImVec2(margin, margin));
        ImGui::SetNextWindowSize(ImVec2(width - 2 * margin, height - 2 * margin));

        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowTitleAlign = ImVec2(0.5f, 0.5f);

        ImGuiIO& io(ImGui::GetIO());
        ImFont* defaultFont = ImGui::GetFont();
        ImFont* titleBarFont = io.Fonts->Fonts[2];
        ImFont* smallFont = io.Fonts->Fonts[3];

        if (ImGui::Begin(DISTRHO_PLUGIN_NAME, nullptr, ImGuiWindowFlags_NoResize + ImGuiWindowFlags_NoCollapse))
        {
            ImGui::Text("File Name stored in plugin:");

            ImGui::SetNextItemWidth(400);
            ImGui::TextWrapped("%s", fFileName.length() > 0 ? (const char*)fFileName : "<Not Yet Specified>");

            if (ImGui::Button("Browse File...")) {
                fNfdThread.startThread();
            }

            if (fNfdThread.gotNewPath) {
                //d_stderr("Main thread: got file path %s, result = %d", fNfdPath, fNfdResult);
                fFileName = String(fNfdThread.pathStream.str().c_str());
                d_stderr("Main thread: got file path %s, result = %d", fFileName.length() > 0 ? (const char*)fFileName : "<NULL>", fNfdThread.nfdResult);

                fNfdThread.gotNewPath = false;
            }
        }
        ImGui::End();
    }

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ImGuiPluginUI)
};

// --------------------------------------------------------------------------------------------------------------------

UI* createUI()
{
    return new ImGuiPluginUI();
}

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO