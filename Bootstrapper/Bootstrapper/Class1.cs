using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Runtime.InteropServices;
using Mono.Cecil;

namespace Doorstop
{
    public static class Entrypoint
    {
        const string BootstrapDir = "ModManager";

        enum ManagerResult
        {
            UseBepInEx = 0,
            SkipBepinex = 1,
            Abort = 2
        }

        public static void Start()
        {
            try
            {

                Log("Entrypoint.Start()");

                LogDoorstopEnv("at Start()");

                var result = RunManagerUI();

                Log("Manager returned: " + result);

                if ((ManagerResult)result == ManagerResult.Abort)
                {
                    Log("Manager requested abort.");
                    Environment.Exit(0);
                }

                if ((ManagerResult)result == ManagerResult.UseBepInEx)
                {
                    Log("Loading BepInEx.");
                    LoadBepInEx();
                }
                else
                {
                    Log("Skipping BepInEx.");
                }
            }
            catch (Exception e)
            {
                Log(e.Message);
            }
        }

        static Dictionary<string, string> SnapshotDoorstopEnv()
        {
            var snapshot = new Dictionary<string, string>();

            foreach (System.Collections.DictionaryEntry e in Environment.GetEnvironmentVariables())
            {
                string key = e.Key?.ToString() ?? "";

                if (key.StartsWith("DOORSTOP", StringComparison.OrdinalIgnoreCase))
                    snapshot[key] = e.Value.ToString();
            }

            return snapshot;
        }

        static void LogDoorstopEnv(string context)
        {
            try
            {
                var vars = SnapshotDoorstopEnv();

                if (vars.Count == 0)
                {
                    Log("No DOORSTOP_* env vars present (" + context + ").");
                    return;
                }

                Log("DOORSTOP_* env vars (" + context + "):");

                foreach (var kvp in vars.OrderBy(k => k.Key))
                    Log("  " + kvp.Key + " = " + kvp.Value);
            }
            catch (Exception ex)
            {
                Log("ERROR reading DOORSTOP_* env vars (" + context + "): " + ex);
            }
        }

        static void RestoreDoorstopEnv(Dictionary<string, string> snapshot)
        {
            try
            {
                var current = SnapshotDoorstopEnv();

                // Restore anything that changed or vanished.
                foreach (var kvp in snapshot)
                {
                    if (!current.TryGetValue(kvp.Key, out var currentValue) ||
                        currentValue != kvp.Value)
                    {
                        Log("Restoring " + kvp.Key + " (was '" +
                            (current.ContainsKey(kvp.Key) ? currentValue : "<missing>") +
                            "', restoring to '" + kvp.Value + "').");

                        Environment.SetEnvironmentVariable(kvp.Key, kvp.Value);
                    }
                }

                // Clear anything that appeared that wasn't there before.
                foreach (var key in current.Keys)
                {
                    if (!snapshot.ContainsKey(key))
                    {
                        Log("Clearing unexpected env var introduced by manager: " + key);
                        Environment.SetEnvironmentVariable(key, null);
                    }
                }
            }
            catch (Exception ex)
            {
                Log("ERROR restoring DOORSTOP_* env vars: " + ex);
            }
        }

        static void LoadBepInEx()
        {
            try
            {
                string path = Path.Combine(
                    AppDomain.CurrentDomain.BaseDirectory,
                    "BepInEx",
                    "core",
                    "BepInEx.Preloader.dll"
                );

                Log("Loading BepInEx from: " + path);
                Log("CurrentDirectory before load: " + Environment.CurrentDirectory);

                LogDoorstopEnv("before BepInEx load");

                if (!File.Exists(path))
                {
                    Log("ERROR: BepInEx.Preloader.dll not found at expected path.");
                    return;
                }

                var asm = Assembly.LoadFrom(path);
                Log("Loaded assembly identity: " + asm.FullName);
                Log("Loaded assembly location: " + asm.Location);

                var runnerType = asm.GetType("BepInEx.Preloader.PreloaderRunner");
                Log("PreloaderRunner type found: " + (runnerType != null));

                var type = asm.GetType("Doorstop.Entrypoint");

                if (type == null)
                {
                    Log("ERROR: Could not find Doorstop.Entrypoint.");
                    return;
                }

                var start = type.GetMethod(
                    "Start",
                    BindingFlags.Public | BindingFlags.Static
                );

                if (start == null)
                {
                    Log("ERROR: Could not find Doorstop.Entrypoint.Start().");
                    return;
                }

                Log($"cwd = {Environment.CurrentDirectory}");
                Log("invoke");
                Environment.SetEnvironmentVariable("DOORSTOP_INVOKE_DLL_PATH", path);
                start.Invoke(null, null);

                Log("BepInEx Doorstop.Entrypoint.Start() returned normally.");
            }
            catch (Exception ex)
            {
                Log("ERROR loading BepInEx: " + ex);
            }
        }

