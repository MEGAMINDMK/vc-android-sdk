# vc-android-sdk
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>Vice City Mod - Starter Template</title>
    <!-- Highlight.js styles -->
    <link rel="stylesheet"
          href="https://cdnjs.cloudflare.com/ajax/libs/highlight.js/11.8.0/styles/default.min.css">
    <link rel="stylesheet"
          href="https://cdnjs.cloudflare.com/ajax/libs/highlight.js-line-numbers.js/2.8.0/styles/line-numbers.min.css">
    <style>
        body {
            font-family: Arial, sans-serif;
            padding: 20px;
        }
        h2 {
            color: #2c3e50;
        }
        .requirements, .starter-template, .compile-guide {
            background-color: #f4f4f4;
            border-left: 4px solid #3498db;
            padding: 10px 15px;
            margin-bottom: 20px;
        }
        a {
            color: #2980b9;
            text-decoration: none;
        }
        a:hover {
            text-decoration: underline;
        }
        code.inline {
            background-color: #eee;
            padding: 2px 6px;
            border-radius: 4px;
            font-family: Consolas, monospace;
        }
    </style>
</head>
<body>

<section id="mod-install" style="background:#eef6ff; border:1px solid #007bff; border-radius:8px; padding:15px; margin: 20px auto; max-width: 600px; font-size: 1rem; text-align: center;">
  <h3 style="color: #0056b3;">Getting the Mod Working</h3>
  <p>To get the mod working properly, you will need to install the <strong>GTA VC Android APK</strong>:</p>
  <a href="gtavc.apk" download style="display: inline-block; margin-top: 10px; padding: 10px 20px; background-color: #007bff; color: white; text-decoration: none; border-radius: 5px; font-weight: bold;">Download gtavc.apk</a>
  <p style="margin-top: 10px; font-size: 0.9rem; color: #333;">Make sure to enable installation from unknown sources in your Android settings before installing.</p>
  <p style="margin-top: 10px; font-size: 0.9rem; color: #333;">Also make sure after installing of app your app should have access to permissions/Files and media/Allow management of all files -> enabled</p>
</section>

<h2>Vice City Mod – Starter Template</h2>

<div class="requirements">
    <strong>Requirements:</strong>
    <ul>
        <li><a href="https://github.com/AndroidModLoader" target="_blank">Android Mod Loader (AML)</a> – must be installed on your device.</li>
        <li><a href="https://developer.android.com/ndk/downloads" target="_blank">Android NDK</a> – required for compiling the mod.</li>
    </ul>
</div>

<div class="starter-template">
    <strong>Here’s a starter template to get you working fast:</strong>
    <p>This C++ mod example hooks into Vice City’s HUD functions using AML. You can build upon this to create your own features!</p>
</div>

<div class="starter-template">
    <strong>Download Basic Template:</strong>
    <p><a href="jni.zip">Download JNI</a><br><br>The <code class="inline">jni</code> folder must be placed inside your NDK project directory.</p>
</div>

<div class="compile-guide">
    <strong>How to Compile Your Mod:</strong>
    <p>Use the following commands inside your NDK project folder to build your mod:</p>
    <pre><code class="bash hljs">
ndk-build clean
ndk-build
    </code></pre>
    <p>This will compile your code into a shared library (.so) which you can then load using AML.</p>
</div>

<div class="starter-template">
    <strong>Where to Place the <code class="inline">.so</code> File:</strong>
    <p>Once your mod is compiled, you’ll get a shared library file (e.g. <code class="inline">libYourMod.so</code>).</p>
    <p>Place this file in the following folder on your Android device:</p>
    <pre><code class="bash hljs">
/storage/emulated/0/data/com.rockstargames.gtavc/mods
    </code></pre>
    <p><strong>Note:</strong> Some file managers show <code class="inline">/storage/emulated/0/</code> as just <code class="inline">Internal Storage/</code>.</p>
    <p>The <code class="inline">mods</code> folder may need to be created manually if it doesn’t already exist.</p>
	<p>Use <a href="https://play.google.com/store/search?q=zarchiver&c=apps">ZArchiver</a></p>
</div>

<pre><code class="cpp hljs line-numbers">

