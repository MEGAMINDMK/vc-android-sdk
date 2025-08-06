# vc-android-sdk
<body>

<section id="mod-install" style="background:#eef6ff; border:1px solid #007bff; border-radius:8px; padding:15px; margin: 20px auto; max-width: 600px; font-size: 1rem; text-align: center;">
  <h3 style="color: #0056b3;">Getting the Mod Working</h3>
  <p>To get the mod working properly, you will need to install the <strong>GTA VC Android APK</strong>:</p>
  <a href="https://gtavcandroid-mgldmglfnc.netlify.app/gtavc.apk" download style="display: inline-block; margin-top: 10px; padding: 10px 20px; background-color: #007bff; color: white; text-decoration: none; border-radius: 5px; font-weight: bold;">Download gtavc.apk</a>
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

The <code class="inline">jni</code> folder must be placed inside your NDK project directory.</p>

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