        static ManagerResult RunManagerUI()
        {
            if (!RuntimeInformation.IsOSPlatform(OSPlatform.Windows))
            {
                Log("Non-Windows platform; skipping manager.");
                return ManagerResult.SkipBepinex;
            }

            string scanFile = "bepinexdlls.tmp";

            Log("Starting plugin scan.");
            Log("Scan file: " + scanFile);

            try
            {
                Helpers.ScanPluginsToFile(scanFile);
                Log("Plugin scan completed.");
            }
            catch (Exception ex)
            {
                Log("ERROR scanning plugins: " + ex);

                
                try
                {
                    if (!File.Exists(scanFile))
                        File.WriteAllText(scanFile, "");
                }
                catch (Exception writeEx)
                {
                    Log("ERROR creating scan file: " + writeEx);
                }
            }

            Process proc;

            var envSnapshot = SnapshotDoorstopEnv();
            string cwdBefore = Environment.CurrentDirectory;

            try
            {
                string arguments =
                    "--bepinex-scan-file \"" +
                    scanFile.Replace("\"", "\\\"") +
                    "\"";

                string managerPath = Path.Combine(
                    BootstrapDir,
                    "manager.exe"
                );

                Log("Starting manager: " + managerPath);
                Log("Arguments: " + arguments);

                var psi = new ProcessStartInfo
                {
                    FileName = managerPath,
                    Arguments = arguments,
                    UseShellExecute = false,

                    // Give the manager its own stdio handles instead of
                    // inheriting ours. Under Wine/Proton, an inherited
                    // console/stdout handle that the child process touches
                    // (or closes) can leave BepInEx's later console/logger
                    // init (StandardOutType = Auto) unable to attach, with
                    // no managed exception raised anywhere.
                    RedirectStandardOutput = true,
                    RedirectStandardError = true,
                    RedirectStandardInput = true,
                    CreateNoWindow = true,

                    // Pin the working directory explicitly rather than
                    // inheriting whatever the current directory happens to
                    // be, so the manager can't cause CWD drift as a side
                    // effect.
                    WorkingDirectory = AppDomain.CurrentDomain.BaseDirectory
                };

                proc = Process.Start(psi);

                // Drain the redirected streams asynchronously so the child
                // never blocks trying to write to a full pipe buffer.
                if (proc != null)
                {
                    proc.OutputDataReceived += (s, e) =>
                    {
                        if (!string.IsNullOrEmpty(e.Data))
                            Log("[manager stdout] " + e.Data);
                    };
                    proc.ErrorDataReceived += (s, e) =>
                    {
                        if (!string.IsNullOrEmpty(e.Data))
                            Log("[manager stderr] " + e.Data);
                    };

                    proc.BeginOutputReadLine();
                    proc.BeginErrorReadLine();
                }
            }
            catch (Exception ex)
            {
                Log("ERROR starting manager: " + ex);

                TryDelete(scanFile);

                return ManagerResult.SkipBepinex;
            }

            if (proc == null)
            {
                Log("ERROR: Process.Start returned null.");

                TryDelete(scanFile);

                return ManagerResult.SkipBepinex;
            }

            try
            {
                proc.WaitForExit();

                Log("Manager exited with code: " + proc.ExitCode);

                return (ManagerResult)proc.ExitCode;
            }
            catch (Exception ex)
            {
                Log("ERROR waiting for manager: " + ex);

                return ManagerResult.SkipBepinex;
            }
            finally
            {
                TryDelete(scanFile);

                // Restore anything the manager process may have changed on
                // our side (env vars, CWD) before we go on to load BepInEx.
                RestoreDoorstopEnv(envSnapshot);

                if (Environment.CurrentDirectory != cwdBefore)
                {
                    Log("CurrentDirectory changed by manager (was '" +
                        cwdBefore + "', now '" + Environment.CurrentDirectory +
                        "'); restoring.");

                    Environment.CurrentDirectory = cwdBefore;
                }
            }
        }

        static void TryDelete(string path)
        {
            try
            {
                if (File.Exists(path))
                {
                    File.Delete(path);
                    Log("Deleted scan file: " + path);
                }
            }
            catch (Exception ex)
            {
                Log("ERROR deleting '" + path + "': " + ex);
            }
        }

        public static void Log(string message)
        {
            try
            {
                string logPath = Path.Combine(
                    AppDomain.CurrentDomain.BaseDirectory,
                    BootstrapDir,
                    "log.txt"
                );

                string directory = Path.GetDirectoryName(logPath);

                if (!Directory.Exists(directory))
                    Directory.CreateDirectory(directory);

                File.AppendAllText(
                    logPath,
                    "[" + DateTime.Now.ToString("yyyy-MM-dd HH:mm:ss.fff") +
                    "] " + message + Environment.NewLine
                );
            }
            catch
            {
                // Logging must never prevent the game from starting.
            }
        }
    }

    public class DllEntry
    {
        public string Path;
        public bool IsBepInEx;

