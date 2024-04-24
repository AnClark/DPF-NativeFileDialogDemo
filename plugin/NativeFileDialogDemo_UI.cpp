/** Copyright (c) AnClark 2024 - GPL-3.0-or-later */

#include "DistrhoUI.hpp"
#include "veramobd.hpp"

#include "nfd.h"

// For ImGuiPluginUI::OsOpenInShell()
#ifdef __WIN32
#include "windows.h"
#endif

START_NAMESPACE_DISTRHO

class ImGuiPluginUI : public UI
{
    String fFileName;
    nfdchar_t *fNfdPath;
    nfdresult_t fNfdResult;

    // ----------------------------------------------------------------------------------------------------------------

public:
   /**
      UI class constructor.
      The UI should be initialized to a default state that matches the plugin side.
    */
    ImGuiPluginUI()
        : UI(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT),
        fFileName(), fNfdPath(NULL), fNfdResult(NFD_OKAY)
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
        if (fNfdPath != NULL)
            free(fNfdPath);
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
                fNfdResult = NFD_OpenDialog(NULL, NULL, &fNfdPath);

                if ( fNfdResult == NFD_OKAY ) {
                    d_stderr("Got file path: %s", fNfdPath);

                    fFileName = String(fNfdPath);
                    setState("file_name", fFileName);
                }
                else if ( fNfdResult == NFD_CANCEL ) {
                    d_stderr("User pressed cancel.");
                }
                else {
                    d_stderr2("Error: %s\n", NFD_GetError() );
                }
            }
        }
        ImGui::End();
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Utilities

    void nfdLoadFile(nfdchar_t *outPath, nfdresult_t& result)
    {
        result = NFD_OpenDialog( NULL, NULL, &outPath );
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