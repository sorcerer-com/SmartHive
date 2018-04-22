using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;

namespace SmartHiveViewer.Models
{
    public static class InstallUtil
    {
        public static void Install()
        {
            string currentDir = Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location);
            string appPath = Path.Combine(
                Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData), 
                "SmartHive");
            string appExePath = Path.Combine(appPath, "SmartHiveViewer.exe");

            if (currentDir == appPath)
            {
                ExecuteFromRemovable();
                return;
            }

            try
            {
                if (!Directory.Exists(appPath))
                    Directory.CreateDirectory(appPath);

                var files = Directory.GetFiles(currentDir);
                foreach (var file in files)
                {
                    var newFile = Path.Combine(appPath, Path.GetFileName(file));
                    File.Copy(file, newFile, true);
                }

                // ask to create desktop shortcut if it isn't already created
                if (!File.Exists(Path.Combine(
                    Environment.GetFolderPath(Environment.SpecialFolder.DesktopDirectory),
                    "Умен Кошер.lnk")))
                {
                    var res = MessageBox.Show("Искате ли да добавите препратка на десктопа?", 
                        "Confim", MessageBoxButton.YesNo, MessageBoxImage.Question);
                    if (res == MessageBoxResult.Yes)
                        CreateDesktopShortcut(appExePath, "Умен Кошер");
                }
            }
            catch
            {
                MessageBox.Show("Не може да се копира последната версия на софтуера", "SmartHive",
                    MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }
        }

        private static void ExecuteFromRemovable()
        {
            var dirs = GetRemovableDrives();
            foreach (var dir in dirs)
            {
                var smartHiveDir = dir.GetDirectories("Viewer").SingleOrDefault();
                if (smartHiveDir == null)
                    continue;

                var smartHiveExe = smartHiveDir.GetFiles("SmartHiveViewer.exe").SingleOrDefault();
                if (smartHiveExe == null)
                    continue;
                
                Process.Start(smartHiveExe.FullName);
                Environment.Exit(0);
            }
        }

        public static List<DirectoryInfo> GetRemovableDrives()
        {
            var result = new List<DirectoryInfo>();

            var driveInfos = DriveInfo.GetDrives().Where(d => d.DriveType == DriveType.Removable);
            foreach (var driveInfo in driveInfos)
            {
                var dir = driveInfo.RootDirectory.GetDirectories("SmartHive").SingleOrDefault();
                if (dir != null)
                    result.Add(dir);
            }

            return result;
        }

        public static void CreateDesktopShortcut(string appPath, string shortcutName = null)
        {
            shortcutName = shortcutName ?? Path.GetFileNameWithoutExtension(appPath);
            var shortcutPath = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.DesktopDirectory), shortcutName + ".lnk");

            var shell = new IWshRuntimeLibrary.WshShell();
            var shortcut = (IWshRuntimeLibrary.IWshShortcut)shell.CreateShortcut(shortcutPath);
            shortcut.Description = "SmartHive Application";
            shortcut.TargetPath = appPath;
            shortcut.WorkingDirectory = Path.GetDirectoryName(appPath);
            shortcut.Save();
        }
    }
}