        public string Name;
        public string Id;
        public string Version;

        public bool Enabled;
    }

    public static class Helpers
    {
        const string PluginsDir = "BepInEx/plugins";

        public static List<DllEntry> ScanPlugins()
        {
            var result = new List<DllEntry>();

            string root = Path.Combine(
                AppDomain.CurrentDomain.BaseDirectory,
                PluginsDir
            );

            Entrypoint.Log("Plugin root: " + root);

            if (!Directory.Exists(root))
            {
                Entrypoint.Log(
                    "Plugin directory does not exist: " + root
                );

                return result;
            }

            IEnumerable<string> files;

            try
            {
                files = Directory.EnumerateFiles(
                    root,
                    "*",
                    SearchOption.AllDirectories
                );
            }
            catch (Exception ex)
            {
                Entrypoint.Log(
                    "ERROR enumerating plugin directory: " + ex
                );

                return result;
            }

            int totalFiles = 0;
            int candidateFiles = 0;

            foreach (string file in files)
            {
                totalFiles++;

                try
                {
                    string extension = Path.GetExtension(file);

                    bool isDll =
                        extension.Equals(
                            ".dll",
                            StringComparison.OrdinalIgnoreCase
                        );

                    bool isDllOff =
                        file.EndsWith(
                            ".dll.off",
                            StringComparison.OrdinalIgnoreCase
                        );

                    if (!isDll && !isDllOff)
                        continue;

                    candidateFiles++;

                    Entrypoint.Log(
                        "Inspecting: " + file
                    );

                    DllEntry entry = InspectAssembly(file);

                    if (!entry.IsBepInEx)
                    {
                        Entrypoint.Log(
                            "Not a BepInEx plugin: " + file
                        );

                        continue;
                    }

                    entry.Enabled = isDll;

                    Entrypoint.Log(
                        "Found BepInEx plugin: " +
                        entry.Name +
                        " [" +
                        entry.Id +
                        "] " +
                        entry.Version +
                        " (" +
                        (entry.Enabled ? "enabled" : "disabled") +
                        ")"
                    );

                    result.Add(entry);
                }
                catch (Exception ex)
                {
                    Entrypoint.Log(
                        "ERROR processing '" +
                        file +
                        "': " +
                        ex
                    );
                }
            }

            Entrypoint.Log(
                "Plugin scan finished. Files: " +
                totalFiles +
                ", candidates: " +
                candidateFiles +
                ", plugins: " +
                result.Count
            );

            return result;
        }

        public static void ScanPluginsToFile(string output)
        {
            var plugins = ScanPlugins();

            Entrypoint.Log(
                "Writing " +
                plugins.Count +
                " plugins to: " +
                output
            );

            using (var writer = new StreamWriter(
                       output,
                       false,
                       new System.Text.UTF8Encoding(false)))
            {
                foreach (var plugin in plugins)
                {
                    writer.WriteLine(plugin.Path ?? "");
                    writer.WriteLine(plugin.Id ?? "");
                    writer.WriteLine(plugin.Name ?? "");
                    writer.WriteLine(plugin.Version ?? "");
                    writer.WriteLine(plugin.Enabled ? "1" : "0");
                }
            }

            Entrypoint.Log(
                "Finished writing scan file."
            );
        }

        static DllEntry InspectAssembly(string path)
        {
            var entry = new DllEntry
            {
                Path = path,
                IsBepInEx = false,
                Enabled = !path.EndsWith(
                    ".dll.off",
                    StringComparison.OrdinalIgnoreCase
                ),
                Name = null,
                Id = null,
                Version = null
            };

            try
            {
                var assembly = AssemblyDefinition.ReadAssembly(
                    path,
                    new ReaderParameters
                    {
                        ReadSymbols = false
                    }
                );

                foreach (var module in assembly.Modules)
                {
                    foreach (var type in GetAllTypes(module.Types))
                    {
                        foreach (var attribute in type.CustomAttributes)
                        {
                            if (attribute.AttributeType.FullName !=
                                "BepInEx.BepInPlugin")
                                continue;

                            entry.IsBepInEx = true;

                            if (attribute.ConstructorArguments.Count >= 3)
                            {
                                entry.Id =
                                    attribute.ConstructorArguments[0]
                                        .Value?.ToString();

                                entry.Name =
                                    attribute.ConstructorArguments[1]
                                        .Value?.ToString();

                                entry.Version =
                                    attribute.ConstructorArguments[2]
                                        .Value?.ToString();
                            }

                            return entry;
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                Entrypoint.Log(
                    "Cecil failed to read '" +
                    path +
                    "': " +
                    ex
                );
            }

            return entry;
        }

        static IEnumerable<TypeDefinition> GetAllTypes(
            IEnumerable<TypeDefinition> types)
        {
            foreach (var type in types)
            {
                yield return type;

                foreach (var nested in GetAllTypes(type.NestedTypes))
                    yield return nested;
            }
        }
    }
}