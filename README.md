# DPF Native File Dialog Demo

This is an example repository to demonstrate how to use Native File Dialog on audio plug-in development with [DISTRHO Plugin Framework](https://github.com/DISTRHO/DPF.git). Useful if you want to open/save files from plug-in UI, for example, importing/exporting plug-in presets or patches.

Native File Dialog support is powered by [Michael Labbe](https://github.com/mlabbe)'s project [_nativefiledialog_](https://github.com/mlabbe/nativefiledialog) ("NFD" below).

---

## Principles

DPF uses a single thread to handle UI events, including rendering. Invoking NFD methods may stuck the thread, then crash the program. For example, Dear ImGui-based plug-ins may constantly pop-up `Forgot to invoke ImGui::EndFrame()?` error.

To avoid this, I run NFD in a dedicated thread using DPF's `DISTRHO::Thread` class. Here is procedure:

1. Inherit `DISTRHO::Thread` as `class NfdThread`, as below:

   ```c++
   // This header provides DISTRHO::Thread
   #include "extra/Thread.hpp"
   
   class NfdThread : public DISTRHO::Thread
   {
       nfdchar_t *fNfdPath;  // Buffer of NFD, storing the result file path
   
   public:
       bool gotNewPath;    // Flag, telling the main thread if we got a new filename
       std::stringstream pathStream;  // Receives the result file path. Can be accessed by the main thread
       nfdresult_t nfdResult;         // Result status ID
   
       NfdThread() : Thread(), fNfdPath(NULL), gotNewPath(false), nfdResult(NFD_OKAY)
       {
       }
   
       void run() override
       {
           // Remember to reset the flag and string stream
           gotNewPath = false;
           pathStream.str("");
   
           // Run NFD
           nfdResult = NFD_OpenDialog(NULL, NULL, &fNfdPath);
   
           if ( nfdResult == NFD_OKAY ) {
               d_stderr("Got file path: %s", fNfdPath);
   
               // Feed the string stream with our new file path
               pathStream << fNfdPath;
               // Remember to set flag, so the main thread can know it's time to get our new path
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
   ```

2. Define an NfdThread instance in UI class, and require waiting for the thread on destructor. For example:

   ```c++
   class ImGuiPluginUI : public UI
   {
       // I don't recommend using pointer and creating instance with `new` keyword,
       // because DISTRHO::Thread works well with standard class definition.
       NfdThread fNfdThread;
       
       String fFileName;
       
       // ... Other definitions and methods ...
       
       ~ImGuiPluginUI()
       {
           // Wait until NFD thread finishes, otherwise your plug-in or host may behave abnormally!
           fNfdThread.stopThread(-1);	// `-1` means wait forever
       }
      	
       // ... Other definitions and methods ...
   };
   ```

3. On UI rendering method (e.g. `DISTRHO::UI::onImGuiDisplay` if using Dear ImGui), invoke `fNfdThread.startThread()` on demand, then wait for result **within the rendering cycle (or `DISTRHO::UI::uiIdle()`, if you are using DGL)**:

   ```c++
   void ImGuiPluginUI::onImGuiDisplay()
   {
        if (ImGui::Begin(DISTRHO_PLUGIN_NAME, nullptr, ImGuiWindowFlags_NoResize + ImGuiWindowFlags_NoCollapse))
        {
            ImGui::Text("File Name stored in plugin:");

            ImGui::SetNextItemWidth(400);
            ImGui::TextWrapped("%s", fFileName.length() > 0 ? (const char*)fFileName : "<Not Yet Specified>");

            // "Browse File" button. Click it to open NFD.
            if (ImGui::Button("Browse File...")) {
                // Launch NFD. Soon a file dialog shows.
                // WARNING: Do NOT poll results here, since the NFD thread runs asynchronously, and you won't get any results!
                fNfdThread.startThread();
            }

            // Check if we got a new file path from NFD.
            // Since DPF executes this method very frequently, we can just poll results here.
            if (fNfdThread.gotNewPath) {
                // Read the new path from fNfdThread's string stream
                fFileName = String(fNfdThread.pathStream.str().c_str());

                d_stderr("Main thread: got file path %s, result = %d", fFileName.length() > 0 ? (const char*)fFileName : "<NULL>", fNfdThread.nfdResult);

                // Remember to reset the flag, otherwise plugin will execute this section forever!
                fNfdThread.gotNewPath = false;
            }
        }
        ImGui::End();
   }
   ```

Read `plugin/NativeFileDialogDemo_UI.cpp` for more details.

## Features

### Cross-platform

Native File Dialog Demo supports Windows, Linux and macOS.

- On Windows, NFD uses the modern Win32 `IFileDialog` API, providing a Vista-style file dialog.
- On Linux, NFD uses GTK's file dialog.
- On macOS, NFD uses Cocoa API.

### Based on Dear ImGui

This demo plugin uses [DPF's Dear ImGui implementation](https://github.com/DISTRHO/DPF-Widgets/) for rapid development and better demonstration.

### Plugin State Support

This demo plugin stores the file path into plugin state, so you can get the last recorded file path every time you load the plugin.

This project should be another good example to demonstrate how to use DPF's plugin state APIs, as well.

## How to Build

Make sure you have installed GCC/Clang, CMake, and GNU Make (or Ninja). **On Windows, you should build in [Msys2](https://msys2.org/) UCRT64 Shell**.

```bash
git clone https://github.com/AnClark/DPF-NativeFileDialogDemo --recursive

cd DPF-NativeFileDialogDemo
cmake -S . -B build
cmake --build build
```

## TODO

[ ] Demonstrate file saving.

## License

MIT License.