#include &lt;mod/amlmod.h&gt;                // Includes the AML (Android Mod Loader) API header to interface with the modding framework.
#include &lt;mod/logger.h&gt;                 // Includes logging utilities to log messages to AML logs (useful for debugging).
#include &lt;mod/config.h&gt;                 // Includes configuration tools for reading/writing mod config files.
#include &lt;stdlib.h&gt;                     // Standard C library for general-purpose functions (e.g., memory allocation, process control).
#include &lt;sys/stat.h&gt;                   // Includes functions for working with file information (e.g., file attributes).
#include &lt;fstream&gt;                      // Includes C++ file stream handling for reading/writing files.
#include &lt;stdint.h&gt;                     // Defines fixed-width integer types (e.g., uint32_t, uintptr_t).
#include &lt;dlfcn.h&gt;                      // Provides functions to handle dynamic loading of shared libraries (like dlopen, dlsym).
#include &lt;string.h&gt;                     // Includes string manipulation functions (e.g., memcpy, strcmp).

// Comments specifying dependencies (helpful for other developers).
// libR1.so - another mod or component required
// libGTASA.so - San Andreas version of the game, mentioned likely for context or comparison

// Registering mod with AML using macro provided by AML framework
MYMODCFG(MEGAMIND.VCMOD, ViceCityMod, 1.0, MEGAMIND)
// Arguments:
// - Internal mod ID
// - Mod name
// - Mod version
// - Author name

// Global variable: handle for the GTA VC shared library
void* hGTAVC = nullptr;                // `hGTAVC` will store the handle to the loaded libGTAVC.so so we can access its symbols

// Global variable: base memory address for libGTAVC.so in the process
uintptr_t pGTAVC = 0;                  // `pGTAVC` holds the base address of the GTA VC library for offset-based modifications

// Hook declaration for the function: CHud::SetBigMessage
// This will override the original SetBigMessage function in-game
DECL_HOOKv(SetBigMessage, unsigned short* text , unsigned short style)
{
    // Define a custom message string using UTF-16 literal (char16_t)
    char16_t myString[] = u"MEGAMIND";

    // Call the original SetBigMessage but replace the message text with "MEGAMIND"
    SetBigMessage(reinterpret_cast&lt;unsigned short*&gt;(myString), 1);
}

// Hook declaration for the function: CHud::SetHelpMessage
// This will override the in-game help message function
DECL_HOOKv(SetHelpMessage, unsigned short* text, bool flag1, bool flag2, bool flag3)
{
    // Define a UTF-16 string "MEGAMIND" to display instead of original help message
    char16_t myString2[] = u"MEGAMIND";

    // Call the original SetHelpMessage but with custom message and custom flags
     SetHelpMessage(reinterpret_cast&lt;unsigned short*&gt;(myString2), true, false, false);

}

// The main entry point called by AML when the mod is loaded
extern "C" void OnModLoad()
{
    // Fetch base address of libGTAVC.so using AML API
    pGTAVC = aml->GetLib("libGTAVC.so");

    // Fetch handle to libGTAVC.so (for symbol hooking via dlsym-like API)
    hGTAVC = aml->GetLibHandle("libGTAVC.so");

    // If the library was found and loaded successfully
    if (pGTAVC && hGTAVC)
    {
        // Hook into CHud::SetBigMessage in the game's HUD system
        // _ZN4CHud13SetBigMessageEPtt is the mangled C++ name (symbol)
        HOOKSYM(SetBigMessage, hGTAVC, "_ZN4CHud13SetBigMessageEPtt");

        // Hook into CHud::SetHelpMessage function
        // _ZN4CHud14SetHelpMessageEPtbbb is the mangled symbol for the method
        HOOKSYM(SetHelpMessage, hGTAVC, "_ZN4CHud14SetHelpMessageEPtbbb");
    }
}

</code></pre>

<a href='/wiki'>Go Back</a>
<!-- Highlight.js and line numbers -->
<script src="https://cdnjs.cloudflare.com/ajax/libs/highlight.js/11.8.0/highlight.min.js"></script>
<script src="https://cdnjs.cloudflare.com/ajax/libs/highlight.js-line-numbers.js/2.8.0/highlightjs-line-numbers.min.js"></script>
<script>
    hljs.highlightAll();
    hljs.initLineNumbersOnLoad();
</script>

</body>
</html>

