using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using UnityEngine;

// Some mods expect an un-namespaced Setup enum; provide a global alias too.
public enum Setup
{
    None = 0,
    PostInit = 1,
    Menu = 12
}

namespace MSCLoader
{
    // Some mods reference MSCLoader.Setup (namespace-level); keep it alongside Mod.Setup.
    public enum Setup
    {
        None = 0,
        PostInit = 1,
        Menu = 12
    }

    // Minimal Game enum placeholder. Real loader sets bit flags; we only need a value.
    public enum Game
    {
        Unknown = 0,
        MySummerCar = 1,
        MyWinterCar = 2
    }

    // Base mod class stubbed for this loader.
    public abstract class Mod
    {
        // Nested Setup enum to match MSCLoader.Mod/Setup expected by mods.
        public enum Setup
        {
            None = 0,
            PostInit = 1,
            Menu = 12
        }

        public virtual string ID => GetType().Name;
        public virtual string Name => ID;
        public virtual string Author => "unknown";
        public virtual string Version => "1.0";
        public virtual string Description => string.Empty;
        public virtual Game SupportedGames => Game.Unknown;

        // Called by the loader after construction.
        public virtual void ModSetup() { }

        // In the real MSCLoader this registers UI callbacks; here we just invoke immediately.
        public void SetupFunction(Setup setup, Action action)
        {
            action?.Invoke();
        }
    }

    // Settings façade used by the mod. We record buttons and click them once after setup.
    public static class Settings
    {
        private static readonly List<SettingsButton> Buttons = new List<SettingsButton>();

        public static SettingsHeader AddHeader(string label, bool a, bool b)
        {
            return new SettingsHeader();
        }

        public static SettingsDropDownList AddDropDownList(string id, string label, string[] options, int defaultIndex, Action onChange, bool visible)
        {
            var ddl = new SettingsDropDownList(options, defaultIndex, onChange);
            return ddl;
        }

        public static SettingsButton AddButton(string label, Action onClick, bool visible)
        {
            var btn = new SettingsButton(onClick);
            Buttons.Add(btn);
            return btn;
        }

        // Run any registered button actions once (simulating a user click).
        public static void RunAllButtonsOnce()
        {
            foreach (var btn in Buttons.ToList())
            {
                btn.Invoke();
            }
        }
    }

    public sealed class SettingsHeader
    {
    }

    public sealed class SettingsButton
    {
        private readonly Action _onClick;

        public SettingsButton(Action onClick)
        {
            _onClick = onClick;
        }

        public void Invoke()
        {
            _onClick?.Invoke();
        }
    }

    public sealed class SettingsDropDownList
    {
        private readonly string[] _options;
        private readonly Action _onChange;
        private int _selectedIndex;

        public SettingsDropDownList(string[] options, int defaultIndex, Action onChange)
        {
            _options = options ?? new string[0];
            _selectedIndex = Math.Max(0, Math.Min(defaultIndex, _options.Length > 0 ? _options.Length - 1 : 0));
            _onChange = onChange;
        }

        public int GetSelectedItemIndex()
        {
            return _selectedIndex;
        }

        public void SetSelectedItemIndex(int index)
        {
            _selectedIndex = Math.Max(0, Math.Min(index, _options.Length > 0 ? _options.Length - 1 : 0));
            _onChange?.Invoke();
        }
    }
}

namespace MSCShim
{
    internal static class ManagedLog
    {
        private static readonly object Gate = new object();
        private static string _path = null;

        public static void Init(string modsDir)
        {
            try
            {
                var root = string.IsNullOrEmpty(modsDir) ? "." : Path.GetDirectoryName(modsDir.TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar));
                _path = Path.Combine(root ?? ".", "maximum.log");
            }
            catch
            {
                _path = null;
            }
        }

        public static void Write(string message)
        {
            try
            {
                if (string.IsNullOrEmpty(_path)) return;
                var line = "[managed] " + message + "\r\n";
                lock (Gate)
                {
                    File.AppendAllText(_path, line);
                }
            }
            catch
            {
                // swallow
            }
        }
    }

    public static class Loader
    {
        // Entry called from native bridge. Loads all managed mod DLLs in the folder and runs their setup.
        public static void Run(string modsDir)
        {
            try
            {
                ManagedLog.Init(modsDir);
                ManagedLog.Write("MSCLoader managed shim starting");
                if (string.IsNullOrEmpty(modsDir) || !Directory.Exists(modsDir))
                {
                    ManagedLog.Write("mods directory missing: " + modsDir);
                    return;
                }

                var selfName = typeof(Loader).Assembly.GetName().Name;
                var dlls = Directory.GetFiles(modsDir, "*.dll", SearchOption.TopDirectoryOnly)
                    .Where(p => !string.Equals(Path.GetFileNameWithoutExtension(p), selfName, StringComparison.OrdinalIgnoreCase))
                    .ToArray();

                ManagedLog.Write($"scanning {dlls.Length} dll(s) in {modsDir}");

                foreach (var path in dlls)
                {
                    ManagedLog.Write($"trying {Path.GetFileName(path)}");
                    TryLoadModAssembly(path);
                }

                // Simulate clicking any buttons that were registered during ModSetup.
                MSCLoader.Settings.RunAllButtonsOnce();
            }
            catch (Exception ex)
            {
                ManagedLog.Write("Loader failure: " + ex);
            }
        }

        private static void TryLoadModAssembly(string path)
        {
            try
            {
                var asm = Assembly.LoadFrom(path);
                ManagedLog.Write($"loaded assembly {Path.GetFileName(path)}");
                foreach (var type in asm.GetTypes())
                {
                    if (!typeof(MSCLoader.Mod).IsAssignableFrom(type) || type.IsAbstract)
                        continue;

                    var mod = (MSCLoader.Mod)Activator.CreateInstance(type);
                    ManagedLog.Write($"init {mod.ID}");
                    mod.ModSetup();
                }
            }
            catch (Exception ex)
            {
                if (ex is TypeLoadException tle)
                {
                    ManagedLog.Write($"failed to load {Path.GetFileName(path)}: TypeLoadException {tle.TypeName} :: {tle}");
                }
                else
                {
                    ManagedLog.Write($"failed to load {Path.GetFileName(path)}: {ex}");
                }
            }
        }
    }
}
